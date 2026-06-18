import * as path from "node:path";
import * as vscode from "vscode";
type DisplayMode = "compact" | "detailed";

interface SyntaxHit {
  uri: vscode.Uri;
  diagnostic: vscode.Diagnostic;
}

type TreeNode = SyntaxCodeNode | SyntaxFileNode;
const FILTER_STATE_KEY = "vitte.topSyntaxErrors.codeFilter";

function labelAsString(label: vscode.TreeItemLabel | string | undefined): string {
  if (typeof label === "string") return label;
  return label?.label ?? "";
}

function syntaxCodeOf(d: vscode.Diagnostic): string | undefined {
  const raw = d.code;
  let code = "";
  if (typeof raw === "string" || typeof raw === "number") {
    code = String(raw).trim().toUpperCase();
  } else if (raw && typeof raw === "object" && "value" in raw) {
    const value = (raw as { value?: unknown }).value;
    if (typeof value === "string" || typeof value === "number") {
      code = String(value).trim().toUpperCase();
    }
  }
  if (!code) return undefined;
  const prefixed = /^([A-Z][A-Z0-9_-]*):(.*)$/.exec(code);
  const prefix = prefixed?.[1] ?? "";
  const base = (prefixed?.[2] ?? code).trim().toUpperCase();
  if (/^E\d{4}$/.test(base)) return base;
  if (prefix === "PARSE") return base || "PARSE";
  if (prefix === "BRACKETS") return code;
  if (d.source === "vitte" && /^E\d{4}$/.test(base)) return base;
  return undefined;
}

function relLabel(uri: vscode.Uri): string {
  const rel = vscode.workspace.asRelativePath(uri, false);
  return rel || uri.fsPath;
}

function severityWeight(s: vscode.DiagnosticSeverity | undefined): number {
  switch (s) {
    case vscode.DiagnosticSeverity.Error: return 4;
    case vscode.DiagnosticSeverity.Warning: return 3;
    case vscode.DiagnosticSeverity.Information: return 2;
    case vscode.DiagnosticSeverity.Hint: return 1;
    default: return 0;
  }
}

function firstSyntaxHit(entries: readonly SyntaxHit[] | undefined): SyntaxHit | undefined {
  if (!entries || entries.length === 0) return undefined;
  return entries[0];
}

class SyntaxCodeNode extends vscode.TreeItem {
  constructor(
    public readonly code: string,
    public readonly entries: SyntaxHit[],
    mode: DisplayMode,
  ) {
    super(code, vscode.TreeItemCollapsibleState.Collapsed);
    this.description = mode === "detailed"
      ? `${entries.length} issue${entries.length > 1 ? "s" : ""}`
      : `${entries.length}`;
    this.contextValue = "vitteTopSyntaxCode";
    this.iconPath = new vscode.ThemeIcon("list-ordered");
    const sample = firstSyntaxHit(entries)?.diagnostic.message ?? "";
    this.tooltip = mode === "detailed"
      ? `${code} - ${entries.length} issue(s)\n${sample}`
      : `${code} (${entries.length})`;
  }
}

class SyntaxFileNode extends vscode.TreeItem {
  constructor(
    public readonly code: string,
    public readonly file: vscode.Uri,
    public readonly entries: SyntaxHit[],
    mode: DisplayMode,
  ) {
    const first = firstSyntaxHit(entries);
    const pos = first?.diagnostic.range.start;
    const baseLabel = relLabel(file);
    const compactLabel = pos ? `${baseLabel} (L${pos.line + 1})` : baseLabel;
    super(mode === "detailed" ? baseLabel : compactLabel, vscode.TreeItemCollapsibleState.None);
    this.description = `${entries.length}x${pos ? ` (L${pos.line + 1}:C${pos.character + 1})` : ""}`;
    this.tooltip = mode === "detailed"
      ? `${relLabel(file)}\n${first?.diagnostic.message ?? ""}`
      : `${code} • ${entries.length}`;
    this.contextValue = "vitteTopSyntaxFile";
    this.iconPath = vscode.ThemeIcon.File;
    if (first?.diagnostic) {
      this.command = {
        command: "vitte.diagnostics.explain",
        title: "Explain diagnostic",
        arguments: [{ uri: file, diagnostic: first.diagnostic }],
      };
    }
  }
}

class TopSyntaxErrorsProvider implements vscode.TreeDataProvider<TreeNode>, vscode.Disposable {
  private readonly emitter = new vscode.EventEmitter<TreeNode | undefined>();
  readonly onDidChangeTreeData: vscode.Event<TreeNode | undefined> = this.emitter.event;
  private nodes: SyntaxCodeNode[] = [];
  private codeFilter = new Set<string>();
  private displayMode: DisplayMode = "compact";

  setCodeFilter(raw: string | undefined): void {
    this.codeFilter = parseCodeFilter(raw);
  }

  getCodeFilterLabel(): string | undefined {
    if (this.codeFilter.size === 0) return undefined;
    return Array.from(this.codeFilter.values()).join(", ");
  }

