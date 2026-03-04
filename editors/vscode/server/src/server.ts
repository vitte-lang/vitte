// server.ts — Vitte/Vit hardened LSP server
// Goals:
// - Full capabilities: completion, hover, symbols, def/refs, rename, formatting, diagnostics, semantic tokens
// - Dynamic config management + watchers + graceful shutdown
// - Lint with debounce, size guards, cancellation
// - Controlled logging, lightweight metrics, pervasive try/catch

import {
  createConnection,
  ProposedFeatures,
  TextDocumentSyncKind,
  TextDocuments,
  TextEdit,
  Location,
  Position,
  Range,
  DidChangeConfigurationNotification,
  FileChangeType,
} from "vscode-languageserver/node";
import type {
  Connection,
  InitializeParams,
  InitializeResult,
  CompletionParams,
  CompletionItem,
  HoverParams,
  Hover,
  DocumentSymbolParams,
  DocumentSymbol,
  DocumentFormattingParams,
  DocumentRangeFormattingParams,
  SemanticTokens,
  SemanticTokensParams,
  SemanticTokensLegend,
  WorkspaceSymbolParams,
  WorkspaceSymbol,
  DefinitionParams,
  ReferenceParams,
  RenameParams,
  PrepareRenameParams,
  WorkspaceEdit,
  CodeActionParams,
  CodeAction,
  Diagnostic,
  CallHierarchyPrepareParams,
  CallHierarchyIncomingCallsParams,
  CallHierarchyOutgoingCallsParams,
  CallHierarchyItem,
  CallHierarchyIncomingCall,
  CallHierarchyOutgoingCall,
  TypeHierarchyPrepareParams,
  TypeHierarchySupertypesParams,
  TypeHierarchySubtypesParams,
  TypeHierarchyItem,
  InlayHint,
  InlayHintParams,
  CancellationToken,
  RemoteWorkspace,
} from "vscode-languageserver/node";
import { TextDocument } from "vscode-languageserver-textdocument";

/* Modules internes */
import { provideCompletions, resolveCompletion, triggerCharacters } from "./completion.js";
import {
  documentSymbols,
  definitionAtPosition,
  referencesAtPosition,
  renameSymbol,
  renameIdentifierByName,
  tokenAtPosition,
  prepareRename,
  workspaceSymbols,
  prepareCallHierarchy,
  callHierarchyIncoming,
  callHierarchyOutgoing,
  prepareTypeHierarchy,
  typeHierarchySupertypes,
  typeHierarchySubtypes,
} from "./navigation.js";
import { provideFormattingEdits, provideRangeFormattingEdits } from "./formatting.js";
import { getSemanticTokensLegend, buildSemanticTokens, provideHover } from "./semantic.js";
import { provideInlayHints, type InlayHintPrefs } from "./inlay.js";
import { lintToPublishable } from "./lint.js";
import { registerCommands } from "./commands.js";
import { indexDocument as indexWorkspaceDocument, removeDocument as removeWorkspaceDocument, clearIndex, exportIndexSnapshot, loadIndexSnapshot, getIndex, type IndexSnapshot } from "./indexer.js";
import * as fs from "node:fs/promises";
import * as path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import type { Dirent } from "node:fs";

const createSemanticTokens: (doc: TextDocument) => SemanticTokens = buildSemanticTokens;

interface MetricAggregate {
  count: number;
  totalMs: number;
  maxMs: number;
  lastMs: number;
  lastAt: number;
  lastUri: string;
  lastCount?: number;
  errorCount: number;
  lastError?: string;
  samples: number[];
}

const metricStore = new Map<string, MetricAggregate>();

/* --------------------------- Connection + documents ----------------------- */
const connection: Connection = createConnection(ProposedFeatures.all);
const documents = new TextDocuments<TextDocument>(TextDocument);
function emptySemanticTokens(): SemanticTokens {
  return { data: [] };
}

/* ------------------------------- Configuration --------------------------- */
interface ServerSettings {
  trace: "off" | "messages" | "verbose";
  traceSampleRate: number;
  lintDebounceMs: number;
  enableFormatting: boolean;
  maxFileSizeKB: number; // above this limit, skip lint + semantic tokens to preserve perf
  requestTimeoutMs: number;
  requestTimeouts: {
    completion?: number;
    hover?: number;
    definition?: number;
    references?: number;
    rename?: number;
    documentSymbols?: number;
    workspaceSymbols?: number;
    semanticTokens?: number;
    formatting?: number;
    inlayHints?: number;
  };
  requestMaxConcurrent: number;
  indexerMaxRssMB: number;
  indexerCacheEnabled: boolean;
  lint: {
    maxLineLength?: number;
    allowTabs?: boolean;
    allowTrailingWhitespace?: boolean;
    enableStyleRules?: boolean;
  };
  inlayHints: {
    parameterHints?: boolean;
    typeHints?: boolean;
    returnHints?: boolean;
    aliasHints?: boolean;
  };
  features: {
    completion: boolean;
    hover: boolean;
    definition: boolean;
    references: boolean;
    rename: boolean;
    documentSymbols: boolean;
    workspaceSymbols: boolean;
    semanticTokens: boolean;
    formatting: boolean;
    lint: boolean;
    inlayHints: boolean;
  };
}

const DEFAULT_SETTINGS: ServerSettings = {
  trace: "off",
  traceSampleRate: 0.01,
  lintDebounceMs: 200,
  enableFormatting: true,
  maxFileSizeKB: 1024, // 1 MB
  requestTimeoutMs: 800,
  requestTimeouts: {},
  requestMaxConcurrent: 4,
  indexerMaxRssMB: 1024,
  indexerCacheEnabled: true,
  lint: {},
  inlayHints: {
    parameterHints: true,
    typeHints: true,
    returnHints: true,
    aliasHints: true,
  },
  features: {
    completion: true,
    hover: true,
    definition: true,
    references: true,
    rename: true,
    documentSymbols: true,
    workspaceSymbols: true,
    semanticTokens: true,
    formatting: true,
    lint: true,
    inlayHints: true,
  },
};

let globalSettings: ServerSettings = { ...DEFAULT_SETTINGS };
let hasConfigurationCapability = false;
let hasWorkspaceFoldersCapability = false;
let workspaceRoot: string | undefined;
let workspaceRoots: string[] | undefined;

