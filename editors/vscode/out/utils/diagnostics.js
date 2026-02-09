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
exports.VITTE_FILE_EXTS = void 0;
exports.summarizeWorkspaceDiagnostics = summarizeWorkspaceDiagnostics;
exports.summarizeDiagnosticsForUris = summarizeDiagnosticsForUris;
exports.diagnosticsLevel = diagnosticsLevel;
exports.formatDiagnosticsSummary = formatDiagnosticsSummary;
exports.summarizeDiagnosticsPerFile = summarizeDiagnosticsPerFile;
exports.buildWorkspaceDiagnosticsReport = buildWorkspaceDiagnosticsReport;
exports.formatDiagnosticsBadge = formatDiagnosticsBadge;
exports.fileSummaryComparator = fileSummaryComparator;
exports.summarizeDiagnosticsByDirectory = summarizeDiagnosticsByDirectory;
exports.openFirstDiagnostic = openFirstDiagnostic;
exports.toJSONReport = toJSONReport;
exports.toCSVReport = toCSVReport;
exports.formatDiagnosticsMarkdownTable = formatDiagnosticsMarkdownTable;
const path = __importStar(require("node:path"));
const vscode = __importStar(require("vscode"));
exports.VITTE_FILE_EXTS = new Set([".vitte", ".vit"]);
const VALID_SEVERITIES = ['error', 'warning', 'information', 'hint'];
function isSeverityKey(value) {
    return typeof value === 'string' && VALID_SEVERITIES.includes(value);
}
function getConfig() {
    const cfg = vscode.workspace.getConfiguration('vitte').get('diagnostics') ?? {};
    const severities = Array.isArray(cfg.severities) ? cfg.severities.filter(isSeverityKey) : [];
    const order = cfg.order === 'warnings' || cfg.order === 'total' || cfg.order === 'path' ? cfg.order : 'errors';
    const maxPerFile = typeof cfg.maxPerFile === 'number' && cfg.maxPerFile >= 0 ? cfg.maxPerFile : 0;
    return {
        severities,
        order,
        maxPerFile,
    };
}
function summarizeWorkspaceDiagnostics() {
    return summarizeDiagnosticsInternal((uri) => isVitteFile(uri));
}
function summarizeDiagnosticsForUris(uris) {
    const wanted = new Set();
    for (const entry of uris) {
        if (!entry)
            continue;
        if (typeof entry === "string") {
            wanted.add(entry);
        }
        else {
            wanted.add(entry.toString());
        }
    }
    if (wanted.size === 0) {
        return summarizeDiagnosticsInternal(() => false);
    }
    return summarizeDiagnosticsInternal((uri) => wanted.has(uri.toString()));
}
function diagnosticsLevel(summary) {
    if (summary.errors > 0)
        return "error";
    if (summary.warnings > 0)
        return "warning";
    return "clean";
}
function formatDiagnosticsSummary(summary) {
    const parts = [];
    const fileCount = vscode.languages.getDiagnostics().filter(([uri]) => isVitteFile(uri)).length;
    if (summary.errors > 0) {
        parts.push(formatCount(summary.errors, "erreur"));
    }
    if (summary.warnings > 0) {
        parts.push(formatCount(summary.warnings, "avertissement"));
    }
    if (summary.info > 0) {
        parts.push(formatCount(summary.info, "info"));
    }
    if (summary.hints > 0) {
        parts.push(formatCount(summary.hints, "suggestion"));
    }
    if (parts.length === 0) {
        return "Diagnostics : aucun problème détecté";
    }
    const header = fileCount > 0 ? `${fileCount} fichier${fileCount > 1 ? 's' : ''} — ` : '';
    return `${header}Diagnostics : ${parts.join(', ')}`;
}
/**
 * Returns a per-file breakdown of diagnostics (filtered to Vitte files and by configured severities).
 */
