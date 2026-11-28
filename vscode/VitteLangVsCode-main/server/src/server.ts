// server.ts — Vitte/Vit hardened LSP server
// Goals:
// - Tested capabilities: completion, hover, symbols, def/refs, rename, diagnostics, semantic tokens
// - Dynamic config management + watchers + graceful shutdown
// - Lint with debounce, size guards, cancellation
// - Controlled logging, lightweight metrics, pervasive try/catch

import {
  createConnection,
  ProposedFeatures,
  TextDocumentSyncKind,
  TextDocuments,
  TextEdit,
  Position,
  Range,
  DidChangeConfigurationNotification,
  FileChangeType,
  CodeLens,
  Range as LspRange,
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
  SemanticTokens,
  SemanticTokensParams,
  SemanticTokensDeltaParams,
  SemanticTokensDelta,
  SemanticTokensEdit,
  SemanticTokensLegend,
  CodeLensParams,
  FoldingRangeParams,
  SelectionRangeParams,
  DocumentLink,
  DocumentLinkParams,
  FoldingRange,
  SelectionRange,
  WorkspaceSymbolParams,
  WorkspaceSymbol,
  DefinitionParams,
  DeclarationParams,
  TypeDefinitionParams,
  ImplementationParams,
  ReferenceParams,
  RenameParams,
  RenameFilesParams,
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
  SignatureHelp,
  SignatureHelpParams,
  CancellationToken,
  RemoteWorkspace,
  Location,
} from "vscode-languageserver/node";
import { TextDocument } from "vscode-languageserver-textdocument";

/* Modules internes */
import { provideCompletions, resolveCompletion, triggerCharacters, configureCompletions } from "./completion.js";
import { findUniqueModulePathFileMove, resolveModuleFiles, type CompletionEnvironment } from "./moduleResolver.js";
import { maskNonCode } from "./lexical.js";
import {
  documentSymbols,
  definitionAtPositionWithImports,
  declarationAtPositionWithImports,
  typeDefinitionAtPositionWithImports,
  implementationAtPositionWithImports,
  referencesAtPositionWithWorkspace,
  resolveRenameTarget,
  renameReferencesForTarget,
  validateRenameName,
  validateRenameLocations,
  type RenameTarget,
  workspaceSymbols,
  prepareCallHierarchy,
  callHierarchyIncoming,
  callHierarchyOutgoing,
  prepareTypeHierarchy,
  typeHierarchySupertypes,
  typeHierarchySubtypes,
} from "./navigation.js";
import { sourceIndex } from "./sourceSymbols.js";
import { getSemanticTokensLegend, buildSemanticTokens, provideHoverWithImports } from "./semantic.js";
import { provideInlayHints, type InlayHintPrefs } from "./inlay.js";
import { provideFoldingRanges, provideSelectionRanges } from "./documentFeatures.js";
import { provideDocumentLinks } from "./documentLinks.js";
import { provideSignatureHelpWithImports } from "./signatureHelp.js";
import { getIntelliSenseCapabilities, isCapabilityAdvertised, LANGUAGE_SERVER_VERSION } from "./intellisenseCapabilities.js";
import { lintToPublishable } from "./lint.js";
import { registerCommands } from "./commands.js";
import { indexDocument as indexWorkspaceDocument, removeDocument as removeWorkspaceDocument, clearIndex, exportIndexSnapshot, loadIndexSnapshot, getIndex, type IndexSnapshot } from "./indexer.js";
import * as fs from "node:fs/promises";
import { createHash } from "node:crypto";
import * as path from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { resolveProjectPath, resolveVitteProjectScope } from "./projectScope";

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
const semanticTokenHistory = new Map<string, Map<string, number[]>>();
let semanticTokenResultSequence = 0;

function rememberSemanticTokens(uri: string, data: number[]): string {
  const resultId = `vitte-${++semanticTokenResultSequence}`;
  // Large documents still receive correct full semantic tokens, but do not
  // retain oversized snapshots solely to calculate future deltas.
  if (data.length > 50_000) return resultId;
  let history = semanticTokenHistory.get(uri);
  if (!history) {
    history = new Map<string, number[]>();
    semanticTokenHistory.set(uri, history);
  }
  semanticTokenHistory.delete(uri);
  semanticTokenHistory.set(uri, history);
  history.set(resultId, data);
  while (history.size > 2) history.delete(history.keys().next().value!);
  while (semanticTokenHistory.size > 32) semanticTokenHistory.delete(semanticTokenHistory.keys().next().value!);
  return resultId;
}

/* ------------------------------- Configuration --------------------------- */
interface ServerSettings {
  trace: "off" | "messages" | "verbose";
  traceSampleRate: number;
  lintDebounceMs: number;
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
    inlayHints?: number;
    signatureHelp?: number;
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
  completion: {
    enabledInComments?: boolean;
    enabledInStrings?: boolean;
    includeSnippets?: boolean;
    workspaceSymbols?: boolean;
    maxItems?: number;
  };
  features: {
    completion: boolean;
    hover: boolean;
    definition: boolean;
    declaration: boolean;
    typeDefinition: boolean;
    implementation: boolean;
    references: boolean;
    rename: boolean;
    documentSymbols: boolean;
    workspaceSymbols: boolean;
    semanticTokens: boolean;
    lint: boolean;
    inlayHints: boolean;
    signatureHelp: boolean;
  };
}

const DEFAULT_SETTINGS: ServerSettings = {
  trace: "off",
  traceSampleRate: 0.01,
  lintDebounceMs: 200,
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
    returnHints: false,
    aliasHints: false,
  },
  completion: {
    enabledInComments: false,
    enabledInStrings: false,
    includeSnippets: true,
    workspaceSymbols: false,
    maxItems: 200,
  },
  features: {
    completion: true,
    hover: true,
    definition: true,
    declaration: true,
    typeDefinition: true,
    implementation: true,
    references: true,
    rename: true,
    documentSymbols: true,
    workspaceSymbols: true,
    semanticTokens: true,
    lint: true,
    inlayHints: true,
    signatureHelp: true,
  },
};

let globalSettings: ServerSettings = { ...DEFAULT_SETTINGS };
let hasConfigurationCapability = false;
let clientSupportsWorkspaceFolders = false;
let workspaceRoot: string | undefined;
let workspaceRoots: string[] | undefined;
let clientSupportsDocumentChanges = false;
let serverShuttingDown = false;
let configurationRequestSequence = 0;
let configurationRevision = 0;
let workspaceContentRevision = 0;
let workspaceFolderRequestSequence = 0;

connection.onInitialize((params: InitializeParams): InitializeResult => {
  const capabilities = params.capabilities;
  hasConfigurationCapability = !!capabilities.workspace?.configuration;
  clientSupportsWorkspaceFolders = Boolean(capabilities.workspace?.workspaceFolders);
  clientSupportsDocumentChanges = Boolean(capabilities.workspace?.workspaceEdit?.documentChanges);
  const initialRoots = (params.workspaceFolders ?? [])
    .map((folder) => folder.uri.startsWith("file://") ? fileURLToPath(folder.uri) : "")
    .filter((root): root is string => root.length > 0);
  if (initialRoots.length > 0) {
    workspaceRoots = initialRoots;
    workspaceRoot = initialRoots[0];
  } else if (params.rootUri?.startsWith("file://")) {
    workspaceRoot = fileURLToPath(params.rootUri);
    workspaceRoots = [workspaceRoot];
  }

  const legend: SemanticTokensLegend = getSemanticTokensLegend();

  const serverCapabilities: NonNullable<InitializeResult["capabilities"]> = {
    textDocumentSync: TextDocumentSyncKind.Incremental,
  };
  if (capabilities.workspace?.workspaceFolders) {
    serverCapabilities.workspace = {
      workspaceFolders: { changeNotifications: true },
    };
  }
  if (capabilities.workspace?.fileOperations?.willRename) {
    serverCapabilities.workspace = {
      ...serverCapabilities.workspace,
      fileOperations: {
        willRename: {
          filters: [".vit", ".vitl", ".vitte"].map((extension) => ({
            scheme: "file",
            pattern: { glob: `**/*${extension}`, matches: "file" as const },
          })),
        },
      },
    };
  }
  if (isCapabilityAdvertised("completion")) {
    serverCapabilities.completionProvider = { triggerCharacters: triggerCharacters(), resolveProvider: true };
  }
  if (isCapabilityAdvertised("hover")) serverCapabilities.hoverProvider = true;
  if (isCapabilityAdvertised("signature-help")) {
    serverCapabilities.signatureHelpProvider = { triggerCharacters: ["(", ",", "{", ":"], retriggerCharacters: [",", ":"] };
  }
  if (isCapabilityAdvertised("document-symbols")) serverCapabilities.documentSymbolProvider = true;
  if (isCapabilityAdvertised("workspace-symbols")) serverCapabilities.workspaceSymbolProvider = true;
  if (isCapabilityAdvertised("definition")) serverCapabilities.definitionProvider = true;
  if (isCapabilityAdvertised("declaration")) serverCapabilities.declarationProvider = true;
  if (isCapabilityAdvertised("type-definition")) serverCapabilities.typeDefinitionProvider = true;
  if (isCapabilityAdvertised("implementation")) serverCapabilities.implementationProvider = true;
  if (isCapabilityAdvertised("references")) serverCapabilities.referencesProvider = true;
  if (isCapabilityAdvertised("rename")) serverCapabilities.renameProvider = { prepareProvider: true };
  if (isCapabilityAdvertised("code-actions")) {
    serverCapabilities.codeActionProvider = { codeActionKinds: ["quickfix"] };
  }
  if (isCapabilityAdvertised("semantic-tokens")) {
    serverCapabilities.semanticTokensProvider = { legend, full: { delta: true }, range: false };
  }
  if (isCapabilityAdvertised("inlay-hints")) serverCapabilities.inlayHintProvider = true;
  if (isCapabilityAdvertised("code-lens")) serverCapabilities.codeLensProvider = { resolveProvider: false };
  if (isCapabilityAdvertised("folding-ranges")) serverCapabilities.foldingRangeProvider = true;
  if (isCapabilityAdvertised("selection-ranges")) serverCapabilities.selectionRangeProvider = true;
  if (isCapabilityAdvertised("document-links")) serverCapabilities.documentLinkProvider = { resolveProvider: false };

  const result: InitializeResult = {
    capabilities: serverCapabilities,
    serverInfo: { name: "vitte-language-server", version: LANGUAGE_SERVER_VERSION },
  };

  return result;
});