connection.onInitialize((params: InitializeParams): InitializeResult => {
  const capabilities = params.capabilities;
  hasConfigurationCapability = !!capabilities.workspace?.configuration;
  hasWorkspaceFoldersCapability = !!capabilities.workspace?.workspaceFolders;

  const legend: SemanticTokensLegend = getSemanticTokensLegend();

  const result: InitializeResult = {
    capabilities: {
      textDocumentSync: TextDocumentSyncKind.Incremental,
      // Enable resolve to enrich completion items lazily and use centralized trigger characters
      completionProvider: { resolveProvider: true, triggerCharacters: triggerCharacters() },
      hoverProvider: true,
      documentSymbolProvider: true,
      documentFormattingProvider: true,
      documentRangeFormattingProvider: true,
      definitionProvider: true,
      referencesProvider: true,
      renameProvider: { prepareProvider: true },
      workspaceSymbolProvider: true,
      codeActionProvider: {
        codeActionKinds: ["quickfix", "source.fixAll", "source.organizeImports", "refactor.rewrite"],
      },
      semanticTokensProvider: { legend, full: true, range: false },
      inlayHintProvider: true,
      callHierarchyProvider: true,
      typeHierarchyProvider: true,
    },
  };

  if (hasWorkspaceFoldersCapability) {
    result.capabilities.workspace = { workspaceFolders: { supported: true } };
  }
  return result;
});

connection.onInitialized(() => {
  registerCommands(connection);
  if (hasConfigurationCapability) {
    void connection.client.register(DidChangeConfigurationNotification.type, undefined);
  }
  // Watch classiques pour compat large versions du client
  connection.onDidChangeWatchedFiles((change) => {
    for (const ev of change.changes) {
      if (ev.type === FileChangeType.Deleted) {
        void connection.sendDiagnostics({ uri: ev.uri, diagnostics: [] });
      }
    }
  });
  void loadIndexCache();
});

connection.onRequest("vitte/metrics", () => {
  const snapshot = Array.from(metricStore.entries()).map(([name, data]) => ({
    name,
    count: data.count,
    averageMs: data.count > 0 ? data.totalMs / data.count : 0,
    maxMs: data.maxMs,
    lastMs: data.lastMs,
    lastAt: data.lastAt,
    lastUri: data.lastUri,
    lastCount: data.lastCount ?? null,
    p95Ms: percentile(data.samples, 95),
    p99Ms: percentile(data.samples, 99),
    errorCount: data.errorCount,
    lastError: data.lastError ?? null,
  }));
  snapshot.sort((a, b) => b.averageMs - a.averageMs);
  return snapshot;
});

connection.onRequest("vitte/metrics.reset", () => {
  metricStore.clear();
  return { ok: true };
});

connection.onRequest("vitte/ping", () => {
  return { ok: true, ts: Date.now() };
});

interface SymbolGraphNode {
  id: string;
  name: string;
  kind: number;
  uri: string;
  line: number;
  character: number;
  containerName?: string;
}

interface SymbolGraphEdge {
  from: string;
  to: string;
  type: "contains";
}

interface SymbolGraphPayload {
  schemaVersion: 1;
  generatedAt: string;
  stats: {
    documents: number;
    symbols: number;
    edges: number;
  };
  nodes: SymbolGraphNode[];
  edges: SymbolGraphEdge[];
  truncated: boolean;
}

connection.onRequest("vitte/symbolGraph", (params?: unknown): SymbolGraphPayload => {
  const maxNodes = readNumberField(params, "maxNodes", 40000, 5000, 200000);
  const index = getIndex();
  const nodes: SymbolGraphNode[] = [];
  const byUriName = new Map<string, string[]>();
  let truncated = false;
  let documents = 0;

  for (const [uri, symbols] of index.entries()) {
    documents += 1;
    for (const s of symbols) {
      if (nodes.length >= maxNodes) {
        truncated = true;
        break;
      }
      const id = `${uri}#${s.line}:${s.character}:${s.kind}:${s.name}`;
      const node: SymbolGraphNode = {
        id,
        name: s.name,
        kind: s.kind,
        uri,
        line: s.line,
        character: s.character,
      };
      if (s.containerName) node.containerName = s.containerName;
      nodes.push(node);
      const key = `${uri}::${s.name}`;
      const list = byUriName.get(key);
      if (list) list.push(id);
      else byUriName.set(key, [id]);
    }
    if (truncated) break;
  }

  const edges: SymbolGraphEdge[] = [];
  for (const n of nodes) {
    if (!n.containerName) continue;
    const key = `${n.uri}::${n.containerName}`;
    const candidates = byUriName.get(key);
    if (!candidates || candidates.length === 0) continue;
    edges.push({ from: candidates[0], to: n.id, type: "contains" });
  }

  return {
    schemaVersion: 1,
    generatedAt: new Date().toISOString(),
    stats: {
      documents,
      symbols: nodes.length,
      edges: edges.length,
    },
    nodes,
    edges,
    truncated,
  };
});

connection.onShutdown(() => {
  for (const t of lintTimers.values()) clearTimeout(t);
  lintTimers.clear();
  void saveIndexCache();
  clearIndex();
});

/* ------------------------------- Config updates --------------------------- */
async function applyConfiguration(): Promise<void> {
  const workspace = Reflect.get(connection, "workspace") as RemoteWorkspace | undefined;
  if (hasConfigurationCapability && workspace) {
    try {
      const cfg = await workspace.getConfiguration({ section: "vitte" }) as Partial<ServerSettings> | null | undefined;
      const lintCfg = await workspace.getConfiguration({ section: "vitte.lint" }) as Partial<ServerSettings["lint"]> | null | undefined;
      const merged: ServerSettings = { ...DEFAULT_SETTINGS, ...(cfg ?? {}) };
      merged.lint = { ...DEFAULT_SETTINGS.lint, ...(lintCfg ?? {}) };
      merged.features = { ...DEFAULT_SETTINGS.features, ...(cfg?.features ?? {}) };
      merged.inlayHints = { ...DEFAULT_SETTINGS.inlayHints, ...(cfg?.inlayHints ?? {}) };
      merged.lintDebounceMs = clampNumber(merged.lintDebounceMs, 0, 2000, DEFAULT_SETTINGS.lintDebounceMs);
      merged.maxFileSizeKB = clampNumber(merged.maxFileSizeKB, 64, 10000, DEFAULT_SETTINGS.maxFileSizeKB);
      merged.requestTimeoutMs = clampNumber(merged.requestTimeoutMs, 100, 5000, DEFAULT_SETTINGS.requestTimeoutMs);
      merged.traceSampleRate = clampNumber(merged.traceSampleRate, 0, 1, DEFAULT_SETTINGS.traceSampleRate);
      merged.requestMaxConcurrent = clampNumber(merged.requestMaxConcurrent, 1, 32, DEFAULT_SETTINGS.requestMaxConcurrent);
      merged.indexerMaxRssMB = clampNumber(merged.indexerMaxRssMB, 256, 8192, DEFAULT_SETTINGS.indexerMaxRssMB);
      merged.indexerCacheEnabled = cfg?.indexerCacheEnabled ?? DEFAULT_SETTINGS.indexerCacheEnabled;
      merged.requestTimeouts = { ...DEFAULT_SETTINGS.requestTimeouts, ...(cfg?.requestTimeouts ?? {}) };
      globalSettings = merged;
    } catch {
      globalSettings = { ...DEFAULT_SETTINGS };
    }
  } else {
    globalSettings = { ...DEFAULT_SETTINGS };
  }
  for (const doc of documents.all()) scheduleLint(doc);
}

