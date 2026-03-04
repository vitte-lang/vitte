import * as vscode from "vscode";
import { summarizeDiagnosticsByDirectory } from "../utils/diagnostics";

interface PackageSummaryNodeData {
  dir: string;
  errors: number;
  warnings: number;
  info: number;
  hints: number;
  files: number;
}

class PackageSummaryNode extends vscode.TreeItem {
  constructor(public readonly data: PackageSummaryNodeData) {
    super(data.dir || "(root)", vscode.TreeItemCollapsibleState.None);
    this.contextValue = "vittePackageProblems";
    this.description = `${data.errors}e ${data.warnings}w (${data.files} files)`;
    this.tooltip = [
      `Package: ${data.dir || "(root)"}`,
      `Errors: ${data.errors}`,
      `Warnings: ${data.warnings}`,
      `Info: ${data.info}`,
      `Hints: ${data.hints}`,
      `Files: ${data.files}`,
    ].join("\n");
    this.iconPath = data.errors > 0
      ? new vscode.ThemeIcon("error")
      : data.warnings > 0
        ? new vscode.ThemeIcon("warning")
        : new vscode.ThemeIcon("pass-filled");
    this.command = {
      command: "vitte.packageProblems.open",
      title: "Open package problems",
      arguments: [data.dir],
    };
  }
}

class PackageProblemsProvider implements vscode.TreeDataProvider<PackageSummaryNode>, vscode.Disposable {
  private readonly emitter = new vscode.EventEmitter<PackageSummaryNode | undefined>();
  readonly onDidChangeTreeData = this.emitter.event;
  private nodes: PackageSummaryNode[] = [];
  private refreshTimer: NodeJS.Timeout | undefined;

  refreshSoon(): void {
    if (this.refreshTimer) clearTimeout(this.refreshTimer);
    this.refreshTimer = setTimeout(() => {
      this.refreshTimer = undefined;
      this.refresh();
    }, 220);
  }

  refresh(): void {
    const rows = summarizeDiagnosticsByDirectory().map((d) => new PackageSummaryNode(d));
    this.nodes = rows;
    this.emitter.fire(undefined);
  }

  getTreeItem(element: PackageSummaryNode): vscode.TreeItem {
    return element;
  }

  getChildren(): vscode.ProviderResult<PackageSummaryNode[]> {
    return this.nodes;
  }

  dispose(): void {
    if (this.refreshTimer) clearTimeout(this.refreshTimer);
    this.emitter.dispose();
  }
}

function inDir(relPath: string, dir: string): boolean {
  if (!dir) return true;
  return relPath === dir || relPath.startsWith(`${dir}/`);
}

export function registerPackageProblemsView(context: vscode.ExtensionContext): void {
  const provider = new PackageProblemsProvider();
  const tree = vscode.window.createTreeView("vittePackageProblems", {
    treeDataProvider: provider,
    showCollapseAll: false,
  });

  const refresh = () => provider.refreshSoon();
  provider.refresh();

  context.subscriptions.push(
    provider,
    tree,
    vscode.languages.onDidChangeDiagnostics(refresh),
    vscode.workspace.onDidSaveTextDocument(refresh),
    vscode.workspace.onDidOpenTextDocument(refresh),
    vscode.workspace.onDidCloseTextDocument(refresh),
    vscode.commands.registerCommand("vitte.packageProblems.refresh", () => provider.refresh()),
    vscode.commands.registerCommand("vitte.packageProblems.open", async (dir: string) => {
      const all = vscode.languages.getDiagnostics();
      let candidate: { uri: vscode.Uri; range: vscode.Range; sev: number } | undefined;
      for (const [uri, list] of all) {
        const rel = vscode.workspace.asRelativePath(uri, false).replace(/\\/g, "/");
        if (!inDir(rel.includes("/") ? rel.slice(0, rel.lastIndexOf("/")) : "", dir ?? "")) continue;
        for (const d of list) {
          const sev = d.severity ?? vscode.DiagnosticSeverity.Hint;
          if (!candidate || sev < candidate.sev) {
            candidate = { uri, range: d.range, sev };
          }
        }
      }
      if (!candidate) return;
      const doc = await vscode.workspace.openTextDocument(candidate.uri);
      const editor = await vscode.window.showTextDocument(doc, { preview: true });
      editor.revealRange(candidate.range, vscode.TextEditorRevealType.InCenter);
      editor.selection = new vscode.Selection(candidate.range.start, candidate.range.start);
    })
  );
}

