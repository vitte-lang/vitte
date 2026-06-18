import * as path from "node:path";
import * as vscode from "vscode";

type SeverityKey = 'error' | 'warning' | 'information' | 'hint';
type DisplayMode = "compact" | "detailed";

// 🟩 Vitte: diagnostics view settings & helpers
interface VitteDiagnosticsConfig {
  /** Allowed severities; when empty or undefined -> show all. */
  severities?: SeverityKey[];
  /** Debounce delay in ms for refresh. */
  refreshDebounceMs?: number;
  /** Visual density in diagnostics view. */
  displayMode?: DisplayMode;
}

const VALID_SEVERITY_MAP: Record<SeverityKey, true> = {
  error: true,
  warning: true,
  information: true,
  hint: true,
};

const DEFAULT_CFG: Readonly<Required<VitteDiagnosticsConfig>> = {
  severities: [],
  refreshDebounceMs: 150,
  displayMode: "compact",
};

type UiLocale = "fr" | "en";
interface UiStrings {
  source: string;
  sourceUnknown: string;
  docs: string;
  explain: string;
  level: string;
  noDiagnostics: string;
  diagnosticCopied: string;
  noDiagnosticAtCursor: string;
  copyExplanation: string;
  explanationCopied: string;
  noExplainableCode: string;
  explainCommandCopied: string;
  noDocumentationLink: string;
  diagnosticPrefix: string;
  externalHelpBadge: string;
}

function currentLocale(): UiLocale {
  const lang = vscode.workspace.getConfiguration("vitte").get<string>("lang", "en").trim().toLowerCase();
  return lang.startsWith("fr") ? "fr" : "en";
}

function uiStrings(): UiStrings {
  if (currentLocale() === "fr") {
    return {
      source: "Source",
      sourceUnknown: "Source: inconnue",
      docs: "Docs",
      explain: "Expliquer",
      level: "Niveau",
      noDiagnostics: "$(pass-filled) Aucun diagnostic Vitte détecté",
      diagnosticCopied: "Diagnostic copié dans le presse-papiers",
      noDiagnosticAtCursor: "Vitte: aucun diagnostic au curseur.",
      copyExplanation: "Copier l'explication",
      explanationCopied: "Explication du diagnostic Vitte copiée",
      noExplainableCode: "Vitte: ce diagnostic n'a pas de code explicable.",
      explainCommandCopied: "Commande vitte --explain copiée",
      noDocumentationLink: "Vitte: ce diagnostic n'a pas de lien de documentation.",
      diagnosticPrefix: "Diagnostic Vitte",
      externalHelpBadge: "aide externe",
    };
  }
  return {
    source: "Source",
    sourceUnknown: "Source: unknown",
    docs: "Docs",
    explain: "Explain",
    level: "Level",
    noDiagnostics: "$(pass-filled) No Vitte diagnostics detected",
    diagnosticCopied: "Diagnostic copied to clipboard",
    noDiagnosticAtCursor: "Vitte: no diagnostic at cursor.",
    copyExplanation: "Copy explanation",
    explanationCopied: "Vitte diagnostic explanation copied",
    noExplainableCode: "Vitte: diagnostic has no explainable code.",
    explainCommandCopied: "Vitte explain command copied",
    noDocumentationLink: "Vitte: diagnostic has no documentation link.",
    diagnosticPrefix: "Vitte diagnostic",
    externalHelpBadge: "external help",
  };
}

function isSeverityKey(value: unknown): value is SeverityKey {
  return typeof value === 'string' && value in VALID_SEVERITY_MAP;
}

function readConfig(): Required<VitteDiagnosticsConfig> {
  const cfg = vscode.workspace.getConfiguration('vitte').get<VitteDiagnosticsConfig>('diagnostics') ?? {};
  const severities = Array.isArray(cfg.severities)
    ? cfg.severities.filter(isSeverityKey)
    : [];
  const refreshDebounceMs = typeof cfg.refreshDebounceMs === 'number'
    ? Math.max(0, cfg.refreshDebounceMs)
    : DEFAULT_CFG.refreshDebounceMs;
  const displayMode: DisplayMode = cfg.displayMode === "detailed" ? "detailed" : "compact";
  return {
    severities,
    refreshDebounceMs,
    displayMode,
  };
}