connection.onDidChangeConfiguration(() => {
  void applyConfiguration();
});

/* --------------------------------- Guards -------------------------------- */
function tooLarge(doc: TextDocument): boolean {
  const kb = Buffer.byteLength(doc.getText(), "utf8") / 1024;
  return kb > (globalSettings.maxFileSizeKB | 0);
}

function cancelled(token?: CancellationToken): boolean { return !!token?.isCancellationRequested; }

async function resolveWorkspaceRoot(): Promise<string | undefined> {
  if (workspaceRoot) return workspaceRoot;
  try {
    const folders = await connection.workspace.getWorkspaceFolders();
    if (!folders || folders.length === 0) return undefined;
    const first = folders[0].uri;
    if (first.startsWith("file://")) {
      workspaceRoot = fileURLToPath(first);
      return workspaceRoot;
    }
  } catch {
    return undefined;
  }
  return undefined;
}

async function resolveWorkspaceRoots(): Promise<string[]> {
  if (workspaceRoots && workspaceRoots.length > 0) return workspaceRoots;
  try {
    const folders = await connection.workspace.getWorkspaceFolders();
    if (!folders || folders.length === 0) return [];
    workspaceRoots = folders
      .map((f) => (f.uri.startsWith("file://") ? fileURLToPath(f.uri) : ""))
      .filter((p) => !!p);
    if (!workspaceRoot && workspaceRoots.length > 0) workspaceRoot = workspaceRoots[0];
    return workspaceRoots;
  } catch {
    return workspaceRoot ? [workspaceRoot] : [];
  }
}

async function getIndexCachePath(): Promise<string | undefined> {
  const root = await resolveWorkspaceRoot();
  if (!root) return undefined;
  return path.join(root, ".vitte", "index-cache.json");
}

async function getIndexCachePaths(): Promise<string[]> {
  const roots = await resolveWorkspaceRoots();
  if (roots.length === 0) {
    const one = await getIndexCachePath();
    return one ? [one] : [];
  }
  return roots.map((r) => path.join(r, ".vitte", "index-cache.json"));
}

async function resolveInlayPrefs(uri: string): Promise<Partial<InlayHintPrefs>> {
  const workspace = Reflect.get(connection, "workspace") as RemoteWorkspace | undefined;
  if (!workspace || !hasConfigurationCapability) {
    return { ...globalSettings.inlayHints };
  }
  try {
    const scoped = await workspace.getConfiguration({ scopeUri: uri, section: "vitte.inlayHints" }) as Partial<InlayHintPrefs> | null | undefined;
    return {
      parameterHints: scoped?.parameterHints ?? globalSettings.inlayHints.parameterHints ?? true,
      typeHints: scoped?.typeHints ?? globalSettings.inlayHints.typeHints ?? true,
      returnHints: scoped?.returnHints ?? globalSettings.inlayHints.returnHints ?? true,
      aliasHints: scoped?.aliasHints ?? globalSettings.inlayHints.aliasHints ?? true,
    };
  } catch {
    return { ...globalSettings.inlayHints };
  }
}

function isIndexSnapshot(value: unknown): value is IndexSnapshot {
  if (!value || typeof value !== "object") return false;
  const v = value as { version?: unknown; entries?: unknown };
  return v.version === 2 && Array.isArray(v.entries);
}

async function loadIndexCache(): Promise<void> {
  if (!globalSettings.indexerCacheEnabled) return;
  const cachePaths = await getIndexCachePaths();
  if (cachePaths.length === 0) return;
  const mergedEntries: IndexSnapshot["entries"] = [];
  for (const cachePath of cachePaths) {
    try {
      const raw = await fs.readFile(cachePath, "utf8");
      const parsed = JSON.parse(raw) as unknown;
      const snapshot: IndexSnapshot | null = isIndexSnapshot(parsed) ? parsed : null;
      if (!snapshot) continue;
      mergedEntries.push(...snapshot.entries);
      connection.console.log(`[index] loaded cache ${cachePath} (${snapshot.entries.length} files)`);
    } catch {
      // ignore cache errors
    }
  }
  if (mergedEntries.length > 0) {
    const unique = new Map<string, IndexSnapshot["entries"][number]>();
    for (const e of mergedEntries) unique.set(e.uri, e);
    const count = loadIndexSnapshot({ version: 2, entries: Array.from(unique.values()) });
    connection.console.log(`[index] merged cache (${count} files)`);
  }
}

async function saveIndexCache(): Promise<void> {
  if (!globalSettings.indexerCacheEnabled) return;
  const cachePaths = await getIndexCachePaths();
  if (cachePaths.length === 0) return;
  for (const cachePath of cachePaths) {
    try {
      await fs.mkdir(path.dirname(cachePath), { recursive: true });
      const snapshot = exportIndexSnapshot();
      await fs.writeFile(cachePath, JSON.stringify(snapshot), "utf8");
    } catch {
      // ignore cache errors
    }
  }
}

/* --------------------------------- Handlers ------------------------------- */

const breaker = { consecutiveErrors: 0, until: 0 };
const BREAKER_THRESHOLD = 5;
const BREAKER_COOLDOWN_MS = 30000;
const requestQueue: (() => void)[] = [];
let activeRequests = 0;

function isBreakerActive(): boolean {
  return Date.now() < breaker.until;
}

function recordSuccess(): void {
  breaker.consecutiveErrors = 0;
}

