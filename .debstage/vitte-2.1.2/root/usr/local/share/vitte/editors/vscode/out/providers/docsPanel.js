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
exports.DocsPanel = void 0;
exports.registerDocsPanel = registerDocsPanel;
const vscode = __importStar(require("vscode"));
const path = __importStar(require("path"));
/**
 * Vitte Docs Panel — searchable documentation webview
 * - Scans workspace for Markdown docs, with configurable include/exclude globs
 * - Debounced search, simple fuzzy scoring, highlighting, pagination
 * - Safe CSP and tidy lifecycle
 */
function registerDocsPanel(ctx) {
    ctx.subscriptions.push(vscode.commands.registerCommand('vitte.openDocs', () => DocsPanel.createOrShow(ctx)));
}
class DocsPanel {
    static createOrShow(ctx) {
        const column = vscode.window.activeTextEditor?.viewColumn ?? vscode.ViewColumn.Active ?? vscode.ViewColumn.One;
        if (DocsPanel.current) {
            DocsPanel.current.panel.reveal(column);
            return;
        }
        const panel = vscode.window.createWebviewPanel('vitteDocs', 'Vitte • Docs', column, {
            enableScripts: true,
            retainContextWhenHidden: true,
            enableFindWidget: true,
        });
        DocsPanel.current = new DocsPanel(ctx, panel);
    }
    constructor(ctx, panel) {
        this.ctx = ctx;
        this.disposables = [];
        this.index = null;
        // Pagination state (in webview we also manage, but keep a server-side cap)
        this.maxIndex = 2000;
        this.panel = panel;
        this.settings = this.loadSearchConfig();
        this.panel.onDidDispose(() => this.dispose(), null, this.disposables);
        this.panel.webview.onDidReceiveMessage((m) => this.onMessage(m), null, this.disposables);
        void this.render();
    }
    dispose() {
        DocsPanel.current = undefined;
        while (this.disposables.length)
            this.disposables.pop()?.dispose();
    }
    // ---- Config ----
    loadSearchConfig() {
        const cfg = vscode.workspace.getConfiguration('vitte');
        const include = cfg.get('docs.include') ?? '**/*.md';
        const exclude = cfg.get('docs.exclude') ?? '**/{node_modules,.git,dist,out,target,build}/**';
        const indexLimit = cfg.get('docs.indexLimit');
        const limit = Math.max(100, Math.min(10000, indexLimit ?? 2000));
        return { include, exclude, limit };
    }
    // ---- Messaging ----
    async onMessage(msg) {
        if (!msg || typeof msg !== 'object')
            return;
        const m = msg;
        switch (m?.type) {
            case 'search': {
                await this.ensureIndex();
                const q = String(m.q ?? '');
                const page = Number.isInteger(m.page) ? Number(m.page) : 1;
                const pageSize = Number.isInteger(m.pageSize) ? Number(m.pageSize) : 100;
                const { items, total } = this.filterHits(q, page, pageSize);
                this.post({ type: 'results', items, total, page, pageSize, query: q });
                return;
            }
            case 'openLink': {
                if (typeof m.href === 'string')
                    void vscode.env.openExternal(vscode.Uri.parse(m.href));
                return;
            }
            case 'openFile': {
                if (typeof m.fsPath === 'string') {
                    const uri = vscode.Uri.file(m.fsPath);
                    void vscode.commands.executeCommand('vscode.open', uri);
                }
                return;
            }
            default:
                return;
        }
    }
    post(payload) {
        void this.panel.webview.postMessage(payload);
    }
    // ---- Render ----
    async render() {
        const webview = this.panel.webview;
        const nonce = String(Date.now());
        const mediaFsPath = path.join(this.ctx.extensionUri.fsPath, 'media');
        const htmlPath = vscode.Uri.file(path.join(mediaFsPath, 'docs.html'));
        const cssPath = vscode.Uri.file(path.join(mediaFsPath, 'docs.css'));
        const jsPath = vscode.Uri.file(path.join(mediaFsPath, 'docs.js'));
        const htmlBytes = await vscode.workspace.fs.readFile(htmlPath);
        let html = Buffer.from(htmlBytes).toString('utf8');
        const csp = `default-src 'none'; style-src ${webview.cspSource}; script-src 'nonce-${nonce}' ${webview.cspSource};`;
        const styleUri = webview.asWebviewUri(cssPath);
        const scriptUri = webview.asWebviewUri(jsPath);
        html = html
            .replace('{{csp}}', csp)
            .replace('{{nonce}}', nonce)
            .replace('{{styleUri}}', styleUri.toString())
            .replace('{{scriptUri}}', scriptUri.toString());
        this.panel.webview.html = html;
    }
    // ---- Indexing ----
    async ensureIndex() {
        if (this.index)
            return;
        const hits = [];
        try {
            const { include, exclude, limit } = this.settings;
            const md = await vscode.workspace.findFiles(include, exclude, Math.min(limit, this.maxIndex));
            for (const uri of md) {
                const title = uri.path.split('/').pop() ?? uri.path;
                hits.push({ uri, title, path: uri.fsPath });
            }
            this.index = hits;
        }
        catch (err) {
            this.index = [];
            const reason = err instanceof Error ? err.message : 'erreur inconnue';
            void vscode.window.showWarningMessage(`Vitte Docs: indexation incomplète (${reason})`);
        }
    }
    filterHits(query, page, pageSize) {
        if (!this.index)
            return { items: [], total: 0 };
        const q = query.trim().toLowerCase();
        let items;
        if (!q) {
            items = this.index.slice(0, this.maxIndex);
        }
        else {
            const scored = this.index
                .map(h => ({ h, score: score(h.title, q) + score(h.path, q) * 0.5 }))
                .filter(x => x.score > 0)
                .sort((a, b) => b.score - a.score)
                .slice(0, this.maxIndex)
                .map(x => x.h);
            items = scored;
        }
        const total = items.length;
        const start = Math.max(0, (page - 1) * pageSize);
        const end = Math.min(total, start + pageSize);
        const slice = items.slice(start, end).map(x => ({ title: x.title, path: x.path, fsPath: x.uri.fsPath }));
        return { items: slice, total };
    }
}
exports.DocsPanel = DocsPanel;
// Simple scoring: substring rank + token match
function score(text, q) {
    const t = text.toLowerCase();
    if (t.includes(q))
        return 10 + Math.max(0, 20 - t.indexOf(q));
    let s = 0;
    const parts = q.split(/\s+/g).filter(Boolean);
    for (const part of parts)
        if (t.includes(part))
            s += 2;
    return s;
}
exports.default = DocsPanel;
//# sourceMappingURL=docsPanel.js.map