function sevToName(s?: vscode.DiagnosticSeverity): 'error' | 'warning' | 'information' | 'hint' | 'unknown' {
  switch (s) {
    case vscode.DiagnosticSeverity.Error: return 'error';
    case vscode.DiagnosticSeverity.Warning: return 'warning';
    case vscode.DiagnosticSeverity.Information: return 'information';
    case vscode.DiagnosticSeverity.Hint: return 'hint';
    default: return 'unknown';
  }
}

function matchesFilter(s: vscode.DiagnosticSeverity | undefined, allowed: SeverityKey[]): boolean {
  if (allowed.length === 0) return true;
  const name = sevToName(s);
  return name !== 'unknown' && allowed.includes(name);
}

function countBySeverity(list: AggregatedDiagnostic[]) {
  let e=0, w=0, i=0, h=0;
  for (const d of list) {
    switch (d.diagnostic.severity) {
      case vscode.DiagnosticSeverity.Error: e++; break;
      case vscode.DiagnosticSeverity.Warning: w++; break;
      case vscode.DiagnosticSeverity.Information: i++; break;
      case vscode.DiagnosticSeverity.Hint: h++; break;
    }
  }
  return { e, w, i, h };
}

function debounce<T extends (...args: unknown[]) => void>(fn: T, ms: number): T {
  let tid: NodeJS.Timeout | undefined;
  const debounced = function(this: ThisParameterType<T>, ...args: Parameters<T>) {
    if (tid) clearTimeout(tid);
    tid = setTimeout(() => fn.apply(this, args), ms);
  };
  return debounced as T;
}

const SUPPORTED_EXTS = new Set([".vitte", ".vit"]);

export interface DiagnosticsView {
  readonly provider: DiagnosticsTreeDataProvider;
  readonly tree: vscode.TreeView<TreeNode> | undefined;
  refresh(): void;
}

type TreeNode = FileNode | DiagnosticNode;

interface AggregatedDiagnostic {
  uri: vscode.Uri;
  diagnostic: vscode.Diagnostic;
  index: number;
}

function openLineText(uri: vscode.Uri, line: number): string {
  const doc = vscode.workspace.textDocuments.find((d) => d.uri.toString() === uri.toString());
  if (!doc) return "";
  if (line < 0 || line >= doc.lineCount) return "";
  return doc.lineAt(line).text;
}

function syntaxSuggestion(entry: AggregatedDiagnostic): string | undefined {
  const rawCode = diagnosticCodeText(entry.diagnostic);
  const code = explainableCodeFromDiagnostic(entry.diagnostic) ?? rawCode;
  if (!code) return undefined;

  if (code === "E0007") {
    const lineText = openLineText(entry.uri, entry.diagnostic.range.start.line).trim();
    if (lineText === "}") {
      return "Suggestion: accolade fermante orpheline au top-level. Supprime `}` ou ferme le bloc précédent.";
    }
    return "Suggestion: place ce code dans `proc ... { ... }` ou `entry ... { ... }`, ou ajoute une déclaration top-level (`space`, `use`, `form`, `pick`, `type`, `const`, `proc`, `entry`).";
  }
  if (code === "E0001") return "Suggestion: ajoute un identifiant valide (lettres/chiffres/`_`, sans commencer par un chiffre).";
  if (code === "E0002") return "Suggestion: ajoute une expression valide (`1`, `name`, `call()`, `{ ... }`).";
  if (code === "E0003") return "Suggestion: ajoute un pattern valide (identifiant, constructeur, pattern tuple/liste).";
  if (code === "E0004") return "Suggestion: ajoute un type valide (`int`, `i32`, `string`, `bool`, `Option[T]`, ...).";
  if (code === "E0005") return "Suggestion: termine le bloc avec `.end`.";
  if (code === "E0006") return "Suggestion: un attribut doit être suivi d’un `proc`.";
  if (code === "E1016") return "Suggestion: import interne refusé. Importe la façade publique du module (`use vitte/<module> as <alias>`).";
  if (code === "E1017") return "Suggestion: conflit de re-export. Remplace les glob imports par des symboles explicites.";
  if (code === "E1018") return "Suggestion: chemin d’import ambigu. Garde soit la forme fichier soit la forme dossier, pas les deux.";
  if (/^VITTE-P\d{4}$/.test(code)) return "Suggestion: diagnostic process. Vérifie timeout/grace/policy/allowlist dans la config process.";
  return undefined;
}