function recordFailure(): void {
  breaker.consecutiveErrors += 1;
  if (breaker.consecutiveErrors >= BREAKER_THRESHOLD) {
    breaker.until = Date.now() + BREAKER_COOLDOWN_MS;
  }
}

function featureEnabled(name: keyof ServerSettings["features"]): boolean {
  return Boolean(globalSettings.features?.[name]);
}

async function withTimeout<T>(task: Promise<T>, ms: number): Promise<T> {
  if (ms <= 0) return task;
  let tid: NodeJS.Timeout | undefined;
  const timeout = new Promise<never>((_, reject) => {
    tid = setTimeout(() => reject(new Error("Request timed out")), ms);
  });
  try {
    return await Promise.race([task, timeout]);
  } finally {
    if (tid) clearTimeout(tid);
  }
}

function timeoutFor(feature: keyof ServerSettings["requestTimeouts"]): number {
  const per = globalSettings.requestTimeouts?.[feature];
  if (typeof per === "number" && Number.isFinite(per) && per > 0) return per;
  return globalSettings.requestTimeoutMs;
}

async function withBackpressure<T>(fn: () => Promise<T>): Promise<T> {
  const max = Math.max(1, globalSettings.requestMaxConcurrent | 0);
  if (activeRequests >= max) {
    return new Promise<T>((resolve, reject) => {
      requestQueue.push(() => {
        void withBackpressure(fn).then(resolve, reject);
      });
    });
  }
  activeRequests += 1;
  try {
    return await fn();
  } finally {
    activeRequests -= 1;
    if (requestQueue.length && activeRequests < max) {
      const next = requestQueue.shift();
      if (next) next();
    }
  }
}

function failFast(feature: keyof ServerSettings["features"]): boolean {
  if (featureEnabled(feature)) return false;
  connection.console.warn(`[disabled] ${feature} is disabled by configuration`);
  return true;
}

function memoryExceeded(): boolean {
  const rss = process.memoryUsage().rss / (1024 * 1024);
  return rss > globalSettings.indexerMaxRssMB;
}

function readNumberField(
  input: unknown,
  field: string,
  fallback: number,
  min: number,
  max: number
): number {
  if (!input || typeof input !== "object") return fallback;
  const value = Reflect.get(input, field);
  if (typeof value !== "number" || !Number.isFinite(value)) return fallback;
  return Math.min(max, Math.max(min, Math.trunc(value)));
}

connection.onCompletion(async (params: CompletionParams, token?: CancellationToken): Promise<CompletionItem[]> => {
  if (failFast("completion")) return [];
  if (isBreakerActive()) return [];
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return [];
    try {
      const result = await withTimeout(Promise.resolve(provideCompletions(doc, params.position)), timeoutFor("completion"));
      metric("completion", start, doc.uri, result.length);
      recordSuccess();
      return result;
    } catch (e) {
      metric("completion", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("completion", e);
      return [];
    }
  });
});

// Allow the client to resolve/enrich completion items on demand (details, docs)
connection.onCompletionResolve(async (item: CompletionItem): Promise<CompletionItem> => {
  if (failFast("completion")) return item;
  if (isBreakerActive()) return item;
  return withBackpressure(async () => {
    const start = now();
    try {
      const resolved = await withTimeout(Promise.resolve(resolveCompletion(item)), timeoutFor("completion"));
      metric("completionResolve", start, "completionResolve");
      recordSuccess();
      return resolved;
    } catch (e) {
      metric("completionResolve", start, "completionResolve", undefined, e);
      recordFailure();
      logErr("completionResolve", e);
      return item;
    }
  });
});

connection.onHover(async (params: HoverParams, token?: CancellationToken): Promise<Hover | null> => {
  if (failFast("hover")) return null;
  if (isBreakerActive()) return null;
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return null;
    try {
      const hover = await withTimeout(Promise.resolve(provideHover(doc, params.position)), timeoutFor("hover"));
      metric("hover", start, doc.uri);
      recordSuccess();
      return hover;
    } catch (e) {
      metric("hover", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("hover", e);
      return null;
    }
  });
});

connection.onDocumentSymbol(async (params: DocumentSymbolParams, token?: CancellationToken): Promise<DocumentSymbol[]> => {
  if (failFast("documentSymbols")) return [];
  if (isBreakerActive()) return [];
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return [];
    try {
      const symbols = await withTimeout(Promise.resolve(documentSymbols(doc)), timeoutFor("documentSymbols"));
      metric("documentSymbols", start, doc.uri, symbols.length);
      recordSuccess();
      return symbols;
    } catch (e) {
      metric("documentSymbols", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("documentSymbols", e);
      return [];
    }
  });
});

connection.onDocumentFormatting(async (params: DocumentFormattingParams, token?: CancellationToken) => {
  if (!globalSettings.enableFormatting) return [];
  if (failFast("formatting")) return [];
  if (isBreakerActive()) return [];
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return [];
    try {
      const edits = await withTimeout(Promise.resolve(provideFormattingEdits(doc, params.options)), timeoutFor("formatting"));
      metric("formatting", start, doc.uri, edits.length);
      recordSuccess();
      return edits;
    } catch (e) {
      metric("formatting", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("formatting", e);
      return [];
    }
  });
});

connection.onDocumentRangeFormatting(async (params: DocumentRangeFormattingParams, token?: CancellationToken) => {
  if (!globalSettings.enableFormatting) return [];
  if (failFast("formatting")) return [];
  if (isBreakerActive()) return [];
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return [];
    try {
      const edits = await withTimeout(
        Promise.resolve(provideRangeFormattingEdits(doc, params.range, params.options)),
        timeoutFor("formatting")
      );
      metric("rangeFormatting", start, doc.uri, edits.length);
      recordSuccess();
      return edits;
    } catch (e) {
      metric("rangeFormatting", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("rangeFormatting", e);
      return [];
    }
  });
});

