import * as vscode from 'vscode';
import * as path from 'path';

// Compat: VS Code versions without Uri.joinPath
function joinPathCompat(base: vscode.Uri, ...parts: string[]): vscode.Uri {
  const anyUri = vscode.Uri as unknown as { joinPath?: (base: vscode.Uri, ...pathSegments: string[]) => vscode.Uri };
  if (typeof anyUri.joinPath === 'function') {
    return anyUri.joinPath(base, ...parts);
  }
  return vscode.Uri.file(path.join(base.fsPath, ...parts));
}

// ---- Types ----
interface NodeBase { id: string; label: string; contextValue: string; collapsible: vscode.TreeItemCollapsibleState }
interface FileNode extends NodeBase { kind: 'file'; uri: vscode.Uri }
interface DirNode extends NodeBase { kind: 'dir'; uri: vscode.Uri }
interface GroupNode extends NodeBase { kind: 'group'; icon?: string; children: Node[] }
interface TargetNode extends NodeBase { kind: 'target'; triple: string; icon?: string }
export type Node = FileNode | DirNode | GroupNode | TargetNode;

// ---- Utils ----
function readWorkspaceRoot(): vscode.Uri | undefined {
  return vscode.workspace.workspaceFolders?.[0]?.uri;
}

async function findOne(glob: string): Promise<vscode.Uri | undefined> {
  const list = await vscode.workspace.findFiles(glob, '**/node_modules/**', 1);
  return list[0];
}

async function findMany(glob: string, limit = 50): Promise<vscode.Uri[]> {
  return vscode.workspace.findFiles(glob, '**/node_modules/**', limit);
}

function mkId(...parts: string[]): string { return parts.join(':'); }

function mkFile(uri: vscode.Uri, label?: string): FileNode {
  return { id: mkId('file', uri.fsPath), label: label ?? path.basename(uri.fsPath), contextValue: 'vitte:file', kind: 'file', uri, collapsible: vscode.TreeItemCollapsibleState.None };
}

function mkDir(uri: vscode.Uri, label?: string): DirNode {
  return { id: mkId('dir', uri.fsPath), label: label ?? path.basename(uri.fsPath), contextValue: 'vitte:dir', kind: 'dir', uri, collapsible: vscode.TreeItemCollapsibleState.Collapsed };
}

function mkGroup(id: string, label: string, icon?: string, children: Node[] = []): GroupNode {
  const group: GroupNode = {
    id,
    label,
    contextValue: 'vitte:group',
    kind: 'group',
    collapsible: vscode.TreeItemCollapsibleState.Collapsed,
    children,
  };
  if (icon !== undefined) {
    group.icon = icon;
  }
  return group;
}

function mkTarget(triple: string): TargetNode {
  return {
    id: mkId('target', triple),
    label: triple,
    contextValue: 'vitte:target',
    kind: 'target',
    collapsible: vscode.TreeItemCollapsibleState.None,
    triple,
    icon: 'rocket',
  };
}

// ---- Provider ----
export class VitteProjectTreeProvider implements vscode.TreeDataProvider<Node> {
  private iconAsset(name: string) {
    // Use the same SVG for light/dark; you can add themed variants later if needed.
    const p = (n: string) => vscode.Uri.file(path.join(this.ctx.extensionUri.fsPath, 'media', 'icons', `${n}.svg`));
    const u = p(name);
    return { light: u, dark: u } as { light: vscode.Uri; dark: vscode.Uri };
  }

private _onDidChangeTreeData = new vscode.EventEmitter<Node | null | undefined>();
readonly onDidChangeTreeData: vscode.Event<Node | null | undefined> = this._onDidChangeTreeData.event;

  constructor(private readonly ctx: vscode.ExtensionContext) {}

  refresh(): void { this._onDidChangeTreeData.fire(undefined); }

  getTreeItem(element: Node): vscode.TreeItem {
    const item = new vscode.TreeItem(element.label, element.collapsible);
    item.contextValue = element.contextValue;

    switch (element.kind) {
      case 'file':
        item.resourceUri = element.uri;
        item.command = { command: 'vscode.open', title: 'Open', arguments: [element.uri] };
        item.iconPath = vscode.ThemeIcon.File;
        break;
      case 'dir':
        item.resourceUri = element.uri;
        item.iconPath = vscode.ThemeIcon.Folder;
        break;
      case 'group':
        // Use packaged SVG (media/icons/vitte.svg or other). Fallback to 'vitte'.
        item.iconPath = this.iconAsset(element.icon ?? 'vitte');
        break;
      case 'target':
        item.iconPath = this.iconAsset(element.icon ?? 'vitte');
        break;
    }
    return item;
  }