function diagnosticCodeText(d: vscode.Diagnostic): string {
  const raw = d.code;
  if (typeof raw === "string" || typeof raw === "number") return String(raw).trim().toUpperCase();
  if (raw && typeof raw === "object" && "value" in raw) {
    const value = (raw as { value?: unknown }).value;
    if (typeof value === "string" || typeof value === "number") return String(value).trim().toUpperCase();
  }
  return "";
}

function vitteExplainHint(code: string): string {
  const u = uiStrings();
  return `${u.explain}: vitte --explain ${code}`;
}

function diagnosticDocUrl(code: string): string | undefined {
  if (/^E\d{4}$/.test(code) || /^VITTE-[A-Z]\d{4}$/.test(code)) {
    return `https://docs.vitte.dev/diagnostics/${code}`;
  }
  return undefined;
}

function diagnosticExplanationMessage(entry: AggregatedDiagnostic): string {
  const u = uiStrings();
  const pos = entry.diagnostic.range.start;
  const code = diagnosticCodeText(entry.diagnostic);
  const explainable = explainableCodeFromDiagnostic(entry.diagnostic);
  const source = entry.diagnostic.source ? `${u.source}: ${entry.diagnostic.source}` : u.sourceUnknown;
  const suggestion = syntaxSuggestion(entry) ?? "Suggestion: inspect the surrounding block and apply the closest Quick Fix.";
  const codePart = code ? `Code: ${code}\n` : "";
  const explainPart = explainable ? `\n${vitteExplainHint(explainable)}` : "";
  const docUrl = explainable ? diagnosticDocUrl(explainable) : undefined;
  const docPart = docUrl ? `\n${u.docs}: ${docUrl}` : "";
  return `${source}\n${codePart}${entry.uri.fsPath}:${pos.line + 1}:${pos.character + 1}\n\n${entry.diagnostic.message}\n\n${suggestion}${explainPart}${docPart}`;
}

function toAggregatedDiagnostic(arg: unknown): AggregatedDiagnostic | undefined {
  if (!arg || typeof arg !== "object") return undefined;
  const maybe = arg as { uri?: unknown; diagnostic?: unknown; index?: unknown };
  if (!(maybe.uri instanceof vscode.Uri)) return undefined;
  const diagnostic = maybe.diagnostic as vscode.Diagnostic | undefined;
  if (!diagnostic || !(diagnostic.range instanceof vscode.Range)) return undefined;
  return { uri: maybe.uri, diagnostic, index: typeof maybe.index === "number" ? maybe.index : 0 };
}

function currentDiagnosticEntry(): AggregatedDiagnostic | undefined {
  const active = vscode.window.activeTextEditor;
  if (!active) return undefined;
  const pos = active.selection.active;
  const hit = vscode.languages.getDiagnostics(active.document.uri)
    .find((d) => d.range.contains(pos) || d.range.start.line === pos.line);
  if (!hit) return undefined;
  return { uri: active.document.uri, diagnostic: hit, index: 0 };
}