connection.onCodeAction(async (params: CodeActionParams): Promise<CodeAction[]> => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return [];
  const text = doc.getText();
  const actions: CodeAction[] = [];

  const mkAction = (title: string, kind: string, edit: WorkspaceEdit): CodeAction => ({
    title,
    kind,
    edit,
    diagnostics: params.context.diagnostics,
  });

  const organizeEdit = computeOrganizeImportsEdit(doc, text);
  if (organizeEdit) {
    actions.push(
      mkAction("Vitte: Fix imports (dedupe + sort)", "source.fixAll", organizeEdit),
      mkAction("Vitte: Organize imports", "source.organizeImports", organizeEdit)
    );
  }

  const lineNo = params.range.start.line;
  const lineRange = Range.create(
    Position.create(lineNo, 0),
    Position.create(lineNo, Number.MAX_SAFE_INTEGER)
  );
  const lineText = doc.getText(lineRange);

  const convertedLine = convertUsePullLine(lineText);
  if (convertedLine && convertedLine !== lineText) {
    actions.push(mkAction("Vitte: Convert use/pull", "refactor.rewrite", {
      changes: { [doc.uri]: [TextEdit.replace(lineRange, convertedLine)] },
    }));
  }

  const aliasLine = addPkgAliasLine(lineText);
  if (aliasLine && aliasLine !== lineText) {
    actions.push(mkAction("Vitte: Add alias *_pkg", "quickfix", {
      changes: { [doc.uri]: [TextEdit.replace(lineRange, aliasLine)] },
    }));
  }

  if (!/^\s*<<<\s+ROLE-CONTRACT\b/m.test(text)) {
    const insertion = buildRoleContractTemplate(text);
    if (insertion) {
      const insertAt = doc.positionAt(text.length);
      actions.push(mkAction("Vitte: Add ROLE-CONTRACT block", "refactor.rewrite", {
        changes: { [doc.uri]: [TextEdit.insert(insertAt, insertion)] },
      }));
    }
  }

  // Diagnostic-driven quick fixes
  for (const d of params.context.diagnostics ?? []) {
    const qf = quickFixForDiagnostic(doc, d);
    if (qf) actions.push(qf);
  }

  return actions;
});

connection.onDefinition(async (params: DefinitionParams, token?: CancellationToken): Promise<Location[]> => {
  if (failFast("definition")) return [];
  if (isBreakerActive()) return [];
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return [];
    try {
      const defs = await withTimeout(Promise.resolve(definitionAtPosition(doc, params.position, params.textDocument.uri)), timeoutFor("definition"));
      const tok = tokenAtPosition(doc, params.position);
      const merged = tok ? dedupeLocations([
        ...defs,
        ...collectOpenDocumentDefinitions(tok, params.textDocument.uri),
      ]) : defs;
      metric("definition", start, doc.uri, merged.length);
      recordSuccess();
      return merged;
    } catch (e) {
      metric("definition", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("definition", e);
      return [];
    }
  });
});

connection.onReferences(async (params: ReferenceParams, token?: CancellationToken): Promise<Location[]> => {
  if (failFast("references")) return [];
  if (isBreakerActive()) return [];
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return [];
    try {
      const refs = await withTimeout(Promise.resolve(referencesAtPosition(doc, params.position, params.textDocument.uri)), timeoutFor("references"));
      const tok = tokenAtPosition(doc, params.position);
      const merged = tok ? dedupeLocations([
        ...refs,
        ...collectOpenDocumentReferences(tok, params.textDocument.uri),
      ]) : refs;
      metric("references", start, doc.uri, merged.length);
      recordSuccess();
      return merged;
    } catch (e) {
      metric("references", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("references", e);
      return [];
    }
  });
});

connection.languages.callHierarchy.onPrepare(async (params: CallHierarchyPrepareParams, token?: CancellationToken): Promise<CallHierarchyItem[] | null> => {
  if (isBreakerActive()) return null;
  return withBackpressure(async () => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return null;
    try {
      return prepareCallHierarchy(doc, params.position, params.textDocument.uri);
    } catch (e) {
      logErr("callHierarchy.prepare", e);
      return null;
    }
  });
});

connection.languages.callHierarchy.onIncomingCalls(async (params: CallHierarchyIncomingCallsParams, token?: CancellationToken): Promise<CallHierarchyIncomingCall[] | null> => {
  if (isBreakerActive()) return null;
  return withBackpressure(async () => {
    const doc = documents.get(params.item.uri);
    if (!doc || cancelled(token)) return null;
    try {
      return callHierarchyIncoming(doc, params.item);
    } catch (e) {
      logErr("callHierarchy.incoming", e);
      return null;
    }
  });
});

connection.languages.callHierarchy.onOutgoingCalls(async (params: CallHierarchyOutgoingCallsParams, token?: CancellationToken): Promise<CallHierarchyOutgoingCall[] | null> => {
  if (isBreakerActive()) return null;
  return withBackpressure(async () => {
    const doc = documents.get(params.item.uri);
    if (!doc || cancelled(token)) return null;
    try {
      return callHierarchyOutgoing(doc, params.item);
    } catch (e) {
      logErr("callHierarchy.outgoing", e);
      return null;
    }
  });
});

connection.languages.typeHierarchy.onPrepare(async (params: TypeHierarchyPrepareParams, token?: CancellationToken): Promise<TypeHierarchyItem[] | null> => {
  if (isBreakerActive()) return null;
  return withBackpressure(async () => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return null;
    try {
      return prepareTypeHierarchy(doc, params.position, params.textDocument.uri);
    } catch (e) {
      logErr("typeHierarchy.prepare", e);
      return null;
    }
  });
});

connection.languages.typeHierarchy.onSupertypes(async (params: TypeHierarchySupertypesParams, token?: CancellationToken): Promise<TypeHierarchyItem[] | null> => {
  if (isBreakerActive()) return null;
  return withBackpressure(async () => {
    const doc = documents.get(params.item.uri);
    if (!doc || cancelled(token)) return null;
    try {
      return typeHierarchySupertypes(doc, params);
    } catch (e) {
      logErr("typeHierarchy.supertypes", e);
      return null;
    }
  });
});

connection.languages.typeHierarchy.onSubtypes(async (params: TypeHierarchySubtypesParams, token?: CancellationToken): Promise<TypeHierarchyItem[] | null> => {
  if (isBreakerActive()) return null;
  return withBackpressure(async () => {
    const doc = documents.get(params.item.uri);
    if (!doc || cancelled(token)) return null;
    try {
      return typeHierarchySubtypes(doc, params);
    } catch (e) {
      logErr("typeHierarchy.subtypes", e);
      return null;
    }
  });
});

connection.onPrepareRename(async (params: PrepareRenameParams, token?: CancellationToken): Promise<{ range: Range; placeholder: string } | null> => {
  if (failFast("rename")) return null;
  if (isBreakerActive()) return null;
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return null;
    try {
      const result = await withTimeout(Promise.resolve(prepareRename(doc, params.position)), timeoutFor("rename"));
      if (result) metric("prepareRename", start, doc.uri);
      recordSuccess();
      return result;
    } catch (e) {
      metric("prepareRename", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("prepareRename", e);
      return null;
    }
  });
});