  async getChildren(element?: Node): Promise<Node[]> {
    if (!element) return await this.buildRoot();
    if (element.kind === 'group') return element.children;
    if (element.kind === 'dir') return await this.readDirChildren(element.uri);
    return [];
  }

  // ---- Builders ----
  private async buildRoot(): Promise<Node[]> {
    const root = readWorkspaceRoot();
    if (!root) return [];

    const nodes: Node[] = [];

    // Config group
    const vitteConfig = (await findOne('vitte.config.json')) ?? (await findOne('.vitte/config.json'));
    const configGroup = mkGroup('group:config', 'Configuration', 'settings');
    if (vitteConfig) configGroup.children.push(mkFile(vitteConfig, 'vitte.config.json'));
    const tasksJson = await findOne('.vscode/tasks.json');
    if (tasksJson) configGroup.children.push(mkFile(tasksJson));
    const launchJson = await findOne('.vscode/launch.json');
    if (launchJson) configGroup.children.push(mkFile(launchJson));
    nodes.push(configGroup);

    // Targets group from config if present
    const targets = await this.readTargetsFromConfig(vitteConfig);
    const targetGroup = mkGroup('group:targets', 'Cibles', 'rocket');
    for (const t of targets) targetGroup.children.push(mkTarget(t));
    nodes.push(targetGroup);

    // Sources group
    const srcGroup = mkGroup('group:sources', 'Sources', 'source-control');
    const modulesDirs = await findMany('{src,modules,crates}/**/*.vitte', 200);
    // Group by folder
    const byDir = new Map<string, vscode.Uri[]>();
    for (const uri of modulesDirs) {
      const dir = path.dirname(uri.fsPath);
      const arr = byDir.get(dir) ?? [];
      arr.push(uri);
      byDir.set(dir, arr);
    }
    for (const [dir, files] of byDir) {
      const children = files.map(f => mkFile(f));
      const label = path.basename(dir);
      const dirGroup = mkGroup(`group:src:${dir}`, label, 'source-control', children);
      srcGroup.children.push(dirGroup);
    }
    nodes.push(srcGroup);

    // Tests group
    const tests = await findMany('**/*_test.vitte', 200);
    const testGroup = mkGroup('group:tests', `Tests (${tests.length})`, 'beaker');
    for (const t of tests) testGroup.children.push(mkFile(t));
    nodes.push(testGroup);

    // Benchmarks group
    const benches = await findMany('**/*_bench.vitte', 200);
    const benchGroup = mkGroup('group:bench', `Benchmarks (${benches.length})`, 'graph');
    for (const b of benches) benchGroup.children.push(mkFile(b));
    nodes.push(benchGroup);

    return nodes;
  }

  private async readDirChildren(dir: vscode.Uri): Promise<Node[]> {
    try {
      const entries = await vscode.workspace.fs.readDirectory(dir);
      const nodes: Node[] = [];
      for (const [name, type] of entries) {
        const uri = joinPathCompat(dir, name);
        if (type === vscode.FileType.File && /\.(vitte|vit)$/i.test(name)) nodes.push(mkFile(uri));
        if (type === vscode.FileType.Directory) nodes.push(mkDir(uri));
      }
      return nodes;
    } catch {
      return [];
    }
  }

  private async readTargetsFromConfig(configUri?: vscode.Uri): Promise<string[]> {
    if (!configUri) return [];
    try {
      const doc = await vscode.workspace.openTextDocument(configUri);
      const json = JSON.parse(doc.getText()) as unknown;
      const rawTargets = (json as { targets?: unknown }).targets;
      if (Array.isArray(rawTargets)) {
        const list: string[] = [];
        for (const entry of rawTargets as unknown[]) {
          if (typeof entry === 'string') {
            list.push(entry);
          } else if (entry && typeof entry === 'object') {
            const triple = (entry as { triple?: unknown }).triple;
            if (typeof triple === 'string') list.push(triple);
          }
        }
        return list;
      }
    } catch {
      // ignore
    }
    return [];
  }
}

// ---- Registration helper ----
export function registerProjectTree(ctx: vscode.ExtensionContext): VitteProjectTreeProvider {
  const provider = new VitteProjectTreeProvider(ctx);
  ctx.subscriptions.push(
    vscode.window.registerTreeDataProvider('vitteExplorer', provider),
    vscode.commands.registerCommand('vitte.refreshExplorer', () => provider.refresh()),
    vscode.commands.registerCommand('vitte.revealInExplorer', (uri: vscode.Uri) => {
      void vscode.commands.executeCommand('revealInExplorer', uri);
    })
  );
  return provider;
}