function explainableCodeFromDiagnostic(diagnostic: vscode.Diagnostic): string | undefined {
  const code = diagnosticCodeText(diagnostic);
  const prefixed = /^([A-Z][A-Z0-9_-]*):(.*)$/.exec(code);
  const base = (prefixed?.[2] ?? code).trim().toUpperCase();
  if (/^E\d{4}$/.test(base) || /^VITTE-[A-Z]\d{4}$/.test(base)) return base;
  return undefined;
}

function explainCommandForEntry(entry: AggregatedDiagnostic): string | undefined {
  const code = explainableCodeFromDiagnostic(entry.diagnostic);
  if (!code) return undefined;
  const lang = vscode.workspace.getConfiguration("vitte").get<string>("lang", "en");
  return `vitte --explain ${code} --lang=${lang}`;
}

function diagnosticsDocUrlForEntry(entry: AggregatedDiagnostic): string | undefined {
  const code = explainableCodeFromDiagnostic(entry.diagnostic);
  return code ? diagnosticDocUrl(code) : undefined;
}

class FileNode extends vscode.TreeItem {
  constructor(
    public readonly uri: vscode.Uri,
    private readonly entries: AggregatedDiagnostic[],
    private readonly displayMode: DisplayMode,
    collState: vscode.TreeItemCollapsibleState
  ) {
    super(relativeLabel(uri), collState);
    const c = countBySeverity(entries);
    const parts: string[] = [];
    if (c.e) parts.push(`${c.e} erreur${c.e>1?'s':''}`);
    if (c.w) parts.push(`${c.w} avertissement${c.w>1?'s':''}`);
    if (c.i) parts.push(`${c.i} info${c.i>1?'s':''}`);
    if (c.h) parts.push(`${c.h} astuce${c.h>1?'s':''}`);
    if (displayMode === "detailed") {
      const total = `${entries.length} ${entries.length > 1 ? "problèmes" : "problème"}`;
      this.description = parts.length ? `${total} • ${parts.join(", ")}` : total;
    } else {
      this.description = `${entries.length}`;
    }
    this.contextValue = "vitteDiagnosticFile";
    this.iconPath = vscode.ThemeIcon.File;
  }

  get children(): DiagnosticNode[] {
    return this.entries.map(entry => new DiagnosticNode(entry, this.displayMode));
  }
}

class DiagnosticNode extends vscode.TreeItem {
  constructor(public readonly entry: AggregatedDiagnostic, displayMode: DisplayMode) {
    const u = uiStrings();
    super(formatDiagnosticNodeLabel(entry, displayMode), vscode.TreeItemCollapsibleState.None);
    const pos = entry.diagnostic.range.start;
    const severityName = sevToName(entry.diagnostic.severity);
    this.description = displayMode === "detailed"
      ? `${severityName} • L${pos.line + 1}:C${pos.character + 1}`
      : `L${pos.line + 1}:C${pos.character + 1}`;
    const parts = [
      entry.diagnostic.message,
      `${entry.uri.fsPath}:${pos.line + 1}:${pos.character + 1}`,
      entry.diagnostic.source ? `${u.source}: ${entry.diagnostic.source}` : ""
    ].filter(Boolean);
    const codeValue = diagnosticCodeText(entry.diagnostic);
    const codeText = codeValue ? `Code: ${codeValue}` : undefined;
    const suggestion = syntaxSuggestion(entry);
    const code = explainableCodeFromDiagnostic(entry.diagnostic);
    const explain = code ? vitteExplainHint(code) : undefined;
    const docUrl = code ? diagnosticDocUrl(code) : undefined;
    const extra = [severityName && `${u.level}: ${severityName}`, codeText, suggestion, explain, docUrl && `${u.docs}: ${docUrl}`].filter(Boolean).join('\n');
    this.tooltip = [parts.join('\n'), extra].filter(Boolean).join('\n');
    this.iconPath = iconForSeverity(entry.diagnostic.severity);
    this.command = {
      command: "vitte.diagnostics.explain",
      title: "Expliquer le diagnostic",
      arguments: [entry]
    };
    this.contextValue = "vitteDiagnostic";
  }
}