// Correction de type: renvoie WorkspaceEdit
connection.onRenameRequest(async (params: RenameParams, token?: CancellationToken): Promise<WorkspaceEdit | null> => {
  if (failFast("rename")) return null;
  if (isBreakerActive()) return null;
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return null;
    try {
      const edits = await withTimeout(Promise.resolve(renameSymbol(doc, params.position, params.newName)), timeoutFor("rename"));
      const prepared = prepareRename(doc, params.position);
      const oldName = prepared?.placeholder;
      const changes: Record<string, TextEdit[]> = { [doc.uri]: edits.map(e => TextEdit.replace(e.range, e.newText)) };
      if (oldName) {
        const workspaceDocs = await collectWorkspaceRenameCandidates(doc.uri);
        for (const d of workspaceDocs) {
          if (d.uri === doc.uri) continue;
          const otherEdits = renameIdentifierByName(d, oldName, params.newName)
            .map(e => TextEdit.replace(e.range, e.newText));
          if (otherEdits.length > 0) changes[d.uri] = otherEdits;
        }
      }
      const total = Object.values(changes).reduce((acc, arr) => acc + arr.length, 0);
      const we: WorkspaceEdit = { changes };
      metric("rename", start, doc.uri, total);
      recordSuccess();
      return we;
    } catch (e) {
      metric("rename", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("rename", e);
      return null;
    }
  });
});

connection.languages.inlayHint.on(async (params: InlayHintParams, token?: CancellationToken): Promise<InlayHint[]> => {
  if (failFast("inlayHints")) return [];
  if (isBreakerActive()) return [];
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return [];
    if (tooLarge(doc) || !featureEnabled("inlayHints")) return [];
    try {
      const prefs = await resolveInlayPrefs(params.textDocument.uri);
      const hints = await withTimeout(
        Promise.resolve(provideInlayHints(doc, params.range, prefs)),
        timeoutFor("inlayHints")
      );
      metric("inlayHints", start, doc.uri, hints.length);
      recordSuccess();
      return hints;
    } catch (e) {
      metric("inlayHints", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("inlayHints", e);
      return [];
    }
  });
});

connection.onWorkspaceSymbol(async (params: WorkspaceSymbolParams, token?: CancellationToken): Promise<WorkspaceSymbol[]> => {
  if (failFast("workspaceSymbols")) return [];
  if (isBreakerActive()) return [];
  return withBackpressure(async () => {
    const start = now();
    if (cancelled(token)) return [];
    try {
      const openDocs = documents.all().map(d => ({ uri: d.uri, doc: d }));
      const symbols = await withTimeout(Promise.resolve(workspaceSymbols(params.query ?? "", openDocs, 200)), timeoutFor("workspaceSymbols"));
      metric("workspaceSymbols", start, params.query ?? "", symbols.length);
      recordSuccess();
      return symbols;
    } catch (e) {
      metric("workspaceSymbols", start, params.query ?? "workspaceSymbols", undefined, e);
      recordFailure();
      logErr("workspaceSymbols", e);
      return [];
    }
  });
});

connection.languages.semanticTokens.on(async (params: SemanticTokensParams, token?: CancellationToken) => {
  if (failFast("semanticTokens")) return emptySemanticTokens();
  if (isBreakerActive()) return emptySemanticTokens();
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return emptySemanticTokens();
    if (tooLarge(doc)) return emptySemanticTokens();
    try {
      const tokens = await withTimeout(Promise.resolve(createSemanticTokens(doc)), timeoutFor("semanticTokens"));
      metric("semanticTokens", start, doc.uri, tokens.data.length / 5);
      recordSuccess();
      return tokens;
    } catch (e) {
      metric("semanticTokens", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("semanticTokens", e);
      return emptySemanticTokens();
    }
  });
});

/* -------------------------------- Diagnostics ----------------------------- */
const lintTimers = new Map<string, NodeJS.Timeout>();
const lintQueue: string[] = [];
const lintQueued = new Set<string>();
let lintWorkerRunning = false;
const lintResultCache = new Map<string, { hash: number; diagnostics: ReturnType<typeof lintToPublishable> }>();

function runLint(doc: TextDocument): void {
  const t0 = now();
  try {
    if (!featureEnabled("lint")) return;
    if (isBreakerActive()) return;
    if (tooLarge(doc)) { void connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] }); return; }
    const text = doc.getText();
    const uri = doc.uri;
    const hash = fastHash(text);
    const cached = lintResultCache.get(uri);
    const diags = cached && cached.hash === hash
      ? (cached.diagnostics ?? [])
      : (lintToPublishable(text, uri, globalSettings.lint) ?? []);
    lintResultCache.set(uri, { hash, diagnostics: diags });
    void connection.sendDiagnostics({ uri, diagnostics: diags });
    metric("lint", t0, uri, diags.length);
  } catch (e) {
    metric("lint", t0, doc?.uri ?? "unknown", undefined, e);
    logErr("lint", e);
  }
}

function scheduleLint(doc: TextDocument): void {
  if (!featureEnabled("lint")) return;
  const key = doc.uri;
  const base = Math.max(0, globalSettings.lintDebounceMs | 0);
  const sizeKB = Buffer.byteLength(doc.getText(), "utf8") / 1024;
  const isVit = doc.uri.endsWith(".vit");
  const sizePenalty = sizeKB > 1024 ? 900 : sizeKB > 512 ? 550 : sizeKB > 256 ? 280 : sizeKB > 128 ? 120 : 0;
  const extPenalty = isVit && sizeKB > 128 ? 120 : 0;
  const delay = Math.min(2500, base + sizePenalty + extPenalty);
  const prev = lintTimers.get(key);
  if (prev) clearTimeout(prev);
  lintTimers.set(key, setTimeout(() => enqueueLint(key), delay));
}

function safeIndexDocument(doc: TextDocument): void {
  if (memoryExceeded()) {
    connection.console.warn("[index] skipped due to memory quota");
    return;
  }
  indexWorkspaceDocument(doc);
}

function enqueueLint(uri: string): void {
  if (lintQueued.has(uri)) return;
  lintQueued.add(uri);
  lintQueue.push(uri);
  void processLintQueue();
}

