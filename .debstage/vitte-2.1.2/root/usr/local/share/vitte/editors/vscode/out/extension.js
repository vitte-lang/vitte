"use strict";
/*
 * extension.ts — VS Code client for Vitte/Vit
 * - Full LSP client (start/stop/restart, traces, status, watchers)
 * - Commands: logs, restart, actions (format/organize/fix), rename, debug demos
 * - Progress UI, output channel, status bar, config-sync, file watcher
 * - Hardened: no-op when no editor, consistent error handling, strict types
 */
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.activate = activate;
exports.deactivate = deactivate;
const path = __importStar(require("node:path"));
const fs = __importStar(require("node:fs"));
const os = __importStar(require("node:os"));
const vscode = __importStar(require("vscode"));
const diagnosticsView_1 = require("./diagnosticsView");
const moduleExplorerView_1 = require("./moduleExplorerView");
const playgroundPanel_1 = require("./providers/playgroundPanel");
const offlineView_1 = require("./providers/offlineView");
const buildTasks_1 = require("./tasks/buildTasks");
const benchTasks_1 = require("./tasks/benchTasks");
const runtimeLocator_1 = require("./debug/runtimeLocator");
const adapterFactory_1 = require("./debug/adapterFactory");
const configurationProvider_1 = require("./debug/configurationProvider");
const telemetry_1 = require("./utils/telemetry");
const quickActions_1 = require("./commands/quickActions");
const metricsView_1 = require("./providers/metricsView");
const packageProblemsView_1 = require("./providers/packageProblemsView");
const moduleGraphView_1 = require("./providers/moduleGraphView");
const projectAssistant_1 = require("./commands/projectAssistant");
const advancedCodeActions_1 = require("./providers/advancedCodeActions");
const vitteCodeLens_1 = require("./providers/vitteCodeLens");
const enterpriseSuite_1 = require("./commands/enterpriseSuite");
const commandCenterView_1 = require("./providers/commandCenterView");
const node_1 = require("vscode-languageclient/node");
const diagnostics_1 = require("./utils/diagnostics");
const suggestions_1 = require("./utils/suggestions");
let client;
let output;
let statusItem;
let statusBaseIcon = "$(rocket)";
const STATUS_LABEL = "Vitte";
let statusLanguageSuffix = "";
let statusBaseTooltip = "Vitte Language Server";
let statusHealthIcon = "";
let statusHealthTooltip = "";
let statusOverrideText;
let statusOverrideTooltip;
let offlineReason;
let offlineBannerShown = false;
const recentStops = [];
const OFFLINE_STATUS_COLOR = new vscode.ThemeColor("statusBarItem.warningForeground");
const OFFLINE_STATUS_BG = new vscode.ThemeColor("statusBarItem.warningBackground");
let offlineRetryTimer;
let offlineRetryMs = 60000;
let lastActivationContext;
let editorLintCollection;
let offlineSince;
const formatOnSaveInFlight = new Set();
let healthCheckTimer;
let healthFailures = 0;
let healthRestartInFlight = false;
let reliabilityAttempts = 0;
let reliabilityNextDelayMs = 30000;
let activationStartedAt = Date.now();
const completionLatencyWindowMs = [];
const completionStreamingCachePrefix = new Map();
const completionStreamingCacheContext = new Map();
const completionStreamingCacheDocument = new Map();
const completionStreamingInFlight = new Map();
const completionPagingState = new Map();
const completionAstFingerprintByDoc = new Map();
const completionAstRefreshTimers = new Map();
let completionStreamingRefreshGuard = false;
let completionNextInvokeIsAutoRefresh = false;
let completionLoadNextPageKey;
const completionTop1StableByKey = new Map();
let completionIdlePrefetchTimer;
let completionPrefetchInFlight = false;
let completionPrefetchExpectedKey;
const typingSpeedByDoc = new Map();
const completionRequestStartedAt = new Map();
const completionRequestRefreshed = new Map();
const completionFirstUsableMarked = new Set();
const completionLspNegativeCache = new Map();
const completionStreamingFirstPaintMs = [];
const completionStreamingEnrichMs = [];
const completionFirstUsableMs = [];
let completionStreamingRefreshCount = 0;
let completionStreamingTimeoutCount = 0;
let completionStreamingTotalCount = 0;
let completionStreamingManualRefreshCount = 0;
let completionStreamingRicherRefreshCount = 0;
let completionStreamingIncompleteRefreshCount = 0;
let completionSuggestionShownCount = 0;
let completionSuggestionAcceptedCount = 0;
let completionSuggestionCanceledCount = 0;
let completionSuggestionPendingAccepted = false;
let completionPendingRejectedItems = [];
const completionShownBySource = new Map();
const completionAcceptedBySource = new Map();
const completionStickyAcceptedByDoc = new Map();
let completionCacheRequestCount = 0;
let completionCacheHitCount = 0;
let completionPrefetchRequestCount = 0;
let completionPrefetchHitCount = 0;
let completionColdStartCount = 0;
const completionColdStartMs = [];
const completionStableListMs = [];
const completionNoRefreshStrictMs = [];
const completionShadowAgreement = [];
const completionShadowDrift = [];
const completionOpenedAtByDoc = new Map();
let completionCacheEvictionCount = 0;
let completionFallbackTimeoutCount = 0;
let completionFallbackNegativeCacheCount = 0;
let completionFallbackOfflineCount = 0;
let completionFallbackCancelCount = 0;
const completionTraceActiveByKey = new Map();
const completionTraceHistory = [];
let suggestionProfilerPanel;
let suggestionProfilerRenderTimer;
function incCounter(map, key, delta = 1) {
    map.set(key, (map.get(key) ?? 0) + delta);
}
function extractItemSource(item) {
    const detail = Array.isArray(item.detail) ? item.detail.join(" ") : (item.detail ?? "");
    const m = /\[source:([a-z]+)\]/i.exec(detail);
    return (m?.[1] ?? "unknown").toLowerCase();
}
function normalizeCompletionLabel(item) {
    const label = typeof item.label === "string" ? item.label : item.label.label;
    return label.trim().toLowerCase();
}
function extractCompletionInsertText(item) {
    if (typeof item.insertText === "string")
        return item.insertText;
    if (item.insertText instanceof vscode.SnippetString)
        return item.insertText.value;
    const label = typeof item.label === "string" ? item.label : item.label.label;
    return label;
}
function recordStickyAcceptedLabel(documentUri, item, limit) {
    const key = normalizeCompletionLabel(item);
    const prev = completionStickyAcceptedByDoc.get(documentUri) ?? [];
    const next = [key, ...prev.filter((v) => v !== key)].slice(0, Math.max(1, limit));
    completionStickyAcceptedByDoc.set(documentUri, next);
}
function withSourceBadge(item, source, force = false) {
    const detail = Array.isArray(item.detail) ? item.detail.join(" ") : (item.detail ?? "");
    if (force) {
        const stripped = detail.replace(/\[source:[a-z]+\]\s*/ig, "").trim();
        item.detail = stripped ? `[source:${source}] ${stripped}` : `[source:${source}]`;
        return item;
    }
    if (/\[source:[a-z]+\]/i.test(detail))
        return item;
    item.detail = detail ? `[source:${source}] ${detail}` : `[source:${source}]`;
    return item;
}
function shouldKeepSuggestionWithLintGuard(item, document, position, enabled) {
    if (!enabled)
        return true;
    const diagnostics = vscode.languages.getDiagnostics(document.uri);
    const hasLineError = diagnostics.some((d) => d.severity === vscode.DiagnosticSeverity.Error && d.range.start.line === position.line);
    if (!hasLineError)
        return true;
    const range = document.getWordRangeAtPosition(position);
    const prefix = range ? document.getText(range).toLowerCase() : "";
    if (item.kind === vscode.CompletionItemKind.Snippet)
        return false;
    const label = typeof item.label === "string" ? item.label : item.label.label;
    const insert = typeof item.insertText === "string"
        ? item.insertText
        : item.insertText instanceof vscode.SnippetString
            ? item.insertText.value
            : label;
    const normalized = insert.replace(/\s+/g, " ").trim().toLowerCase();
    if (normalized.includes("\n"))
        return false;
    if (prefix.length === 0)
        return true;
    return normalized.startsWith(prefix) || normalized.includes(prefix);
}
function suggestionText(item) {
    const label = typeof item.label === "string" ? item.label : item.label.label;
    const insert = typeof item.insertText === "string"
        ? item.insertText
        : item.insertText instanceof vscode.SnippetString
            ? item.insertText.value
            : label;
    return `${label} ${insert}`.toLowerCase();
}
function shouldKeepSuggestionWithSecurityGuard(item, enabled, blockedPatterns) {
    if (!enabled)
        return true;
    const text = suggestionText(item);
    for (const pattern of blockedPatterns) {
        if (!pattern || pattern.trim().length === 0)
            continue;
        try {
            const re = new RegExp(pattern, "i");
            if (re.test(text))
                return false;
        }
        catch {
            // Ignore invalid regex patterns from config.
        }
    }
    return true;
}
function shouldKeepSuggestionWithPatternLists(item, whitelistPatterns, blacklistPatterns) {
    const text = suggestionText(item);
    let whitelisted = whitelistPatterns.length === 0;
    for (const pattern of whitelistPatterns) {
        if (!pattern || pattern.trim().length === 0)
            continue;
        try {
            const re = new RegExp(pattern, "i");
            if (re.test(text)) {
                whitelisted = true;
                break;
            }
        }
        catch {
            // Ignore invalid regex from user config.
        }
    }
    if (!whitelisted)
        return false;
    for (const pattern of blacklistPatterns) {
        if (!pattern || pattern.trim().length === 0)
            continue;
        try {
            const re = new RegExp(pattern, "i");
            if (re.test(text))
                return false;
        }
        catch {
            // Ignore invalid regex from user config.
        }
    }
    return true;
}
function shouldKeepSuggestionWithDiagnosticContext(item, document, position, enabled, minTokenLen) {
    if (!enabled)
        return true;
    const diagnostics = vscode.languages.getDiagnostics(document.uri)
        .filter((d) => d.range.start.line === position.line);
    if (diagnostics.length === 0)
        return true;
    const text = suggestionText(item);
    const wordRange = document.getWordRangeAtPosition(position);
    const prefix = wordRange ? document.getText(wordRange).toLowerCase() : "";
    if (prefix && text.includes(prefix))
        return true;
    const tokens = new Set();
    for (const d of diagnostics) {
        const parts = d.message.toLowerCase().split(/[^a-z0-9_]+/g);
        for (const t of parts) {
            if (t.length >= minTokenLen)
                tokens.add(t);
        }
    }
    if (tokens.size === 0)
        return true;
    for (const t of tokens) {
        if (text.includes(t))
            return true;
    }
    return false;
}
function mapEstimatedMb(map) {
    let bytes = 0;
    for (const [k, v] of map) {
        bytes += k.length * 2;
        bytes += 16;
        for (const it of v.items) {
            const lbl = typeof it.label === "string" ? it.label : it.label.label;
            bytes += lbl.length * 2 + 64;
        }
    }
    return bytes / (1024 * 1024);
}
function touchMapEntry(map, key, value) {
    if (map.has(key))
        map.delete(key);
    map.set(key, value);
}
function evictLruEntries(map, maxEntries) {
    let evicted = 0;
    while (map.size > maxEntries) {
        const oldest = map.keys().next().value;
        if (oldest === undefined)
            break;
        map.delete(oldest);
        evicted += 1;
    }
    return evicted;
}
function trimCompletionCaches(maxEntries, softMemoryMb) {
    completionCacheEvictionCount += evictLruEntries(completionStreamingCachePrefix, maxEntries);
    completionCacheEvictionCount += evictLruEntries(completionStreamingCacheContext, maxEntries);
    completionCacheEvictionCount += evictLruEntries(completionStreamingCacheDocument, maxEntries);
    const rssMb = process.memoryUsage().rss / (1024 * 1024);
    const cacheMb = mapEstimatedMb(completionStreamingCachePrefix)
        + mapEstimatedMb(completionStreamingCacheContext)
        + mapEstimatedMb(completionStreamingCacheDocument);
    if (rssMb <= softMemoryMb && cacheMb <= Math.max(8, softMemoryMb * 0.25))
        return;
    const target = Math.max(32, Math.floor(maxEntries * 0.8));
    completionCacheEvictionCount += evictLruEntries(completionStreamingCachePrefix, target);
    completionCacheEvictionCount += evictLruEntries(completionStreamingCacheContext, target);
    completionCacheEvictionCount += evictLruEntries(completionStreamingCacheDocument, target);
}
function getValidCacheItems(map, key, ttlMs) {
    const now = Date.now();
    const entry = map.get(key);
    if (!entry)
        return [];
    if ((now - entry.ts) > ttlMs) {
        map.delete(key);
        return [];
    }
    // touch for LRU behavior
    map.delete(key);
    map.set(key, entry);
    return entry.items;
}
function clearCacheMapByUriPrefix(map, uriPrefix) {
    for (const key of map.keys()) {
        if (key.startsWith(uriPrefix))
            map.delete(key);
    }
}
function invalidateCompletionCachesForDocument(document) {
    const uriPrefix = `${document.uri.toString()}#`;
    clearCacheMapByUriPrefix(completionStreamingCachePrefix, uriPrefix);
    clearCacheMapByUriPrefix(completionStreamingCacheContext, uriPrefix);
    clearCacheMapByUriPrefix(completionStreamingCacheDocument, uriPrefix);
    clearCacheMapByUriPrefix(completionStreamingInFlight, uriPrefix);
    clearCacheMapByUriPrefix(completionPagingState, uriPrefix);
}
function resetSuggestionRuntimeState() {
    completionStreamingCachePrefix.clear();
    completionStreamingCacheContext.clear();
    completionStreamingCacheDocument.clear();
    completionStreamingInFlight.clear();
    completionPagingState.clear();
    completionTop1StableByKey.clear();
    completionStickyAcceptedByDoc.clear();
    completionLspNegativeCache.clear();
    completionRequestStartedAt.clear();
    completionRequestRefreshed.clear();
    completionFirstUsableMarked.clear();
    completionLatencyWindowMs.length = 0;
    completionStreamingFirstPaintMs.length = 0;
    completionStreamingEnrichMs.length = 0;
    completionFirstUsableMs.length = 0;
    completionColdStartMs.length = 0;
    completionStableListMs.length = 0;
    completionNoRefreshStrictMs.length = 0;
    completionShadowAgreement.length = 0;
    completionShadowDrift.length = 0;
    completionStreamingRefreshCount = 0;
    completionStreamingTimeoutCount = 0;
    completionStreamingTotalCount = 0;
    completionStreamingManualRefreshCount = 0;
    completionStreamingRicherRefreshCount = 0;
    completionStreamingIncompleteRefreshCount = 0;
    completionSuggestionShownCount = 0;
    completionSuggestionAcceptedCount = 0;
    completionSuggestionCanceledCount = 0;
    completionSuggestionPendingAccepted = false;
    completionPendingRejectedItems = [];
    completionShownBySource.clear();
    completionAcceptedBySource.clear();
    completionCacheRequestCount = 0;
    completionCacheHitCount = 0;
    completionPrefetchRequestCount = 0;
    completionPrefetchHitCount = 0;
    completionColdStartCount = 0;
    completionCacheEvictionCount = 0;
    completionFallbackTimeoutCount = 0;
    completionFallbackNegativeCacheCount = 0;
    completionFallbackOfflineCount = 0;
    completionFallbackCancelCount = 0;
    completionTraceActiveByKey.clear();
    completionTraceHistory.length = 0;
    refreshSuggestionProfilerPanel();
}
function flattenDocSymbols(symbols, out, parent = "") {
    for (const s of symbols) {
        const seg = `${parent}/${s.kind}:${s.name}:${s.range.start.line}:${s.range.end.line}`;
        out.push(seg);
        if (s.children.length > 0) {
            flattenDocSymbols(s.children, out, `${parent}/${s.name}`);
        }
    }
}
async function computeAstFingerprint(document) {
    try {
        const raw = await vscode.commands.executeCommand("vscode.executeDocumentSymbolProvider", document.uri);
        if (!raw || raw.length === 0)
            return undefined;
        const entries = [];
        if (raw[0] instanceof vscode.DocumentSymbol) {
            flattenDocSymbols(raw, entries, "");
        }
        else {
            for (const s of raw) {
                entries.push(`${s.containerName ?? ""}/${s.kind}:${s.name}:${s.location.range.start.line}:${s.location.range.end.line}`);
            }
        }
        if (entries.length === 0)
            return undefined;
        entries.sort((a, b) => a.localeCompare(b));
        return `${document.languageId}|${entries.slice(0, 1200).join("|")}`;
    }
    catch {
        return undefined;
    }
}
function scheduleAstCacheInvalidation(document, invalidateOnChange) {
    const key = document.uri.toString();
    const prev = completionAstRefreshTimers.get(key);
    if (prev)
        clearTimeout(prev);
    const timer = setTimeout(() => {
        void (async () => {
            const fp = await computeAstFingerprint(document);
            if (!fp)
                return;
            const old = completionAstFingerprintByDoc.get(key);
            if (invalidateOnChange && old && old !== fp) {
                invalidateCompletionCachesForDocument(document);
            }
            completionAstFingerprintByDoc.set(key, fp);
        })();
        completionAstRefreshTimers.delete(key);
    }, invalidateOnChange ? 220 : 60);
    completionAstRefreshTimers.set(key, timer);
}
function scheduleCompletionIdlePrefetch(editor) {
    if (!editor)
        return;
    const cfg = vscode.workspace.getConfiguration("vitte");
    const enabled = cfg.get("suggestions.idlePrefetch", true);
    if (!enabled)
        return;
    const radius = Math.max(0, Math.min(6, cfg.get("suggestions.prefetchLineRadius", 1)));
    const directional = cfg.get("suggestions.prefetchDirectional", true);
    if (completionIdlePrefetchTimer)
        clearTimeout(completionIdlePrefetchTimer);
    completionIdlePrefetchTimer = setTimeout(async () => {
        if (completionPrefetchInFlight)
            return;
        completionPrefetchInFlight = true;
        try {
            const doc = editor.document;
            const base = editor.selection.active;
            const positions = [];
            const direction = cfg.get("suggestions.prefetchDirection", "forward");
            const deltas = directional
                ? (direction === "forward"
                    ? [0, 1, 2, -1, 3, -2]
                    : [0, -1, -2, 1, -3, 2])
                : [];
            if (directional) {
                for (const d of deltas) {
                    if (Math.abs(d) > radius)
                        continue;
                    const line = base.line + d;
                    if (line < 0 || line >= doc.lineCount)
                        continue;
                    const lineLen = doc.lineAt(line).text.length;
                    const character = Math.min(base.character, lineLen);
                    positions.push(new vscode.Position(line, character));
                }
            }
            else {
                for (let delta = -radius; delta <= radius; delta += 1) {
                    const line = base.line + delta;
                    if (line < 0 || line >= doc.lineCount)
                        continue;
                    const lineLen = doc.lineAt(line).text.length;
                    const character = Math.min(base.character, lineLen);
                    positions.push(new vscode.Position(line, character));
                }
            }
            completionPrefetchRequestCount += positions.length;
            for (const pos of positions) {
                completionPrefetchExpectedKey = (0, suggestions_1.getCompletionRequestKey)(doc, pos);
                await vscode.commands.executeCommand("vscode.executeCompletionItemProvider", doc.uri, pos);
            }
        }
        catch {
            // Best-effort prefetch.
        }
        finally {
            completionPrefetchExpectedKey = undefined;
            completionPrefetchInFlight = false;
        }
    }, 180);
}
function seedDedupKey(item) {
    const label = (typeof item.label === "string" ? item.label : item.label.label).trim().toLowerCase();
    const kind = String(item.kind ?? 0);
    const insert = typeof item.insertText === "string"
        ? item.insertText
        : item.insertText instanceof vscode.SnippetString
            ? item.insertText.value
            : "";
    return `${label}|${kind}|${insert.replace(/\s+/g, " ").trim().toLowerCase()}`;
}
function pushWindowSample(target, value, max = 256) {
    target.push(value);
    if (target.length > max)
        target.shift();
}
function pushSuggestionTraceHistory(entry, max = 240) {
    completionTraceHistory.push(entry);
    if (completionTraceHistory.length > max)
        completionTraceHistory.shift();
}
function getSuggestionTraceHistory(limit = 80) {
    return completionTraceHistory.slice(Math.max(0, completionTraceHistory.length - limit));
}
function escapeHtml(input) {
    return input
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/\"/g, "&quot;")
        .replace(/'/g, "&#39;");
}
function renderSuggestionProfilerHtml() {
    const rows = getSuggestionTraceHistory(80).reverse();
    const body = rows.length === 0
        ? `<tr><td colspan="11">No suggestion request captured yet.</td></tr>`
        : rows.map((r) => {
            const ts = new Date(r.startedAt).toLocaleTimeString();
            const p = r.firstPaintMs !== undefined ? `${r.firstPaintMs}ms` : "—";
            const e = r.enrichMs !== undefined ? `${r.enrichMs}ms` : "—";
            const u = r.firstUsableMs !== undefined ? `${r.firstUsableMs}ms` : "—";
            const s = r.stableMs !== undefined ? `${r.stableMs}ms` : "—";
            const strict = r.noRefreshStrict ? "yes" : "no";
            return `<tr>
        <td>${escapeHtml(ts)}</td>
        <td>${escapeHtml(r.languageId)}</td>
        <td title="${escapeHtml(r.uriShort)}">${escapeHtml(r.uriShort)}</td>
        <td>${escapeHtml(p)}</td>
        <td>${escapeHtml(e)}</td>
        <td>${escapeHtml(u)}</td>
        <td>${escapeHtml(s)}</td>
        <td>${escapeHtml(r.refreshCause)}</td>
        <td>${escapeHtml(r.fallbackCause)}</td>
        <td>${r.shownCount}</td>
        <td title="${escapeHtml(r.sourceMix ?? "")}">${escapeHtml(r.sourceMix ?? "—")}</td>
        <td>${escapeHtml(strict)}</td>
      </tr>`;
        }).join("");
    return `<!doctype html>
<html>
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <style>
    body { font-family: var(--vscode-font-family); font-size: 12px; margin: 12px; color: var(--vscode-foreground); background: var(--vscode-editor-background); }
    h2 { margin: 0 0 10px 0; font-size: 14px; }
    .muted { opacity: .8; margin-bottom: 10px; }
    table { border-collapse: collapse; width: 100%; table-layout: fixed; }
    th, td { border: 1px solid var(--vscode-panel-border); padding: 4px 6px; text-align: left; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
    th { position: sticky; top: 0; background: var(--vscode-sideBar-background); z-index: 1; }
  </style>
</head>
<body>
  <h2>Vitte Suggestion Profiler</h2>
  <div class="muted">Timeline per request: first paint, enrich, first usable, stability, refresh cause, fallback cause.</div>
  <table>
    <thead>
      <tr>
        <th>Time</th><th>Lang</th><th>File</th><th>First paint</th><th>Enrich</th><th>First usable</th><th>Stable</th><th>Refresh</th><th>Fallback</th><th>Shown</th><th>Sources</th><th>No-refresh</th>
      </tr>
    </thead>
    <tbody>${body}</tbody>
  </table>
</body>
</html>`;
}
function refreshSuggestionProfilerPanel() {
    if (!suggestionProfilerPanel)
        return;
    suggestionProfilerPanel.webview.html = renderSuggestionProfilerHtml();
}
function openSuggestionProfilerPanel(context) {
    if (suggestionProfilerPanel) {
        suggestionProfilerPanel.reveal(vscode.ViewColumn.Beside);
        refreshSuggestionProfilerPanel();
        return;
    }
    suggestionProfilerPanel = vscode.window.createWebviewPanel("vitteSuggestionProfiler", "Vitte Suggestion Profiler", vscode.ViewColumn.Beside, { enableFindWidget: true });
    refreshSuggestionProfilerPanel();
    suggestionProfilerRenderTimer = setInterval(() => {
        refreshSuggestionProfilerPanel();
    }, 1200);
    suggestionProfilerPanel.onDidDispose(() => {
        suggestionProfilerPanel = undefined;
        if (suggestionProfilerRenderTimer) {
            clearInterval(suggestionProfilerRenderTimer);
            suggestionProfilerRenderTimer = undefined;
        }
    }, undefined, context.subscriptions);
}
function getAdaptiveRefreshDebounceMs(documentUri, minMs, maxMs) {
    const sample = typingSpeedByDoc.get(documentUri);
    if (!sample)
        return minMs;
    // Faster typing (small interval) => larger debounce.
    const interval = Math.max(20, Math.min(1200, sample.intervalEwmaMs));
    const normalized = 1 - Math.min(1, Math.max(0, (interval - 60) / 540));
    return Math.round(minMs + (maxMs - minMs) * normalized);
}
function listAgreementAndDrift(primary, shadow, topK = 10) {
    const a = primary.slice(0, topK).map((it) => normalizeCompletionLabel(it));
    const b = shadow.slice(0, topK).map((it) => normalizeCompletionLabel(it));
    if (a.length === 0 || b.length === 0)
        return { agreement: 0, drift: 0 };
    const bIdx = new Map();
    for (let i = 0; i < b.length; i += 1)
        bIdx.set(b[i], i);
    let common = 0;
    let driftSum = 0;
    for (let i = 0; i < a.length; i += 1) {
        const j = bIdx.get(a[i]);
        if (j === undefined)
            continue;
        common += 1;
        driftSum += Math.abs(i - j);
    }
    return {
        agreement: common / Math.max(1, Math.min(a.length, b.length)),
        drift: common > 0 ? driftSum / common : topK,
    };
}
function buildLoadMoreCompletionItem() {
    const item = new vscode.CompletionItem("Load next suggestions page…", vscode.CompletionItemKind.Reference);
    item.detail = "[source:ui] pagination";
    item.sortText = "zzzzzz_load_more";
    item.filterText = "load next suggestions page";
    item.insertText = "";
    item.command = {
        command: "vitte.suggestions.loadNextPage",
        title: "Load next suggestions page",
    };
    item.documentation = new vscode.MarkdownString("Load more completion results for the current prefix.");
    return item;
}
function updateTop1StableInlineCandidate(requestKey, items) {
    const real = items.find((it) => extractItemSource(it) !== "ui");
    if (!real) {
        completionTop1StableByKey.delete(requestKey);
        return;
    }
    const text = extractCompletionInsertText(real).trim();
    if (!text || text.includes("\n") || text.includes("\t$")) {
        completionTop1StableByKey.delete(requestKey);
        return;
    }
    const prev = completionTop1StableByKey.get(requestKey);
    const now = Date.now();
    if (!prev || prev.text !== text || (now - prev.lastAt) > 5000) {
        completionTop1StableByKey.set(requestKey, { text, count: 1, lastAt: now });
        return;
    }
    completionTop1StableByKey.set(requestKey, { text, count: Math.min(12, prev.count + 1), lastAt: now });
}
function completeCompletionRequestMetrics(key, endedAt, expectedStartedAt) {
    const started = completionRequestStartedAt.get(key);
    if (expectedStartedAt !== undefined && started !== expectedStartedAt)
        return;
    const trace = completionTraceActiveByKey.get(key);
    if (expectedStartedAt !== undefined && trace && trace.startedAt !== expectedStartedAt)
        return;
    if (started !== undefined) {
        const refreshed = completionRequestRefreshed.get(key) ?? false;
        if (!refreshed) {
            pushWindowSample(completionNoRefreshStrictMs, endedAt - started);
        }
        if (trace) {
            trace.noRefreshStrict = !refreshed;
            if (trace.stableMs === undefined) {
                trace.stableMs = Math.max(0, endedAt - trace.startedAt);
            }
            pushSuggestionTraceHistory({ ...trace });
            refreshSuggestionProfilerPanel();
        }
    }
    completionRequestStartedAt.delete(key);
    completionRequestRefreshed.delete(key);
    completionFirstUsableMarked.delete(key);
    completionTraceActiveByKey.delete(key);
}
function percentileLocal(values, q) {
    if (values.length === 0)
        return 0;
    const xs = [...values].sort((a, b) => a - b);
    const i = Math.max(0, Math.min(xs.length - 1, Math.round((xs.length - 1) * q)));
    return xs[i] ?? 0;
}
function getStreamingCompletionStats() {
    const total = Math.max(1, completionStreamingTotalCount);
    const totalShown = Math.max(1, completionSuggestionShownCount);
    const cacheReq = Math.max(1, completionCacheRequestCount);
    const prefetchReq = Math.max(1, completionPrefetchRequestCount);
    const acceptanceBySource = ["local", "cache", "lsp", "plugin", "unknown"]
        .map((src) => {
        const shown = completionShownBySource.get(src) ?? 0;
        const accepted = completionAcceptedBySource.get(src) ?? 0;
        const rate = shown > 0 ? (accepted / shown) * 100 : 0;
        return `${src}:${rate.toFixed(1)}%`;
    })
        .join(" | ");
    return {
        firstPaintP50Ms: percentileLocal(completionStreamingFirstPaintMs, 0.5),
        firstPaintP95Ms: percentileLocal(completionStreamingFirstPaintMs, 0.95),
        enrichP50Ms: percentileLocal(completionStreamingEnrichMs, 0.5),
        enrichP95Ms: percentileLocal(completionStreamingEnrichMs, 0.95),
        refreshCount: completionStreamingRefreshCount,
        refreshManualCount: completionStreamingManualRefreshCount,
        refreshRicherCount: completionStreamingRicherRefreshCount,
        refreshIncompleteCount: completionStreamingIncompleteRefreshCount,
        timeoutCount: completionStreamingTimeoutCount,
        totalCount: completionStreamingTotalCount,
        timeoutRate: completionStreamingTimeoutCount / total,
        ttfsP50Ms: percentileLocal(completionStreamingFirstPaintMs, 0.5),
        ttfsP95Ms: percentileLocal(completionStreamingFirstPaintMs, 0.95),
        enrichDeltaP50Ms: percentileLocal(completionStreamingEnrichMs, 0.5),
        enrichDeltaP95Ms: percentileLocal(completionStreamingEnrichMs, 0.95),
        acceptanceRate: completionSuggestionAcceptedCount / totalShown,
        cancelRate: completionSuggestionCanceledCount / totalShown,
        acceptanceBySource,
        cacheHitRate: completionCacheHitCount / cacheReq,
        prefetchHitRate: completionPrefetchHitCount / prefetchReq,
        coldStartCount: completionColdStartCount,
        coldStartP50Ms: percentileLocal(completionColdStartMs, 0.5),
        coldStartP95Ms: percentileLocal(completionColdStartMs, 0.95),
        stableListP50Ms: percentileLocal(completionStableListMs, 0.5),
        stableListP95Ms: percentileLocal(completionStableListMs, 0.95),
        cacheEvictions: completionCacheEvictionCount,
        firstUsableP50Ms: percentileLocal(completionFirstUsableMs, 0.5),
        firstUsableP95Ms: percentileLocal(completionFirstUsableMs, 0.95),
        noRefreshStrictP50Ms: percentileLocal(completionNoRefreshStrictMs, 0.5),
        noRefreshStrictP95Ms: percentileLocal(completionNoRefreshStrictMs, 0.95),
        fallbackTimeoutCount: completionFallbackTimeoutCount,
        fallbackNegativeCacheCount: completionFallbackNegativeCacheCount,
        fallbackOfflineCount: completionFallbackOfflineCount,
        fallbackCancelCount: completionFallbackCancelCount,
        shadowAgreementP50: percentileLocal(completionShadowAgreement, 0.5),
        shadowAgreementP95: percentileLocal(completionShadowAgreement, 0.95),
        shadowDriftP50: percentileLocal(completionShadowDrift, 0.5),
        shadowDriftP95: percentileLocal(completionShadowDrift, 0.95),
    };
}
function getExtensionVersion(ext) {
    const manifest = ext?.packageJSON;
    if (!manifest || typeof manifest !== "object")
        return "unknown";
    const version = manifest.version;
    return typeof version === "string" && version.length > 0 ? version : "unknown";
}
const LANGUAGES = ["vitte", "vit"];
const WATCH_PATTERNS = [
    "**/*.{vitte,vit}",
    "**/vitte.toml",
    "**/.vitteconfig"
];
const LANGUAGE_SET = new Set(LANGUAGES);
const DEFAULT_COMMAND_SHORTCUTS = [
    { label: "Build", command: "vitte.build", icon: "$(tools)", tooltip: "Build the workspace (vitte.build)", statusBar: true, startup: true },
    { label: "Run", command: "vitte.run", icon: "$(debug-start)", tooltip: "Build and run the entry point (vitte.run)", statusBar: true, startup: true },
    { label: "Test", command: "vitte.test", icon: "$(beaker)", tooltip: "Execute the test suite (vitte.test)", statusBar: true, startup: true },
];
const COMMAND_MENU_ENTRIES = [
    { label: "Project Assistant", description: "Create project/module/package", command: "vitte.projectAssistant" },
    { label: "New Project", description: "Scaffold a new Vitte project", command: "vitte.newProject" },
    { label: "New Module", description: "Scaffold a new module file", command: "vitte.newModule" },
    { label: "New Package", description: "Scaffold a new package", command: "vitte.newPackage" },
    { label: "Generate VS Code Config", description: "Create .vscode/tasks.json + launch.json", command: "vitte.generateWorkspaceConfig" },
    { label: "Doctor", description: "check + lint + fmt + test + perf", command: "vitte.doctor" },
    { label: "Clean workspace", description: "Remove build outputs", command: "vitte.clean" },
    { label: "Bench workspace", description: "Run vitte.bench", command: "vitte.bench" },
    { label: "Bench extension CI", description: "Activation/memory/completion latency snapshot", command: "vitte.benchExtensionCi" },
    { label: "Export perf session", description: "Export activation/p95/memory session JSON", command: "vitte.exportPerfSession" },
    { label: "Open bench report", description: "Latest bench report", command: "vitte.benchReport" },
    { label: "Diagnostics ▸ Refresh", description: "Re-scan diagnostics", command: "vitte.diagnostics.refresh" },
    { label: "Diagnostics ▸ Next issue", description: "Jump to next diagnostic", command: "editor.action.marker.next" },
    { label: "Quick Actions", description: "Interactive menu", command: "vitte.quickActions" },
    { label: "Server log", description: "Open log output", command: "vitte.showServerLog" },
    { label: "Server metrics", description: "Show performance snapshot", command: "vitte.showServerMetrics" },
    { label: "Suggestions ▸ Load next page", description: "Increment completion page and refresh suggest", command: "vitte.suggestions.loadNextPage" },
    { label: "Suggestions ▸ Reset learning", description: "Clear local suggestion learning state", command: "vitte.suggestions.resetLearning" },
    { label: "Suggestions ▸ Export diagnostics", description: "Export suggestion diagnostics JSON snapshot", command: "vitte.suggestions.exportDiagnostics" },
    { label: "Suggestions ▸ Open profiler", description: "Open request timeline for suggestions", command: "vitte.suggestions.openProfiler" },
    { label: "Detect toolchain", description: "Scan for Vitte runtimes", command: "vitte.detectToolchain" },
];
let fileWatchers = [];
let commandButtonItems = [];
let commandMenuButton;
const DIAGNOSTICS_REFRESH_KEY = "vitte.quickActions.diagRefresh";
function logServerResolution(message) {
    const text = `[vitte] ${message}`;
    output?.appendLine(text);
}
function readCommandShortcuts() {
    const cfg = vscode.workspace.getConfiguration("vitte");
    const raw = cfg.get("commandShortcuts");
    if (!Array.isArray(raw)) {
        return DEFAULT_COMMAND_SHORTCUTS.map((item) => ({ ...item }));
    }
    const sanitized = [];
    for (const entry of raw) {
        if (!entry || typeof entry !== "object")
            continue;
        const data = entry;
        const label = typeof data.label === "string" && data.label.trim().length > 0 ? data.label.trim() : undefined;
        const command = typeof data.command === "string" && data.command.trim().length > 0 ? data.command.trim() : undefined;
        if (!label || !command)
            continue;
        const shortcut = { label, command };
        const icon = typeof data.icon === "string" && data.icon.trim().length > 0 ? data.icon.trim() : undefined;
        if (icon)
            shortcut.icon = icon;
        const tooltip = typeof data.tooltip === "string" && data.tooltip.trim().length > 0 ? data.tooltip : undefined;
        if (tooltip)
            shortcut.tooltip = tooltip;
        if (typeof data.statusBar === "boolean")
            shortcut.statusBar = data.statusBar;
        if (typeof data.startup === "boolean")
            shortcut.startup = data.startup;
        sanitized.push(shortcut);
    }
    return sanitized.length > 0 ? sanitized : DEFAULT_COMMAND_SHORTCUTS.map((item) => ({ ...item }));
}
function getWorkspaceId() {
    return vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
}
function readDiagnosticsRefreshState(context) {
    const workspaceId = getWorkspaceId();
    if (!workspaceId)
        return { stale: true };
    const map = context.globalState.get(DIAGNOSTICS_REFRESH_KEY, {});
    const ts = map[workspaceId];
    if (!ts)
        return { stale: true };
    const stale = Date.now() - ts > 10 * 60 * 1000;
    return { lastRefresh: ts, stale };
}
function labelForCommand(command) {
    switch (command) {
        case "vitte.build": return "Build";
        case "vitte.run": return "Run";
        case "vitte.test": return "Run Tests";
        case "vitte.diagnostics.refresh": return "Refresh Diagnostics";
        default: return command.replace(/^vitte\./, "");
    }
}
function applyStatusBar() {
    if (!statusItem)
        return;
    let text;
    let tooltipParts;
    if (statusOverrideText !== undefined) {
        text = statusOverrideText;
        tooltipParts = [statusOverrideTooltip ?? statusBaseTooltip];
    }
    else {
        const suffix = statusLanguageSuffix ? ` (${statusLanguageSuffix})` : "";
        text = `${statusBaseIcon} ${STATUS_LABEL}${suffix}`;
        tooltipParts = [statusBaseTooltip];
        if (statusOverrideTooltip) {
            tooltipParts.push(statusOverrideTooltip);
        }
    }
    if (statusHealthIcon) {
        text = `${text} ${statusHealthIcon}`;
    }
    if (statusHealthTooltip) {
        tooltipParts.push(statusHealthTooltip);
    }
    statusItem.text = text;
    statusItem.tooltip = tooltipParts.filter(Boolean).join("\n");
    statusItem.accessibilityInformation = {
        label: text.replace(/\$\([^)]+\)/g, "").trim(),
        role: "status"
    };
    if (offlineReason) {
        statusItem.color = OFFLINE_STATUS_COLOR;
        statusItem.backgroundColor = OFFLINE_STATUS_BG;
    }
    else {
        statusItem.color = undefined;
        statusItem.backgroundColor = undefined;
    }
}
function setStatusBase(icon, tooltip) {
    statusBaseIcon = icon;
    statusBaseTooltip = tooltip;
    statusOverrideText = undefined;
    statusOverrideTooltip = undefined;
    applyStatusBar();
}
function setStatusLanguageSuffix(lang) {
    statusLanguageSuffix = lang ?? "";
    applyStatusBar();
}
function setStatusOverride(text, tooltip) {
    statusOverrideText = text;
    statusOverrideTooltip = tooltip;
    applyStatusBar();
}
function isOfflineEnabled() {
    return vscode.workspace.getConfiguration("vitte").get("server.offline", false);
}
function shouldFormatOnSave(doc) {
    if (!LANGUAGE_SET.has(doc.languageId))
        return false;
    if (doc.isUntitled)
        return false;
    return vscode.workspace.getConfiguration("vitte").get("format.onSave", false);
}
function isOfflinePermanent() {
    return vscode.workspace.getConfiguration("vitte").get("server.offlinePermanent", false);
}
function isOfflineEffective() {
    return isOfflineEnabled() || isOfflinePermanent();
}
function setOfflineStatus(reason) {
    offlineReason = reason;
    offlineSince = offlineSince ?? Date.now();
    setStatusBase("$(circle-slash)", "Vitte LSP: offline");
    const forced = isOfflinePermanent() ? "Offline permanent (user-forced)" : "Offline";
    const since = offlineSince ? ` since ${new Date(offlineSince).toLocaleTimeString()}` : "";
    setStatusOverride(`$(circle-slash) Vitte ${forced.toUpperCase()}`, `${reason}${since}`);
    void setServerOnlineContext(false);
    logOfflineReason(reason);
    void showOfflineBanner(reason);
    scheduleOfflineRetry();
    void vscode.commands.executeCommand("vitte.offline.refresh");
}
async function setServerOnlineContext(online) {
    try {
        await vscode.commands.executeCommand("setContext", "vitte.serverOnline", online);
    }
    catch { /* noop */ }
    try {
        await vscode.commands.executeCommand("setContext", "vitte.serverOffline", !online);
    }
    catch { /* noop */ }
}
function logOfflineReason(reason) {
    try {
        output.appendLine(`[offline] ${reason}`);
    }
    catch { /* noop */ }
    void appendOfflineLog(reason);
}
async function showOfflineBanner(reason) {
    if (offlineBannerShown)
        return;
    const mute = vscode.workspace.getConfiguration("vitte").get("server.offlineMuteBanner", false);
    if (mute)
        return;
    offlineBannerShown = true;
    const selection = await vscode.window.showWarningMessage(`Vitte server offline: ${reason}`, "Explain Offline", "Open Settings");
    if (selection === "Explain Offline") {
        await vscode.commands.executeCommand("vitte.offline.explain");
    }
    else if (selection === "Open Settings") {
        void vscode.commands.executeCommand("workbench.action.openSettings", "vitte.server");
    }
}
async function setOfflineMode(enabled, reason) {
    const config = vscode.workspace.getConfiguration("vitte");
    const hasWorkspace = Boolean(vscode.workspace.workspaceFolders?.length);
    const target = hasWorkspace ? vscode.ConfigurationTarget.Workspace : vscode.ConfigurationTarget.Global;
    await config.update("server.offline", enabled, target);
    if (enabled) {
        setOfflineStatus(reason ?? "Offline mode enabled.");
    }
    else {
        offlineReason = undefined;
        offlineBannerShown = false;
        offlineSince = undefined;
        cancelOfflineRetry();
        statusItem.color = undefined;
        statusItem.backgroundColor = undefined;
        setStatusBase("$(rocket)", "Vitte Language Server");
        await setServerOnlineContext(true);
    }
}
function scheduleOfflineRetry() {
    if (isOfflineEffective())
        return;
    const cfg = vscode.workspace.getConfiguration("vitte");
    const enabled = cfg.get("server.autoRetry", true);
    if (!enabled)
        return;
    const base = clampNumber(cfg.get("server.autoRetryBaseMs", 60000), 10000, 300000, 60000);
    const max = clampNumber(cfg.get("server.autoRetryMaxMs", 300000), base, 900000, 300000);
    if (!offlineRetryTimer) {
        offlineRetryMs = Math.max(base, offlineRetryMs);
        offlineRetryMs = Math.min(offlineRetryMs, max);
        offlineRetryTimer = setTimeout(() => {
            offlineRetryTimer = undefined;
            void (async () => {
                try {
                    const ok = await restartClient(lastActivationContext ?? undefined);
                    if (ok) {
                        offlineRetryMs = base;
                        cancelOfflineRetry();
                        return;
                    }
                    throw new Error("restart failed");
                }
                catch {
                    offlineRetryMs = Math.min(offlineRetryMs * 2, max);
                    scheduleOfflineRetry();
                }
            })();
        }, offlineRetryMs);
    }
}
function cancelOfflineRetry() {
    if (offlineRetryTimer)
        clearTimeout(offlineRetryTimer);
    offlineRetryTimer = undefined;
    offlineRetryMs = 60000;
}
async function appendOfflineLog(reason) {
    try {
        const folder = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
        const base = folder ?? os.tmpdir();
        const dir = path.join(base, ".vitte");
        await fs.promises.mkdir(dir, { recursive: true });
        const file = path.join(dir, "offline.log");
        const line = `${new Date().toISOString()} ${reason}\n`;
        await fs.promises.appendFile(file, line, "utf8");
    }
    catch { /* noop */ }
}
function clampNumber(value, min, max, fallback) {
    if (!Number.isFinite(value))
        return fallback;
    return Math.min(max, Math.max(min, value));
}
function refreshDiagnosticsStatus() {
    const summary = (0, diagnostics_1.summarizeWorkspaceDiagnostics)();
    const level = (0, diagnostics_1.diagnosticsLevel)(summary);
    switch (level) {
        case "error":
            statusHealthIcon = "$(error)";
            break;
        case "warning":
            statusHealthIcon = "$(warning)";
            break;
        default:
            statusHealthIcon = "$(pass-filled)";
    }
    statusHealthTooltip = (0, diagnostics_1.formatDiagnosticsSummary)(summary);
    applyStatusBar();
}
function ensureFileWatchers(context) {
    if (fileWatchers.length === 0) {
        fileWatchers = WATCH_PATTERNS.map((pattern) => {
            const watcher = vscode.workspace.createFileSystemWatcher(pattern);
            context.subscriptions.push(watcher);
            return watcher;
        });
    }
    return fileWatchers;
}
function updateCommandButtons(context) {
    for (const item of commandButtonItems) {
        try {
            item.dispose();
        }
        catch { /* noop */ }
    }
    commandButtonItems = [];
    if (commandMenuButton) {
        try {
            commandMenuButton.dispose();
        }
        catch { /* noop */ }
        commandMenuButton = undefined;
    }
    const shortcuts = readCommandShortcuts();
    const visible = shortcuts.filter((entry) => entry.statusBar !== false);
    let priority = 1000;
    for (const shortcut of visible) {
        const icon = shortcut.icon ?? "$(rocket)";
        const item = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, priority--);
        item.text = `${icon} ${shortcut.label}`;
        item.command = shortcut.command;
        item.tooltip = shortcut.tooltip ?? shortcut.command;
        item.name = `Vitte ${shortcut.label}`;
        item.accessibilityInformation = { label: `${shortcut.label} button`, role: "button" };
        item.show();
        commandButtonItems.push(item);
        context.subscriptions.push(item);
    }
    commandMenuButton = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, priority--);
    commandMenuButton.text = "$(list-selection) Vitte";
    commandMenuButton.command = "vitte.showCommandMenu";
    commandMenuButton.tooltip = "Show additional Vitte commands";
    commandMenuButton.name = "Vitte Command Menu";
    commandMenuButton.accessibilityInformation = { label: "Vitte command menu", role: "button" };
    commandMenuButton.show();
    context.subscriptions.push(commandMenuButton);
}
async function showStartupCommandPrompt(context) {
    if (process.env.VSCODE_TESTING === "1")
        return;
    const shortcuts = readCommandShortcuts().filter((entry) => entry.startup !== false);
    if (shortcuts.length === 0)
        return;
    const diagSummary = (0, diagnostics_1.summarizeWorkspaceDiagnostics)();
    const diagState = readDiagnosticsRefreshState(context);
    const recommendedCommands = new Set();
    if (diagSummary.errors > 0) {
        recommendedCommands.add("vitte.test");
    }
    if (diagState.stale || diagSummary.warnings > 0) {
        recommendedCommands.add("vitte.diagnostics.refresh");
    }
    const items = [];
    const seen = new Set();
    const ordered = [...shortcuts].sort((a, b) => {
        const aRec = recommendedCommands.has(a.command);
        const bRec = recommendedCommands.has(b.command);
        if (aRec && !bRec)
            return -1;
        if (!aRec && bRec)
            return 1;
        return a.label.localeCompare(b.label);
    });
    for (const shortcut of ordered) {
        if (seen.has(shortcut.command))
            continue;
        const recommended = recommendedCommands.has(shortcut.command);
        items.push({
            title: recommended ? `⭐ ${shortcut.label}` : shortcut.label,
            command: shortcut.command,
        });
        seen.add(shortcut.command);
    }
    for (const command of recommendedCommands) {
        if (seen.has(command))
            continue;
        items.unshift({
            title: `⭐ ${labelForCommand(command)}`,
            command,
        });
        seen.add(command);
    }
    if (items.length === 0)
        return;
    const moreItem = { title: "More…" };
    const dismissItem = { title: "Dismiss", isCloseAffordance: true };
    const selection = await vscode.window.showInformationMessage("Vitte is ready — run a command:", ...items, moreItem, dismissItem);
    if (!selection || selection === dismissItem)
        return;
    if (selection === moreItem) {
        await vscode.commands.executeCommand("vitte.showCommandMenu");
        return;
    }
    if (selection.command) {
        await vscode.commands.executeCommand(selection.command);
    }
}
async function activate(context) {
    activationStartedAt = Date.now();
    lastActivationContext = context;
    try {
        output = vscode.window.createOutputChannel("Vitte Language Server", { log: true });
    }
    catch {
        output = vscode.window.createOutputChannel("Vitte Language Server");
    }
    output.appendLine("[activate] begin");
    output.appendLine("[activate] Vitte extension activated");
    try {
        statusItem = vscode.window.createStatusBarItem("vitte.status", vscode.StatusBarAlignment.Right, 100);
    }
    catch {
        statusItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
    }
    statusItem.name = "Vitte LSP";
    statusItem.command = "vitte.showServerLog";
    context.subscriptions.push(output, statusItem);
    setStatusBase("$(rocket)", "Vitte Language Server");
    refreshDiagnosticsStatus();
    statusItem.show();
    updateCommandButtons(context);
    void showStartupCommandPrompt(context);
    void setServerOnlineContext(false);
    // Register command shortcuts early so they're available even if later init fails.
    context.subscriptions.push(vscode.commands.registerCommand('vitte.openPlayground', () => playgroundPanel_1.PlaygroundPanel.createOrShow(context)), vscode.commands.registerCommand("vitte.debugActivationStatus", async () => {
        const ext = vscode.extensions.getExtension("vittestudio.vitte-studio")
            ?? vscode.extensions.getExtension("VitteStudio.vitte-studio");
        const clientState = client ? node_1.State[client.state] : "none";
        const details = [
            `id=${ext?.id ?? "unknown"}`,
            `version=${getExtensionVersion(ext)}`,
            `isActive=${String(ext?.isActive ?? false)}`,
            `clientState=${clientState}`,
        ].join("\n");
        output.appendLine(`[activate-debug]\n${details}`);
        output.show(true);
        await vscode.window.showInformationMessage(`Vitte activation status:\n${details}`);
    }));
    void startClient(context).catch((err) => {
        const message = err instanceof Error ? err.message : String(err);
        output.appendLine(`[activate] startClient failed: ${message}`);
    });
    if (isOfflineEffective()) {
        void showOfflineBanner(offlineReason ?? (isOfflinePermanent()
            ? "Offline permanent (user-forced)."
            : "Offline mode is enabled (vitte.server.offline)."));
    }
    editorLintCollection = vscode.languages.createDiagnosticCollection("vitte-lint");
    context.subscriptions.push(editorLintCollection);
    context.subscriptions.push(vscode.workspace.onDidOpenTextDocument((doc) => {
        updateEditorLint(doc);
        completionOpenedAtByDoc.set(doc.uri.toString(), Date.now());
        scheduleAstCacheInvalidation(doc, false);
    }));
    context.subscriptions.push(vscode.workspace.onDidChangeTextDocument((e) => {
        const docKey = e.document.uri.toString();
        const now = Date.now();
        const prevTyping = typingSpeedByDoc.get(docKey);
        if (prevTyping) {
            const dt = Math.max(1, now - prevTyping.lastEditAt);
            const ewma = prevTyping.intervalEwmaMs * 0.75 + dt * 0.25;
            typingSpeedByDoc.set(docKey, { lastEditAt: now, intervalEwmaMs: ewma });
        }
        else {
            typingSpeedByDoc.set(docKey, { lastEditAt: now, intervalEwmaMs: 180 });
        }
        updateEditorLint(e.document);
        scheduleAstCacheInvalidation(e.document, true);
    }));
    context.subscriptions.push(vscode.workspace.onDidCloseTextDocument((doc) => {
        editorLintCollection?.delete(doc.uri);
        const key = doc.uri.toString();
        completionOpenedAtByDoc.delete(key);
        typingSpeedByDoc.delete(key);
        completionStickyAcceptedByDoc.delete(key);
        clearCacheMapByUriPrefix(completionTop1StableByKey, `${key}#`);
        completionAstFingerprintByDoc.delete(key);
        const t = completionAstRefreshTimers.get(key);
        if (t) {
            clearTimeout(t);
            completionAstRefreshTimers.delete(key);
        }
    }));
    for (const doc of vscode.workspace.textDocuments) {
        updateEditorLint(doc);
        completionOpenedAtByDoc.set(doc.uri.toString(), Date.now());
        scheduleAstCacheInvalidation(doc, false);
    }
    // Debug & runtime tooling
    (0, configurationProvider_1.registerDebugConfigurationProvider)(context);
    (0, adapterFactory_1.registerDebugFactory)(context);
    (0, runtimeLocator_1.registerRuntimeLocatorCommand)(context);
    (0, buildTasks_1.registerBuildTasks)(context);
    (0, benchTasks_1.registerBenchTasks)(context);
    (0, quickActions_1.registerQuickActions)(context);
    (0, projectAssistant_1.registerProjectAssistant)(context, output, () => client, () => Date.now() - activationStartedAt);
    (0, advancedCodeActions_1.registerAdvancedCodeActions)(context);
    (0, vitteCodeLens_1.registerVitteCodeLens)(context);
    (0, enterpriseSuite_1.registerEnterpriseSuite)(context, {
        output,
        getClient: () => client,
        getCrashCount: () => recentStops.length,
    });
    try {
        await (0, telemetry_1.registerTelemetry)(context);
    }
    catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        output.appendLine(`[activate] telemetry init failed: ${message}`);
    }
    // Commandes
    context.subscriptions.push(vscode.commands.registerCommand("vitte.showServerLog", () => {
        output.show(true);
    }), vscode.commands.registerCommand("vitte.showServerMetrics", async () => {
        if (isOfflineEffective())
            return showOfflineNoop("metrics");
        if (!client) {
            const reason = offlineReason ? ` (${offlineReason})` : "";
            void vscode.window.showWarningMessage(`Vitte server is not running.${reason}`);
            return;
        }
        try {
            const stats = await client.sendRequest("vitte/metrics");
            if (!stats || stats.length === 0) {
                void vscode.window.showInformationMessage("Vitte: no metrics available yet.");
                return;
            }
            const timestamp = new Date().toLocaleTimeString();
            output.appendLine(`[metrics] Snapshot ${timestamp}`);
            for (const entry of stats) {
                const avg = entry.averageMs.toFixed(2);
                const last = entry.lastMs.toFixed(2);
                const max = entry.maxMs.toFixed(2);
                const p99 = typeof entry.p99Ms === "number" ? entry.p99Ms.toFixed(2) : null;
                const when = entry.lastAt ? new Date(entry.lastAt).toLocaleTimeString() : "n/a";
                const countInfo = typeof entry.lastCount === "number" ? ` n=${entry.lastCount}` : "";
                const errInfo = entry.errorCount ? ` errors=${entry.errorCount}` : "";
                const p99Info = p99 ? ` p99=${p99}ms` : "";
                const lastErr = entry.lastError ? ` lastErr="${entry.lastError}"` : "";
                output.appendLine(`  ${entry.name.padEnd(18)} avg=${avg}ms last=${last}ms max=${max}ms${p99Info} count=${entry.count}${countInfo}${errInfo} last=${when} uri=${entry.lastUri}${lastErr}`);
            }
            output.show(true);
        }
        catch (err) {
            const message = err instanceof Error ? err.message : String(err);
            void vscode.window.showErrorMessage(`Vitte: unable to fetch server metrics (${message})`);
        }
    }), vscode.commands.registerCommand("vitte.metrics.reset", async () => {
        if (isOfflineEffective())
            return showOfflineNoop("metrics reset");
        if (!client) {
            void vscode.window.showWarningMessage("Vitte server is not running.");
            return;
        }
        try {
            await client.sendRequest("vitte/metrics.reset");
            void vscode.window.showInformationMessage("Vitte: metrics reset.");
            void vscode.commands.executeCommand("vitte.metrics.refresh");
        }
        catch (err) {
            const message = err instanceof Error ? err.message : String(err);
            void vscode.window.showErrorMessage(`Vitte: unable to reset metrics (${message})`);
        }
    }), vscode.commands.registerCommand("vitte.exportPerfSession", async () => {
        if (!client || client.state !== node_1.State.Running) {
            void vscode.window.showWarningMessage("Vitte server is not running.");
            return;
        }
        try {
            const stats = await client.sendRequest("vitte/metrics");
            const by = new Map(stats.map((s) => [s.name, s]));
            const mem = process.memoryUsage();
            const payload = {
                ts: new Date().toISOString(),
                activationMs: Date.now() - activationStartedAt,
                rssMB: Number((mem.rss / (1024 * 1024)).toFixed(2)),
                completionP95Ms: by.get("completion")?.p95Ms ?? null,
                hoverP95Ms: by.get("hover")?.p95Ms ?? null,
                renameP95Ms: by.get("rename")?.p95Ms ?? null,
                metrics: stats,
            };
            const folder = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
            if (!folder)
                return;
            const dir = path.join(folder, ".vitte-cache", "diagnostics");
            await fs.promises.mkdir(dir, { recursive: true });
            const file = path.join(dir, "perf-session.json");
            await fs.promises.writeFile(file, `${JSON.stringify(payload, null, 2)}\n`, "utf8");
            void vscode.window.showInformationMessage(`Perf session exported: ${file}`);
        }
        catch (err) {
            void vscode.window.showErrorMessage(`Export perf session failed: ${String(err)}`);
        }
    }), vscode.commands.registerCommand("vitte.pingServer", async () => {
        if (isOfflineEffective())
            return showOfflineNoop("ping");
        if (!client) {
            void vscode.window.showWarningMessage("Vitte server is not running.");
            return;
        }
        try {
            const res = await client.sendRequest("vitte/ping");
            void vscode.window.showInformationMessage(`Vitte: pong (${res.ok ? "ok" : "fail"}) at ${new Date(res.ts).toLocaleTimeString()}`);
        }
        catch (err) {
            const message = err instanceof Error ? err.message : String(err);
            void vscode.window.showErrorMessage(`Vitte: ping failed (${message})`);
        }
    }), vscode.commands.registerCommand("vitte.showCommandMenu", async () => {
        const pickItems = COMMAND_MENU_ENTRIES.map((entry) => {
            const item = {
                label: entry.label,
                command: entry.command,
                detail: entry.detail ?? entry.command,
            };
            if (entry.description) {
                item.description = entry.description;
            }
            return item;
        });
        const pick = await vscode.window.showQuickPick(pickItems, {
            title: "Vitte commands",
            placeHolder: "Select a command to run",
            matchOnDetail: true,
        });
        if (!pick)
            return;
        await vscode.commands.executeCommand(pick.command);
    }), vscode.commands.registerCommand("vitte.suggestions.loadNextPage", async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor)
            return;
        const cfg = vscode.workspace.getConfiguration("vitte");
        const maxPages = Math.max(1, Math.min(10, cfg.get("suggestions.maxPages", 5)));
        const key = (0, suggestions_1.getCompletionRequestKey)(editor.document, editor.selection.active);
        const current = completionPagingState.get(key)?.page ?? 1;
        const nextPage = Math.min(maxPages, current + 1);
        completionPagingState.set(key, { page: nextPage, ts: Date.now() });
        completionLoadNextPageKey = key;
        await vscode.commands.executeCommand("editor.action.triggerSuggest");
        vscode.window.setStatusBarMessage(`Vitte suggestions page: ${nextPage}/${maxPages}`, 1800);
    }), vscode.commands.registerCommand("vitte.suggestions.resetLearning", async () => {
        (0, suggestions_1.resetSuggestionLearningState)();
        resetSuggestionRuntimeState();
        await vscode.commands.executeCommand("vitte.metrics.refresh");
        void vscode.window.showInformationMessage("Vitte suggestions: local learning state reset.");
    }), vscode.commands.registerCommand("vitte.suggestions.exportDiagnostics", async () => {
        try {
            const folder = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
            if (!folder) {
                void vscode.window.showWarningMessage("Vitte: open a workspace to export suggestion diagnostics.");
                return;
            }
            const cfg = vscode.workspace.getConfiguration("vitte");
            const payload = {
                ts: new Date().toISOString(),
                workspace: folder,
                streaming: getStreamingCompletionStats(),
                profilerRecent: getSuggestionTraceHistory(120),
                learning: (0, suggestions_1.getSuggestionLearningSnapshot)(),
                config: {
                    intentMode: cfg.get("suggestions.intentMode"),
                    topN: cfg.get("suggestions.topN"),
                    minN: cfg.get("suggestions.minN"),
                    maxN: cfg.get("suggestions.maxN"),
                    pageSize: cfg.get("suggestions.pageSize"),
                    maxPages: cfg.get("suggestions.maxPages"),
                    lspHardTimeoutMs: cfg.get("suggestions.lspHardTimeoutMs"),
                    rankingDeadlineMs: cfg.get("suggestions.rankingDeadlineMs"),
                },
            };
            const dir = path.join(folder, ".vitte-cache", "diagnostics");
            await fs.promises.mkdir(dir, { recursive: true });
            const file = path.join(dir, "suggestions-diagnostics.json");
            await fs.promises.writeFile(file, `${JSON.stringify(payload, null, 2)}\n`, "utf8");
            void vscode.window.showInformationMessage(`Vitte suggestions diagnostics exported: ${file}`);
        }
        catch (err) {
            const message = err instanceof Error ? err.message : String(err);
            void vscode.window.showErrorMessage(`Vitte: failed to export suggestions diagnostics (${message})`);
        }
    }), vscode.commands.registerCommand("vitte.suggestions.openProfiler", async () => {
        openSuggestionProfilerPanel(context);
    }), vscode.commands.registerCommand("vitte.restartServer", async () => {
        if (isOfflinePermanent()) {
            return showOfflineNoop("restart");
        }
        await vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: "Vitte: restarting language server…",
        }, async () => {
            const ok = await restartClient(context);
            if (!ok) {
                showOfflineNoop("restart");
            }
        });
        vscode.window.setStatusBarMessage("Vitte server restart attempted.", 3000);
    }), vscode.commands.registerCommand("vitte.runAction", async () => {
        const pick = await vscode.window.showQuickPick([
            {
                label: "Format document",
                description: "editor.action.formatDocument",
                detail: "Apply the configured formatter to the active file.",
                action: "format",
            },
            {
                label: "Organize imports",
                description: "editor.action.organizeImports",
                detail: "Trie et nettoie les imports du document courant.",
                action: "organizeImports",
            },
            {
                label: "Fix all",
                description: "source.fixAll",
                detail: "Run available quick fixes.",
                action: "fixAll",
            }
        ], { title: "Vitte: run a quick action" });
        if (!pick)
            return;
        await runBuiltinAction(pick.action);
    }), vscode.commands.registerCommand("vitte.runActionWithArgs", async () => {
        const action = await vscode.window.showInputBox({ prompt: "Action (format | organizeImports | fixAll)", value: "format" });
        if (!action)
            return;
        await runBuiltinAction(action.trim());
    }), vscode.commands.registerCommand("vitte.formatDocument", async () => runBuiltinAction("format")), vscode.commands.registerCommand("vitte.organizeImports", async () => runBuiltinAction("organizeImports")), vscode.commands.registerCommand("vitte.fixAll", async () => runBuiltinAction("fixAll")), vscode.commands.registerCommand("vitte.renameSymbol", async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor)
            return;
        const doc = editor.document;
        const pos = editor.selection.active;
        const oldName = doc.getText(doc.getWordRangeAtPosition(pos) ?? new vscode.Range(pos, pos)) || "symbol";
        const newName = await vscode.window.showInputBox({
            prompt: `Rename '${oldName}' to`,
            value: oldName,
            ignoreFocusOut: true,
        });
        if (!newName || newName === oldName)
            return;
        const renameEdit = await vscode.commands.executeCommand("vscode.executeDocumentRenameProvider", doc.uri, pos, newName);
        if (!renameEdit)
            return;
        const conflicts = await detectRenameConflicts(renameEdit, newName);
        if (conflicts.length > 0) {
            const preview = conflicts.slice(0, 8).join(", ");
            void vscode.window.showErrorMessage(`Rename blocked: symbol '${newName}' already exists in ${conflicts.length} file(s): ${preview}${conflicts.length > 8 ? "…" : ""}`);
            return;
        }
        const preview = await summarizeWorkspaceEditDetailed(renameEdit);
        const choice = await vscode.window.showInformationMessage(`Rename preview: ${preview.fileCount} file(s), ${preview.editCount} edit(s). ${preview.details.join(" | ")}`, "Apply", "Cancel");
        if (choice !== "Apply")
            return;
        const snapshots = await captureWorkspaceEditSnapshots(renameEdit);
        try {
            const ok = await vscode.workspace.applyEdit(renameEdit);
            if (!ok)
                throw new Error("applyEdit returned false");
        }
        catch (err) {
            await rollbackWorkspaceEditSnapshots(snapshots);
            await writeRenameReport({
                ts: new Date().toISOString(),
                oldName,
                newName,
                fileCount: preview.fileCount,
                editCount: preview.editCount,
                applied: false,
                error: String(err),
                files: preview.details,
            });
            void vscode.window.showErrorMessage(`Vitte rename rollback: ${String(err)}`);
            return;
        }
        await writeRenameReport({
            ts: new Date().toISOString(),
            oldName,
            newName,
            fileCount: preview.fileCount,
            editCount: preview.editCount,
            applied: true,
            files: preview.details,
        });
    }), vscode.commands.registerCommand("vitte.applyEditSample", async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor)
            return;
        const edit = new vscode.WorkspaceEdit();
        edit.insert(editor.document.uri, new vscode.Position(0, 0), "// Edited by Vitte sample\n");
        await vscode.workspace.applyEdit(edit);
    }), vscode.commands.registerCommand("vitte.progressSample", async () => {
        await vscode.window.withProgress({ location: vscode.ProgressLocation.Window, title: "Vitte: Running task" }, async (p) => {
            p.report({ message: "Step 1/3" });
            await sleep(250);
            p.report({ message: "Step 2/3" });
            await sleep(250);
            p.report({ message: "Step 3/3" });
            await sleep(250);
        });
    }), vscode.commands.registerCommand("vitte.showInfo", async () => {
        const cfg = vscode.workspace.getConfiguration("vitte");
        const trace = cfg.get("trace.server", "off");
        const offline = cfg.get("server.offline", false);
        const offlinePermanent = cfg.get("server.offlinePermanent", false);
        const offlineMsg = offlinePermanent ? " (offline permanent)" : (offline ? " (offline)" : "");
        await vscode.window.showInformationMessage(`Vitte LSP — trace: ${trace}${offlineMsg}`);
    }), vscode.commands.registerCommand("vitte.offline.explain", async () => {
        const uri = vscode.Uri.file(path.join(context.extensionPath, "media", "offline.md"));
        await vscode.commands.executeCommand("vscode.open", uri);
        void vscode.commands.executeCommand("vitte.offline.openLog");
    }), vscode.commands.registerCommand("vitte.offline.copyReport", async () => {
        const report = await readOfflineReport();
        await vscode.env.clipboard.writeText(report);
        void vscode.window.showInformationMessage("Vitte: offline report copied to clipboard.");
    }), vscode.commands.registerCommand("vitte.goOffline", async () => {
        await setOfflineMode(true, "Manual offline mode enabled.");
        if (client) {
            try {
                await client.stop();
            }
            catch { /* noop */ }
            client = undefined;
        }
    }), vscode.commands.registerCommand("vitte.debug.runFile", async () => { await runDebugCurrentFile(); }), vscode.commands.registerCommand("vitte.debug.attachServer", async () => { await attachDebugServer(); }));
    // Refresh status depending on the active editor
    context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(updateStatusText));
    context.subscriptions.push(vscode.languages.registerInlineCompletionItemProvider(LANGUAGES.map((id) => ({ language: id, scheme: "file" })), {
        provideInlineCompletionItems(document, position, _ctx, _token) {
            const cfg = vscode.workspace.getConfiguration("vitte");
            const enabled = cfg.get("suggestions.ghostTextTop1Stable", true);
            if (!enabled)
                return [];
            const threshold = Math.max(1, Math.min(8, cfg.get("suggestions.ghostTextStableThreshold", 2)));
            const maxAgeMs = Math.max(200, Math.min(10000, cfg.get("suggestions.ghostTextMaxAgeMs", 2500)));
            const key = (0, suggestions_1.getCompletionRequestKey)(document, position);
            const candidate = completionTop1StableByKey.get(key);
            if (!candidate)
                return [];
            if (candidate.count < threshold)
                return [];
            if ((Date.now() - candidate.lastAt) > maxAgeMs)
                return [];
            const wordRange = document.getWordRangeAtPosition(position);
            const prefix = wordRange ? document.getText(wordRange) : "";
            if (prefix.length > 0) {
                const lhs = candidate.text.toLowerCase();
                const rhs = prefix.toLowerCase();
                if (!lhs.startsWith(rhs))
                    return [];
                if (lhs === rhs)
                    return [];
            }
            const range = wordRange ?? new vscode.Range(position, position);
            const item = new vscode.InlineCompletionItem(candidate.text, range);
            return [item];
        },
    }));
    context.subscriptions.push(vscode.window.onDidChangeTextEditorSelection((e) => {
        scheduleCompletionIdlePrefetch(e.textEditor);
    }));
    scheduleCompletionIdlePrefetch(vscode.window.activeTextEditor ?? undefined);
    updateStatusText(vscode.window.activeTextEditor ?? undefined);
    // Relance si config Vitte change
    context.subscriptions.push(vscode.workspace.onDidChangeConfiguration(async (e) => {
        if (e.affectsConfiguration("vitte.commandShortcuts")) {
            updateCommandButtons(context);
        }
        if (e.affectsConfiguration("vitte")) {
            await restartClient(context);
        }
        if (e.affectsConfiguration("vitte.server.offlinePermanent")) {
            if (isOfflinePermanent()) {
                setOfflineStatus("Offline permanent (user-forced).");
            }
        }
        if (e.affectsConfiguration("vitte.lint") || e.affectsConfiguration("vitte.features.lint") || e.affectsConfiguration("vitte.server.offline") || e.affectsConfiguration("vitte.server.offlinePermanent")) {
            for (const doc of vscode.workspace.textDocuments)
                updateEditorLint(doc);
        }
    }));
    // Diagnostics view for both beginners and power users
    (0, diagnosticsView_1.registerDiagnosticsView)(context);
    (0, moduleExplorerView_1.registerModuleExplorerView)(context);
    (0, metricsView_1.registerMetricsView)(context, () => client, getStreamingCompletionStats);
    (0, packageProblemsView_1.registerPackageProblemsView)(context);
    (0, moduleGraphView_1.registerModuleGraphView)(context);
    (0, commandCenterView_1.registerCommandCenterView)(context, () => client);
    (0, offlineView_1.registerOfflineView)(context, () => offlineReason, () => output, () => formatOfflineSince(), () => {
        const summary = (0, diagnostics_1.summarizeWorkspaceDiagnostics)();
        const total = summary.errors + summary.warnings + summary.info + summary.hints;
        if (total === 0)
            return "No local diagnostics";
        return `${summary.errors} errors, ${summary.warnings} warnings`;
    });
    context.subscriptions.push(vscode.languages.onDidChangeDiagnostics(() => refreshDiagnosticsStatus()));
    context.subscriptions.push(vscode.workspace.onDidSaveTextDocument(async (doc) => {
        if (!shouldFormatOnSave(doc))
            return;
        if (formatOnSaveInFlight.has(doc.uri.toString()))
            return;
        const editor = vscode.window.visibleTextEditors.find((e) => e.document.uri.toString() === doc.uri.toString());
        if (!editor)
            return;
        formatOnSaveInFlight.add(doc.uri.toString());
        try {
            await vscode.window.showTextDocument(editor.document, { preview: false, preserveFocus: true });
            await vscode.commands.executeCommand("editor.action.formatDocument");
            if (editor.document.isDirty) {
                await editor.document.save();
            }
        }
        catch (err) {
            output.appendLine(`[format.onSave] ${String(err)}`);
        }
        finally {
            formatOnSaveInFlight.delete(doc.uri.toString());
        }
    }));
    if (isOfflineEnabled()) {
        setOfflineStatus("Offline mode is enabled (vitte.server.offline).");
    }
    if (isOfflinePermanent()) {
        setOfflineStatus("Offline permanent (user-forced).");
    }
    startHealthChecks(context);
    if (process.env.VSCODE_TESTING === "1") {
        const api = {
            getStatusText: () => statusItem?.text ?? "",
            getStatusTooltip: () => {
                const tip = statusItem?.tooltip;
                if (typeof tip === "string")
                    return tip;
                if (tip instanceof vscode.MarkdownString) {
                    return tip.value ?? "";
                }
                return "";
            },
            getClientState: () => client?.state,
            runAction: async (action) => {
                await runBuiltinAction(action);
            },
            restart: async () => {
                await restartClient(context);
            },
            resolveServerModuleForTest: (ctx) => resolveServerModule(ctx),
        };
        return api;
    }
    return undefined;
}
async function deactivate() {
    try {
        await client?.stop();
    }
    catch { /* noop */ }
    client = undefined;
    if (suggestionProfilerRenderTimer) {
        clearInterval(suggestionProfilerRenderTimer);
        suggestionProfilerRenderTimer = undefined;
    }
    try {
        suggestionProfilerPanel?.dispose();
    }
    catch { /* noop */ }
    suggestionProfilerPanel = undefined;
    if (healthCheckTimer) {
        clearInterval(healthCheckTimer);
        healthCheckTimer = undefined;
    }
    healthFailures = 0;
    healthRestartInFlight = false;
    reliabilityAttempts = 0;
    reliabilityNextDelayMs = 30000;
    recentStops.length = 0;
    offlineBannerShown = false;
    for (const watcher of fileWatchers) {
        try {
            watcher.dispose();
        }
        catch { /* noop */ }
    }
    fileWatchers = [];
}
/* --------------------------------- LSP ----------------------------------- */
function resolveServerModule(context) {
    // Permet d’overrider via settings: vitte.serverPath
    const cfgPath = vscode.workspace.getConfiguration("vitte").get("serverPath");
    if (cfgPath) {
        if (fs.existsSync(cfgPath)) {
            logServerResolution(`Using custom server: ${cfgPath}`);
            return cfgPath;
        }
        logServerResolution(`Custom server path not found: ${cfgPath}`);
    }
    const nested = context.asAbsolutePath(path.join("server", "out", "src", "server.js"));
    if (fs.existsSync(nested)) {
        logServerResolution(`Using packaged server (server/out/src): ${nested}`);
        return nested;
    }
    const legacyNested = context.asAbsolutePath(path.join("server", "out", "server.js"));
    if (fs.existsSync(legacyNested)) {
        logServerResolution(`Using legacy server (server/out): ${legacyNested}`);
        return legacyNested;
    }
    const bundled = context.asAbsolutePath(path.join("out", "server.js"));
    if (fs.existsSync(bundled)) {
        logServerResolution(`Using embedded server: ${bundled}`);
        return bundled;
    }
    const message = "Module serveur Vitte introuvable (out/server.js ou server/out/server.js)";
    logServerResolution(message);
    throw new Error(message);
}
async function startClient(context) {
    if (client)
        return true; // already running
    if (!context)
        return false;
    if (isOfflineEffective()) {
        const reason = isOfflinePermanent()
            ? "Offline permanent (user-forced)."
            : "Offline mode is enabled (vitte.server.offline).";
        setOfflineStatus(reason);
        return false;
    }
    let serverModule;
    try {
        serverModule = resolveServerModule(context);
    }
    catch (err) {
        const msg = err instanceof Error ? err.message : String(err);
        setOfflineStatus(msg);
        void vscode.window.showWarningMessage(`Vitte: server offline — ${msg}`);
        return false;
    }
    const debugOptions = { execArgv: ["--nolazy", "--inspect=6009"] };
    const serverOptions = {
        run: { module: serverModule, transport: node_1.TransportKind.ipc },
        debug: { module: serverModule, transport: node_1.TransportKind.ipc, options: debugOptions },
    };
    const documentSelector = LANGUAGES.flatMap((id) => ([
        { scheme: "file", language: id },
        { scheme: "untitled", language: id },
        { scheme: "vscode-notebook-cell", language: id }
    ]));
    const watchers = ensureFileWatchers(context);
    const clientOptions = {
        documentSelector,
        outputChannel: output,
        revealOutputChannelOn: node_1.RevealOutputChannelOn.Never,
        synchronize: {
            configurationSection: "vitte",
            fileEvents: watchers
        },
        middleware: {
            provideCompletionItem: async (document, position, completionContext, token, next) => {
                const key = (0, suggestions_1.getCompletionRequestKey)(document, position);
                const contextKey = (0, suggestions_1.getCompletionContextKey)(document, position);
                const documentKey = (0, suggestions_1.getCompletionDocumentKey)(document);
                const isPrefetch = completionPrefetchExpectedKey === key;
                const forceLoadNextPage = completionLoadNextPageKey === key;
                if (forceLoadNextPage)
                    completionLoadNextPageKey = undefined;
                if (!isPrefetch && completionSuggestionPendingAccepted) {
                    completionSuggestionCanceledCount += 1;
                    completionFallbackCancelCount += 1;
                    if (completionPendingRejectedItems.length > 0) {
                        (0, suggestions_1.recordSuggestionRejections)(completionPendingRejectedItems);
                        completionPendingRejectedItems = [];
                    }
                    completionSuggestionPendingAccepted = false;
                }
                if (!isPrefetch && completionContext.triggerKind === vscode.CompletionTriggerKind.Invoke) {
                    if (completionNextInvokeIsAutoRefresh) {
                        completionNextInvokeIsAutoRefresh = false;
                    }
                    else {
                        completionStreamingManualRefreshCount += 1;
                    }
                }
                if (!isPrefetch)
                    completionStreamingTotalCount += 1;
                const cfg = vscode.workspace.getConfiguration("vitte");
                const tuning = {
                    adaptiveRanking: cfg.get("suggestions.adaptiveRanking", true),
                    dynamicBudget: cfg.get("suggestions.dynamicBudget", true),
                    showExplainLabels: cfg.get("suggestions.showExplainLabels", true),
                    showMatchHighlights: cfg.get("suggestions.showMatchHighlights", true),
                    rankingVariant: cfg.get("suggestions.rankingVariant", "default"),
                    churnPenaltyEnabled: cfg.get("suggestions.churnPenaltyEnabled", true),
                    churnPenaltyWeight: cfg.get("suggestions.churnPenaltyWeight", 1.0),
                    typeBudgetEnabled: cfg.get("suggestions.typeBudgetEnabled", true),
                    functionBudgetRatio: cfg.get("suggestions.functionBudgetRatio", 0.45),
                    variableBudgetRatio: cfg.get("suggestions.variableBudgetRatio", 0.35),
                    snippetBudgetRatio: cfg.get("suggestions.snippetBudgetRatio", 0.15),
                    topN: cfg.get("suggestions.topN", 50),
                    minN: cfg.get("suggestions.minN", 20),
                    maxN: cfg.get("suggestions.maxN", 80),
                    slowMs: cfg.get("suggestions.slowMs", 180),
                    intentMode: cfg.get("suggestions.intentMode", "auto"),
                    rankingDeadlineMs: Math.max(3, Math.min(120, cfg.get("suggestions.rankingDeadlineMs", 25))),
                };
                const pagingEnabled = cfg.get("suggestions.pagination", true);
                const pageSize = Math.max(5, Math.min(200, cfg.get("suggestions.pageSize", 20)));
                const maxPages = Math.max(1, Math.min(10, cfg.get("suggestions.maxPages", 5)));
                const pageResetMs = Math.max(500, Math.min(30000, cfg.get("suggestions.pageResetMs", 5000)));
                const streamingEnabled = cfg.get("suggestions.streaming", true);
                const firstPaintMs = cfg.get("suggestions.streamingFirstPaintMs", 75);
                const localBatchSize = cfg.get("suggestions.streamingLocalBatchSize", 40);
                const cachePrefixTtlMs = cfg.get("suggestions.cachePrefixTtlMs", 1500);
                const cacheContextTtlMs = cfg.get("suggestions.cacheContextTtlMs", 4000);
                const cacheDocumentTtlMs = cfg.get("suggestions.cacheDocumentTtlMs", 12000);
                const cacheMaxEntries = Math.max(32, Math.min(5000, cfg.get("suggestions.cacheMaxEntries", 1200)));
                const cacheSoftMemoryMb = Math.max(128, Math.min(8192, cfg.get("suggestions.cacheSoftMemoryMb", 1536)));
                const shadowRankingEnabled = cfg.get("suggestions.shadowRankingEnabled", false);
                const shadowRankingVariant = cfg.get("suggestions.shadowRankingVariant", "scope_heavy");
                const autoRefresh = cfg.get("suggestions.streamingAutoRefresh", true);
                const autoRefreshDebounceMinMs = Math.max(40, Math.min(500, cfg.get("suggestions.autoRefreshDebounceMinMs", 80)));
                const autoRefreshDebounceMaxMs = Math.max(autoRefreshDebounceMinMs, Math.min(1000, cfg.get("suggestions.autoRefreshDebounceMaxMs", 260)));
                const lspHardTimeoutMs = Math.max(50, Math.min(1000, cfg.get("suggestions.lspHardTimeoutMs", 120)));
                const lintGuard = cfg.get("suggestions.lintGuard", true);
                const diagnosticContextFilterEnabled = cfg.get("suggestions.diagnosticContextFilterEnabled", true);
                const diagnosticContextMinTokenLen = Math.max(2, Math.min(12, cfg.get("suggestions.diagnosticContextMinTokenLen", 4)));
                const securityGuardEnabled = cfg.get("suggestions.securityGuardEnabled", true);
                const securityBlockedPatterns = cfg.get("suggestions.securityBlockedPatterns", [
                    "eval\\s*\\(",
                    "exec\\s*\\(",
                    "rm\\s+-rf",
                    "curl\\s+.+\\|\\s*sh",
                    "password\\s*=",
                ]) ?? [];
                const workspaceWhitelistPatterns = cfg.get("suggestions.workspaceWhitelistPatterns", []) ?? [];
                const workspaceBlacklistPatterns = cfg.get("suggestions.workspaceBlacklistPatterns", []) ?? [];
                const stickyTopEnabled = cfg.get("suggestions.stickyTopEnabled", true);
                const stickyTopCount = Math.max(1, Math.min(32, cfg.get("suggestions.stickyTopCount", 8)));
                const negativeCacheTimeoutThreshold = Math.max(1, Math.min(12, cfg.get("suggestions.negativeCacheTimeoutThreshold", 3)));
                const negativeCacheHoldMs = Math.max(300, Math.min(15000, cfg.get("suggestions.negativeCacheHoldMs", 2500)));
                const highLoadThreshold = Math.max(1, Math.min(16, cfg.get("suggestions.highLoadThreshold", 6)));
                const toItems = (out) => {
                    if (!out)
                        return [];
                    return Array.isArray(out) ? out : out.items;
                };
                let requestedPage = 1;
                const isManualInvoke = !isPrefetch
                    && completionContext.triggerKind === vscode.CompletionTriggerKind.Invoke
                    && !completionNextInvokeIsAutoRefresh
                    && !forceLoadNextPage;
                if (pagingEnabled) {
                    const pageState = completionPagingState.get(key);
                    const pageNow = Date.now();
                    if (forceLoadNextPage && pageState) {
                        requestedPage = Math.min(maxPages, pageState.page);
                    }
                    else if (isManualInvoke && pageState && (pageNow - pageState.ts) <= pageResetMs) {
                        requestedPage = Math.min(maxPages, pageState.page + 1);
                    }
                    else if (isManualInvoke) {
                        requestedPage = 1;
                    }
                    else {
                        requestedPage = pageState?.page ?? 1;
                    }
                    completionPagingState.set(key, { page: requestedPage, ts: pageNow });
                }
                const budgetOverride = pagingEnabled ? requestedPage * pageSize : undefined;
                const finalize = (items, isIncomplete) => {
                    const guarded = items.filter((item) => (shouldKeepSuggestionWithLintGuard(item, document, position, lintGuard)
                        && shouldKeepSuggestionWithDiagnosticContext(item, document, position, diagnosticContextFilterEnabled, diagnosticContextMinTokenLen)
                        && shouldKeepSuggestionWithSecurityGuard(item, securityGuardEnabled, securityBlockedPatterns)
                        && shouldKeepSuggestionWithPatternLists(item, workspaceWhitelistPatterns, workspaceBlacklistPatterns)));
                    const stickyLabels = stickyTopEnabled
                        ? new Set((completionStickyAcceptedByDoc.get(document.uri.toString()) ?? []).slice(0, stickyTopCount))
                        : undefined;
                    const ranked = (0, suggestions_1.rankAndTrimCompletionItems)(guarded, document, position, tuning, completionLatencyWindowMs, budgetOverride, stickyLabels);
                    if (!isPrefetch && shadowRankingEnabled) {
                        const shadowTuning = {
                            ...tuning,
                            showExplainLabels: false,
                            showMatchHighlights: false,
                            rankingVariant: shadowRankingVariant,
                        };
                        const shadow = (0, suggestions_1.rankAndTrimCompletionItems)(guarded, document, position, shadowTuning, completionLatencyWindowMs, budgetOverride, stickyLabels);
                        const cmp = listAgreementAndDrift(ranked, shadow, 10);
                        pushWindowSample(completionShadowAgreement, cmp.agreement);
                        pushWindowSample(completionShadowDrift, cmp.drift);
                    }
                    const pagingIncomplete = pagingEnabled && guarded.length > ranked.length;
                    const listItems = pagingIncomplete ? [...ranked, buildLoadMoreCompletionItem()] : ranked;
                    if (!isPrefetch)
                        updateTop1StableInlineCandidate(key, listItems);
                    if (!isPrefetch) {
                        (0, suggestions_1.updateSuggestionChurnForRequest)(key, listItems.filter((it) => extractItemSource(it) !== "ui"));
                    }
                    if (!isPrefetch) {
                        completionSuggestionShownCount += listItems.length;
                        completionSuggestionPendingAccepted = listItems.length > 0;
                        completionPendingRejectedItems = listItems.filter((it) => extractItemSource(it) !== "ui").slice(0, 50);
                        const sourceCounts = new Map();
                        if (listItems.length >= 3 && !completionFirstUsableMarked.has(key)) {
                            pushWindowSample(completionFirstUsableMs, Date.now() - requestStartedAt);
                            completionFirstUsableMarked.add(key);
                            const trace = completionTraceActiveByKey.get(key);
                            if (trace)
                                trace.firstUsableMs = Math.max(0, Date.now() - requestStartedAt);
                        }
                        for (const item of listItems) {
                            const src = extractItemSource(item);
                            incCounter(completionShownBySource, src);
                            incCounter(sourceCounts, src);
                        }
                        const trace = completionTraceActiveByKey.get(key);
                        if (trace) {
                            trace.shownCount = listItems.filter((it) => extractItemSource(it) !== "ui").length;
                            trace.sourceMix = [...sourceCounts.entries()]
                                .filter(([src]) => src !== "ui")
                                .sort((a, b) => b[1] - a[1])
                                .map(([src, count]) => `${src}:${count}`)
                                .join(" | ");
                        }
                    }
                    return new vscode.CompletionList(listItems, isIncomplete || pagingIncomplete);
                };
                const now = Date.now();
                const requestStartedAt = now;
                const requestUri = document.uri.toString();
                const requestUriShort = vscode.workspace.asRelativePath(document.uri, false) || path.basename(document.uri.fsPath);
                if (!isPrefetch) {
                    completionRequestStartedAt.set(key, now);
                    completionRequestRefreshed.set(key, false);
                    completionFirstUsableMarked.delete(key);
                    completionTraceActiveByKey.set(key, {
                        id: `${key}@${requestStartedAt}`,
                        requestKey: key,
                        languageId: document.languageId,
                        uriShort: requestUriShort,
                        startedAt: requestStartedAt,
                        refreshCause: "none",
                        fallbackCause: "none",
                        shownCount: 0,
                    });
                }
                if (!isPrefetch)
                    completionCacheRequestCount += 1;
                const cachedItemsPrefix = getValidCacheItems(completionStreamingCachePrefix, key, cachePrefixTtlMs).map((item) => withSourceBadge(item, "cache", true));
                const cachedItemsContext = getValidCacheItems(completionStreamingCacheContext, contextKey, cacheContextTtlMs).map((item) => withSourceBadge(item, "cache", true));
                const cachedItemsDocument = getValidCacheItems(completionStreamingCacheDocument, documentKey, cacheDocumentTtlMs).map((item) => withSourceBadge(item, "cache", true));
                if (!isPrefetch && (cachedItemsPrefix.length > 0 || cachedItemsContext.length > 0 || cachedItemsDocument.length > 0)) {
                    completionCacheHitCount += 1;
                }
                if (!isPrefetch && cachedItemsPrefix.length > 0 && completionPrefetchRequestCount > 0) {
                    completionPrefetchHitCount += 1;
                }
                const load = os.loadavg()[0] ?? 0;
                const highLoad = load >= highLoadThreshold;
                const localItems = highLoad
                    ? (0, suggestions_1.buildPrefixOnlyLocalBatch)(document, position, Math.max(8, Math.min(localBatchSize, 20)))
                    : (0, suggestions_1.buildLocalCompletionBatch)(document, position, localBatchSize);
                const negativeCacheKey = `${document.languageId}`;
                const neg = completionLspNegativeCache.get(negativeCacheKey);
                const negativeCacheActive = !!neg && neg.until > Date.now();
                const seedMap = new Map();
                for (const item of [...cachedItemsPrefix, ...cachedItemsContext, ...cachedItemsDocument, ...localItems]) {
                    seedMap.set(seedDedupKey(item), item);
                }
                const seeded = [...seedMap.values()];
                const requestPromise = (async () => {
                    if (negativeCacheActive) {
                        if (!isPrefetch)
                            completionFallbackNegativeCacheCount += 1;
                        if (!isPrefetch) {
                            const trace = completionTraceActiveByKey.get(key);
                            if (trace)
                                trace.fallbackCause = "negative_cache";
                        }
                        return { out: null, items: [] };
                    }
                    const t0 = Date.now();
                    const out = await Promise.race([
                        Promise.resolve(next(document, position, completionContext, token)),
                        new Promise((resolve) => setTimeout(() => resolve(null), lspHardTimeoutMs)),
                    ]);
                    const dt = Date.now() - t0;
                    completionLatencyWindowMs.push(dt);
                    if (completionLatencyWindowMs.length > 128)
                        completionLatencyWindowMs.shift();
                    if (!out && !isPrefetch) {
                        completionStreamingTimeoutCount += 1;
                        completionFallbackTimeoutCount += 1;
                        const trace = completionTraceActiveByKey.get(key);
                        if (trace)
                            trace.fallbackCause = "timeout";
                        const prev = completionLspNegativeCache.get(negativeCacheKey);
                        const timeouts = (prev?.timeouts ?? 0) + 1;
                        const until = timeouts >= negativeCacheTimeoutThreshold ? Date.now() + negativeCacheHoldMs : 0;
                        completionLspNegativeCache.set(negativeCacheKey, { timeouts, until });
                    }
                    else if (!isPrefetch) {
                        completionLspNegativeCache.delete(negativeCacheKey);
                    }
                    const items = toItems(out).map((item) => withSourceBadge(item, "lsp"));
                    const ts = Date.now();
                    touchMapEntry(completionStreamingCachePrefix, key, { items, ts });
                    touchMapEntry(completionStreamingCacheContext, contextKey, { items, ts });
                    touchMapEntry(completionStreamingCacheDocument, documentKey, { items, ts });
                    trimCompletionCaches(cacheMaxEntries, cacheSoftMemoryMb);
                    return { out, items };
                })();
                if (!streamingEnabled) {
                    const { out, items } = await requestPromise;
                    const result = finalize(items.length > 0 ? items : seeded, !out || Array.isArray(out) ? false : Boolean(out.isIncomplete));
                    if (!isPrefetch)
                        completeCompletionRequestMetrics(key, Date.now(), requestStartedAt);
                    return result;
                }
                const inFlight = completionStreamingInFlight.get(key);
                if (!inFlight) {
                    completionStreamingInFlight.set(key, requestPromise.then(async ({ out, items }) => {
                        if (!isPrefetch)
                            pushWindowSample(completionStreamingEnrichMs, Date.now() - now);
                        if (!isPrefetch)
                            pushWindowSample(completionStableListMs, Date.now() - now);
                        if (!isPrefetch) {
                            const trace = completionTraceActiveByKey.get(key);
                            if (trace) {
                                trace.enrichMs = Math.max(0, Date.now() - requestStartedAt);
                                trace.stableMs = Math.max(0, Date.now() - requestStartedAt);
                            }
                        }
                        const isIncomplete = !out || Array.isArray(out) ? false : out.isIncomplete;
                        if (isPrefetch || !autoRefresh || items.length === 0)
                            return;
                        const editor = vscode.window.activeTextEditor;
                        if (!editor || editor.document.uri.toString() !== requestUri)
                            return;
                        if (editor.selection.active.line !== position.line)
                            return;
                        if (Math.abs(editor.selection.active.character - position.character) > 2)
                            return;
                        if (completionStreamingRefreshGuard)
                            return;
                        completionStreamingRefreshGuard = true;
                        try {
                            if (isIncomplete || items.length > seeded.length) {
                                completionStreamingRefreshCount += 1;
                                if (isIncomplete) {
                                    completionStreamingIncompleteRefreshCount += 1;
                                    const trace = completionTraceActiveByKey.get(key);
                                    if (trace)
                                        trace.refreshCause = "incomplete";
                                }
                                else {
                                    completionStreamingRicherRefreshCount += 1;
                                    const trace = completionTraceActiveByKey.get(key);
                                    if (trace)
                                        trace.refreshCause = "richer";
                                }
                                completionRequestRefreshed.set(key, true);
                                const docKey = document.uri.toString();
                                const editBefore = typingSpeedByDoc.get(docKey)?.lastEditAt ?? 0;
                                const debounceMs = getAdaptiveRefreshDebounceMs(docKey, autoRefreshDebounceMinMs, autoRefreshDebounceMaxMs);
                                await new Promise((resolve) => setTimeout(resolve, debounceMs));
                                const editAfter = typingSpeedByDoc.get(docKey)?.lastEditAt ?? 0;
                                if (editAfter > editBefore)
                                    return;
                                completionNextInvokeIsAutoRefresh = true;
                                await vscode.commands.executeCommand("editor.action.triggerSuggest");
                            }
                        }
                        finally {
                            setTimeout(() => { completionStreamingRefreshGuard = false; }, 120);
                        }
                    }).finally(() => {
                        if (!isPrefetch)
                            completeCompletionRequestMetrics(key, Date.now(), requestStartedAt);
                        completionStreamingInFlight.delete(key);
                    }));
                }
                else if (!isPrefetch) {
                    void inFlight.finally(() => {
                        completeCompletionRequestMetrics(key, Date.now(), requestStartedAt);
                    });
                }
                const raced = await Promise.race([
                    requestPromise.then((v) => ({ type: "full", ...v })),
                    new Promise((resolve) => setTimeout(() => resolve({ type: "timeout" }), firstPaintMs)),
                ]);
                if (raced.type === "full") {
                    if (!isPrefetch) {
                        pushWindowSample(completionStreamingFirstPaintMs, Date.now() - now);
                        const trace = completionTraceActiveByKey.get(key);
                        if (trace && trace.firstPaintMs === undefined) {
                            trace.firstPaintMs = Math.max(0, Date.now() - requestStartedAt);
                        }
                        const opened = completionOpenedAtByDoc.get(document.uri.toString());
                        if (opened) {
                            completionColdStartCount += 1;
                            pushWindowSample(completionColdStartMs, Date.now() - opened);
                            completionOpenedAtByDoc.delete(document.uri.toString());
                        }
                        pushWindowSample(completionStableListMs, Date.now() - now);
                    }
                    return finalize(raced.items, !raced.out || Array.isArray(raced.out) ? false : Boolean(raced.out.isIncomplete));
                }
                if (!isPrefetch) {
                    completionStreamingTimeoutCount += 1;
                    completionFallbackTimeoutCount += 1;
                    const trace = completionTraceActiveByKey.get(key);
                    if (trace) {
                        trace.firstPaintMs = Math.max(0, Date.now() - requestStartedAt);
                        trace.fallbackCause = "timeout";
                    }
                    pushWindowSample(completionStreamingFirstPaintMs, Date.now() - now);
                }
                return finalize(seeded, true);
            },
            resolveCompletionItem: async (item, token, next) => {
                (0, suggestions_1.recordSuggestionUsage)(item);
                const src = extractItemSource(item);
                if (src !== "ui") {
                    completionSuggestionAcceptedCount += 1;
                    incCounter(completionAcceptedBySource, src);
                    const stickyTopCount = Math.max(1, Math.min(32, vscode.workspace.getConfiguration("vitte").get("suggestions.stickyTopCount", 8)));
                    const docUri = vscode.window.activeTextEditor?.document.uri.toString();
                    if (docUri) {
                        recordStickyAcceptedLabel(docUri, item, stickyTopCount * 4);
                    }
                }
                completionPendingRejectedItems = [];
                completionSuggestionPendingAccepted = false;
                return Promise.resolve(next(item, token));
            },
            provideDocumentFormattingEdits: async (doc, options, token, next) => {
                try {
                    return await next(doc, options, token);
                }
                catch {
                    await vscode.commands.executeCommand("editor.action.formatDocument");
                    return [];
                }
            },
        },
        initializationOptions: {
        // Extension → Serveur: options d’init (libre)
        },
    };
    client = new node_1.LanguageClient("vitte-lsp", "Vitte Language Server", serverOptions, clientOptions);
    client.onTelemetry((e) => {
        output.appendLine(`[telemetry] ${JSON.stringify(e)}`);
    });
    wireClientState(client);
    await client.start();
    return true;
}
async function restartClient(context) {
    if (client) {
        setStatusBase("$(sync)", "Vitte LSP: restarting…");
        try {
            await client.stop();
        }
        catch { /* noop */ }
        client = undefined;
    }
    if (isOfflineEffective()) {
        const reason = isOfflinePermanent()
            ? "Offline permanent (user-forced)."
            : "Offline mode is enabled (vitte.server.offline).";
        setOfflineStatus(reason);
        return false;
    }
    return startClient(context);
}
function startHealthChecks(context) {
    if (healthCheckTimer)
        clearInterval(healthCheckTimer);
    let lastBudgetAlert = 0;
    healthCheckTimer = setInterval(() => {
        void (async () => {
            if (isOfflineEffective())
                return;
            if (!client || client.state !== node_1.State.Running)
                return;
            try {
                await client.sendRequest("vitte/ping");
                healthFailures = 0;
                try {
                    const metrics = await client.sendRequest("vitte/metrics");
                    const cfg = vscode.workspace.getConfiguration("vitte");
                    const budgets = {
                        completion: cfg.get("semanticBudget.completionP95Ms", 900),
                        hover: cfg.get("semanticBudget.hoverP95Ms", 600),
                        rename: cfg.get("semanticBudget.renameP95Ms", 1200),
                        references: cfg.get("semanticBudget.referencesP95Ms", 1200),
                    };
                    const over = [];
                    for (const key of Object.keys(budgets)) {
                        const m = metrics.find((x) => x.name === key);
                        const p95 = m?.p95Ms ?? m?.averageMs ?? 0;
                        if (p95 > budgets[key])
                            over.push(`${key} ${p95.toFixed(1)}>${budgets[key]}`);
                    }
                    if (over.length > 0 && Date.now() - lastBudgetAlert > 120000) {
                        lastBudgetAlert = Date.now();
                        void vscode.window.showWarningMessage(`Vitte semantic budget exceeded: ${over.join(" | ")}`);
                    }
                }
                catch {
                    // ignore budget telemetry errors
                }
            }
            catch (err) {
                healthFailures += 1;
                output.appendLine(`[health] ping failure #${healthFailures}: ${String(err)}`);
                if (healthFailures < 2)
                    return;
                if (healthRestartInFlight)
                    return;
                const cfg = vscode.workspace.getConfiguration("vitte");
                const maxAttempts = Math.max(1, cfg.get("reliability.maxRestartAttempts", 3));
                const baseRetry = Math.max(1000, cfg.get("reliability.baseRetryMs", 30000));
                const maxRetry = Math.max(baseRetry, cfg.get("reliability.maxRetryMs", 300000));
                const cooldownMs = Math.max(10000, cfg.get("reliability.cooldownMs", 120000));
                if (reliabilityAttempts >= maxAttempts) {
                    setStatusOverride(undefined, `Reliability guard open-circuit (${reliabilityAttempts}/${maxAttempts})`);
                    setTimeout(() => {
                        reliabilityAttempts = 0;
                        reliabilityNextDelayMs = baseRetry;
                    }, cooldownMs);
                    return;
                }
                healthRestartInFlight = true;
                try {
                    const jitter = Math.floor(Math.random() * Math.max(1, Math.floor(reliabilityNextDelayMs * 0.25)));
                    await sleep(reliabilityNextDelayMs + jitter);
                    const ok = await restartClient(context);
                    if (!ok) {
                        setOfflineStatus("Health check restart failed.");
                    }
                    else {
                        healthFailures = 0;
                        reliabilityAttempts = 0;
                        reliabilityNextDelayMs = baseRetry;
                    }
                    reliabilityAttempts += 1;
                    reliabilityNextDelayMs = Math.min(maxRetry, Math.max(baseRetry, reliabilityNextDelayMs * 2));
                }
                finally {
                    healthRestartInFlight = false;
                }
            }
        })();
    }, 30000);
}
function wireClientState(c) {
    c.onDidChangeState((e) => {
        if (e.newState === node_1.State.Starting) {
            setStatusBase("$(gear)", "Vitte LSP: starting");
            void setServerOnlineContext(false);
        }
        else if (e.newState === node_1.State.Running) {
            offlineReason = undefined;
            setStatusBase("$(check)", "Vitte LSP: running");
            void setServerOnlineContext(true);
        }
        else if (e.newState === node_1.State.Stopped) {
            setStatusBase("$(debug-stop)", "Vitte LSP: stopped");
            void setServerOnlineContext(false);
            const now = Date.now();
            recentStops.push(now);
            while (recentStops.length) {
                const first = recentStops[0];
                if (first === undefined)
                    break;
                if ((now - first) <= 120000)
                    break;
                recentStops.shift();
            }
            if (!isOfflineEnabled() && recentStops.length >= 3) {
                setOfflineStatus("Server stopped repeatedly (3x in 2 minutes).");
            }
        }
    });
    c.onNotification("vitte/status", (msg) => {
        const text = typeof msg?.text === "string" ? msg.text : undefined;
        const tooltip = typeof msg?.tooltip === "string" ? msg.tooltip : undefined;
        if (text !== undefined || tooltip !== undefined) {
            setStatusOverride(text, tooltip);
        }
    });
    c.onNotification("vitte/log", (msg) => {
        output.appendLine(typeof msg === "string" ? msg : JSON.stringify(msg));
    });
}
/* ----------------------------- Actions utilitaires ------------------------ */
async function runBuiltinAction(action) {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        void vscode.window.showWarningMessage("Open a Vitte/Vit document before running this action.");
        return;
    }
    const languageId = editor.document.languageId;
    if (!LANGUAGE_SET.has(languageId)) {
        void vscode.window.showWarningMessage("Les actions Vitte ne sont disponibles que pour les fichiers Vitte/Vit.");
        return;
    }
    switch (action) {
        case "format":
            await vscode.commands.executeCommand("editor.action.formatDocument");
            return;
        case "organizeImports":
            await vscode.commands.executeCommand("editor.action.organizeImports");
            return;
        case "fixAll":
            await vscode.commands.executeCommand("editor.action.codeAction", {
                kind: vscode.CodeActionKind.SourceFixAll.value,
                apply: "first"
            });
            return;
        default:
            void vscode.window.showWarningMessage(`Action inconnue: ${action}`);
            return;
    }
}
function showOfflineNoop(action) {
    const reason = offlineReason ?? (isOfflinePermanent()
        ? "Offline permanent (user-forced)."
        : "Offline mode is enabled (vitte.server.offline).");
    void vscode.window.showWarningMessage(`Vitte: ${action} unavailable while offline — ${reason}`);
}
function formatOfflineSince() {
    if (!offlineSince)
        return "unknown";
    const seconds = Math.floor((Date.now() - offlineSince) / 1000);
    if (seconds < 60)
        return `${seconds}s`;
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60)
        return `${minutes}m`;
    const hours = Math.floor(minutes / 60);
    if (hours < 24)
        return `${hours}h`;
    const days = Math.floor(hours / 24);
    return `${days}d`;
}
async function readOfflineReport() {
    const cfg = vscode.workspace.getConfiguration("vitte");
    const report = [];
    report.push(`# Vitte Offline Report`);
    report.push(`- offline: ${isOfflineEffective()}`);
    report.push(`- offlinePermanent: ${isOfflinePermanent()}`);
    report.push(`- offlineSince: ${offlineSince ? new Date(offlineSince).toISOString() : "unknown"}`);
    report.push(`- offlineReason: ${offlineReason ?? "unknown"}`);
    report.push(`- autoRetry: ${cfg.get("server.autoRetry", true)}`);
    report.push(`- autoRetryBaseMs: ${cfg.get("server.autoRetryBaseMs", 60000)}`);
    report.push(`- autoRetryMaxMs: ${cfg.get("server.autoRetryMaxMs", 300000)}`);
    report.push(`- workspaceFolders: ${(vscode.workspace.workspaceFolders ?? []).length}`);
    report.push(`- openEditors: ${vscode.workspace.textDocuments.length}`);
    report.push(`- diagnostics (local): ${(0, diagnostics_1.summarizeWorkspaceDiagnostics)().errors} errors, ${(0, diagnostics_1.summarizeWorkspaceDiagnostics)().warnings} warnings`);
    report.push(`- offlineLog: ${getOfflineLogPathSafe()}`);
    const tail = await readOfflineLogTail(30);
    if (tail) {
        report.push(`\n## offline.log (last 30 lines)\n${tail}`);
    }
    return report.join("\n");
}
function getOfflineLogPathSafe() {
    try {
        const folder = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
        const base = folder ?? os.tmpdir();
        return path.join(base, ".vitte", "offline.log");
    }
    catch {
        return "unknown";
    }
}
async function readOfflineLogTail(lines) {
    try {
        const filePath = getOfflineLogPathSafe();
        const content = await fs.promises.readFile(filePath, "utf8");
        const rows = content.trim().split(/\r?\n/);
        return rows.slice(-lines).join("\n");
    }
    catch {
        return "";
    }
}
function isVitteDocument(doc) {
    return LANGUAGE_SET.has(doc.languageId);
}
function updateEditorLint(doc) {
    if (!editorLintCollection)
        return;
    const cfg = vscode.workspace.getConfiguration("vitte");
    if (!cfg.get("features.lint", true)) {
        editorLintCollection.delete(doc.uri);
        return;
    }
    if (!isVitteDocument(doc)) {
        editorLintCollection.delete(doc.uri);
        return;
    }
    const lintCfg = cfg.get("lint") ?? {};
    const maxLineLength = typeof lintCfg.maxLineLength === "number" ? lintCfg.maxLineLength : 120;
    const allowTabs = lintCfg.allowTabs === true;
    const allowTrailing = lintCfg.allowTrailingWhitespace === true;
    const diagnostics = [];
    const lines = doc.getText().split(/\r?\n/);
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        if (line === undefined)
            continue;
        if (!allowTabs && line.includes("\t")) {
            const idx = line.indexOf("\t");
            diagnostics.push(new vscode.Diagnostic(new vscode.Range(i, idx, i, idx + 1), "Tabulation détectée. Utiliser des espaces.", vscode.DiagnosticSeverity.Warning));
        }
        if (!allowTrailing) {
            const m = /[ \t]+$/.exec(line);
            if (m) {
                const start = m.index ?? Math.max(0, line.length - m[0].length);
                const trailing = new vscode.Diagnostic(new vscode.Range(i, start, i, line.length), "Espaces en fin de ligne.", vscode.DiagnosticSeverity.Hint);
                trailing.tags = [vscode.DiagnosticTag.Unnecessary];
                diagnostics.push(trailing);
            }
        }
        if (maxLineLength > 0 && line.length > maxLineLength) {
            diagnostics.push(new vscode.Diagnostic(new vscode.Range(i, maxLineLength, i, line.length), `Ligne trop longue (${line.length} > ${maxLineLength}).`, vscode.DiagnosticSeverity.Hint));
        }
    }
    diagnostics.push(...buildBracketDiagnostics(lines));
    editorLintCollection.set(doc.uri, diagnostics);
}
function buildBracketDiagnostics(lines) {
    const diagnostics = [];
    const stack = [];
    const pairs = { ")": "(", "]": "[", "}": "{" };
    const openers = new Set(["(", "[", "{"]);
    const closers = new Set([")", "]", "}"]);
    for (let lineIndex = 0; lineIndex < lines.length; lineIndex++) {
        const rawLine = lines[lineIndex] ?? "";
        const codeLine = normalizeForBracketScan(rawLine);
        for (let col = 0; col < codeLine.length; col++) {
            const char = codeLine[col];
            if (!char)
                continue;
            if (openers.has(char)) {
                stack.push({ char: char, line: lineIndex, col });
                continue;
            }
            if (!closers.has(char))
                continue;
            const expected = pairs[char];
            const top = stack[stack.length - 1];
            if (!top || top.char !== expected) {
                diagnostics.push(new vscode.Diagnostic(new vscode.Range(lineIndex, col, lineIndex, col + 1), `Parenthèse/accolade fermante inattendue "${char}".`, vscode.DiagnosticSeverity.Error));
                continue;
            }
            stack.pop();
        }
    }
    for (const entry of stack) {
        const expected = entry.char === "(" ? ")" : entry.char === "[" ? "]" : "}";
        diagnostics.push(new vscode.Diagnostic(new vscode.Range(entry.line, entry.col, entry.line, entry.col + 1), `Parenthèse/accolade ouvrante "${entry.char}" non fermée (attendu: "${expected}").`, vscode.DiagnosticSeverity.Error));
    }
    return diagnostics;
}
function normalizeForBracketScan(line) {
    let result = "";
    let inSingle = false;
    let inDouble = false;
    let inTemplate = false;
    let escaped = false;
    for (let i = 0; i < line.length; i++) {
        const ch = line[i] ?? "";
        const next = i + 1 < line.length ? (line[i + 1] ?? "") : "";
        if (!inSingle && !inDouble && !inTemplate && ch === "/" && next === "/") {
            break;
        }
        if (escaped) {
            escaped = false;
            result += " ";
            continue;
        }
        if (ch === "\\") {
            escaped = true;
            result += " ";
            continue;
        }
        if (!inDouble && !inTemplate && ch === "'") {
            inSingle = !inSingle;
            result += " ";
            continue;
        }
        if (!inSingle && !inTemplate && ch === "\"") {
            inDouble = !inDouble;
            result += " ";
            continue;
        }
        if (!inSingle && !inDouble && ch === "`") {
            inTemplate = !inTemplate;
            result += " ";
            continue;
        }
        result += inSingle || inDouble || inTemplate ? " " : ch;
    }
    return result;
}
function summarizeWorkspaceEdit(edit) {
    const entries = edit.entries();
    let editCount = 0;
    for (const [, edits] of entries)
        editCount += edits.length;
    return { fileCount: entries.length, editCount };
}
async function summarizeWorkspaceEditDetailed(edit) {
    const base = summarizeWorkspaceEdit(edit);
    const details = [];
    for (const [uri, edits] of edit.entries()) {
        const rel = vscode.workspace.asRelativePath(uri, false);
        details.push(`${rel}: ${edits.length}`);
        if (details.length >= 12)
            break;
    }
    return { ...base, details };
}
async function detectRenameConflicts(edit, newName) {
    const conflictFiles = [];
    const declRx = new RegExp(`\\b(?:proc|fn|entry|let|const|static|type|struct|form|trait|enum|union)\\s+${newName}\\b`);
    for (const [uri] of edit.entries()) {
        try {
            const doc = await vscode.workspace.openTextDocument(uri);
            if (!["vitte", "vit"].includes(doc.languageId))
                continue;
            if (declRx.test(doc.getText())) {
                conflictFiles.push(vscode.workspace.asRelativePath(uri, false));
            }
        }
        catch {
            // ignore unreadable file
        }
    }
    return conflictFiles;
}
async function writeRenameReport(data) {
    const folder = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    if (!folder)
        return;
    const dir = path.join(folder, ".vitte-cache", "rename");
    try {
        await fs.promises.mkdir(dir, { recursive: true });
        const file = path.join(dir, `rename-${Date.now()}.json`);
        await fs.promises.writeFile(file, `${JSON.stringify(data, null, 2)}\n`, "utf8");
    }
    catch {
        // ignore report write errors
    }
}
async function captureWorkspaceEditSnapshots(edit) {
    const snapshots = new Map();
    for (const [uri] of edit.entries()) {
        try {
            const doc = await vscode.workspace.openTextDocument(uri);
            snapshots.set(uri.toString(), doc.getText());
        }
        catch {
            // ignore unreadable files
        }
    }
    return snapshots;
}
async function rollbackWorkspaceEditSnapshots(snapshots) {
    if (snapshots.size === 0)
        return;
    const rollback = new vscode.WorkspaceEdit();
    for (const [uriText, original] of snapshots) {
        const uri = vscode.Uri.parse(uriText);
        try {
            const doc = await vscode.workspace.openTextDocument(uri);
            const full = new vscode.Range(0, 0, doc.lineCount, 0);
            rollback.replace(uri, full, original);
        }
        catch {
            // ignore unavailable files
        }
    }
    await vscode.workspace.applyEdit(rollback);
}
function sleep(ms) { return new Promise(res => setTimeout(res, ms)); }
function updateStatusText(editor) {
    const lang = editor?.document?.languageId;
    if (lang && LANGUAGE_SET.has(lang)) {
        setStatusLanguageSuffix(lang);
        return;
    }
    setStatusLanguageSuffix("");
}
/* -------------------------------- Debug demo ------------------------------ */
async function runDebugCurrentFile() {
    const editor = vscode.window.activeTextEditor;
    if (!editor)
        return;
    const folder = vscode.workspace.workspaceFolders?.[0];
    const cfg = {
        type: "vitte",
        name: "Vitte: Launch current file",
        request: "launch",
        program: editor.document.fileName,
        cwd: folder?.uri.fsPath ?? path.dirname(editor.document.fileName),
        stopOnEntry: true,
        args: []
    };
    await vscode.debug.startDebugging(folder, cfg);
}
async function attachDebugServer() {
    const portStr = await vscode.window.showInputBox({ prompt: "Port du serveur Vitte", value: "9333" });
    if (!portStr)
        return;
    const folder = vscode.workspace.workspaceFolders?.[0];
    const cfg = {
        type: "vitte",
        name: "Vitte: Attach",
        request: "attach",
        port: Number.parseInt(portStr, 10),
    };
    if (!Number.isInteger(cfg.port) || cfg.port <= 0) {
        void vscode.window.showErrorMessage("Port Vitte invalide.");
        return;
    }
    await vscode.debug.startDebugging(folder, cfg);
}
//# sourceMappingURL=extension.js.map