class DiagnosticsTreeDataProvider implements vscode.TreeDataProvider<TreeNode>, vscode.Disposable {
  private readonly onDidChangeEmitter = new vscode.EventEmitter<TreeNode | undefined>();
  readonly onDidChangeTreeData: vscode.Event<TreeNode | undefined> = this.onDidChangeEmitter.event;

  private nodes: FileNode[] = [];
  private cfg: Required<VitteDiagnosticsConfig> = DEFAULT_CFG;
  private schedule = debounce(() => this.refresh(), this.cfg.refreshDebounceMs);

  refresh(): void {
    this.cfg = readConfig();
    try {
      // rebuild nodes from diagnostics with filtering
      this.nodes = buildFileNodes(this.cfg);
    } catch (err) {
      this.nodes = [];
      const msg = err instanceof Error ? err.message : String(err);
      console.warn(`[vitte] diagnostics refresh failed: ${msg}`);
    }
    this.onDidChangeEmitter.fire(undefined);
  }

  refreshDebounced(): void {
    // re-create debouncer if delay changed
    this.schedule = debounce(() => this.refresh(), this.cfg.refreshDebounceMs);
    this.schedule();
  }

  hasItems(): boolean {
    return this.nodes.length > 0;
  }

  getTreeItem(element: TreeNode): vscode.TreeItem {
    return element;
  }

  getChildren(element?: TreeNode): vscode.ProviderResult<TreeNode[]> {
    if (!element) return this.nodes;
    if (element instanceof FileNode) return element.children;
    return [];
  }

  dispose(): void {
    this.onDidChangeEmitter.dispose();
  }
}