function processLintQueue(): void {
  if (lintWorkerRunning) return;
  lintWorkerRunning = true;
  try {
    while (lintQueue.length > 0) {
      const uri = lintQueue.shift()!;
      lintQueued.delete(uri);
      const doc = documents.get(uri);
      if (!doc) continue;
      runLint(doc);
    }
  } finally {
    lintWorkerRunning = false;
  }
}

/* --------------------------------- Events -------------------------------- */

documents.onDidOpen((e) => { safeIndexDocument(e.document); scheduleLint(e.document); });
documents.onDidChangeContent((e) => { safeIndexDocument(e.document); scheduleLint(e.document); });
documents.onDidClose((e) => {
  removeWorkspaceDocument(e.document.uri);
  void connection.sendDiagnostics({ uri: e.document.uri, diagnostics: [] });
  const timer = lintTimers.get(e.document.uri);
  if (timer) clearTimeout(timer);
  lintTimers.delete(e.document.uri);
  lintResultCache.delete(e.document.uri);
});

/* --------------------------------- Launch -------------------------------- */

documents.listen(connection);
connection.listen();

/* --------------------------------- Utils --------------------------------- */

function now(): number {
  if (typeof process !== "undefined" && typeof process.hrtime === "function") {
    const [sec, nanosec] = process.hrtime();
    return sec * 1000 + nanosec / 1e6;
  }
  return Date.now();
}

function metric(what: string, startMs: number, uri: string, n?: number, err?: unknown) {
  const elapsed = now() - startMs;
  recordMetric(what, elapsed, uri, n, err);
}

function recordMetric(what: string, elapsed: number, uri: string, n?: number, err?: unknown) {
  const entry = metricStore.get(what) ?? {
    count: 0,
    totalMs: 0,
    maxMs: 0,
    lastMs: 0,
    lastAt: 0,
    lastUri: "",
    errorCount: 0,
    samples: [],
  };
  entry.count += 1;
  entry.totalMs += elapsed;
  entry.maxMs = Math.max(entry.maxMs, elapsed);
  entry.lastMs = elapsed;
  entry.lastAt = Date.now();
  entry.lastUri = uri;
  entry.lastCount = typeof n === "number" ? n : undefined;
  if (err) {
    entry.errorCount += 1;
    entry.lastError = formatError(err);
  }
  entry.samples.push(elapsed);
  if (entry.samples.length > 1000) entry.samples.shift();
  metricStore.set(what, entry);

  if (globalSettings.trace === "verbose") {
    if (Math.random() < (globalSettings.traceSampleRate || 0)) {
      connection.console.log(
        `[metric] ${what} ${elapsed.toFixed(1)}ms uri=${uri}${typeof n === "number" ? ` n=${n}` : ""}`
      );
    }
  }
}

const errorRateLimit = new Map<string, number>();
const ERROR_LOG_INTERVAL_MS = 2000;

function formatError(err: unknown): string {
  if (err instanceof Error) return `${err.name}: ${err.message}`;
  if (typeof err === "string") return err;
  try {
    return JSON.stringify(err);
  } catch {
    return "[unknown error]";
  }
}

function logErr(ctx: string, err: unknown) {
  const msg = formatError(err);
  const nowTs = Date.now();
  const last = errorRateLimit.get(ctx) ?? 0;
  if (nowTs - last < ERROR_LOG_INTERVAL_MS) return;
  errorRateLimit.set(ctx, nowTs);
  if (globalSettings.trace === "verbose" && err instanceof Error && err.stack) {
    connection.console.error(`[${ctx}] ${msg}\n${err.stack}`);
  } else {
    connection.console.error(`[${ctx}] ${msg}`);
  }
}

function clampNumber(value: number, min: number, max: number, fallback: number): number {
  if (!Number.isFinite(value)) return fallback;
  return Math.min(max, Math.max(min, value));
}

function fastHash(s: string): number {
  let h = 2166136261;
  for (let i = 0; i < s.length; i++) {
    h ^= s.charCodeAt(i);
    h = Math.imul(h, 16777619);
  }
  return h >>> 0;
}

function percentile(samples: number[], p: number): number {
  if (!samples.length) return 0;
  const sorted = [...samples].sort((a, b) => a - b);
  const idx = Math.min(sorted.length - 1, Math.max(0, Math.ceil((p / 100) * sorted.length) - 1));
  return sorted[idx];
}

function computeOrganizeImportsEdit(doc: TextDocument, text: string): WorkspaceEdit | null {
  const lines = text.replace(/\r\n/g, "\n").split("\n");
  const importIdx: number[] = [];
  const imports: string[] = [];
  for (let i = 0; i < lines.length; i++) {
    const l = lines[i] ?? "";
    if (/^\s*(use|pull)\b/.test(l)) {
      importIdx.push(i);
      imports.push(l.trim());
    }
  }
  if (importIdx.length === 0) return null;
  const sorted = Array.from(new Set(imports)).sort((a, b) => a.localeCompare(b));
  const first = importIdx[0];
  const last = importIdx[importIdx.length - 1];
  const before = lines.slice(0, first);
  const after = lines.slice(last + 1);
  const body = [...before, ...sorted, ...after].join("\n");
  const newText = text.endsWith("\n") && !body.endsWith("\n") ? `${body}\n` : body;
  if (newText === text) return null;
  const lastLine = Math.max(0, doc.lineCount - 1);
  const endText = doc.getText(Range.create(Position.create(lastLine, 0), Position.create(lastLine, Number.MAX_SAFE_INTEGER)));
  const full = Range.create(Position.create(0, 0), Position.create(lastLine, endText.length));
  return { changes: { [doc.uri]: [TextEdit.replace(full, newText)] } };
}

function convertUsePullLine(line: string): string | null {
  if (/^\s*use\b/.test(line)) return line.replace(/^(\s*)use\b/, "$1pull");
  if (/^\s*pull\b/.test(line)) return line.replace(/^(\s*)pull\b/, "$1use");
  return null;
}

function addPkgAliasLine(line: string): string | null {
  const m = /^(\s*use\s+)([A-Za-z0-9_./:-]+)(\s*)$/.exec(line);
  if (!m) return null;
  if (/\sas\s+[A-Za-z_][A-Za-z0-9_]*\s*$/.test(line)) return null;
  const pathPart = m[2];
  const raw = pathPart.split(/[/.:-]/).filter(Boolean).pop() ?? "pkg";
  const base = raw.replace(/[^A-Za-z0-9_]/g, "").toLowerCase();
  const alias = `${base || "pkg"}_pkg`;
  return `${m[1]}${pathPart} as ${alias}${m[3]}`;
}