function summarizeDiagnosticsPerFile() {
    const cfg = getConfig();
    const allowed = new Set(cfg.severities);
    const map = new Map();
    for (const [uri, diagnostics] of vscode.languages.getDiagnostics()) {
        if (!isVitteFile(uri))
            continue;
        for (const d of diagnostics) {
            // Severity filter
            if (allowed.size) {
                const ok = ((d.severity === vscode.DiagnosticSeverity.Error && allowed.has('error')) ||
                    (d.severity === vscode.DiagnosticSeverity.Warning && allowed.has('warning')) ||
                    (d.severity === vscode.DiagnosticSeverity.Information && allowed.has('information')) ||
                    (d.severity === vscode.DiagnosticSeverity.Hint && allowed.has('hint')));
                if (!ok)
                    continue;
            }
            const key = uri.toString();
            let rec = map.get(key);
            if (!rec) {
                rec = { uri, filePath: uri.fsPath, errors: 0, warnings: 0, info: 0, hints: 0, total: 0 };
                map.set(key, rec);
            }
            switch (d.severity) {
                case vscode.DiagnosticSeverity.Error:
                    rec.errors++;
                    break;
                case vscode.DiagnosticSeverity.Warning:
                    rec.warnings++;
                    break;
                case vscode.DiagnosticSeverity.Information:
                    rec.info++;
                    break;
                case vscode.DiagnosticSeverity.Hint:
                    rec.hints++;
                    break;
                default:
                    rec.info++;
                    break;
            }
            rec.total++;
        }
    }
    const list = Array.from(map.values());
    list.sort(fileSummaryComparator(cfg.order));
    if (cfg.maxPerFile > 0 && list.length > cfg.maxPerFile) {
        return list.slice(0, cfg.maxPerFile);
    }
    return list;
}
/**
 * Build a workspace report with both totals and per-file stats.
 */
function buildWorkspaceDiagnosticsReport() {
    const perFile = summarizeDiagnosticsPerFile();
    const total = perFile.reduce((acc, f) => {
        acc.errors += f.errors;
        acc.warnings += f.warnings;
        acc.info += f.info;
        acc.hints += f.hints;
        return acc;
    }, { errors: 0, warnings: 0, info: 0, hints: 0 });
    return { total, perFile };
}
/**
 * Render a concise status text (e.g., for status bar): "⛔ 2 • ⚠ 5 • ℹ 1 • ✳ 0".
 */
