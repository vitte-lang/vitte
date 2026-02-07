import * as path from "node:path";
import * as vscode from "vscode";
import type { DiagnosticsSummary } from "./utils/diagnostics";
import {
  diagnosticsLevel,
  formatDiagnosticsSummary,
  summarizeDiagnosticsForUris,
  VITTE_FILE_EXTS,
} from "./utils/diagnostics";

type ModuleTreeNode = ModuleNode | SymbolNode;

interface ModuleEntry {
  readonly id: string;
  readonly label: string;
  readonly description?: string;
  readonly tooltip: string;
  readonly summary: DiagnosticsSummary;
  readonly children: SymbolNode[];
}

interface ModuleBucket {
  symbol?: vscode.SymbolInformation;
  children: SymbolNode[];
  uris: Set<string>;
}

function createBucket(): ModuleBucket {
  return { children: [], uris: new Set<string>() };
}

class ModuleNode extends vscode.TreeItem {
  constructor(private readonly entry: ModuleEntry) {
    super(entry.label, vscode.TreeItemCollapsibleState.Collapsed);
    this.contextValue = "vitteModuleNode";
    if (entry.description !== undefined) {
      this.description = entry.description;
    }
    this.tooltip = entry.tooltip;
    this.iconPath = iconForSummary(entry.summary);
  }

  get children(): SymbolNode[] {
    return this.entry.children;
  }
}

class SymbolNode extends vscode.TreeItem {
  constructor(public readonly symbol: vscode.SymbolInformation) {
    super(symbol.name, vscode.TreeItemCollapsibleState.None);
    this.description = symbolDetail(symbol);
    this.tooltip = symbolTooltip(symbol);
    this.iconPath = iconForSymbol(symbol.kind);
    this.command = {
      command: "vitte.modules.openSymbol",
      title: "Ouvrir le symbole",
      arguments: [symbol]
    };
    this.contextValue = "vitteModuleSymbol";
  }
}

export class ModuleExplorerProvider implements vscode.TreeDataProvider<ModuleTreeNode>, vscode.Disposable {
  private readonly emitter = new vscode.EventEmitter<ModuleTreeNode | undefined>();
  readonly onDidChangeTreeData = this.emitter.event;

  private nodes: ModuleNode[] = [];
  private tree?: vscode.TreeView<ModuleTreeNode>;
  private pendingRefresh: NodeJS.Timeout | undefined;

  setTree(tree: vscode.TreeView<ModuleTreeNode>): void {
    this.tree = tree;
  }

  getTreeItem(element: ModuleTreeNode): vscode.TreeItem {
    return element;
  }

  getChildren(element?: ModuleTreeNode): vscode.ProviderResult<ModuleTreeNode[]> {
    if (!element) return this.nodes;
    if (element instanceof ModuleNode) return element.children;
    return [];
  }

  dispose(): void {
    this.emitter.dispose();
    if (this.pendingRefresh) clearTimeout(this.pendingRefresh);
    this.pendingRefresh = undefined;
  }

  refreshSoon(): void {
    if (this.pendingRefresh) clearTimeout(this.pendingRefresh);
    this.pendingRefresh = setTimeout(() => {
      this.pendingRefresh = undefined;
      void this.refresh();
    }, 250);
  }

  async refresh(): Promise<void> {
    try {
      this.pendingRefresh = undefined;
      const entries = await buildModuleEntries();
      this.nodes = entries.map(entry => new ModuleNode(entry));
      this.emitter.fire(undefined);
      if (this.tree) {
        this.tree.message = this.nodes.length === 0 ? "Aucun module Vitte détecté" : "";
      }
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      if (this.tree) {
        this.tree.message = "Impossible d’analyser les modules";
      }
      void vscode.window.showWarningMessage(`Vitte : échec de l’analyse des modules — ${message}`);
    }
  }
}

export function registerModuleExplorerView(context: vscode.ExtensionContext): ModuleExplorerProvider {
  const provider = new ModuleExplorerProvider();
  const tree = vscode.window.createTreeView<ModuleTreeNode>("vitteModules", {
    treeDataProvider: provider,
    showCollapseAll: true
  });
  provider.setTree(tree);
  context.subscriptions.push(tree, provider);

  const signals = [
    vscode.workspace.onDidChangeWorkspaceFolders(() => provider.refreshSoon()),
    vscode.workspace.onDidSaveTextDocument((doc) => {
      if (isVitteDocument(doc)) provider.refreshSoon();
    }),
    vscode.workspace.onDidOpenTextDocument((doc) => {
      if (isVitteDocument(doc)) provider.refreshSoon();
    }),
    vscode.workspace.onDidCloseTextDocument((doc) => {
      if (isVitteDocument(doc)) provider.refreshSoon();
    }),
    vscode.languages.onDidChangeDiagnostics(() => provider.refreshSoon()),
    vscode.commands.registerCommand("vitte.modules.refresh", async () => {
      await provider.refresh();
    }),
    vscode.commands.registerCommand("vitte.modules.openSymbol", async (symbol: vscode.SymbolInformation) => {
      if (!symbol?.location) return;
      const doc = await vscode.workspace.openTextDocument(symbol.location.uri);
      const editor = await vscode.window.showTextDocument(doc, { preview: true });
      editor.revealRange(symbol.location.range, vscode.TextEditorRevealType.InCenter);
      editor.selection = new vscode.Selection(symbol.location.range.start, symbol.location.range.start);
    })
  ];
  for (const disposable of signals) context.subscriptions.push(disposable);

  // Initial population.
  void provider.refresh();

  return provider;
}

