"use strict";
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
exports.getCompletionRequestKey = getCompletionRequestKey;
exports.getCompletionContextKey = getCompletionContextKey;
exports.getCompletionDocumentKey = getCompletionDocumentKey;
exports.recordSuggestionUsage = recordSuggestionUsage;
exports.recordSuggestionRejections = recordSuggestionRejections;
exports.updateSuggestionChurnForRequest = updateSuggestionChurnForRequest;
exports.resetSuggestionLearningState = resetSuggestionLearningState;
exports.getSuggestionLearningSnapshot = getSuggestionLearningSnapshot;
exports.computeDynamicBudget = computeDynamicBudget;
exports.buildLocalCompletionBatch = buildLocalCompletionBatch;
exports.buildPrefixOnlyLocalBatch = buildPrefixOnlyLocalBatch;
exports.rankAndTrimCompletionItems = rankAndTrimCompletionItems;
const os = __importStar(require("node:os"));
const path = __importStar(require("node:path"));
const vscode = __importStar(require("vscode"));
const usageScore = new Map();
const rejectionScore = new Map();
const churnPenaltyScore = new Map();
const churnPrevOrderByRequest = new Map();
function getWordPrefix(document, position) {
    const range = document.getWordRangeAtPosition(position);
    return range ? document.getText(range) : "";
}
function getCompletionRequestKey(document, position) {
    const prefix = getWordPrefix(document, position).toLowerCase();
    return `${document.uri.toString()}#${position.line}:${position.character}:${prefix}`;
}
function getCompletionContextKey(document, position) {
    const line = document.lineAt(position.line).text;
    const head = line.slice(0, position.character).trim().toLowerCase().replace(/\s+/g, " ");
    const prev = position.line > 0
        ? document.lineAt(position.line - 1).text.trim().toLowerCase().replace(/\s+/g, " ")
        : "";
    return `${document.uri.toString()}#L${position.line}:${prev.slice(-40)}|${head.slice(-60)}`;
}
function getCompletionDocumentKey(document) {
    const maxLines = Math.min(document.lineCount, 200);
    const lines = [];
    for (let i = 0; i < maxLines; i += 1) {
        const t = document.lineAt(i).text.trim();
        if (/^(?:use|import|module|package|namespace)\b/i.test(t))
            lines.push(t.toLowerCase());
        if (lines.length >= 24)
            break;
    }
    return `${document.uri.toString()}#${document.languageId}:${document.lineCount}:${lines.join("|")}`;
}
function normalizeLabel(label) {
    const base = typeof label === "string" ? label : label.label;
    return base.trim().toLowerCase();
}
function decayRejected(raw) {
    if (!raw)
        return 0;
    const ageMs = Date.now() - raw.lastAt;
    const halfLifeMs = 45000;
    const decay = Math.pow(0.5, ageMs / halfLifeMs);
    return raw.count * decay;
}
function normalizeInsertText(insertText) {
    if (!insertText)
        return "";
    if (typeof insertText === "string")
        return insertText.replace(/\s+/g, " ").trim().toLowerCase();
    if (insertText instanceof vscode.SnippetString)
        return insertText.value.replace(/\s+/g, " ").trim().toLowerCase();
    return "";
}
function canonicalSuggestionKey(item) {
    const label = normalizeLabel(item.label);
    const kind = String(item.kind ?? 0);
    const insert = normalizeInsertText(item.insertText);
    return `${label}|${kind}|${insert}`;
}
function extractImports(document) {
    const maxLines = Math.min(document.lineCount, 250);
    const imports = [];
    for (let i = 0; i < maxLines; i += 1) {
        const t = document.lineAt(i).text.trim();
        const m = /^(?:use|import)\s+([A-Za-z0-9_./:-]+)/.exec(t);
        if (m?.[1])
            imports.push(m[1].toLowerCase());
    }
    return imports;
}
function collectDocumentWords(document) {
    const out = new Set();
    const maxLines = Math.min(document.lineCount, 600);
    for (let i = 0; i < maxLines; i += 1) {
        const line = document.lineAt(i).text;
        const matches = line.match(/[A-Za-z_][A-Za-z0-9_]{1,}/g);
        if (!matches)
            continue;
        for (const m of matches)
            out.add(m);
    }
    return [...out];
}
function percentile(values, q) {
    if (values.length === 0)
        return 0;
    const xs = [...values].sort((a, b) => a - b);
    const i = Math.max(0, Math.min(xs.length - 1, Math.round((xs.length - 1) * q)));
    return xs[i] ?? 0;
}
function resolveIntentMode(mode, document) {
    if (mode !== "auto")
        return mode;
    const p = document.uri.fsPath.toLowerCase();
    if (/(^|\/)(test|tests|__tests__|spec|specs)(\/|$)|\.(test|spec)\./.test(p))
        return "test";
    if (/(^|\/)(docs|documentation)(\/|$)|\.md$/.test(p))
        return "docs";
    return "code";
}
function scopeSymbols(document, position) {
    const out = new Set();
    const from = Math.max(0, position.line - 80);
    const to = Math.min(document.lineCount - 1, position.line + 40);
    let depth = 0;
    for (let i = position.line; i >= from; i -= 1) {
        const t = document.lineAt(i).text;
        for (let j = t.length - 1; j >= 0; j -= 1) {
            const ch = t[j];
            if (ch === "{")
                depth -= 1;
            else if (ch === "}")
                depth += 1;
        }
        if (depth < -2)
            break;
        const varMatches = t.match(/\b(?:let|var|const|mut)\s+([A-Za-z_][A-Za-z0-9_]*)/g) ?? [];
        for (const m of varMatches) {
            const n = m.split(/\s+/).pop();
            if (n)
                out.add(n.toLowerCase());
        }
        const fnMatch = /\b(?:fn|function|def)\s+([A-Za-z_][A-Za-z0-9_]*)/.exec(t);
        if (fnMatch?.[1])
            out.add(fnMatch[1].toLowerCase());
    }
    for (let i = position.line + 1; i <= to; i += 1) {
        const t = document.lineAt(i).text;
        const varMatches = t.match(/\b(?:let|var|const|mut)\s+([A-Za-z_][A-Za-z0-9_]*)/g) ?? [];
        for (const m of varMatches) {
            const n = m.split(/\s+/).pop();
            if (n)
                out.add(n.toLowerCase());
        }
    }
    return out;
}
function weightedEditDistance(a, b) {
    const m = a.length;
    const n = b.length;
    if (m === 0)
        return n;
    if (n === 0)
        return m;
    const dp = [];
    for (let i = 0; i <= m; i += 1) {
        const row = [];
        for (let j = 0; j <= n; j += 1)
            row.push(0);
        dp.push(row);
    }
    for (let i = 0; i <= m; i += 1)
        dp[i][0] = i;
    for (let j = 0; j <= n; j += 1)
        dp[0][j] = j;
    for (let i = 1; i <= m; i += 1) {
        for (let j = 1; j <= n; j += 1) {
            const ca = a.charAt(i - 1);
            const cb = b.charAt(j - 1);
            const subCost = ca === cb ? 0 : (ca.toLowerCase() === cb.toLowerCase() ? 0.35 : 1);
            const del = dp[i - 1][j] + 1;
            const ins = dp[i][j - 1] + 1;
            const sub = dp[i - 1][j - 1] + subCost;
            let best = Math.min(del, ins, sub);
            if (i > 1 && j > 1 && a.charAt(i - 1) === b.charAt(j - 2) && a.charAt(i - 2) === b.charAt(j - 1)) {
                best = Math.min(best, dp[i - 2][j - 2] + 0.75);
            }
            dp[i][j] = best;
        }
    }
    return dp[m][n];
}
function fuzzyScore(label, prefix) {
    if (!prefix)
        return 0;
    if (label.startsWith(prefix))
        return 5;
    const d = weightedEditDistance(prefix, label.slice(0, Math.max(prefix.length + 4, Math.min(label.length, 32))));
    const norm = d / Math.max(1, prefix.length);
    return Math.max(0, 3.5 - norm * 2.4);
}
function languageWeight(document, label) {
    const lang = document.languageId.toLowerCase();
    if (lang === "markdown" || lang === "md") {
        if (/(readme|doc|guide|example|section|title)/.test(label))
            return 1.25;
        if (/(fn|class|struct|impl|interface)/.test(label))
            return -0.6;
        return 0;
    }
    if (lang === "typescript" || lang === "javascript") {
        if (/(promise|await|then|catch|map|filter|reduce|interface|type)/.test(label))
            return 1.2;
        return 0;
    }
    if (lang === "vitte" || lang === "vit") {
        if (/(gives|module|package|use|pub|impl|match)/.test(label))
            return 1.25;
        return 0;
    }
    return 0;
}
function computeMatchHint(label, prefix) {
    if (!prefix)
        return undefined;
    if (label.startsWith(prefix))
        return `^${prefix}`;
    if (label.includes(prefix))
        return `*${prefix}`;
    let pi = 0;
    const hits = [];
    for (let i = 0; i < label.length && pi < prefix.length; i += 1) {
        if (label[i] === prefix[pi]) {
            hits.push(String(i));
            pi += 1;
        }
    }
    if (pi === prefix.length && hits.length > 0) {
        return `fuzzy@${hits.slice(0, 6).join(",")}`;
    }
    return undefined;
}
function clampRatio(v, fallback) {
    if (typeof v !== "number" || !Number.isFinite(v))
        return fallback;
    return Math.max(0, Math.min(1, v));
}
function clampWeight(v, fallback) {
    if (typeof v !== "number" || !Number.isFinite(v))
        return fallback;
    return Math.max(0, Math.min(4, v));
}
function bucketForItem(item) {
    switch (item.kind) {
        case vscode.CompletionItemKind.Function:
        case vscode.CompletionItemKind.Method:
        case vscode.CompletionItemKind.Constructor:
            return "function";
        case vscode.CompletionItemKind.Variable:
        case vscode.CompletionItemKind.Field:
        case vscode.CompletionItemKind.Property:
        case vscode.CompletionItemKind.Constant:
            return "variable";
        case vscode.CompletionItemKind.Snippet:
            return "snippet";
        default:
            return "other";
    }
}
function applyTypeBudget(items, budget, tuning) {
    if (!tuning.typeBudgetEnabled)
        return items.slice(0, budget);
    const fnRatio = clampRatio(tuning.functionBudgetRatio, 0.45);
    const varRatio = clampRatio(tuning.variableBudgetRatio, 0.35);
    const snRatio = clampRatio(tuning.snippetBudgetRatio, 0.15);
    let fnCap = Math.floor(budget * fnRatio);
    let varCap = Math.floor(budget * varRatio);
    let snCap = Math.floor(budget * snRatio);
    while (fnCap + varCap + snCap > budget) {
        if (fnCap >= varCap && fnCap >= snCap && fnCap > 0)
            fnCap -= 1;
        else if (varCap >= snCap && varCap > 0)
            varCap -= 1;
        else if (snCap > 0)
            snCap -= 1;
        else
            break;
    }
    const out = [];
    let fnCount = 0;
    let varCount = 0;
    let snCount = 0;
    for (const item of items) {
        if (out.length >= budget)
            break;
        const b = bucketForItem(item);
        if (b === "function") {
            if (fnCount >= fnCap)
                continue;
            fnCount += 1;
            out.push(item);
            continue;
        }
        if (b === "variable") {
            if (varCount >= varCap)
                continue;
            varCount += 1;
            out.push(item);
            continue;
        }
        if (b === "snippet") {
            if (snCount >= snCap)
                continue;
            snCount += 1;
            out.push(item);
            continue;
        }
        out.push(item);
    }
    if (out.length < budget) {
        for (const item of items) {
            if (out.length >= budget)
                break;
            if (out.includes(item))
                continue;
            out.push(item);
        }
    }
    return out.slice(0, budget);
}
function recordSuggestionUsage(item) {
    const key = normalizeLabel(item.label);
    usageScore.set(key, (usageScore.get(key) ?? 0) + 1);
}
function recordSuggestionRejections(items) {
    const now = Date.now();
    for (const item of items) {
        const key = normalizeLabel(item.label);
        const prev = rejectionScore.get(key);
        const prevVal = decayRejected(prev);
        rejectionScore.set(key, { count: Math.min(10, prevVal + 1), lastAt: now });
    }
}
function updateSuggestionChurnForRequest(requestKey, items) {
    const labels = items
        .map((it) => normalizeLabel(it.label))
        .filter((v, i, arr) => v.length > 0 && arr.indexOf(v) === i);
    const prev = churnPrevOrderByRequest.get(requestKey) ?? [];
    if (prev.length > 0 && labels.length > 0) {
        const prevIdx = new Map();
        for (let i = 0; i < prev.length; i += 1)
            prevIdx.set(prev[i], i);
        for (let i = 0; i < labels.length; i += 1) {
            const label = labels[i];
            const old = prevIdx.get(label);
            if (old === undefined)
                continue;
            const move = Math.abs(old - i);
            if (move <= 1)
                continue;
            const delta = Math.min(2.5, move * 0.12);
            const prevPenalty = churnPenaltyScore.get(label) ?? 0;
            churnPenaltyScore.set(label, Math.min(6, prevPenalty * 0.8 + delta));
        }
    }
    churnPrevOrderByRequest.set(requestKey, labels.slice(0, 200));
}
function resetSuggestionLearningState() {
    usageScore.clear();
    rejectionScore.clear();
    churnPenaltyScore.clear();
    churnPrevOrderByRequest.clear();
}
function getSuggestionLearningSnapshot() {
    return {
        usageEntries: usageScore.size,
        rejectionEntries: rejectionScore.size,
        churnPenaltyEntries: churnPenaltyScore.size,
        churnRequestEntries: churnPrevOrderByRequest.size,
    };
}
function computeDynamicBudget(tuning, latencyWindowMs) {
    let budget = tuning.topN;
    if (!tuning.dynamicBudget)
        return Math.max(tuning.minN, Math.min(tuning.maxN, budget));
    const p95 = percentile(latencyWindowMs, 0.95);
    const load = os.loadavg()[0] ?? 0;
    if (p95 >= tuning.slowMs * 1.6 || load >= 8)
        budget = Math.max(tuning.minN, Math.floor(tuning.topN * 0.45));
    else if (p95 >= tuning.slowMs || load >= 4)
        budget = Math.max(tuning.minN, Math.floor(tuning.topN * 0.7));
    return Math.max(tuning.minN, Math.min(tuning.maxN, budget));
}
function buildLocalCompletionBatch(document, position, maxItems = 40) {
    const prefix = getWordPrefix(document, position).toLowerCase();
    const imports = extractImports(document);
    const words = collectDocumentWords(document);
    const candidates = new Set();
    for (const w of words) {
        const lw = w.toLowerCase();
        if (!prefix || lw.startsWith(prefix) || lw.includes(prefix))
            candidates.add(w);
    }
    for (const imp of imports) {
        const tail = imp.split("/").pop() ?? imp;
        if (!prefix || tail.toLowerCase().includes(prefix))
            candidates.add(tail);
    }
    const sorted = [...candidates].sort((a, b) => a.localeCompare(b)).slice(0, maxItems);
    return sorted.map((label) => {
        const item = new vscode.CompletionItem(label, vscode.CompletionItemKind.Text);
        item.detail = "[source:local] local-stream";
        return item;
    });
}
function buildPrefixOnlyLocalBatch(document, position, maxItems = 16) {
    const prefix = getWordPrefix(document, position).toLowerCase();
    if (!prefix)
        return [];
    const words = collectDocumentWords(document);
    const candidates = words
        .filter((w) => w.toLowerCase().startsWith(prefix))
        .slice(0, maxItems);
    return candidates.map((label) => {
        const item = new vscode.CompletionItem(label, vscode.CompletionItemKind.Text);
        item.detail = "[source:local] local-prefix-fast";
        return item;
    });
}
function rankAndTrimCompletionItems(items, document, position, tuning, latencyWindowMs, budgetOverride, stickyTopLabels) {
    if (items.length === 0)
        return items;
    const deduped = [];
    const seen = new Set();
    for (const item of items) {
        const key = canonicalSuggestionKey(item);
        if (seen.has(key))
            continue;
        seen.add(key);
        deduped.push(item);
    }
    const budget = Math.max(1, Math.floor(budgetOverride ?? computeDynamicBudget(tuning, latencyWindowMs)));
    if (!tuning.adaptiveRanking)
        return applyTypeBudget(deduped, budget, tuning);
    const prefix = getWordPrefix(document, position).toLowerCase();
    const imports = extractImports(document);
    const fileStem = path.basename(document.fileName, path.extname(document.fileName)).toLowerCase();
    const intent = resolveIntentMode(tuning.intentMode, document);
    const scoped = scopeSymbols(document, position);
    const startedAt = Date.now();
    const deadlineMs = Math.max(1, Math.floor(tuning.rankingDeadlineMs ?? 25));
    const variant = tuning.rankingVariant ?? "default";
    const wPrefixStart = variant === "prefix_heavy" ? 5.4 : (variant === "scope_heavy" ? 3.2 : 4.0);
    const wPrefixContains = variant === "prefix_heavy" ? 2.8 : (variant === "scope_heavy" ? 1.6 : 2.0);
    const wScope = variant === "scope_heavy" ? 4.6 : (variant === "prefix_heavy" ? 2.3 : 3.2);
    const wFuzzy = variant === "prefix_heavy" ? 1.2 : (variant === "scope_heavy" ? 0.7 : 1.0);
    const wUsage = variant === "scope_heavy" ? 0.9 : 1.0;
    const ranked = [];
    for (const item of deduped) {
        if ((Date.now() - startedAt) > deadlineMs) {
            // Deadline guard: keep remaining candidates with neutral score to bound CPU cost.
            ranked.push({ item, score: 0, why: ["deadline-fallback"] });
            continue;
        }
        const label = normalizeLabel(item.label);
        const why = [];
        let score = 0;
        const matchHint = computeMatchHint(label, prefix);
        if (prefix.length > 0) {
            if (label.startsWith(prefix)) {
                score += wPrefixStart;
                why.push("prefix");
            }
            else if (label.includes(prefix)) {
                score += wPrefixContains;
                why.push("contains-prefix");
            }
            const fz = fuzzyScore(label, prefix);
            score += fz * wFuzzy;
            if (fz > 0)
                why.push("fuzzy");
        }
        if (fileStem.length > 0 && label.includes(fileStem)) {
            score += 1.5;
            why.push("same-file-context");
        }
        if (imports.some((imp) => label.includes(imp.split("/").pop() ?? imp))) {
            score += 1.25;
            why.push("matches-import");
        }
        if (scoped.has(label)) {
            score += wScope;
            why.push("same-scope");
        }
        const usage = usageScore.get(label) ?? 0;
        if (usage > 0) {
            score += Math.min(3, usage * 0.2) * wUsage;
            why.push("recently-used");
        }
        if (stickyTopLabels?.has(label)) {
            score += 2.8;
            why.push("sticky-top");
        }
        const rejected = decayRejected(rejectionScore.get(label));
        if (rejected > 0) {
            score -= Math.min(4.5, rejected * 0.9);
            why.push("recently-rejected");
        }
        if (tuning.churnPenaltyEnabled) {
            const churnWeight = clampWeight(tuning.churnPenaltyWeight, 1.0);
            const churn = churnPenaltyScore.get(label) ?? 0;
            if (churn > 0) {
                score -= Math.min(4.5, churn * churnWeight);
                why.push("churn-penalty");
            }
        }
        if (item.kind === vscode.CompletionItemKind.Snippet) {
            score -= 0.5;
        }
        if (intent === "test") {
            if (/(test|assert|expect|mock|stub|fixture)/.test(label))
                score += 1.5;
        }
        else if (intent === "refactor") {
            if (/(rename|extract|move|refactor|import|export)/.test(label))
                score += 1.5;
        }
        else if (intent === "docs") {
            if (/(doc|comment|readme|example|guide)/.test(label))
                score += 1.5;
        }
        else if (intent === "terminal") {
            if (/(run|build|test|exec|bash|shell|cmd)/.test(label))
                score += 1.5;
        }
        score += languageWeight(document, label);
        if (tuning.showExplainLabels && why.length > 0) {
            const whyText = `why: ${why.join(", ")} | intent:${intent}`;
            const docText = typeof item.documentation === "string"
                ? item.documentation
                : item.documentation instanceof vscode.MarkdownString
                    ? item.documentation.value
                    : "";
            const note = `\n\n${whyText}\nReasoning: ${why.join(", ")}.`;
            item.documentation = new vscode.MarkdownString((docText || "").trim() + note);
        }
        if (tuning.showMatchHighlights && matchHint) {
            const details = Array.isArray(item.detail) ? item.detail.join(" ") : (item.detail ?? "");
            item.detail = details ? `${details} · match:${matchHint}` : `match:${matchHint}`;
        }
        ranked.push({ item, score, why });
    }
    ranked.sort((a, b) => b.score - a.score || normalizeLabel(a.item.label).localeCompare(normalizeLabel(b.item.label)));
    const ordered = ranked.map((r) => r.item);
    return applyTypeBudget(ordered, budget, tuning);
}
//# sourceMappingURL=suggestions.js.map