function formatDiagnosticsBadge(summary) {
    const parts = [];
    parts.push(`⛔ ${summary.errors}`);
    parts.push(`⚠ ${summary.warnings}`);
    parts.push(`ℹ ${summary.info}`);
    parts.push(`✳ ${summary.hints}`);
    return parts.join(' • ');
}
/** Choose a comparator for FileDiagnosticsSummary based on config.order */
function fileSummaryComparator(order) {
    switch (order) {
        case 'warnings':
            return (a, b) => b.warnings - a.warnings || b.errors - a.errors || b.total - a.total || a.filePath.localeCompare(b.filePath);
        case 'total':
            return (a, b) => b.total - a.total || b.errors - a.errors || b.warnings - a.warnings || a.filePath.localeCompare(b.filePath);
        case 'path':
            return (a, b) => a.filePath.localeCompare(b.filePath);
        case 'errors':
        default:
            return (a, b) => b.errors - a.errors || b.warnings - a.warnings || b.total - a.total || a.filePath.localeCompare(b.filePath);
    }
}
function summarizeDiagnosticsByDirectory() {
    const cfg = getConfig();
    const allowed = new Set(cfg.severities);
    const folders = vscode.workspace.workspaceFolders ?? [];
    const map = new Map();
    const rel = (uri) => {
        for (const f of folders) {
            const root = f.uri.fsPath;
            if (uri.fsPath.startsWith(root)) {
                const p = path.relative(root, uri.fsPath);
                const d = path.dirname(p);
                return d === '.' ? '' : (d.split(path.sep)[0] ?? '');
            }
        }
        return '';
    };
    for (const [uri, diagnostics] of vscode.languages.getDiagnostics()) {
        if (!isVitteFile(uri))
            continue;
        const top = rel(uri);
        let rec = map.get(top);
        if (!rec) {
            rec = { dir: top, errors: 0, warnings: 0, info: 0, hints: 0, files: 0, _set: new Set() };
            map.set(top, rec);
        }
        for (const d of diagnostics) {
            if (allowed.size > 0) {
                const ok = ((d.severity === vscode.DiagnosticSeverity.Error && allowed.has('error')) ||
                    (d.severity === vscode.DiagnosticSeverity.Warning && allowed.has('warning')) ||
                    (d.severity === vscode.DiagnosticSeverity.Information && allowed.has('information')) ||
                    (d.severity === vscode.DiagnosticSeverity.Hint && allowed.has('hint')));
                if (!ok)
                    continue;
            }
            switch (d.severity) {
                case vscode.DiagnosticSeverity.Error:
                    rec.errors++;
                    break;
                case vscode.DiagnosticSeverity.Warning:
                    rec.warnings++;
                    break;
                case vscode.DiagnosticSeverity.Information:
                    rec.info++;
                    break;
                case vscode.DiagnosticSeverity.Hint:
                    rec.hints++;
                    break;
            }
            rec._set.add(uri.toString());
        }
    }
    const out = [];
    for (const r of map.values()) {
        out.push({ dir: r.dir ?? '/', errors: r.errors, warnings: r.warnings, info: r.info, hints: r.hints, files: r._set.size });
    }
    out.sort((a, b) => b.errors - a.errors || b.warnings - a.warnings || b.files - a.files || a.dir.localeCompare(b.dir));
    return out;
}
/** Open first diagnostic of given severity preference (errors→warnings→info→hints). */
async function openFirstDiagnostic(prefer = ['error', 'warning', 'information', 'hint']) {
    let best;
    for (const [uri, list] of vscode.languages.getDiagnostics()) {
        if (!isVitteFile(uri))
            continue;
        const idx = list.findIndex((d) => {
            const sev = severityToName(d.severity);
            return sev !== 'unknown' && prefer.includes(sev);
        });
        if (idx >= 0) {
            best = { uri, index: idx };
            break;
        }
    }
    if (!best)
        return false;
    const list = vscode.languages.getDiagnostics(best.uri);
    const diag = list && list.length > best.index ? list[best.index] : undefined;
    if (!diag)
        return false;
    const doc = await vscode.workspace.openTextDocument(best.uri);
    const editor = await vscode.window.showTextDocument(doc, { preview: true });
    editor.revealRange(diag.range, vscode.TextEditorRevealType.InCenter);
    editor.selection = new vscode.Selection(diag.range.start, diag.range.start);
    return true;
}
function severityToName(sev) {
    switch (sev) {
        case vscode.DiagnosticSeverity.Error: return 'error';
        case vscode.DiagnosticSeverity.Warning: return 'warning';
        case vscode.DiagnosticSeverity.Information: return 'information';
        case vscode.DiagnosticSeverity.Hint: return 'hint';
        default: return 'unknown';
    }
}
/** Export helpers: JSON & CSV */
function toJSONReport(report) {
    return JSON.stringify(report, null, 2);
}
function toCSVReport(report) {
    const rows = [];
    rows.push('file,errors,warnings,info,hints,total');
    for (const f of report.perFile) {
        const esc = (s) => '"' + s.replace(/"/g, '""') + '"';
        rows.push([esc(f.filePath), f.errors, f.warnings, f.info, f.hints, f.total].join(','));
    }
    return rows.join('\n');
}
/**
 * Render a Markdown table for panel/webview from a per-file list.
 */
function formatDiagnosticsMarkdownTable(perFile) {
    if (!perFile.length)
        return '*Aucun diagnostic Vitte.*';
    const header = '| Fichier | Erreurs | Avert. | Infos | Astuces | Total |\n|---|---:|---:|---:|---:|---:|';
    const rows = perFile.map((f) => `| ${escapeMd(path.basename(f.filePath))} | ${f.errors} | ${f.warnings} | ${f.info} | ${f.hints} | ${f.total} |`);
    return [header, ...rows].join('\n');
}
function escapeMd(s) {
    return s.replace(/[|*_`\\]/g, (m) => `\\${m}`);
}
function summarizeDiagnosticsInternal(predicate) {
    const summary = { errors: 0, warnings: 0, info: 0, hints: 0 };
    const cfg = getConfig();
    const allowed = new Set(cfg.severities);
    const filterBySeverity = (sev) => {
        if (!allowed.size)
            return true;
        switch (sev) {
            case vscode.DiagnosticSeverity.Error: return allowed.has('error');
            case vscode.DiagnosticSeverity.Warning: return allowed.has('warning');
            case vscode.DiagnosticSeverity.Information: return allowed.has('information');
            case vscode.DiagnosticSeverity.Hint: return allowed.has('hint');
            default: return true;
        }
    };
    for (const [uri, diagnostics] of vscode.languages.getDiagnostics()) {
        if (!predicate(uri))
            continue;
        for (const diagnostic of diagnostics) {
            if (!filterBySeverity(diagnostic.severity))
                continue;
            switch (diagnostic.severity) {
                case vscode.DiagnosticSeverity.Error:
                    summary.errors++;
                    break;
                case vscode.DiagnosticSeverity.Warning:
                    summary.warnings++;
                    break;
                case vscode.DiagnosticSeverity.Information:
                    summary.info++;
                    break;
                case vscode.DiagnosticSeverity.Hint:
                    summary.hints++;
                    break;
                default:
                    summary.info++;
            }
        }
    }
    return summary;
}
function isVitteFile(uri) {
    if (uri.scheme !== "file")
        return false;
    const ext = path.extname(uri.fsPath).toLowerCase();
    return exports.VITTE_FILE_EXTS.has(ext);
}
function formatCount(count, label) {
    const plural = count > 1 ? "s" : "";
    return `${count} ${label}${plural}`;
}
//# sourceMappingURL=diagnostics.js.map