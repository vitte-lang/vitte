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
exports.registerDiagnosticsView = registerDiagnosticsView;
const path = __importStar(require("node:path"));
const vscode = __importStar(require("vscode"));
const VALID_SEVERITY_MAP = {
    error: true,
    warning: true,
    information: true,
    hint: true,
};
const DEFAULT_CFG = {
    severities: [],
    refreshDebounceMs: 150,
};
function isSeverityKey(value) {
    return typeof value === 'string' && value in VALID_SEVERITY_MAP;
}
function readConfig() {
    const cfg = vscode.workspace.getConfiguration('vitte').get('diagnostics') ?? {};
    const severities = Array.isArray(cfg.severities)
        ? cfg.severities.filter(isSeverityKey)
        : [];
    const refreshDebounceMs = typeof cfg.refreshDebounceMs === 'number'
        ? Math.max(0, cfg.refreshDebounceMs)
        : DEFAULT_CFG.refreshDebounceMs;
    return {
        severities,
        refreshDebounceMs,
    };
}
function sevToName(s) {
    switch (s) {
        case vscode.DiagnosticSeverity.Error: return 'error';
        case vscode.DiagnosticSeverity.Warning: return 'warning';
        case vscode.DiagnosticSeverity.Information: return 'information';
        case vscode.DiagnosticSeverity.Hint: return 'hint';
        default: return 'unknown';
    }
}
function matchesFilter(s, allowed) {
    if (allowed.length === 0)
        return true;
    const name = sevToName(s);
    return name !== 'unknown' && allowed.includes(name);
}
function countBySeverity(list) {
    let e = 0, w = 0, i = 0, h = 0;
    for (const d of list) {
        switch (d.diagnostic.severity) {
            case vscode.DiagnosticSeverity.Error:
                e++;
                break;
            case vscode.DiagnosticSeverity.Warning:
                w++;
                break;
            case vscode.DiagnosticSeverity.Information:
                i++;
                break;
            case vscode.DiagnosticSeverity.Hint:
                h++;
                break;
        }
    }
    return { e, w, i, h };
}
function debounce(fn, ms) {
    let tid;
    const debounced = function (...args) {
        if (tid)
            clearTimeout(tid);
        tid = setTimeout(() => fn.apply(this, args), ms);
    };
    return debounced;
}
const SUPPORTED_EXTS = new Set([".vitte", ".vit"]);
class FileNode extends vscode.TreeItem {
    constructor(uri, entries, collState) {
        super(relativeLabel(uri), collState);
        this.uri = uri;
        this.entries = entries;
        const c = countBySeverity(entries);
        const parts = [];
        if (c.e)
            parts.push(`${c.e} erreur${c.e > 1 ? 's' : ''}`);
        if (c.w)
            parts.push(`${c.w} avertissement${c.w > 1 ? 's' : ''}`);
        if (c.i)
            parts.push(`${c.i} info${c.i > 1 ? 's' : ''}`);
        if (c.h)
            parts.push(`${c.h} astuce${c.h > 1 ? 's' : ''}`);
        this.description = parts.length ? parts.join(', ') : `${entries.length} ${entries.length > 1 ? 'problèmes' : 'problème'}`;
        this.contextValue = "vitteDiagnosticFile";
        this.iconPath = vscode.ThemeIcon.File;
    }
    get children() {
        return this.entries.map(entry => new DiagnosticNode(entry));
    }
}
class DiagnosticNode extends vscode.TreeItem {
    constructor(entry) {
        super(entry.diagnostic.message, vscode.TreeItemCollapsibleState.None);
        this.entry = entry;
        const pos = entry.diagnostic.range.start;
        this.description = `L${pos.line + 1}:C${pos.character + 1}`;
        const parts = [
            entry.diagnostic.message,
            `${entry.uri.fsPath}:${pos.line + 1}:${pos.character + 1}`,
            entry.diagnostic.source ? `Source: ${entry.diagnostic.source}` : ""
        ].filter(Boolean);
        const severityName = sevToName(entry.diagnostic.severity);
        const codeValue = entry.diagnostic.code;
        const codeText = typeof codeValue === 'string' || typeof codeValue === 'number'
            ? `Code: ${String(codeValue)}`
            : undefined;
        const extra = [severityName && `Niveau: ${severityName}`, codeText].filter(Boolean).join('\n');
        this.tooltip = [parts.join('\n'), extra].filter(Boolean).join('\n');
        this.iconPath = iconForSeverity(entry.diagnostic.severity);
        this.command = {
            command: "vitte.diagnostics.open",
            title: "Ouvrir le diagnostic",
            arguments: [entry]
        };
        this.contextValue = "vitteDiagnostic";
    }
}
class DiagnosticsTreeDataProvider {
    constructor() {
        this.onDidChangeEmitter = new vscode.EventEmitter();
        this.onDidChangeTreeData = this.onDidChangeEmitter.event;
        this.nodes = [];
        this.cfg = DEFAULT_CFG;
        this.schedule = debounce(() => this.refresh(), this.cfg.refreshDebounceMs);
    }
    refresh() {
        this.cfg = readConfig();
        // rebuild nodes from diagnostics with filtering
        this.nodes = buildFileNodes(this.cfg);
        this.onDidChangeEmitter.fire(undefined);
    }
    refreshDebounced() {
        // re-create debouncer if delay changed
        this.schedule = debounce(() => this.refresh(), this.cfg.refreshDebounceMs);
        this.schedule();
    }
    hasItems() {
        return this.nodes.length > 0;
    }
    getTreeItem(element) {
        return element;
    }
    getChildren(element) {
        if (!element)
            return this.nodes;
        if (element instanceof FileNode)
            return element.children;
        return [];
    }
    dispose() {
        this.onDidChangeEmitter.dispose();
    }
}
function registerDiagnosticsView(context) {
    const provider = new DiagnosticsTreeDataProvider();
    const tree = vscode.window.createTreeView("vitteDiagnostics", { treeDataProvider: provider });
    const refresh = () => {
        provider.refresh();
        if (provider.hasItems()) {
            tree.message = '';
        }
        else {
            tree.message = '$(pass-filled) Aucun diagnostic Vitte détecté';
        }
    };
    const refreshDebounced = () => {
        provider.refreshDebounced();
        if (!provider.hasItems())
            tree.message = '$(pass-filled) Aucun diagnostic Vitte détecté';
    };
    refresh();
    context.subscriptions.push(tree, provider, vscode.languages.onDidChangeDiagnostics(refreshDebounced), vscode.workspace.onDidCloseTextDocument(refreshDebounced), vscode.workspace.onDidOpenTextDocument(refreshDebounced), vscode.workspace.onDidSaveTextDocument(refreshDebounced), vscode.commands.registerCommand("vitte.diagnostics.refresh", refresh), vscode.commands.registerCommand("vitte.diagnostics.open", async (entry) => {
        if (!entry?.uri)
            return;
        const doc = await vscode.workspace.openTextDocument(entry.uri);
        const editor = await vscode.window.showTextDocument(doc, { preserveFocus: false, preview: true });
        const range = entry.diagnostic.range;
        editor.revealRange(range, vscode.TextEditorRevealType.InCenter);
        editor.selection = new vscode.Selection(range.start, range.start);
    }), vscode.commands.registerCommand('vitte.diagnostics.copy', async (entry) => {
        if (!entry)
            return;
        const pos = entry.diagnostic.range.start;
        const text = `${entry.uri.fsPath}:${pos.line + 1}:${pos.character + 1} — ${entry.diagnostic.message}`;
        await vscode.env.clipboard.writeText(text);
        void vscode.window.setStatusBarMessage('Diagnostic copié dans le presse-papiers', 2000);
    }));
    vscode.window.onDidChangeActiveTextEditor(refresh, undefined, context.subscriptions);
    return { provider, tree, refresh };
}
function buildFileNodes(cfg) {
    const entries = collectDiagnostics(cfg);
    const byFile = new Map();
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
        return new FileNode(head.uri, list, vscode.TreeItemCollapsibleState.Expanded);
    });
}
function collectDiagnostics(cfg) {
    const all = vscode.languages.getDiagnostics();
    const collected = [];
    for (const [uri, diagnostics] of all) {
        if (uri.scheme !== 'file')
            continue;
        if (!SUPPORTED_EXTS.has(path.extname(uri.fsPath)))
            continue;
        diagnostics.forEach((diagnostic, index) => {
            if (!matchesFilter(diagnostic.severity, cfg.severities))
                return;
            collected.push({ uri, diagnostic, index });
        });
    }
    return collected;
}
function compareDiagnostics(a, b) {
    const severityDiff = (a.diagnostic.severity ?? vscode.DiagnosticSeverity.Information) -
        (b.diagnostic.severity ?? vscode.DiagnosticSeverity.Information);
    if (severityDiff !== 0)
        return severityDiff;
    const lineDiff = a.diagnostic.range.start.line - b.diagnostic.range.start.line;
    if (lineDiff !== 0)
        return lineDiff;
    return a.diagnostic.range.start.character - b.diagnostic.range.start.character;
}
function compareFiles(a, b) {
    const uriA = a[0]?.uri;
    const uriB = b[0]?.uri;
    return relativeLabel(uriA).localeCompare(relativeLabel(uriB));
}
function relativeLabel(uri) {
    if (!uri)
        return "";
    const rel = vscode.workspace.asRelativePath(uri, false);
    return rel || uri.fsPath;
}
function iconForSeverity(severity) {
    const makeIcon = (id) => ({ id });
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
//# sourceMappingURL=diagnosticsView.js.map