  refresh(): void {
    this.displayMode = readDisplayMode();
    const byCode = new Map<string, SyntaxHit[]>();
    for (const [uri, list] of vscode.languages.getDiagnostics()) {
      if (uri.scheme !== "file") continue;
      const ext = path.extname(uri.fsPath).toLowerCase();
      if (ext !== ".vit" && ext !== ".vitte") continue;
      for (const diagnostic of list) {
        const code = syntaxCodeOf(diagnostic);
        if (!code) continue;
        if (this.codeFilter.size > 0 && !this.codeFilter.has(code.toUpperCase())) continue;
        const arr = byCode.get(code) ?? [];
        arr.push({ uri, diagnostic });
        byCode.set(code, arr);
      }
    }
    const nodes = Array.from(byCode.entries())
      .map(([code, entries]) => new SyntaxCodeNode(code, entries, this.displayMode))
      .sort((a, b) => {
        const sa = severityWeight(firstSyntaxHit(a.entries)?.diagnostic.severity);
        const sb = severityWeight(firstSyntaxHit(b.entries)?.diagnostic.severity);
        if (sa !== sb) return sb - sa;
        const c = b.entries.length - a.entries.length;
        if (c !== 0) return c;
        return a.code.localeCompare(b.code);
      });
    this.nodes = nodes;
    this.emitter.fire(undefined);
  }

  hasItems(): boolean {
    return this.nodes.length > 0;
  }

  getTreeItem(element: TreeNode): vscode.TreeItem {
    return element;
  }

  getChildren(element?: TreeNode): vscode.ProviderResult<TreeNode[]> {
    if (!element) return this.nodes;
    if (!(element instanceof SyntaxCodeNode)) return [];
    const byFile = new Map<string, SyntaxHit[]>();
    for (const entry of element.entries) {
      const key = entry.uri.toString();
      const arr = byFile.get(key) ?? [];
      arr.push(entry);
      byFile.set(key, arr);
    }
    return Array.from(byFile.entries())
      .map(([uriKey, entries]) => new SyntaxFileNode(element.code, vscode.Uri.parse(uriKey), entries, this.displayMode))
      .sort((a, b) => b.entries.length - a.entries.length || labelAsString(a.label).localeCompare(labelAsString(b.label)));
  }

  dispose(): void {
    this.emitter.dispose();
  }
}

export function registerTopSyntaxErrorsView(context: vscode.ExtensionContext): void {
  const provider = new TopSyntaxErrorsProvider();
  let tree: vscode.TreeView<TreeNode> | undefined;
  try {
    tree = vscode.window.createTreeView<TreeNode>("vitteTopSyntaxErrors", { treeDataProvider: provider });
  } catch {
    // Keep commands active even when the view contribution is not present.
  }
  const initialFilter = context.workspaceState.get<string>(FILTER_STATE_KEY);
  provider.setCodeFilter(initialFilter);

  const refresh = () => {
    provider.refresh();
    if (!tree) return;
    const filterLabel = provider.getCodeFilterLabel();
    if (!provider.hasItems()) {
      tree.message = filterLabel
        ? `$(filter) No syntax errors for: ${filterLabel}`
        : "$(pass-filled) No syntax errors";
      return;
    }
    tree.message = filterLabel ? `$(filter) Filter: ${filterLabel}` : "";
  };

  refresh();
  context.subscriptions.push(
    provider,
    vscode.languages.onDidChangeDiagnostics(refresh),
    vscode.workspace.onDidOpenTextDocument(refresh),
    vscode.workspace.onDidCloseTextDocument(refresh),
    vscode.workspace.onDidChangeConfiguration((e) => {
      if (!e.affectsConfiguration("vitte.topSyntaxErrors.displayMode")) return;
      refresh();
    }),
    vscode.commands.registerCommand("vitte.topSyntaxErrors.refresh", refresh),
    vscode.commands.registerCommand("vitte.topSyntaxErrors.setCodeFilter", async () => {
      const current = provider.getCodeFilterLabel() ?? "";
      const raw = await vscode.window.showInputBox({
        title: "Top Syntax Errors filter",
        prompt: "Filter by syntax code (examples: E0001, E0007, parse). Use commas or spaces for multiple values.",
        placeHolder: "E0001, E0007",
        value: current,
        ignoreFocusOut: true,
      });
      if (raw === undefined) return;
      provider.setCodeFilter(raw);
      await context.workspaceState.update(FILTER_STATE_KEY, raw.trim());
      refresh();
    }),
    vscode.commands.registerCommand("vitte.topSyntaxErrors.clearCodeFilter", async () => {
      provider.setCodeFilter("");
      await context.workspaceState.update(FILTER_STATE_KEY, "");
      refresh();
    }),
  );
  if (tree) {
    context.subscriptions.push(tree);
  }
}

function parseCodeFilter(raw: string | undefined): Set<string> {
  if (!raw) return new Set<string>();
  const parts = raw
    .split(/[,\s]+/)
    .map((entry) => entry.trim().toUpperCase())
    .filter(Boolean);
  return new Set(parts);
}

function readDisplayMode(): DisplayMode {
  const cfg = vscode.workspace.getConfiguration("vitte");
  return cfg.get<DisplayMode>("topSyntaxErrors.displayMode", "compact") === "detailed"
    ? "detailed"
    : "compact";
}