export function registerDiagnosticsView(context: vscode.ExtensionContext): DiagnosticsView {
  const provider = new DiagnosticsTreeDataProvider();
  let tree: vscode.TreeView<TreeNode> | undefined;
  try {
    tree = vscode.window.createTreeView<TreeNode>("vitteDiagnostics", { treeDataProvider: provider });
  } catch {
    // Keep diagnostics commands available even when the view contribution is not present.
  }

  const refresh = () => {
    const u = uiStrings();
    provider.refresh();
    if (!tree) return;
    if (provider.hasItems()) {
      tree.message = '';
    } else {
      tree.message = u.noDiagnostics;
    }
  };

  const refreshDebounced = () => {
    const u = uiStrings();
    provider.refreshDebounced();
    if (!tree) return;
    if (!provider.hasItems()) tree.message = u.noDiagnostics;
  };

  refresh();

  context.subscriptions.push(
    provider,
    vscode.languages.onDidChangeDiagnostics(refreshDebounced),
    vscode.workspace.onDidCloseTextDocument(refreshDebounced),
    vscode.workspace.onDidOpenTextDocument(refreshDebounced),
    vscode.workspace.onDidSaveTextDocument(refreshDebounced),
    vscode.commands.registerCommand("vitte.diagnostics.refresh", refresh),
    vscode.commands.registerCommand("vitte.diagnostics.open", async (arg?: unknown) => {
      const entry = toAggregatedDiagnostic(arg);
      if (!entry) return;
      const doc = await vscode.workspace.openTextDocument(entry.uri);
      const editor = await vscode.window.showTextDocument(doc, { preserveFocus: false, preview: true });
      const range = entry.diagnostic.range;
      editor.revealRange(range, vscode.TextEditorRevealType.InCenter);
      editor.selection = new vscode.Selection(range.start, range.start);
    }),
    vscode.commands.registerCommand('vitte.diagnostics.copy', async (arg?: unknown) => {
      const u = uiStrings();
      const entry = toAggregatedDiagnostic(arg);
      if (!entry) return;
      const pos = entry.diagnostic.range.start;
      const text = `${entry.uri.fsPath}:${pos.line + 1}:${pos.character + 1} — ${entry.diagnostic.message}`;
      await vscode.env.clipboard.writeText(text);
      void vscode.window.setStatusBarMessage(u.diagnosticCopied, 2000);
    }),
    vscode.commands.registerCommand("vitte.diagnostics.explain", async (arg?: unknown) => {
      const u = uiStrings();
      let entry = toAggregatedDiagnostic(arg);
      entry ??= currentDiagnosticEntry();
      if (!entry) {
        void vscode.window.showInformationMessage(u.noDiagnosticAtCursor);
        return;
      }
      const doc = await vscode.workspace.openTextDocument(entry.uri);
      const editor = await vscode.window.showTextDocument(doc, { preserveFocus: false, preview: true });
      const range = entry.diagnostic.range;
      editor.revealRange(range, vscode.TextEditorRevealType.InCenter);
      editor.selection = new vscode.Selection(range.start, range.start);
      const detail = diagnosticExplanationMessage(entry);
      void vscode.window.showInformationMessage(`${u.diagnosticPrefix}: ${entry.diagnostic.message}`, u.copyExplanation)
        .then(async (choice) => {
          if (choice !== u.copyExplanation) return;
          await vscode.env.clipboard.writeText(detail);
          void vscode.window.setStatusBarMessage(u.explanationCopied, 2000);
        });
    }),
    vscode.commands.registerCommand("vitte.diagnostics.copyExplainCommand", async (arg?: unknown) => {
      const u = uiStrings();
      let entry = toAggregatedDiagnostic(arg);
      entry ??= currentDiagnosticEntry();
      if (!entry) return;
      const command = explainCommandForEntry(entry);
      if (!command) {
        void vscode.window.showInformationMessage(u.noExplainableCode);
        return;
      }
      await vscode.env.clipboard.writeText(command);
      void vscode.window.setStatusBarMessage(u.explainCommandCopied, 2000);
    }),
    vscode.commands.registerCommand("vitte.diagnostics.openDoc", async (arg?: unknown) => {
      const u = uiStrings();
      let entry = toAggregatedDiagnostic(arg);
      entry ??= currentDiagnosticEntry();
      if (!entry) return;
      const docUrl = diagnosticsDocUrlForEntry(entry);
      if (!docUrl) {
        void vscode.window.showInformationMessage(u.noDocumentationLink);
        return;
      }
      await vscode.env.openExternal(vscode.Uri.parse(docUrl));
    }),
    vscode.workspace.onDidChangeConfiguration((e) => {
      if (!e.affectsConfiguration("vitte.diagnostics.displayMode") && !e.affectsConfiguration("vitte.lang")) return;
      refresh();
    }),
  );
  if (tree) {
    context.subscriptions.push(tree);
  }

  vscode.window.onDidChangeActiveTextEditor(refresh, undefined, context.subscriptions);

  return { provider, tree, refresh };
}

function buildFileNodes(cfg: Required<VitteDiagnosticsConfig>): FileNode[] {
  const entries = collectDiagnostics(cfg);
  const byFile = new Map<string, AggregatedDiagnostic[]>();
  for (const entry of entries) {
    const key = entry.uri.toString();
    const arr = byFile.get(key) ?? [];
    arr.push(entry);
    byFile.set(key, arr);
  }

  const perFile = Array.from(byFile.values())
    .map(list => list.slice().sort(compareDiagnostics))
    .filter(list => list.length > 0);

  perFile.sort(compareFiles);

  return perFile.map(list => {
    const [head] = list;
    if (!head) {
      throw new Error('Invariant: diagnostics list is empty');
    }
    return new FileNode(head.uri, list, cfg.displayMode, vscode.TreeItemCollapsibleState.Expanded);
  });
}

function collectDiagnostics(cfg: Required<VitteDiagnosticsConfig>): AggregatedDiagnostic[] {
  const all = vscode.languages.getDiagnostics();
  const collected: AggregatedDiagnostic[] = [];
  for (const [uri, diagnostics] of all) {
    if (uri.scheme !== 'file') continue;
    if (!SUPPORTED_EXTS.has(path.extname(uri.fsPath))) continue;
    if (!Array.isArray(diagnostics)) continue;
    diagnostics.forEach((diagnostic, index) => {
      if (!matchesFilter(diagnostic.severity, cfg.severities)) return;
      collected.push({ uri, diagnostic, index });
    });
  }
  return collected;
}