function buildRoleContractTemplate(text: string): string {
  const m = /^\s*space\s+([A-Za-z0-9_./-]+)/m.exec(text);
  const pkg = m?.[1] ?? "my/package";
  const prefix = text.endsWith("\n") ? "\n" : "\n\n";
  return (
    `${prefix}<<< ROLE-CONTRACT\n` +
    `package: ${pkg}\n` +
    `role: Responsibility\n` +
    `input_contract: Explicit normalized inputs\n` +
    `output_contract: Stable explicit outputs\n` +
    `boundary: No business policy decisions\n` +
    `>>>\n`
  );
}

function quickFixForDiagnostic(doc: TextDocument, d: Diagnostic): CodeAction | null {
  const code = String(d.code ?? "");
  const lineNo = d.range.start.line;
  const lineRange = Range.create(
    Position.create(lineNo, 0),
    Position.create(lineNo, Number.MAX_SAFE_INTEGER)
  );
  const lineText = doc.getText(lineRange);

  if (code === "format.trailingWhitespace") {
    const fixed = lineText.replace(/[ \t]+$/g, "");
    if (fixed === lineText) return null;
    return {
      title: "Vitte: Trim trailing whitespace",
      kind: "quickfix",
      diagnostics: [d],
      edit: { changes: { [doc.uri]: [TextEdit.replace(lineRange, fixed)] } },
    };
  }

  if (code === "format.tabs") {
    const fixed = lineText.replace(/\t/g, "  ");
    if (fixed === lineText) return null;
    return {
      title: "Vitte: Convert tabs to spaces",
      kind: "quickfix",
      diagnostics: [d],
      edit: { changes: { [doc.uri]: [TextEdit.replace(lineRange, fixed)] } },
    };
  }

  if (code === "style.usePath") {
    const fixed = lineText.replace(/::/g, "/").replace(/\s*\/\s*/g, "/").replace(/\s*\.\s*/g, ".");
    if (fixed === lineText) return null;
    return {
      title: "Vitte: Normalize use/pull path",
      kind: "quickfix",
      diagnostics: [d],
      edit: { changes: { [doc.uri]: [TextEdit.replace(lineRange, fixed)] } },
    };
  }

  return null;
}

function dedupeLocations(locations: Location[]): Location[] {
  const seen = new Set<string>();
  const out: Location[] = [];
  for (const l of locations) {
    const k = `${l.uri}:${l.range.start.line}:${l.range.start.character}:${l.range.end.line}:${l.range.end.character}`;
    if (seen.has(k)) continue;
    seen.add(k);
    out.push(l);
  }
  return out;
}

function isPathLikeToken(token: string): boolean {
  return /[./:-]/.test(token);
}

function collectOpenDocumentDefinitions(token: string, currentUri: string): Location[] {
  const out: Location[] = [];
  const pathLike = isPathLikeToken(token);
  for (const d of documents.all()) {
    if (d.uri === currentUri) continue;
    const text = d.getText();
    const rx = pathLike
      ? new RegExp(`\\b(?:module|space|import|use|pull|entry\\s+[A-Za-z_][A-Za-z0-9_]*\\s+at)\\s+${escapeForRegex(token)}\\b`, "g")
      : new RegExp(`\\b(?:fn|proc|form|trait|type|struct|enum|union|const|let|static|share)\\s+${escapeForRegex(token)}\\b`, "g");
    let m: RegExpExecArray | null;
    while ((m = rx.exec(text))) {
      const idx = (m.index ?? 0) + m[0].lastIndexOf(token);
      const start = d.positionAt(idx);
      const end = d.positionAt(idx + token.length);
      out.push(Location.create(d.uri, Range.create(start, end)));
      if (m[0].length === 0) rx.lastIndex++;
    }
  }
  return out;
}

function collectOpenDocumentReferences(token: string, currentUri: string): Location[] {
  const out: Location[] = [];
  const pathLike = isPathLikeToken(token);
  const rx = pathLike
    ? new RegExp(`(?<![A-Za-z0-9_./:-])${escapeForRegex(token)}(?![A-Za-z0-9_./:-])`, "g")
    : new RegExp(`(?<![A-Za-z0-9_])${escapeForRegex(token)}(?![A-Za-z0-9_])`, "g");
  for (const d of documents.all()) {
    if (d.uri === currentUri) continue;
    const text = d.getText();
    let m: RegExpExecArray | null;
    while ((m = rx.exec(text))) {
      const idx = m.index ?? 0;
      const start = d.positionAt(idx);
      const end = d.positionAt(idx + token.length);
      out.push(Location.create(d.uri, Range.create(start, end)));
      if (m[0].length === 0) rx.lastIndex++;
    }
    rx.lastIndex = 0;
  }
  return out;
}

function escapeForRegex(value: string): string {
  return value.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

async function collectWorkspaceRenameCandidates(primaryUri: string): Promise<TextDocument[]> {
  const out = new Map<string, TextDocument>();
  for (const d of documents.all()) out.set(d.uri, d);

  const roots = await resolveWorkspaceRoots();
  if (roots.length === 0) return Array.from(out.values());

  for (const root of roots) {
    const files = await walkVitteFiles(root, 1200);
    for (const file of files) {
      const uri = pathToFileURL(file).toString();
      if (uri === primaryUri || out.has(uri)) continue;
      try {
        const raw = await fs.readFile(file, "utf8");
        out.set(uri, TextDocument.create(uri, file.endsWith(".vit") ? "vit" : "vitte", 0, raw));
      } catch {
        // ignore unreadable files
      }
    }
  }
  return Array.from(out.values());
}

async function walkVitteFiles(root: string, limit: number): Promise<string[]> {
  const out: string[] = [];
  const queue: string[] = [root];
  const skip = new Set([".git", "node_modules", ".vscode", "out", "dist", "build", "target", ".next"]);

  while (queue.length > 0 && out.length < limit) {
    const dir = queue.shift()!;
    let entries: Dirent[];
    try {
      entries = await fs.readdir(dir, { withFileTypes: true });
    } catch {
      continue;
    }
    for (const e of entries) {
      if (out.length >= limit) break;
      if (e.isDirectory()) {
        if (skip.has(e.name)) continue;
        queue.push(path.join(dir, e.name));
        continue;
      }
      if (!e.isFile()) continue;
      if (!e.name.endsWith(".vit") && !e.name.endsWith(".vitte")) continue;
      out.push(path.join(dir, e.name));
    }
  }
  return out;
}