connection.onInitialized(() => {
  if (clientSupportsWorkspaceFolders) {
    connection.workspace.onDidChangeWorkspaceFolders(async () => {
      const requestSequence = ++workspaceFolderRequestSequence;
      configurationRevision += 1;
      workspaceContentRevision += 1;
      try {
        const folders = await connection.workspace.getWorkspaceFolders();
        if (!folders || requestSequence !== workspaceFolderRequestSequence) return;
        workspaceRoots = folders.map((folder) => folder.uri.startsWith("file://") ? fileURLToPath(folder.uri) : "").filter(Boolean);
        workspaceRoot = workspaceRoots[0];
        workspaceSourceCache.clear();
        clearIndex();
        for (const doc of documents.all()) safeIndexDocument(doc);
      } catch (error) {
        logErr("workspaceFolders.changed", error);
      }
    });
  }
  registerCommands(connection);
  if (hasConfigurationCapability) {
    void connection.client.register(DidChangeConfigurationNotification.type, undefined);
  }
  // Watch classiques pour compat large versions du client
  connection.onDidChangeWatchedFiles((change) => {
    let invalidateSources = false;
    let invalidateProjectState = false;
    for (const ev of change.changes) {
      let filePath = "";
      try { if (ev.uri.startsWith("file://")) filePath = fileURLToPath(ev.uri); } catch { /* malformed URI */ }
      const name = path.basename(filePath).toLocaleLowerCase();
      if (/\.(?:vit|vitl|vitte)$/i.test(filePath)) {
        invalidateSources = true;
        if (ev.type === FileChangeType.Deleted) {
          removeWorkspaceDocument(ev.uri);
          void connection.sendDiagnostics({ uri: ev.uri, diagnostics: [] });
        }
      }
      if (["vitte.toml", ".vitteconfig", "vitte.config.json", "config", "config.json"].includes(name)) {
        invalidateProjectState = true;
      }
    }
    if (invalidateSources) { workspaceSourceCache.clear(); workspaceContentRevision += 1; }
    if (invalidateProjectState) {
      configurationRevision += 1;
      workspaceContentRevision += 1;
      workspaceSourceCache.clear();
      lintResultCache.clear();
      clearIndex();
      for (const doc of documents.all()) {
        void connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] });
        safeIndexDocument(doc);
        scheduleLint(doc);
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

connection.onRequest("vitte/intellisenseCapabilities", () => {
  return getIntelliSenseCapabilities(globalSettings.features);
});

connection.onRequest("vitte/capabilityMatrix", () => {
  return getIntelliSenseCapabilities(globalSettings.features);
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

connection.onShutdown(async () => {
  serverShuttingDown = true;
  for (const t of lintTimers.values()) clearTimeout(t);
  lintTimers.clear();
  for (const timer of indexTimers.values()) clearTimeout(timer);
  indexTimers.clear();
  lintQueue.length = 0;
  lintQueued.clear();
  lintResultCache.clear();
  semanticTokenHistory.clear();
  workspaceSourceCache.clear();
  await saveIndexCache();
  clearIndex();
});

/* ------------------------------- Config updates --------------------------- */
async function applyConfiguration(): Promise<void> {
  const requestSequence = ++configurationRequestSequence;
  const workspace = Reflect.get(connection, "workspace") as RemoteWorkspace | undefined;
  if (hasConfigurationCapability && workspace) {
    try {
      const cfg = await workspace.getConfiguration({ section: "vitte" }) as Partial<ServerSettings> | null | undefined;
      const lintCfg = await workspace.getConfiguration({ section: "vitte.lint" }) as Partial<ServerSettings["lint"]> | null | undefined;
      const completionCfg = await workspace.getConfiguration({ section: "vitte.completion" }) as Partial<ServerSettings["completion"]> | null | undefined;
      const merged: ServerSettings = { ...DEFAULT_SETTINGS, ...(cfg ?? {}) };
      merged.lint = { ...DEFAULT_SETTINGS.lint, ...(lintCfg ?? {}) };
      merged.features = { ...DEFAULT_SETTINGS.features, ...(cfg?.features ?? {}) };
      merged.inlayHints = { ...DEFAULT_SETTINGS.inlayHints, ...(cfg?.inlayHints ?? {}) };
      merged.completion = {
        ...DEFAULT_SETTINGS.completion,
        ...(cfg?.completion ?? {}),
        ...(completionCfg ?? {}),
      };
      merged.lintDebounceMs = clampNumber(merged.lintDebounceMs, 0, 2000, DEFAULT_SETTINGS.lintDebounceMs);
      merged.maxFileSizeKB = clampNumber(merged.maxFileSizeKB, 64, 10000, DEFAULT_SETTINGS.maxFileSizeKB);
      merged.requestTimeoutMs = clampNumber(merged.requestTimeoutMs, 100, 5000, DEFAULT_SETTINGS.requestTimeoutMs);
      merged.traceSampleRate = clampNumber(merged.traceSampleRate, 0, 1, DEFAULT_SETTINGS.traceSampleRate);
      merged.requestMaxConcurrent = clampNumber(merged.requestMaxConcurrent, 1, 32, DEFAULT_SETTINGS.requestMaxConcurrent);
      merged.indexerMaxRssMB = clampNumber(merged.indexerMaxRssMB, 256, 8192, DEFAULT_SETTINGS.indexerMaxRssMB);
      merged.indexerCacheEnabled = cfg?.indexerCacheEnabled ?? DEFAULT_SETTINGS.indexerCacheEnabled;
      merged.requestTimeouts = { ...DEFAULT_SETTINGS.requestTimeouts, ...(cfg?.requestTimeouts ?? {}) };
      if (requestSequence !== configurationRequestSequence) return;
      globalSettings = merged;
      configureCompletions(globalSettings.completion);
    } catch {
      if (requestSequence !== configurationRequestSequence) return;
      globalSettings = { ...DEFAULT_SETTINGS };
      configureCompletions(globalSettings.completion);
    }
  } else {
    if (requestSequence !== configurationRequestSequence) return;
    globalSettings = { ...DEFAULT_SETTINGS };
    configureCompletions(globalSettings.completion);
  }
  configurationRevision += 1;
  // Configuration affects diagnostics and feature output even when the source
  // text did not change. Drop derived state before scheduling fresh results.
  lintResultCache.clear();
  workspaceSourceCache.clear();
  clearIndex();
  for (const doc of documents.all()) {
    safeIndexDocument(doc);
    scheduleLint(doc);
  }
}

connection.onDidChangeConfiguration(() => {
  configurationRevision += 1;
  for (const timer of lintTimers.values()) clearTimeout(timer);
  lintTimers.clear();
  lintQueue.length = 0;
  lintQueued.clear();
  lintResultCache.clear();
  for (const doc of documents.all()) void connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] });
  void applyConfiguration();
});

/* --------------------------------- Guards -------------------------------- */
function tooLarge(doc: TextDocument): boolean {
  const kb = Buffer.byteLength(doc.getText(), "utf8") / 1024;
  return kb > (globalSettings.maxFileSizeKB | 0);
}

function cancelled(token?: CancellationToken): boolean { return !!token?.isCancellationRequested; }

function documentIsCurrent(doc: TextDocument, token?: CancellationToken, expectedConfigurationRevision = configurationRevision): boolean {
  return !cancelled(token) && configurationRevision === expectedConfigurationRevision && documents.get(doc.uri)?.version === doc.version;
}

function requestDocumentIsCurrent(
  doc: TextDocument,
  requestedVersion: number | undefined,
  token: CancellationToken | undefined,
  requestedConfigurationRevision: number,
): boolean {
  return doc.version === requestedVersion && documentIsCurrent(doc, token, requestedConfigurationRevision);
}

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

async function resolveCompletionEnvironment(uri: string): Promise<CompletionEnvironment> {
  const roots = await resolveWorkspaceRoots();
  const workspace = Reflect.get(connection, "workspace") as RemoteWorkspace | undefined;
  let config: Record<string, unknown> = {};
  if (workspace && hasConfigurationCapability) {
    try {
      config = await workspace.getConfiguration({ scopeUri: uri, section: "vitte" }) as Record<string, unknown> ?? {};
    } catch {
      config = {};
    }
  }
  const readSetting = (key: string): unknown => {
    if (Object.prototype.hasOwnProperty.call(config, key)) return config[key];
    let current: unknown = config;
    for (const segment of key.split(".")) {
      if (!current || typeof current !== "object") return undefined;
      current = (current as Record<string, unknown>)[segment];
    }
    return current;
  };
  const configuredRoot = readSetting("project.root");
  const scope = resolveVitteProjectScope(uri, roots, typeof configuredRoot === "string" ? configuredRoot : undefined);
  const folderRoot = scope.workspaceRoot;
  const projectRoot = scope.projectRoot;
  const configuredPaths = readSetting("modulePaths");
  const modulePaths = Array.isArray(configuredPaths)
    ? configuredPaths.filter((value): value is string => typeof value === "string" && value.trim().length > 0).map((value) => resolveProjectPath(value.trim(), projectRoot))
    : [];
  const configuredCompiler = readSetting("compiler.path");
  const compilerPath = typeof configuredCompiler === "string" && configuredCompiler.trim()
    ? resolveProjectPath(configuredCompiler.trim(), folderRoot)
    : undefined;
  // A document receives only its own workspace folder and project roots. This
  // keeps module lookup isolated when multi-root folders use different toolchains.
  return { workspaceRoots: [folderRoot], projectRoot, modulePaths, compilerPath };
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
  const folderSequence = workspaceFolderRequestSequence;
  const cachePaths = await getIndexCachePaths();
  if (cachePaths.length === 0 || folderSequence !== workspaceFolderRequestSequence) return;
  const mergedEntries: IndexSnapshot["entries"] = [];
  for (const cachePath of cachePaths) {
    try {
      const cacheStat = await fs.stat(cachePath);
      if (!cacheStat.isFile() || cacheStat.size > 32 * 1024 * 1024) continue;
      const raw = await fs.readFile(cachePath, "utf8");
      const parsed = JSON.parse(raw) as unknown;
      const snapshot: IndexSnapshot | null = isIndexSnapshot(parsed) ? parsed : null;
      if (!snapshot) continue;
      const cacheRoot = path.dirname(path.dirname(cachePath));
      let validatedBytes = 0;
      for (const entry of snapshot.entries.slice(0, 4096)) {
        if (mergedEntries.length >= 4096) break;
        if (validatedBytes >= 16 * 1024 * 1024) break;
        if (!isPathWithinRoots(entry.uri, [cacheRoot])) continue;
        try {
          const sourcePath = fileURLToPath(entry.uri);
          const sourceStat = await fs.stat(sourcePath);
          if (!sourceStat.isFile() || sourceStat.size > 1024 * 1024 || validatedBytes + sourceStat.size > 16 * 1024 * 1024) continue;
          const currentText = await fs.readFile(sourcePath, "utf8");
          validatedBytes += sourceStat.size;
          // Cache snapshots can outlive branch switches. Only restore entries
          // whose current disk content still matches the cached source hash.
          if (fastHash(currentText) === entry.hash) mergedEntries.push(entry);
        } catch { /* removed or inaccessible since the snapshot */ }
      }
      connection.console.log(`[index] validated cache ${cachePath} (${mergedEntries.length} current files)`);
    } catch {
      // ignore cache errors
    }
  }
  if (mergedEntries.length > 0 && folderSequence === workspaceFolderRequestSequence) {
    const unique = new Map<string, IndexSnapshot["entries"][number]>();
    for (const e of mergedEntries) unique.set(e.uri, e);
    const count = loadIndexSnapshot({ version: 2, entries: Array.from(unique.values()) });
    // Open buffers are authoritative, including unsaved contents.
    for (const doc of documents.all()) indexWorkspaceDocument(doc);
    connection.console.log(`[index] merged cache (${count} files)`);
  }
}

async function saveIndexCache(): Promise<void> {
  if (!globalSettings.indexerCacheEnabled) return;
  const cachePaths = await getIndexCachePaths();
  if (cachePaths.length === 0) return;
  const snapshot = exportIndexSnapshot();
  for (const cachePath of cachePaths) {
    try {
      const cacheRoot = path.dirname(path.dirname(cachePath));
      const scopedSnapshot: IndexSnapshot = {
        version: 2,
        entries: snapshot.entries.filter((entry) => isPathWithinRoots(entry.uri, [cacheRoot])).slice(0, 4096),
      };
      await fs.mkdir(path.dirname(cachePath), { recursive: true });
      const temporaryPath = `${cachePath}.${process.pid}.tmp`;
      await fs.writeFile(temporaryPath, JSON.stringify(scopedSnapshot), "utf8");
      await fs.rename(temporaryPath, cachePath);
    } catch {
      await fs.unlink(`${cachePath}.${process.pid}.tmp`).catch(() => undefined);
      // ignore cache errors
    }
  }
}

/* --------------------------------- Handlers ------------------------------- */

const breaker = { consecutiveErrors: 0, until: 0 };
const BREAKER_THRESHOLD = 5;
const BREAKER_COOLDOWN_MS = 30000;
const MAX_QUEUED_REQUESTS = 64;
const requestQueue: Array<() => void> = [];
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

async function withBackpressure<T>(fn: () => Promise<T>, token?: CancellationToken): Promise<T> {
  if (cancelled(token)) throw new Error("Request cancelled.");
  const max = Math.max(1, globalSettings.requestMaxConcurrent | 0);
  if (activeRequests >= max) {
    if (requestQueue.length >= MAX_QUEUED_REQUESTS) {
      connection.console.warn(`[request] queue full (${MAX_QUEUED_REQUESTS}); rejecting request`);
      throw new Error("Language server is busy; retry the request shortly.");
    }
    return new Promise<T>((resolve, reject) => {
      let cancellationSubscription: { dispose(): void } | undefined;
      const queuedTask = (): void => {
        cancellationSubscription?.dispose();
        if (cancelled(token)) { reject(new Error("Request cancelled.")); return; }
        void withBackpressure(fn, token).then(resolve, reject);
      };
      cancellationSubscription = token?.onCancellationRequested(() => {
        const queueIndex = requestQueue.indexOf(queuedTask);
        if (queueIndex < 0) return;
        requestQueue.splice(queueIndex, 1);
        cancellationSubscription?.dispose();
        reject(new Error("Request cancelled."));
      });
      requestQueue.push(queuedTask);
      if (cancelled(token)) {
        const queueIndex = requestQueue.indexOf(queuedTask);
        if (queueIndex >= 0) requestQueue.splice(queueIndex, 1);
        cancellationSubscription?.dispose();
        reject(new Error("Request cancelled."));
      }
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

const unavailableNotices = new Set<string>();

function explainUnavailable(feature: string, reason: string): void {
  const key = `${feature}:${reason}`;
  if (unavailableNotices.has(key)) return;
  if (unavailableNotices.size >= 256) unavailableNotices.clear();
  unavailableNotices.add(key);
  const message = `Vitte ${feature} did not return a result: ${reason}. See “Vitte: Show Capabilities” for scope and status.`;
  connection.console.warn(`[capability] ${message}`);
  void connection.sendNotification("window/showMessage", { type: 2, message });
}

function failFast(feature: keyof ServerSettings["features"]): boolean {
  if (!featureEnabled(feature)) {
    explainUnavailable(feature, `disabled by vitte.features.${feature}`);
    return true;
  }
  if (isBreakerActive()) {
    explainUnavailable(feature, "language server is paused after repeated request failures");
    return true;
  }
  return false;
}

function explainLargeDocument(feature: string): void {
  explainUnavailable(feature, `Le document dépasse la limite configurée de ${globalSettings.maxFileSizeKB} Kio.`);
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
  const value: unknown = Reflect.get(input, field);
  if (typeof value !== "number" || !Number.isFinite(value)) return fallback;
  return Math.min(max, Math.max(min, Math.trunc(value)));
}

function withHierarchyBackpressure<T>(
  contextName: string,
  uri: string | undefined,
  token: CancellationToken | undefined,
  compute: (doc: TextDocument) => T,
): Promise<T | null> {
  if (contextName.startsWith("callHierarchy") && !isCapabilityAdvertised("call-hierarchy")) return Promise.resolve(null);
  if (contextName.startsWith("typeHierarchy") && !isCapabilityAdvertised("type-hierarchy")) return Promise.resolve(null);
  if (isBreakerActive()) return Promise.resolve(null);
  const requestedVersion = uri ? documents.get(uri)?.version : undefined;
  const requestedConfigurationRevision = configurationRevision;
  return withBackpressure(() => {
    if (!uri) return Promise.resolve(null);
    const doc = documents.get(uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return Promise.resolve(null);
    if (tooLarge(doc)) { explainLargeDocument(contextName); return Promise.resolve(null); }
    try {
      return Promise.resolve(compute(doc));
    } catch (e) {
      logErr(contextName, e);
      return Promise.resolve(null);
    }
  }, token);
}

connection.onCompletion(async (params: CompletionParams, token?: CancellationToken): Promise<CompletionItem[]> => {
  if (failFast("completion")) return [];
  if (isBreakerActive()) return [];
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
    if (tooLarge(doc)) { explainLargeDocument("completion"); return []; }
    try {
      const result = await withTimeout((async () => {
        const environment = await resolveCompletionEnvironment(doc.uri);
        if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
        return provideCompletions(doc, params.position, environment);
      })(), timeoutFor("completion"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      metric("completion", start, doc.uri, result.length);
      recordSuccess();
      return result;
    } catch (e) {
      metric("completion", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("completion", e);
      return [];
    }
  }, token);
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
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
    if (tooLarge(doc)) { explainLargeDocument("hover"); return null; }
    try {
      const environment = await resolveCompletionEnvironment(doc.uri);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
      const hover = await withTimeout(provideHoverWithImports(doc, params.position, environment), timeoutFor("hover"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
      metric("hover", start, doc.uri);
      recordSuccess();
      return hover;
    } catch (e) {
      metric("hover", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("hover", e);
      return null;
    }
  }, token);
});

connection.onSignatureHelp(async (
  params: SignatureHelpParams,
  token?: CancellationToken,
): Promise<SignatureHelp | null> => {
  if (failFast("signatureHelp")) return null;
  if (isBreakerActive()) return null;
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
    if (tooLarge(doc)) { explainLargeDocument("signatureHelp"); return null; }
    try {
      const help = await withTimeout((async () => {
        const environment = await resolveCompletionEnvironment(doc.uri);
        if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
        return provideSignatureHelpWithImports(doc, params.position, environment);
      })(), timeoutFor("signatureHelp"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
      metric("signatureHelp", start, doc.uri, help?.signatures.length ?? 0);
      recordSuccess();
      return help ?? null;
    } catch (e) {
      metric("signatureHelp", start, doc.uri, undefined, e);
      recordFailure();
      logErr("signatureHelp", e);
      return null;
    }
  }, token);
});

connection.onDocumentSymbol(async (params: DocumentSymbolParams, token?: CancellationToken): Promise<DocumentSymbol[]> => {
  if (failFast("documentSymbols")) return [];
  if (isBreakerActive()) return [];
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
    if (tooLarge(doc)) { explainLargeDocument("documentSymbols"); return []; }
    try {
      const symbols = await withTimeout(Promise.resolve(documentSymbols(doc)), timeoutFor("documentSymbols"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      metric("documentSymbols", start, doc.uri, symbols.length);
      recordSuccess();
      return symbols;
    } catch (e) {
      metric("documentSymbols", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("documentSymbols", e);
      return [];
    }
  }, token);
});

connection.onCodeAction((params: CodeActionParams): CodeAction[] => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return [];
  const actions: CodeAction[] = [];

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
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
    if (tooLarge(doc)) { explainLargeDocument("definition"); return []; }
    try {
      const environment = await resolveCompletionEnvironment(doc.uri);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      const outcome = await withTimeout(definitionAtPositionWithImports(doc, params.position, environment), timeoutFor("definition"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      if (outcome.locations.length === 0 && outcome.unavailableReason) explainUnavailable("definition", outcome.unavailableReason);
      metric("definition", start, doc.uri, outcome.locations.length);
      recordSuccess();
      return outcome.locations;
    } catch (e) {
      metric("definition", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("definition", e);
      return [];
    }
  }, token);
});

connection.onDeclaration(async (params: DeclarationParams, token?: CancellationToken): Promise<Location[]> => {
  if (failFast("declaration")) return [];
  if (isBreakerActive()) return [];
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  return withBackpressure(async () => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
    if (tooLarge(doc)) { explainLargeDocument("declaration"); return []; }
    try {
      const environment = await resolveCompletionEnvironment(doc.uri);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      const outcome = await withTimeout(declarationAtPositionWithImports(doc, params.position, environment), timeoutFor("definition"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      if (!outcome.locations.length && outcome.unavailableReason) explainUnavailable("declaration", outcome.unavailableReason);
      return outcome.locations;
    } catch (error) {
      logErr("declaration", error);
      return [];
    }
  }, token);
});

connection.onTypeDefinition(async (params: TypeDefinitionParams, token?: CancellationToken): Promise<Location[]> => {
  if (failFast("typeDefinition")) return [];
  if (isBreakerActive()) return [];
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  return withBackpressure(async () => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
    if (tooLarge(doc)) { explainLargeDocument("typeDefinition"); return []; }
    try {
      const environment = await resolveCompletionEnvironment(doc.uri);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      const outcome = await withTimeout(typeDefinitionAtPositionWithImports(doc, params.position, environment), timeoutFor("definition"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      if (!outcome.locations.length && outcome.unavailableReason) explainUnavailable("type definition", outcome.unavailableReason);
      return outcome.locations;
    } catch (error) {
      logErr("typeDefinition", error);
      return [];
    }
  }, token);
});

connection.onImplementation(async (params: ImplementationParams, token?: CancellationToken): Promise<Location[]> => {
  if (failFast("implementation")) return [];
  if (isBreakerActive()) return [];
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  const requestedWorkspaceRevision = workspaceContentRevision;
  return withBackpressure(async () => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
    if (tooLarge(doc)) { explainLargeDocument("implementation"); return []; }
    try {
      const environment = await resolveCompletionEnvironment(doc.uri);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      const scan = await collectWorkspaceSourceDocuments(environment, token, () => requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision) && workspaceContentRevision === requestedWorkspaceRevision);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      if (workspaceContentRevision !== requestedWorkspaceRevision) return [];
      if (!scan.complete) { explainUnavailable("implementation", scan.reason ?? "Le scan du workspace a atteint sa limite de ressources."); return []; }
      const outcome = await withTimeout(implementationAtPositionWithImports(doc, params.position, environment, scan.documents), timeoutFor("definition"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      if (!outcome.locations.length && outcome.unavailableReason) explainUnavailable("implementation", outcome.unavailableReason);
      return outcome.locations;
    } catch (error) {
      logErr("implementation", error);
      return [];
    }
  }, token);
});

connection.onReferences(async (params: ReferenceParams, token?: CancellationToken): Promise<Location[]> => {
  if (failFast("references")) return [];
  if (isBreakerActive()) return [];
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  const requestedWorkspaceRevision = workspaceContentRevision;
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
    if (tooLarge(doc)) { explainLargeDocument("references"); return []; }
    try {
      const environment = await resolveCompletionEnvironment(doc.uri);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      const scan = await collectWorkspaceSourceDocuments(environment, token, () => requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision) && workspaceContentRevision === requestedWorkspaceRevision);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      if (workspaceContentRevision !== requestedWorkspaceRevision) return [];
      if (!scan.complete) { explainUnavailable("references", scan.reason ?? "Le scan du workspace a atteint sa limite de ressources."); return []; }
      const outcome = await withTimeout(referencesAtPositionWithWorkspace(doc, params.position, environment, scan.documents, params.context.includeDeclaration), timeoutFor("references"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      if (!outcome.locations.length && outcome.unavailableReason) explainUnavailable("references", outcome.unavailableReason);
      metric("references", start, doc.uri, outcome.locations.length);
      recordSuccess();
      return outcome.locations;
    } catch (e) {
      metric("references", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("references", e);
      return [];
    }
  }, token);
});

connection.languages.callHierarchy.onPrepare((params: CallHierarchyPrepareParams, token?: CancellationToken): Promise<CallHierarchyItem[] | null> => {
  return withHierarchyBackpressure(
    "callHierarchy.prepare",
    params.textDocument.uri,
    token,
    (doc) => prepareCallHierarchy(doc, params.position, params.textDocument.uri),
  );
});

connection.languages.callHierarchy.onIncomingCalls((params: CallHierarchyIncomingCallsParams, token?: CancellationToken): Promise<CallHierarchyIncomingCall[] | null> => {
  return withHierarchyBackpressure(
    "callHierarchy.incoming",
    params.item.uri,
    token,
    (doc) => callHierarchyIncoming(doc, params.item),
  );
});

connection.languages.callHierarchy.onOutgoingCalls((params: CallHierarchyOutgoingCallsParams, token?: CancellationToken): Promise<CallHierarchyOutgoingCall[] | null> => {
  return withHierarchyBackpressure(
    "callHierarchy.outgoing",
    params.item.uri,
    token,
    (doc) => callHierarchyOutgoing(doc, params.item),
  );
});

connection.languages.typeHierarchy.onPrepare((params: TypeHierarchyPrepareParams, token?: CancellationToken): Promise<TypeHierarchyItem[] | null> => {
  return withHierarchyBackpressure(
    "typeHierarchy.prepare",
    params.textDocument.uri,
    token,
    (doc) => prepareTypeHierarchy(doc, params.position, params.textDocument.uri),
  );
});

connection.languages.typeHierarchy.onSupertypes((params: TypeHierarchySupertypesParams, token?: CancellationToken): Promise<TypeHierarchyItem[] | null> => {
  return withHierarchyBackpressure(
    "typeHierarchy.supertypes",
    params.item.uri,
    token,
    (doc) => typeHierarchySupertypes(doc, params),
  );
});

connection.languages.typeHierarchy.onSubtypes((params: TypeHierarchySubtypesParams, token?: CancellationToken): Promise<TypeHierarchyItem[] | null> => {
  return withHierarchyBackpressure(
    "typeHierarchy.subtypes",
    params.item.uri,
    token,
    (doc) => typeHierarchySubtypes(doc, params),
  );
});

connection.onPrepareRename(async (params: PrepareRenameParams, token?: CancellationToken): Promise<{ range: Range; placeholder: string } | null> => {
  if (failFast("rename")) return null;
  if (isBreakerActive()) return null;
  // Rename is an interactive request and must not wait behind background
  // semantic work. Its own computation is bounded below.
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  return (async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
    if (tooLarge(doc)) { explainLargeDocument("rename"); return null; }
    try {
      const environment = await resolveCompletionEnvironment(doc.uri);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
      const result = await withTimeout(resolveRenameTarget(doc, params.position, environment), timeoutFor("rename"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
      if (!result.target && result.unavailableReason) explainUnavailable("rename", result.unavailableReason);
      if (result.target) metric("prepareRename", start, doc.uri);
      recordSuccess();
      return result.target ? { range: result.target.range, placeholder: result.target.name } : null;
    } catch (e) {
      metric("prepareRename", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("prepareRename", e);
      return null;
    }
  })();
});

// Correction de type: renvoie WorkspaceEdit
connection.onRenameRequest(async (params: RenameParams, token?: CancellationToken): Promise<WorkspaceEdit | null> => {
  if (failFast("rename")) return null;
  if (isBreakerActive()) return null;
  // Rename is an interactive request and must not wait behind background
  // semantic work. Its own computation is bounded below.
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  return (async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
    if (tooLarge(doc)) { explainLargeDocument("rename"); return null; }
    try {
      const environment = await resolveCompletionEnvironment(doc.uri);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
      const resolution = await withTimeout(resolveRenameTarget(doc, params.position, environment), timeoutFor("rename"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
      const target = resolution.target;
      if (!target) {
        explainUnavailable("rename", resolution.unavailableReason ?? "Le symbole sous le curseur n’est pas renommable.");
        return null;
      }
      const invalidName = validateRenameName(params.newName);
      if (invalidName) {
        explainUnavailable("rename", invalidName);
        return null;
      }
      if (!clientSupportsDocumentChanges) {
        explainUnavailable("rename", "Le client ne prend pas en charge les edits versionnés; le renommage est refusé pour protéger les buffers modifiés.");
        return null;
      }

      const scan = requiresWorkspaceRenameScan(target)
        ? await collectWorkspaceRenameDocuments(environment, renameTargetUri(target), token, () => requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision))
        : openDocumentRenameSnapshot(doc.uri);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
      if (requiresWorkspaceRenameScan(target) && !scan.complete) {
        explainUnavailable("rename", scan.reason ?? "Le workspace n’a pas pu être indexé entièrement; aucun edit partiel ne sera proposé.");
        return null;
      }
      const referenceOutcome = await withTimeout(
        renameReferencesForTarget(doc, params.position, target, environment, scan.documents),
        Math.max(timeoutFor("rename"), 2500),
      );
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
      if (referenceOutcome.unavailableReason || referenceOutcome.locations.length === 0) {
        explainUnavailable("rename", referenceOutcome.unavailableReason ?? "Aucune référence fiable n’a été trouvée.");
        return null;
      }
      const validation = validateRenameLocations(target.name, params.newName, referenceOutcome.locations, scan.documents);
      if (validation) {
        explainUnavailable("rename", validation);
        return null;
      }
      const snapshotError = await validateRenameSnapshot(scan, token, () => requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return null;
      if (snapshotError) {
        explainUnavailable("rename", snapshotError);
        return null;
      }

      const grouped = new Map<string, TextEdit[]>();
      for (const location of referenceOutcome.locations) {
        const edits = grouped.get(location.uri) ?? [];
        edits.push(TextEdit.replace(location.range, params.newName));
        grouped.set(location.uri, edits);
      }
      const documentChanges = Array.from(grouped, ([uri, edits]) => ({
        textDocument: { uri, version: documents.get(uri)?.version ?? null },
        edits: edits.sort((left, right) => right.range.start.line - left.range.start.line || right.range.start.character - left.range.start.character),
      }));
      const total = documentChanges.reduce((count, change) => count + change.edits.length, 0);
      const we: WorkspaceEdit = { documentChanges };
      metric("rename", start, doc.uri, total);
      recordSuccess();
      return we;
    } catch (e) {
      metric("rename", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("rename", e);
      return null;
    }
  })();
});

connection.workspace.onWillRenameFiles(async (params: RenameFilesParams, token?: CancellationToken): Promise<WorkspaceEdit | null> => {
  const requestedConfigurationRevision = configurationRevision;
  const requestedWorkspaceRevision = workspaceContentRevision;
  const keepGoing = (): boolean => configurationRevision === requestedConfigurationRevision && workspaceContentRevision === requestedWorkspaceRevision;
  if (!clientSupportsDocumentChanges) {
    explainUnavailable("file rename", "Le client ne prend pas en charge les edits versionnés; les imports ne peuvent pas être modifiés en sécurité.");
    return null;
  }
  if (params.files.length === 0) return null;
  try {
    const environment = await resolveCompletionEnvironment(params.files[0]?.oldUri ?? "");
    if (cancelled(token) || !keepGoing()) return null;
    const edit = await prepareModulePathFileRename(params, environment, token, keepGoing);
    if (cancelled(token) || !keepGoing()) return null;
    if (edit.reason) {
      explainUnavailable("file rename", edit.reason);
      return null;
    }
    return edit.workspaceEdit ?? null;
  } catch (error) {
    logErr("willRenameFiles", error);
    explainUnavailable("file rename", "La correspondance du fichier avec les chemins de modules n’a pas pu être vérifiée.");
    return null;
  }
});

connection.languages.inlayHint.on(async (params: InlayHintParams, token?: CancellationToken): Promise<InlayHint[]> => {
  if (failFast("inlayHints")) return [];
  if (isBreakerActive()) return [];
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
    if (tooLarge(doc)) { explainLargeDocument("inlayHints"); return []; }
    if (!featureEnabled("inlayHints")) return [];
    try {
      const environment = await resolveCompletionEnvironment(doc.uri);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      const prefs = await resolveInlayPrefs(params.textDocument.uri);
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      const hints = await withTimeout(
        provideInlayHints(doc, params.range, prefs, environment),
        timeoutFor("inlayHints")
      );
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
      metric("inlayHints", start, doc.uri, hints.length);
      recordSuccess();
      return hints;
    } catch (e) {
      metric("inlayHints", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("inlayHints", e);
      return [];
    }
  }, token);
});

connection.onCodeLens((params: CodeLensParams): CodeLens[] => {
  if (!isCapabilityAdvertised("code-lens")) return [];
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return [];
  if (tooLarge(doc)) { explainLargeDocument("code lens"); return []; }
  return sourceIndex(doc).symbols
    .filter((symbol) => symbol.role === "procedure" || symbol.role === "type")
    .map((symbol) => {
      const position = doc.positionAt(symbol.nameStart);
      const lens = CodeLens.create(LspRange.create(position, position));
      lens.command = {
        title: "Find references",
        command: "vitte.codelens.openRefs",
        arguments: [doc.uri, position],
      };
      return lens;
    });
});

connection.onFoldingRanges((params: FoldingRangeParams): FoldingRange[] => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc || !isCapabilityAdvertised("folding-ranges")) return [];
  if (tooLarge(doc)) { explainLargeDocument("folding ranges"); return []; }
  return provideFoldingRanges(doc);
});

connection.onSelectionRanges((params: SelectionRangeParams): SelectionRange[] => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc || !isCapabilityAdvertised("selection-ranges")) return [];
  if (tooLarge(doc)) { explainLargeDocument("selection ranges"); return []; }
  return provideSelectionRanges(doc, params.positions);
});

connection.onDocumentLinks(async (params: DocumentLinkParams, token?: CancellationToken): Promise<DocumentLink[]> => {
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  const doc = documents.get(params.textDocument.uri);
  if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision) || !isCapabilityAdvertised("document-links")) return [];
  if (tooLarge(doc)) { explainLargeDocument("document links"); return []; }
  try {
    const environment = await resolveCompletionEnvironment(doc.uri);
    if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return [];
    const links = await provideDocumentLinks(doc, environment);
    return requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision) ? links : [];
  } catch (error) {
    logErr("documentLinks", error);
    return [];
  }
});

connection.onWorkspaceSymbol(async (params: WorkspaceSymbolParams, token?: CancellationToken): Promise<WorkspaceSymbol[]> => {
  if (failFast("workspaceSymbols")) return [];
  if (isBreakerActive()) return [];
  const requestedConfigurationRevision = configurationRevision;
  const requestedWorkspaceRevision = workspaceContentRevision;
  return withBackpressure(async () => {
    const start = now();
    if (cancelled(token) || configurationRevision !== requestedConfigurationRevision || workspaceContentRevision !== requestedWorkspaceRevision) return [];
    try {
      const roots = await resolveWorkspaceRoots();
      const sourceDocsByUri = new Map<string, TextDocument>();
      const scanDeadline = Date.now() + 450;
      for (const root of roots) {
        if (cancelled(token) || configurationRevision !== requestedConfigurationRevision || workspaceContentRevision !== requestedWorkspaceRevision) return [];
        if (sourceDocsByUri.size >= 256 || Date.now() >= scanDeadline) {
          explainUnavailable("workspace symbols", "Le scan a dépassé sa limite de 256 documents ou de 450 ms.");
          return [];
        }
        const environment = await resolveCompletionEnvironment(pathToFileURL(root).toString());
        if (cancelled(token) || configurationRevision !== requestedConfigurationRevision || workspaceContentRevision !== requestedWorkspaceRevision) return [];
        const scan = await collectWorkspaceSourceDocuments(environment, token, () => configurationRevision === requestedConfigurationRevision && workspaceContentRevision === requestedWorkspaceRevision, scanDeadline);
        if (!scan.complete) {
          explainUnavailable("workspace symbols", scan.reason ?? "Le workspace symbol scan exceeded its resource budget.");
          return [];
        }
        for (const item of scan.documents) {
          if (cancelled(token)) return [];
          sourceDocsByUri.set(item.uri, item.doc);
          if (sourceDocsByUri.size >= 256) break;
        }
      }
      if (cancelled(token) || configurationRevision !== requestedConfigurationRevision || workspaceContentRevision !== requestedWorkspaceRevision) return [];
      const sourceDocs = [...sourceDocsByUri].map(([uri, doc]) => ({ uri, doc }));
      const symbols = await withTimeout(Promise.resolve(workspaceSymbols(params.query ?? "", sourceDocs, 200)), timeoutFor("workspaceSymbols"));
      metric("workspaceSymbols", start, params.query ?? "", symbols.length);
      recordSuccess();
      return cancelled(token) || configurationRevision !== requestedConfigurationRevision || workspaceContentRevision !== requestedWorkspaceRevision ? [] : symbols;
    } catch (e) {
      metric("workspaceSymbols", start, params.query ?? "workspaceSymbols", undefined, e);
      recordFailure();
      logErr("workspaceSymbols", e);
      return [];
    }
  }, token);
});

connection.languages.semanticTokens.on(async (params: SemanticTokensParams, token?: CancellationToken) => {
  if (failFast("semanticTokens")) return emptySemanticTokens();
  if (isBreakerActive()) return emptySemanticTokens();
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  return withBackpressure(async () => {
    const start = now();
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return emptySemanticTokens();
    if (tooLarge(doc)) { explainLargeDocument("semantic tokens"); return emptySemanticTokens(); }
    try {
      const tokens = await withTimeout(Promise.resolve(createSemanticTokens(doc)), timeoutFor("semanticTokens"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return emptySemanticTokens();
      tokens.resultId = rememberSemanticTokens(doc.uri, tokens.data);
      metric("semanticTokens", start, doc.uri, tokens.data.length / 5);
      recordSuccess();
      return tokens;
    } catch (e) {
      metric("semanticTokens", start, doc?.uri ?? "unknown", undefined, e);
      recordFailure();
      logErr("semanticTokens", e);
      return emptySemanticTokens();
    }
  }, token);
});

connection.languages.semanticTokens.onDelta(async (params: SemanticTokensDeltaParams, token?: CancellationToken): Promise<SemanticTokens | SemanticTokensDelta> => {
  const requestedVersion = documents.get(params.textDocument.uri)?.version;
  const requestedConfigurationRevision = configurationRevision;
  const full = async (): Promise<SemanticTokens> => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return { ...emptySemanticTokens(), resultId: rememberSemanticTokens(params.textDocument.uri, []) };
    if (tooLarge(doc)) { explainLargeDocument("semantic tokens"); return { ...emptySemanticTokens(), resultId: rememberSemanticTokens(params.textDocument.uri, []) }; }
    try {
      const tokens = await withTimeout(Promise.resolve(createSemanticTokens(doc)), timeoutFor("semanticTokens"));
      if (!requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return { ...emptySemanticTokens(), resultId: rememberSemanticTokens(params.textDocument.uri, []) };
      tokens.resultId = rememberSemanticTokens(doc.uri, tokens.data);
      return tokens;
    } catch (error) {
      logErr("semanticTokensDelta", error);
      return { ...emptySemanticTokens(), resultId: rememberSemanticTokens(doc.uri, []) };
    }
  };
  if (failFast("semanticTokens") || isBreakerActive()) return full();
  return withBackpressure(async () => {
    const doc = documents.get(params.textDocument.uri);
    if (!doc || !requestDocumentIsCurrent(doc, requestedVersion, token, requestedConfigurationRevision)) return full();
    const previous = semanticTokenHistory.get(doc.uri)?.get(params.previousResultId);
    if (!previous) return full();
    const current = await full();
    const next = current.data;
    let prefix = 0;
    const oldCount = Math.floor(previous.length / 5), newCount = Math.floor(next.length / 5);
    while (prefix < oldCount && prefix < newCount && sameSemanticToken(previous, next, prefix)) prefix++;
    if (prefix === oldCount && prefix === newCount) return { resultId: current.resultId, edits: [] };
    let suffix = 0;
    while (suffix < oldCount - prefix && suffix < newCount - prefix && sameSemanticToken(previous, next, oldCount - suffix - 1, newCount - suffix - 1)) suffix++;
    const edit: SemanticTokensEdit = {
      start: prefix * 5,
      deleteCount: (oldCount - prefix - suffix) * 5,
      data: next.slice(prefix * 5, (newCount - suffix) * 5),
    };
    return { resultId: current.resultId, edits: [edit] };
  }, token);
});

function sameSemanticToken(left: number[], right: number[], leftIndex: number, rightIndex = leftIndex): boolean {
  const offsetLeft = leftIndex * 5, offsetRight = rightIndex * 5;
  for (let field = 0; field < 5; field++) if (left[offsetLeft + field] !== right[offsetRight + field]) return false;
  return true;
}

/* -------------------------------- Diagnostics ----------------------------- */
const lintTimers = new Map<string, NodeJS.Timeout>();
const lintQueue: string[] = [];
const lintQueued = new Set<string>();
let lintWorkerRunning = false;
const lintResultCache = new Map<string, { hash: number; diagnostics: ReturnType<typeof lintToPublishable> }>();

function runLint(doc: TextDocument): void {
  const t0 = now();
  try {
    if (!featureEnabled("lint") || isBreakerActive()) {
      void connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] });
      return;
    }
    if (tooLarge(doc)) { void connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] }); return; }
    const text = doc.getText();
    const uri = doc.uri;
    const hash = fastHash(`${text}\0${JSON.stringify(globalSettings.lint)}`);
    const cached = lintResultCache.get(uri);
    const diags = cached?.hash === hash
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
  if (!featureEnabled("lint")) {
    const timer = lintTimers.get(doc.uri);
    if (timer) clearTimeout(timer);
    lintTimers.delete(doc.uri);
    void connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] });
    return;
  }
  const key = doc.uri;
  const base = Math.max(0, globalSettings.lintDebounceMs | 0);
  const sizeKB = Buffer.byteLength(doc.getText(), "utf8") / 1024;
  const isVit = doc.uri.endsWith(".vit") || doc.uri.endsWith(".vitl");
  const sizePenalty = sizeKB > 1024 ? 900 : sizeKB > 512 ? 550 : sizeKB > 256 ? 280 : sizeKB > 128 ? 120 : 0;
  const extPenalty = isVit && sizeKB > 128 ? 120 : 0;
  const delay = Math.min(2500, base + sizePenalty + extPenalty);
  const prev = lintTimers.get(key);
  if (prev) clearTimeout(prev);
  lintTimers.set(key, setTimeout(() => enqueueLint(key), delay));
}

function safeIndexDocument(doc: TextDocument): void {
  const maxIndexedDocuments = 4096;
  const alreadyIndexed = getIndex().has(doc.uri);
  if (tooLarge(doc) || memoryExceeded() || (!alreadyIndexed && getIndex().size >= maxIndexedDocuments)) {
    removeWorkspaceDocument(doc.uri);
    const prior = indexTimers.get(doc.uri);
    if (prior) clearTimeout(prior);
    indexTimers.delete(doc.uri);
    connection.console.warn(`[index] skipped document within configured resource limits (${safeDocumentLabel(doc.uri)})`);
    return;
  }
  // Debounce full source indexing during typing. The open TextDocument remains
  // authoritative for live LSP requests; this index serves workspace views.
  removeWorkspaceDocument(doc.uri);
  const prior = indexTimers.get(doc.uri);
  if (prior) clearTimeout(prior);
  const version = doc.version;
  indexTimers.set(doc.uri, setTimeout(() => {
    indexTimers.delete(doc.uri);
    const current = documents.get(doc.uri);
    if (!current || current.version !== version || serverShuttingDown) return;
    if (memoryExceeded() || getIndex().size >= maxIndexedDocuments) return;
    indexWorkspaceDocument(current);
  }, 120));
}

const MAX_LINT_QUEUE = 512;
const indexTimers = new Map<string, NodeJS.Timeout>();

function enqueueLint(uri: string): void {
  if (lintQueued.has(uri)) return;
  if (lintQueue.length >= MAX_LINT_QUEUE) {
    const dropped = lintQueue.shift();
    if (dropped) {
      lintQueued.delete(dropped);
      void connection.sendDiagnostics({ uri: dropped, diagnostics: [] });
      connection.console.warn(`[lint] queue limit reached; cleared pending diagnostics for ${safeDocumentLabel(dropped)}`);
    }
  }
  lintQueued.add(uri);
  lintQueue.push(uri);
  void processLintQueue();
}

function processLintQueue(): void {
  if (lintWorkerRunning) return;
  lintWorkerRunning = true;
  const next = (): void => {
    if (serverShuttingDown) { lintWorkerRunning = false; return; }
    const uri = lintQueue.shift();
    if (!uri) { lintWorkerRunning = false; return; }
    lintQueued.delete(uri);
    const doc = documents.get(uri);
    if (doc) runLint(doc);
    setImmediate(next);
  };
  setImmediate(next);
}

/* --------------------------------- Events -------------------------------- */

documents.onDidOpen((e) => { workspaceContentRevision += 1; safeIndexDocument(e.document); scheduleLint(e.document); });
documents.onDidChangeContent((e) => {
  workspaceContentRevision += 1;
  safeIndexDocument(e.document);
  void connection.sendDiagnostics({ uri: e.document.uri, diagnostics: [] });
  scheduleLint(e.document);
});
documents.onDidClose((e) => {
  workspaceContentRevision += 1;
  removeWorkspaceDocument(e.document.uri);
  void connection.sendDiagnostics({ uri: e.document.uri, diagnostics: [] });
  const timer = lintTimers.get(e.document.uri);
  if (timer) clearTimeout(timer);
  lintTimers.delete(e.document.uri);
  const indexTimer = indexTimers.get(e.document.uri);
  if (indexTimer) clearTimeout(indexTimer);
  indexTimers.delete(e.document.uri);
  lintResultCache.delete(e.document.uri);
  semanticTokenHistory.delete(e.document.uri);
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

function safeDocumentLabel(uri: string): string {
  try { return path.basename(fileURLToPath(uri)) || "workspace"; } catch { return "document"; }
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

function quickFixForDiagnostic(doc: TextDocument, d: Diagnostic): CodeAction | null {
  const code = String(d.code ?? "");
  const lineNo = d.range.start.line;
  const lineRange = lineRangeAt(doc, lineNo);
  const lineText = doc.getText(lineRange);

  if (code === "format.trailingWhitespace") {
    const fixed = lineText.replace(/[ \t]+$/g, "");
    if (fixed === lineText) return null;
    return {
      title: "Vitte: Trim trailing whitespace",
      kind: "quickfix",
      isPreferred: false,
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
      isPreferred: false,
      diagnostics: [d],
      edit: { changes: { [doc.uri]: [TextEdit.replace(lineRange, fixed)] } },
    };
  }

  return null;
}

function lineRangeAt(doc: TextDocument, lineNo: number): Range {
  const line = Math.max(0, Math.min(lineNo, doc.lineCount - 1));
  const text = doc.getText(Range.create(Position.create(line, 0), Position.create(line + 1, 0)));
  const lineText = text.replace(/\r?\n$/, "");
  return Range.create(Position.create(line, 0), Position.create(line, lineText.length));
}

interface RenameWorkspaceSnapshot {
  documents: { uri: string; doc: TextDocument }[];
  complete: boolean;
  reason?: string;
  diskFiles: Map<string, { path: string; mtimeMs: number; size: number; contentHash: string }>;
  openVersions: Map<string, number>;
  roots?: string[];
  directories?: Map<string, { mtimeMs: number; ctimeMs: number }>;
}

function requiresWorkspaceRenameScan(target: RenameTarget): boolean {
  return target.kind === "imported-symbol" || (target.kind === "source" && target.symbol.isPublic === true);
}

function renameTargetUri(target: RenameTarget): string | undefined {
  if (target.kind === "source") return target.symbol.location.uri;
  if (target.kind === "imported-symbol" || target.kind === "import-alias") return target.symbol.location?.uri;
  return target.module.uri;
}

function openDocumentRenameSnapshot(primaryUri?: string): RenameWorkspaceSnapshot {
  const openDocuments = primaryUri ? [documents.get(primaryUri)].filter((doc): doc is TextDocument => Boolean(doc)) : documents.all();
  const docs = openDocuments.map((doc) => ({ uri: doc.uri, doc }));
  return {
    documents: docs,
    complete: true,
    diskFiles: new Map(),
    openVersions: new Map(docs.map(({ uri, doc }) => [uri, doc.version])),
  };
}

async function collectWorkspaceRenameDocuments(
  environment: CompletionEnvironment,
  targetUri?: string,
  token?: CancellationToken,
  keepGoing: () => boolean = () => true,
): Promise<RenameWorkspaceSnapshot> {
  const roots = Array.from(new Set([...(environment.workspaceRoots ?? []), environment.projectRoot, ...(environment.modulePaths ?? [])]
    .filter((root): root is string => Boolean(root)).map((root) => path.resolve(root))));
  const inRoots = (uri: string): boolean => {
    if (!uri.startsWith("file:")) return false;
    let file: string;
    try { file = path.resolve(fileURLToPath(uri)); } catch { return false; }
    return roots.some((root) => {
      const relative = path.relative(root, file);
      return relative === "" || (relative !== ".." && !relative.startsWith(`..${path.sep}`) && !path.isAbsolute(relative));
    });
  };
  const openDocuments = documents.all().filter((doc) => inRoots(doc.uri) || doc.uri === targetUri);
  const open: RenameWorkspaceSnapshot = {
    documents: openDocuments.map((doc) => ({ uri: doc.uri, doc })),
    complete: true,
    diskFiles: new Map(),
    openVersions: new Map(openDocuments.map((doc) => [doc.uri, doc.version])),
    roots,
    directories: new Map(),
  };
  const out = new Map(open.documents.map((entry) => [entry.uri, entry.doc]));
  const diskFiles = new Map<string, { path: string; mtimeMs: number; size: number; contentHash: string }>();
  if (roots.length === 0) {
    return { ...open, complete: false, reason: "Aucune racine de workspace n’est définie pour rechercher toutes les références.", diskFiles };
  }
  if (targetUri && !inRoots(targetUri)) {
    return { ...open, complete: false, reason: "La déclaration ciblée se trouve hors des racines indexées; les références du projet ne peuvent pas être vérifiées.", diskFiles };
  }

  const realRoots: string[] = [];
  try {
    for (const root of roots) realRoots.push(await fs.realpath(root));
  } catch {
    return { ...open, complete: false, reason: "Une racine de workspace ou de modules n’est pas accessible; le renommage est refusé.", diskFiles };
  }
  const realPathWithinRoots = (candidate: string): boolean => realRoots.some((root) => {
    const relative = path.relative(normalizedPathForComparison(root), normalizedPathForComparison(candidate));
    return relative === "" || (!relative.startsWith(`..${path.sep}`) && relative !== ".." && !path.isAbsolute(relative));
  });
  const openRealPaths = new Set<string>();
  for (const document of openDocuments) {
    try { openRealPaths.add(normalizedPathForComparison(await fs.realpath(fileURLToPath(document.uri)))); } catch { /* untitled or removed buffer */ }
  }
  if (targetUri) {
    try { openRealPaths.add(normalizedPathForComparison(await fs.realpath(fileURLToPath(targetUri)))); } catch { /* the target read reports the failure */ }
  }

  const deadline = Date.now() + 2000;
  const fileLimit = 1024;
  const totalByteLimit = 64 * 1024 * 1024;
  const filePaths = new Set<string>();
  const visitedDirectories = new Set<string>();
  const maxQueuedDirectories = 4096;
  const directorySnapshots = new Map<string, { mtimeMs: number; ctimeMs: number }>();
  const queue = [...roots];
  const skip = new Set([".git", "node_modules", ".vscode", "out", "dist", "build", "target", ".next"]);
  let totalBytes = 0;
  let complete = true;
  let reason: string | undefined;

  while (queue.length > 0) {
    if (cancelled(token) || !keepGoing()) {
      complete = false;
      reason = "Le renommage a été annulé après un changement de document, configuration ou workspace.";
      break;
    }
    if (Date.now() >= deadline) {
      complete = false;
      reason = "Le scan de renommage a atteint sa limite de temps; aucun résultat partiel ne sera appliqué.";
      break;
    }
    const directory = queue.shift()!;
    const normalizedDirectory = path.resolve(directory);
    let realDirectory: string;
    try { realDirectory = await fs.realpath(normalizedDirectory); } catch {
      complete = false;
      reason = `Le dossier « ${normalizedDirectory} » ne peut pas être résolu; l’ensemble des références est inconnu.`;
      continue;
    }
    if (!realPathWithinRoots(realDirectory)) continue;
    const canonicalDirectory = normalizedPathForComparison(realDirectory);
    if (visitedDirectories.has(canonicalDirectory)) continue;
    visitedDirectories.add(canonicalDirectory);
    let directoryHandle: Awaited<ReturnType<typeof fs.opendir>>;
    try {
      directoryHandle = await fs.opendir(normalizedDirectory);
      const stats = await fs.stat(realDirectory);
      directorySnapshots.set(realDirectory, { mtimeMs: stats.mtimeMs, ctimeMs: stats.ctimeMs });
    } catch {
      complete = false;
      reason = `Le dossier « ${normalizedDirectory} » ne peut pas être lu; l’ensemble des références est inconnu.`;
      continue;
    }
    try {
      for await (const entry of directoryHandle) {
        if (cancelled(token) || !keepGoing()) {
          complete = false;
          reason = "Le renommage a été annulé après un changement de document, configuration ou workspace.";
          break;
        }
        if (Date.now() >= deadline) {
          complete = false;
          reason = "Le scan de renommage a atteint sa limite de temps; aucun résultat partiel ne sera appliqué.";
          break;
        }
        const absolute = path.join(normalizedDirectory, entry.name);
        let isDirectory = entry.isDirectory();
        let isFile = entry.isFile();
        if (entry.isSymbolicLink()) {
          try {
            const targetStats = await fs.stat(absolute);
            const realTarget = await fs.realpath(absolute);
            if (!realPathWithinRoots(realTarget)) continue;
            isDirectory = targetStats.isDirectory();
            isFile = targetStats.isFile();
          } catch { continue; }
        }
        if (isDirectory) {
          if (skip.has(entry.name)) continue;
          if (queue.length >= maxQueuedDirectories) {
            complete = false;
            reason = "Le parcours du workspace dépasse la limite de 4 096 dossiers.";
            break;
          }
          queue.push(absolute);
          continue;
        }
        if (!isFile || !/\.(?:vit|vitl|vitte)$/i.test(entry.name)) continue;
        try {
          const realFile = await fs.realpath(absolute);
          if (realPathWithinRoots(realFile)) filePaths.add(path.resolve(realFile));
        } catch {
          complete = false;
          reason = `Le fichier « ${absolute} » ne peut pas être résolu; l’ensemble des références est inconnu.`;
        }
        if (filePaths.size > fileLimit) {
          complete = false;
          reason = `Le workspace dépasse la limite sûre de ${fileLimit} fichiers Vitte pour le renommage.`;
          break;
        }
      }
    } catch {
      complete = false;
      reason = `Le contenu du dossier « ${normalizedDirectory} » n’a pas pu être parcouru complètement.`;
    }
    if (!complete) break;
  }

  for (const file of filePaths) {
    if (cancelled(token) || !keepGoing()) {
      complete = false;
      reason = "Le renommage a été annulé après un changement de document, configuration ou workspace.";
      break;
    }
    if (Date.now() >= deadline) {
      complete = false;
      reason = "Le scan de renommage a atteint sa limite de temps; aucun résultat partiel ne sera appliqué.";
      break;
    }
    if (openRealPaths.has(normalizedPathForComparison(file))) continue;
    const uri = pathToFileURL(file).toString();
    if (out.has(uri)) continue;
    try {
      const stats = await fs.stat(file);
      if (cancelled(token) || !keepGoing()) {
        complete = false;
        reason = "Le renommage a été annulé après un changement de document, configuration ou workspace.";
        break;
      }
      if (!stats.isFile() || stats.size > 1024 * 1024 || totalBytes + stats.size > totalByteLimit) {
        complete = false;
        reason = `Le fichier « ${file} » dépasse la limite de lecture sûre pour le renommage.`;
        continue;
      }
      totalBytes += stats.size;
      const source = await fs.readFile(file, "utf8");
      if (cancelled(token) || !keepGoing()) {
        complete = false;
        reason = "Le renommage a été annulé après un changement de document, configuration ou workspace.";
        break;
      }
      out.set(uri, TextDocument.create(uri, file.endsWith(".vitte") ? "vitte" : "vit", 0, source));
      diskFiles.set(uri, { path: file, mtimeMs: stats.mtimeMs, size: stats.size, contentHash: hashRenameText(source) });
    } catch {
      complete = false;
      reason = `Le fichier « ${file} » a changé ou n’est pas lisible pendant le scan de renommage.`;
    }
  }

  if (targetUri && !out.has(targetUri)) {
    try {
      const file = fileURLToPath(targetUri);
      if (!inRoots(targetUri)) {
        complete = false;
        reason = "La déclaration ciblée se trouve hors des racines indexées; les références du projet ne peuvent pas être vérifiées.";
      }
      const stats = await fs.stat(file);
      if (stats.size > 1024 * 1024) throw new Error("source exceeds safe file size");
      const source = await fs.readFile(file, "utf8");
      out.set(targetUri, TextDocument.create(targetUri, file.endsWith(".vitte") ? "vitte" : "vit", 0, source));
      diskFiles.set(targetUri, { path: file, mtimeMs: stats.mtimeMs, size: stats.size, contentHash: hashRenameText(source) });
    } catch {
      complete = false;
      reason ??= "La déclaration source n’a pas pu être lue pour vérifier le renommage.";
    }
  }

  return {
    documents: Array.from(out, ([uri, doc]) => ({ uri, doc })),
    complete,
    reason,
    diskFiles,
    openVersions: open.openVersions,
    roots,
    directories: directorySnapshots,
  };
}

async function validateRenameSnapshot(
  snapshot: RenameWorkspaceSnapshot,
  token?: CancellationToken,
  keepGoing: () => boolean = () => true,
): Promise<string | undefined> {
  if (cancelled(token) || !keepGoing()) return "Le renommage a été annulé après un changement de document, configuration ou workspace.";
  if (snapshot.roots) {
    for (const doc of documents.all()) {
      if (!doc.uri.startsWith("file:") || snapshot.openVersions.has(doc.uri)) continue;
      let file: string;
      try { file = path.resolve(fileURLToPath(doc.uri)); } catch { continue; }
      if (snapshot.roots.some((root) => {
        const relative = path.relative(root, file);
        return relative === "" || (relative !== ".." && !relative.startsWith(`..${path.sep}`) && !path.isAbsolute(relative));
      })) return "Un nouveau buffer Vitte du workspace a été ouvert pendant l’analyse; relancez le renommage.";
    }
  }
  for (const [uri, version] of snapshot.openVersions) {
    const current = documents.get(uri);
    if (current?.version !== version) return "Un buffer ouvert a changé pendant l’analyse; relancez le renommage.";
  }
  for (const file of snapshot.diskFiles.values()) {
    if (cancelled(token) || !keepGoing()) return "Le renommage a été annulé après un changement de document, configuration ou workspace.";
    try {
      const current = await fs.stat(file.path);
      if (current.mtimeMs !== file.mtimeMs || current.size !== file.size) return `Le fichier « ${file.path} » a changé pendant l’analyse; relancez le renommage.`;
      const content = await fs.readFile(file.path, "utf8");
      if (hashRenameText(content) !== file.contentHash) return `Le contenu de « ${file.path} » a changé pendant l’analyse; relancez le renommage.`;
    } catch {
      return `Le fichier « ${file.path} » a disparu pendant l’analyse; relancez le renommage.`;
    }
  }
  for (const [directory, before] of snapshot.directories ?? []) {
    if (cancelled(token) || !keepGoing()) return "Le renommage a été annulé après un changement de document, configuration ou workspace.";
    try {
      const current = await fs.stat(directory);
      if (current.mtimeMs !== before.mtimeMs || current.ctimeMs !== before.ctimeMs) {
        return `Le dossier « ${directory} » a changé pendant l’analyse; relancez le renommage.`;
      }
    } catch {
      return `Le dossier « ${directory} » a changé pendant l’analyse; relancez le renommage.`;
    }
  }
  return undefined;
}

function hashRenameText(text: string): string {
  return createHash("sha256").update(text, "utf8").digest("hex");
}

interface FileModuleMapping {
  oldUri: string;
  oldPath: string;
  newPath: string;
  oldModule: string;
  newModule: string;
}

async function prepareModulePathFileRename(
  params: RenameFilesParams,
  environment: CompletionEnvironment,
  token?: CancellationToken,
  keepGoing: () => boolean = () => true,
): Promise<{ workspaceEdit?: WorkspaceEdit; reason?: string }> {
  const roots = Array.from(new Set([...(environment.workspaceRoots ?? []), environment.projectRoot, ...(environment.modulePaths ?? [])]
    .filter((root): root is string => Boolean(root)).map((root) => path.resolve(root))));
  const mappings: FileModuleMapping[] = [];
  for (const file of params.files) {
    if (cancelled(token) || !keepGoing()) return { reason: "Le renommage du fichier a été annulé après un changement de configuration ou de workspace." };
    if (!file.oldUri.startsWith("file:") || !file.newUri.startsWith("file:")) continue;
    const oldPath = fileURLToPath(file.oldUri);
    const newPath = fileURLToPath(file.newUri);
    if (!/\.(?:vit|vitl|vitte)$/i.test(oldPath) || !/\.(?:vit|vitl|vitte)$/i.test(newPath)) continue;
    const result = findUniqueModulePathFileMove(oldPath, newPath, roots);
    if (!result.mapping) {
      return { reason: `Le fichier « ${oldPath} » correspond à ${result.candidateCount} mappings de projet; les chemins de modules ne seront pas devinés.` };
    }
    const mapping = result.mapping;
    const resolvedOld = await resolveModuleFiles(mapping.oldModule, file.oldUri, environment);
    if (cancelled(token) || !keepGoing()) return { reason: "Le renommage du fichier a été annulé après un changement de configuration ou de workspace." };
    if (resolvedOld.length !== 1 || path.resolve(resolvedOld[0] ?? "") !== path.resolve(oldPath)) {
      return { reason: `Le chemin « ${mapping.oldModule} » ne désigne pas uniquement « ${oldPath} » dans les racines Vitte.` };
    }
    if (mapping.newModule !== mapping.oldModule) {
      const resolvedNew = await resolveModuleFiles(mapping.newModule, file.oldUri, environment);
      if (cancelled(token) || !keepGoing()) return { reason: "Le renommage du fichier a été annulé après un changement de configuration ou de workspace." };
      if (resolvedNew.some((candidate) => path.resolve(candidate) !== path.resolve(oldPath))) {
        return { reason: `Le nouveau chemin « ${mapping.newModule} » existe déjà ou se résout vers plusieurs fichiers.` };
      }
    }
    mappings.push({ oldUri: file.oldUri, oldPath, newPath, oldModule: mapping.oldModule, newModule: mapping.newModule });
  }
  if (mappings.length === 0) return {};

  const scan = await collectWorkspaceRenameDocuments(environment, mappings[0]?.oldUri, token, keepGoing);
  if (!scan.complete) return { reason: scan.reason ?? "Le workspace n’a pas pu être indexé entièrement avant le renommage du fichier." };
  const grouped = new Map<string, TextEdit[]>();
  const imports = /\b(?:use|pull)\s+([A-Za-z_]\w*(?:\s*(?:::|\/)\s*[A-Za-z_]\w*)*)/g;
  for (const { uri, doc } of scan.documents) {
    if (cancelled(token) || !keepGoing()) return { reason: "Le renommage du fichier a été annulé après un changement de configuration ou de workspace." };
    const code = maskNonCode(doc.getText());
    for (const mapping of mappings) {
      if (uri === mapping.oldUri && mapping.newModule !== mapping.oldModule) {
        const namespace = /\bspace\s+([A-Za-z_]\w*(?:(?:::|\/)\s*[A-Za-z_]\w*)*)/g;
        let spaceMatch: RegExpExecArray | null;
        while ((spaceMatch = namespace.exec(code))) {
          const name = spaceMatch[1]?.replace(/\s+/g, "").replace(/\//g, "::");
          if (name !== mapping.oldModule) continue;
          const start = spaceMatch.index + spaceMatch[0].indexOf(spaceMatch[1] ?? "");
          addFileRenameEdit(grouped, uri, doc, start, (spaceMatch[1] ?? "").length, mapping.newModule);
        }
      }
      imports.lastIndex = 0;
      let importMatch: RegExpExecArray | null;
      while ((importMatch = imports.exec(code))) {
        const rawPath = importMatch[1] ?? "";
        const normalized = rawPath.replace(/\s+/g, "").replace(/\//g, "::");
        if (normalized !== mapping.oldModule) continue;
        const resolved = await resolveModuleFiles(normalized, uri, environment);
        if (cancelled(token) || !keepGoing()) return { reason: "Le renommage du fichier a été annulé après un changement de configuration ou de workspace." };
        if (resolved.length > 1) {
          return { reason: `L’import « ${mapping.oldModule} » se résout vers plusieurs modules depuis « ${uri} »; le renommage du fichier est refusé.` };
        }
        if (resolved.length !== 1 || path.resolve(resolved[0] ?? "") !== path.resolve(mapping.oldPath)) continue;
        const oldSegments = mapping.oldModule.split("::");
        const prefixPattern = new RegExp(`^[A-Za-z_]\\w*(?:\\s*(?:::|\\/)\\s*[A-Za-z_]\\w*){${Math.max(0, oldSegments.length - 1)}}`);
        const rawPrefix = prefixPattern.exec(rawPath)?.[0];
        if (!rawPrefix || rawPrefix.replace(/\s+/g, "").replace(/\//g, "::") !== mapping.oldModule) continue;
        const separator = rawPrefix.includes("/") && !rawPrefix.includes("::") ? "/" : "::";
        const replacement = mapping.newModule.replace(/::/g, separator);
        const pathStart = importMatch.index + importMatch[0].indexOf(rawPath);
        addFileRenameEdit(grouped, uri, doc, pathStart, rawPrefix.length, replacement);
      }
    }
  }
  const snapshotError = await validateRenameSnapshot(scan, token, keepGoing);
  if (snapshotError) return { reason: snapshotError };
  if (grouped.size === 0) return {};
  const documentChanges = Array.from(grouped, ([uri, edits]) => ({
    textDocument: { uri, version: documents.get(uri)?.version ?? null },
    edits: edits.sort((left, right) => right.range.start.line - left.range.start.line || right.range.start.character - left.range.start.character),
  }));
  return { workspaceEdit: { documentChanges } };
}

function addFileRenameEdit(grouped: Map<string, TextEdit[]>, uri: string, doc: TextDocument, start: number, length: number, replacement: string): void {
  const edits = grouped.get(uri) ?? [];
  edits.push(TextEdit.replace(Range.create(doc.positionAt(start), doc.positionAt(start + length)), replacement));
  grouped.set(uri, edits);
}

interface WorkspaceSourceScan {
  documents: { uri: string; doc: TextDocument }[];
  complete: boolean;
  reason?: string;
}

async function walkVitteFiles(root: string, limit: number, deadline: number, token?: CancellationToken, keepGoing: () => boolean = () => true): Promise<{ paths: string[]; complete: boolean }> {
  const out: string[] = [];
  const queue: string[] = [root];
  const visitedDirectories = new Set<string>();
  const maxQueuedDirectories = 4096;
  let realRoot: string;
  try { realRoot = await fs.realpath(root); } catch { return { paths: out, complete: false }; }
  let complete = true;
  const skip = new Set([".git", "node_modules", ".vscode", "out", "dist", "build", "target", ".next"]);

  while (queue.length > 0 && out.length < limit && Date.now() < deadline && !cancelled(token) && keepGoing()) {
    const dir = queue.shift()!;
    let realDirectory: string;
    try { realDirectory = await fs.realpath(dir); } catch { complete = false; continue; }
    const relativeDirectory = path.relative(realRoot, realDirectory);
    if (relativeDirectory.startsWith(`..${path.sep}`) || relativeDirectory === ".." || path.isAbsolute(relativeDirectory)) continue;
    if (visitedDirectories.has(realDirectory)) continue;
    visitedDirectories.add(realDirectory);
    let directoryHandle: Awaited<ReturnType<typeof fs.opendir>>;
    try {
      directoryHandle = await fs.opendir(dir);
    } catch {
      complete = false;
      continue;
    }
    try {
      for await (const e of directoryHandle) {
        if (out.length >= limit || Date.now() >= deadline || cancelled(token) || !keepGoing()) break;
        const entryPath = path.join(dir, e.name);
        let isDirectory = e.isDirectory();
        let isFile = e.isFile();
        if (e.isSymbolicLink()) {
          try {
            const target = await fs.stat(entryPath);
            isDirectory = target.isDirectory();
            isFile = target.isFile();
            const realTarget = await fs.realpath(entryPath);
            const relativeTarget = path.relative(realRoot, realTarget);
            if (relativeTarget.startsWith(`..${path.sep}`) || relativeTarget === ".." || path.isAbsolute(relativeTarget)) continue;
          } catch { continue; }
        }
        if (isDirectory) {
          if (skip.has(e.name)) continue;
          if (queue.length >= maxQueuedDirectories) return { paths: out, complete: false };
          queue.push(entryPath);
          continue;
        }
        if (!isFile) continue;
        const extension = path.extname(e.name).toLocaleLowerCase();
        if (extension !== ".vit" && extension !== ".vitl" && extension !== ".vitte") continue;
        out.push(entryPath);
      }
    } catch {
      return { paths: out, complete: false };
    }
  }
  if (cancelled(token) || !keepGoing() || Date.now() >= deadline || out.length >= limit || queue.length > 0) complete = false;
  return { paths: out, complete };
}

const workspaceSourceCache = new Map<string, { mtimeMs: number; ctimeMs: number; size: number; doc: TextDocument }>();

function normalizedPathForComparison(value: string): string {
  const normalized = path.resolve(value);
  return process.platform === "win32" ? normalized.toLocaleLowerCase("en-US") : normalized;
}

function isPathWithinRoots(uri: string, roots: string[]): boolean {
  if (!uri.startsWith("file://") || roots.length === 0) return false;
  let candidate: string;
  try { candidate = normalizedPathForComparison(fileURLToPath(uri)); } catch { return false; }
  return roots.some((root) => {
    const normalizedRoot = normalizedPathForComparison(root);
    const relative = path.relative(normalizedRoot, candidate);
    return relative === "" || (!relative.startsWith(`..${path.sep}`) && relative !== ".." && !path.isAbsolute(relative));
  });
}

async function collectWorkspaceSourceDocuments(environment: CompletionEnvironment, token?: CancellationToken, keepGoing: () => boolean = () => true, sharedDeadline?: number): Promise<WorkspaceSourceScan> {
  const out = new Map<string, TextDocument>();
  const roots = Array.from(new Set([...(environment.workspaceRoots ?? []), environment.projectRoot, ...(environment.modulePaths ?? [])].filter((root): root is string => Boolean(root))));
  if (roots.length === 0) return { documents: [], complete: true };
  const cacheScope = createHash("sha256").update(JSON.stringify({
    roots: roots.map(normalizedPathForComparison).sort(),
    compilerPath: environment.compilerPath ? normalizedPathForComparison(environment.compilerPath) : "",
  })).digest("hex").slice(0, 16);
  // Use open buffers only when they belong to this project. Their unsaved text
  // takes precedence over the on-disk copy below.
  const totalByteLimit = 16 * 1024 * 1024;
  let totalBytes = 0;
  for (const document of documents.all()) {
    if (cancelled(token) || !keepGoing()) return { documents: [], complete: false, reason: "La requête a été annulée après un changement de document ou de workspace." };
    if (!isPathWithinRoots(document.uri, roots)) continue;
    const bytes = Buffer.byteLength(document.getText(), "utf8");
    if (bytes > 1024 * 1024) return { documents: [], complete: false, reason: "Un buffer ouvert dépasse la limite de 1 Mio par fichier." };
    if (out.size >= 256 || totalBytes + bytes > totalByteLimit) return { documents: [], complete: false, reason: "Le scan dépasse sa limite de 256 documents ou de 16 Mio." };
    totalBytes += bytes;
    out.set(document.uri, document);
  }
  const deadline = Math.min(Date.now() + 450, sharedDeadline ?? Number.POSITIVE_INFINITY);
  const fileLimit = 256;
  let reason: string | undefined;
  for (const root of roots) {
    if (cancelled(token) || !keepGoing()) return { documents: [], complete: false, reason: "La requête a été annulée après un changement de document ou de workspace." };
    if (Date.now() >= deadline) { reason = "Le scan du workspace a dépassé sa limite de 450 ms."; break; }
    if (out.size >= fileLimit) { reason = "Le scan du workspace a dépassé sa limite de 256 documents."; break; }
    const walk = await walkVitteFiles(root, fileLimit - out.size, deadline, token, keepGoing);
    if (!walk.complete) { reason = cancelled(token) || !keepGoing() ? "La requête a été annulée après un changement de document ou de workspace." : "Le scan du workspace a atteint une limite de temps, de parcours ou d’accès aux dossiers."; break; }
    for (const file of walk.paths) {
      if (cancelled(token) || !keepGoing()) return { documents: [], complete: false, reason: "La requête a été annulée après un changement de document ou de workspace." };
      if (Date.now() >= deadline) { reason = "Le scan du workspace a dépassé sa limite de 450 ms."; break; }
      if (out.size >= fileLimit) { reason = "Le scan du workspace a dépassé sa limite de 256 documents."; break; }
      if (totalBytes >= totalByteLimit) { reason = "Le scan du workspace a dépassé sa limite de 16 Mio."; break; }
      const uri = pathToFileURL(file).toString();
      if (out.has(uri)) continue;
      try {
        const stats = await fs.stat(file);
        if (cancelled(token) || !keepGoing()) return { documents: [], complete: false, reason: "Request cancelled because the document or request changed." };
        if (!stats.isFile() || stats.size > 1024 * 1024 || totalBytes + stats.size > totalByteLimit) {
          reason = "Un fichier source dépasse la limite de 1 Mio par fichier ou de 16 Mio pour le workspace.";
          break;
        }
        totalBytes += stats.size;
        const cacheKey = `${cacheScope}\0${uri}`;
        const cached = workspaceSourceCache.get(cacheKey);
        if (cached && cached.mtimeMs === stats.mtimeMs && cached.ctimeMs === stats.ctimeMs && cached.size === stats.size) {
          out.set(uri, cached.doc);
          continue;
        }
        const source = await fs.readFile(file, "utf8");
        if (cancelled(token) || !keepGoing()) return { documents: [], complete: false, reason: "La requête a été annulée après un changement de document ou de workspace." };
        const doc = TextDocument.create(uri, path.extname(file).toLocaleLowerCase() === ".vitte" ? "vitte" : "vit", 0, source);
        if (workspaceSourceCache.size >= 512) {
          const oldest = workspaceSourceCache.keys().next().value;
          if (oldest) workspaceSourceCache.delete(oldest);
        }
        workspaceSourceCache.set(cacheKey, { mtimeMs: stats.mtimeMs, ctimeMs: stats.ctimeMs, size: stats.size, doc });
        out.set(uri, doc);
      } catch {
        reason = "Un fichier source a changé ou est devenu inaccessible pendant le scan du workspace.";
        break;
      }
    }
    if (reason) break;
  }
  if (!reason && (cancelled(token) || !keepGoing())) reason = "La requête a été annulée après un changement de document ou de workspace.";
  return { documents: Array.from(out, ([uri, doc]) => ({ uri, doc })), complete: !reason, ...(reason ? { reason } : {}) };
}
