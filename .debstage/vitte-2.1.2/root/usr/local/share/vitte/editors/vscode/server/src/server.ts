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
  SemanticTokens,
  SemanticTokensParams,
  SemanticTokensLegend,
  WorkspaceSymbolParams,
  WorkspaceSymbol,
  DefinitionParams,
  Location,
  ReferenceParams,
  RenameParams,
  PrepareRenameParams,
  Range,
  WorkspaceEdit,
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
  prepareRename,
  workspaceSymbols,
} from "./navigation.js";
import { provideFormattingEdits } from "./formatting.js";
import { getSemanticTokensLegend, buildSemanticTokens, provideHover } from "./semantic.js";
import { lintToPublishable } from "./lint.js";
import { registerCommands } from "./commands.js";
import { indexDocument as indexWorkspaceDocument, removeDocument as removeWorkspaceDocument, clearIndex, exportIndexSnapshot, loadIndexSnapshot, type IndexSnapshot } from "./indexer.js";
import * as fs from "node:fs/promises";
import * as path from "node:path";
import { fileURLToPath } from "node:url";

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
  },
};

let globalSettings: ServerSettings = { ...DEFAULT_SETTINGS };
let hasConfigurationCapability = false;
let hasWorkspaceFoldersCapability = false;
let workspaceRoot: string | undefined;

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
      definitionProvider: true,
      referencesProvider: true,
      renameProvider: { prepareProvider: true },
      workspaceSymbolProvider: true,
      semanticTokensProvider: { legend, full: true, range: false },
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

async function getIndexCachePath(): Promise<string | undefined> {
  const root = await resolveWorkspaceRoot();
  if (!root) return undefined;
  return path.join(root, ".vitte", "index-cache.json");
}

function isIndexSnapshot(value: unknown): value is IndexSnapshot {
  if (!value || typeof value !== "object") return false;
  const v = value as { version?: unknown; entries?: unknown };
  return v.version === 1 && Array.isArray(v.entries);
}

async function loadIndexCache(): Promise<void> {
  if (!globalSettings.indexerCacheEnabled) return;
  const cachePath = await getIndexCachePath();
  if (!cachePath) return;
  try {
    const raw = await fs.readFile(cachePath, "utf8");
    const parsed = JSON.parse(raw) as unknown;
    const snapshot: IndexSnapshot | null = isIndexSnapshot(parsed) ? parsed : null;
    const count = loadIndexSnapshot(snapshot);
    connection.console.log(`[index] loaded cache (${count} files)`);
  } catch {
    // ignore cache errors
  }
}

async function saveIndexCache(): Promise<void> {
  if (!globalSettings.indexerCacheEnabled) return;
  const cachePath = await getIndexCachePath();
  if (!cachePath) return;
  try {
    await fs.mkdir(path.dirname(cachePath), { recursive: true });
    const snapshot = exportIndexSnapshot();
    await fs.writeFile(cachePath, JSON.stringify(snapshot), "utf8");
  } catch {
    // ignore cache errors
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

connection.onDefinition(async (params: DefinitionParams, token?: CancellationToken): Promise<Location[]> => {
  if (failFast("definition")) return [];
  if (isBreakerActive()) return [];
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || cancelled(token)) return [];
    try {
      const defs = await withTimeout(Promise.resolve(definitionAtPosition(doc, params.position, params.textDocument.uri)), timeoutFor("definition"));
      metric("definition", start, doc.uri, defs.length);
      recordSuccess();
      return defs;
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
      metric("references", start, doc.uri, refs.length);
      recordSuccess();
      return refs;
    } catch (e) {
      metric("references", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("references", e);
      return [];
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
      const we: WorkspaceEdit = { changes: { [doc.uri]: edits.map(e => TextEdit.replace(e.range, e.newText)) } };
      metric("rename", start, doc.uri, edits.length);
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

function runLint(doc: TextDocument): void {
  const t0 = now();
  try {
    if (!featureEnabled("lint")) return;
    if (isBreakerActive()) return;
    if (tooLarge(doc)) { void connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] }); return; }
    const text = doc.getText();
    const uri = doc.uri;
    const diags = lintToPublishable(text, uri, globalSettings.lint) ?? [];
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
  const delay = Math.max(0, globalSettings.lintDebounceMs | 0);
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

function percentile(samples: number[], p: number): number {
  if (!samples.length) return 0;
  const sorted = [...samples].sort((a, b) => a - b);
  const idx = Math.min(sorted.length - 1, Math.max(0, Math.ceil((p / 100) * sorted.length) - 1));
  return sorted[idx];
}