async function buildModuleEntries(): Promise<ModuleEntry[]> {
  const symbols = await vscode.commands.executeCommand<vscode.SymbolInformation[]>(
    "vscode.executeWorkspaceSymbolProvider",
    ""
  );
  const list = (symbols ?? []).filter((sym) => isAcceptableSymbol(sym));

  const byModule = new Map<string, ModuleBucket>();

  for (const sym of list) {
    const moduleKey = moduleNameForSymbol(sym);
    let bucket = byModule.get(moduleKey);
    if (!bucket) {
      bucket = createBucket();
      byModule.set(moduleKey, bucket);
    }
    if (!bucket.symbol && isModuleSymbol(sym)) {
      bucket.symbol = sym;
    }
    if (!isModuleSymbol(sym)) {
      bucket.children.push(new SymbolNode(sym));
    }
    bucket.uris.add(sym.location.uri.toString());
  }

  const entries: ModuleEntry[] = [];
  for (const [name, data] of byModule) {
    const summary = summarizeDiagnosticsForUris(data.uris);
    const fileCount = data.uris.size;
    let description: string | undefined;
    if (fileCount > 1) {
      description = `${fileCount} fichiers`;
    } else {
      const [singleUri] = Array.from(data.uris);
      if (singleUri) description = uriToBasename(singleUri);
    }
    const tooltipParts = [name];
    if (description) tooltipParts.push(description);
    tooltipParts.push(formatDiagnosticsSummary(summary));
    const entry: ModuleEntry = {
      id: name,
      label: name,
      tooltip: tooltipParts.join("\n"),
      summary,
      children: [...data.children].sort(sortSymbolNodes),
      ...(description ? { description } : {})
    };
    entries.push(entry);
  }

  entries.sort((a, b) => {
    const severityDiff = severityWeight(a.summary) - severityWeight(b.summary);
    if (severityDiff !== 0) return severityDiff;
    return a.label.localeCompare(b.label);
  });

  return entries;
}

function sortSymbolNodes(a: SymbolNode, b: SymbolNode): number {
  const kindDiff = a.symbol.kind - b.symbol.kind;
  if (kindDiff !== 0) return kindDiff;
  return a.symbol.name.localeCompare(b.symbol.name);
}

function severityWeight(summary: DiagnosticsSummary): number {
  const level = diagnosticsLevel(summary);
  switch (level) {
    case "error": return 0;
    case "warning": return 1;
    default: return 2;
  }
}

function isAcceptableSymbol(sym: vscode.SymbolInformation | undefined): sym is vscode.SymbolInformation {
  if (!sym?.location?.uri) return false;
  if (sym.location.uri.scheme !== "file") return false;
  const ext = path.extname(sym.location.uri.fsPath).toLowerCase();
  return VITTE_FILE_EXTS.has(ext);
}

function moduleNameForSymbol(sym: vscode.SymbolInformation): string {
  if (isModuleSymbol(sym)) return sym.name;
  if (sym.containerName) return sym.containerName;
  return path.basename(sym.location.uri.fsPath);
}

function isModuleSymbol(sym: vscode.SymbolInformation): boolean {
  return sym.kind === vscode.SymbolKind.Module || sym.kind === vscode.SymbolKind.Namespace;
}

function iconForSummary(summary: DiagnosticsSummary): vscode.ThemeIcon {
  const level = diagnosticsLevel(summary);
  if (level === "error") return themeIcon("error");
  if (level === "warning") return themeIcon("warning");
  return themeIcon("pass-filled");
}

function iconForSymbol(kind: vscode.SymbolKind): vscode.ThemeIcon {
  switch (kind) {
    case vscode.SymbolKind.Function:
    case vscode.SymbolKind.Method:
      return themeIcon("symbol-method");
    case vscode.SymbolKind.Struct:
    case vscode.SymbolKind.Class:
      return themeIcon("symbol-structure");
    case vscode.SymbolKind.Enum:
      return themeIcon("symbol-enum");
    case vscode.SymbolKind.Interface:
    case vscode.SymbolKind.TypeParameter:
      return themeIcon("symbol-interface");
    case vscode.SymbolKind.Variable:
    case vscode.SymbolKind.Constant:
      return themeIcon("symbol-variable");
    default:
      return themeIcon("symbol-misc");
  }
}

function symbolDetail(sym: vscode.SymbolInformation): string {
  return path.basename(sym.location.uri.fsPath);
}

function symbolTooltip(sym: vscode.SymbolInformation): string {
  const range = sym.location.range;
  const position = `L${range.start.line + 1}:C${range.start.character + 1}`;
  return `${sym.name} (${vscode.SymbolKind[sym.kind]})\n${sym.location.uri.fsPath}:${position}`;
}

function uriToBasename(uri: string): string {
  try {
    return path.basename(vscode.Uri.parse(uri).fsPath);
  } catch {
    return uri;
  }
}

function themeIcon(id: string): vscode.ThemeIcon {
  return { id } as vscode.ThemeIcon;
}

function isVitteDocument(document: vscode.TextDocument): boolean {
  if (document.uri.scheme !== "file") return false;
  return VITTE_FILE_EXTS.has(path.extname(document.uri.fsPath).toLowerCase());
}