function compareDiagnostics(a: AggregatedDiagnostic, b: AggregatedDiagnostic): number {
  const severityDiff = (a.diagnostic.severity ?? vscode.DiagnosticSeverity.Information) -
    (b.diagnostic.severity ?? vscode.DiagnosticSeverity.Information);
  if (severityDiff !== 0) return severityDiff;
  const lineDiff = a.diagnostic.range.start.line - b.diagnostic.range.start.line;
  if (lineDiff !== 0) return lineDiff;
  return a.diagnostic.range.start.character - b.diagnostic.range.start.character;
}

function compareFiles(
  a: readonly AggregatedDiagnostic[] | undefined,
  b: readonly AggregatedDiagnostic[] | undefined,
): number {
  const headA = firstEntry(a);
  const headB = firstEntry(b);
  const sevA = severityOrder(headA?.diagnostic.severity);
  const sevB = severityOrder(headB?.diagnostic.severity);
  if (sevA !== sevB) return sevA - sevB;
  const countA = a?.length ?? 0;
  const countB = b?.length ?? 0;
  if (countA !== countB) return countB - countA;
  const uriA = headA?.uri;
  const uriB = headB?.uri;
  return relativeLabel(uriA).localeCompare(relativeLabel(uriB));
}

function firstEntry(
  list: readonly AggregatedDiagnostic[] | undefined,
): AggregatedDiagnostic | undefined {
  if (!list || list.length === 0) return undefined;
  return list[0];
}

function relativeLabel(uri: vscode.Uri | undefined): string {
  if (!uri) return "";
  const rel = vscode.workspace.asRelativePath(uri, false);
  return rel || uri.fsPath;
}

function iconForSeverity(severity: vscode.DiagnosticSeverity | undefined): vscode.ThemeIcon {
  const makeIcon = (id: string): vscode.ThemeIcon => ({ id } as vscode.ThemeIcon);
  switch (severity) {
    case vscode.DiagnosticSeverity.Error:
      return makeIcon("error");
    case vscode.DiagnosticSeverity.Warning:
      return makeIcon("warning");
    case vscode.DiagnosticSeverity.Information:
      return makeIcon("info");
    case vscode.DiagnosticSeverity.Hint:
      return makeIcon("lightbulb");
    default:
      return makeIcon("question");
  }
}

function severityOrder(severity: vscode.DiagnosticSeverity | undefined): number {
  switch (severity) {
    case vscode.DiagnosticSeverity.Error: return 0;
    case vscode.DiagnosticSeverity.Warning: return 1;
    case vscode.DiagnosticSeverity.Information: return 2;
    case vscode.DiagnosticSeverity.Hint: return 3;
    default: return 4;
  }
}

function compactText(text: string, maxLen = 100): string {
  if (text.length <= maxLen) return text;
  return `${text.slice(0, Math.max(0, maxLen - 1)).trimEnd()}…`;
}

function hasExternalHelpBadge(diagnostic: vscode.Diagnostic): boolean {
  return /\b(?:help-source|source-aide):\s*external\b/i.test(diagnostic.message);
}

function formatDiagnosticNodeLabel(entry: AggregatedDiagnostic, displayMode: DisplayMode): string {
  const codeValue = diagnosticCodeText(entry.diagnostic);
  const prefix = codeValue ? `[${codeValue}] ` : "";
  const message = displayMode === "detailed"
    ? entry.diagnostic.message
    : compactText(entry.diagnostic.message, 100);
  const badge = hasExternalHelpBadge(entry.diagnostic) ? ` [${uiStrings().externalHelpBadge}]` : "";
  return `${prefix}${message}${badge}`;
}
