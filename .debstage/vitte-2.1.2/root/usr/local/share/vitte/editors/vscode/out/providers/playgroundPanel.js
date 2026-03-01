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
exports.PlaygroundPanel = void 0;
exports.registerPlaygroundPanel = registerPlaygroundPanel;
const vscode = __importStar(require("vscode"));
const path = __importStar(require("path"));
const fs = __importStar(require("fs"));
const cp = __importStar(require("child_process"));
const runtimeLocator_1 = require("../debug/runtimeLocator");
/**
 * Vitte Playground Panel
 * - Local, offline: edit a Vitte snippet, format it, run it via vitte-runtime.
 * - Stores temp files under ${workspace}/.vitte/playground/
 */
function registerPlaygroundPanel(ctx) {
    ctx.subscriptions.push(vscode.commands.registerCommand('vitte.openPlayground', () => PlaygroundPanel.createOrShow(ctx)));
}
class PlaygroundPanel {
    static createOrShow(ctx) {
        const column = vscode.window.activeTextEditor?.viewColumn ?? vscode.ViewColumn.Two;
        if (PlaygroundPanel.current) {
            PlaygroundPanel.current.panel.reveal(column);
            return;
        }
        const panel = vscode.window.createWebviewPanel('vittePlayground', 'Vitte • Playground', column, {
            enableScripts: true,
            retainContextWhenHidden: true,
            enableFindWidget: true,
        });
        PlaygroundPanel.current = new PlaygroundPanel(ctx, panel);
    }
    constructor(ctx, panel) {
        this.ctx = ctx;
        this.disposables = [];
        this.panel = panel;
        this.panel.onDidDispose(() => this.dispose(), null, this.disposables);
        this.panel.webview.onDidReceiveMessage((m) => this.onMessage(m), null, this.disposables);
        void this.render();
    }
    dispose() {
        PlaygroundPanel.current = undefined;
        try {
            this.terminal?.dispose();
        }
        catch { }
        while (this.disposables.length)
            this.disposables.pop()?.dispose();
    }
    // ---- Messaging ----
    async onMessage(msg) {
        if (!msg || typeof msg !== 'object')
            return;
        const m = msg;
        switch (m.type) {
            case 'format': {
                const text = typeof m.text === 'string' ? m.text : '';
                const formatted = await this.formatCode(text);
                this.post({ type: 'formatted', text: formatted ?? text });
                return;
            }
            case 'run': {
                const text = typeof m.text === 'string' ? m.text : '';
                const save = Boolean(m.save);
                await this.runSnippet(text, save);
                return;
            }
            case 'save': {
                const text = typeof m.text === 'string' ? m.text : '';
                const file = await this.saveSnippet(text);
                this.post({ type: 'saved', file });
                return;
            }
            case 'openFolder': {
                const folder = await this.ensurePlaygroundDir();
                if (folder)
                    void vscode.commands.executeCommand('revealFileInOS', vscode.Uri.file(folder));
                return;
            }
            default:
                return;
        }
    }
    post(payload) {
        void this.panel.webview.postMessage(payload);
    }
    // ---- Tooling helpers ----
    async getTooling() {
        const located = await (0, runtimeLocator_1.locateVitteRuntime)();
        const runtime = located.runtimePath ?? 'vitte-runtime';
        const out = { runtime };
        if (typeof located.fmtPath === 'string')
            out.fmt = located.fmtPath;
        return out;
    }
    async ensurePlaygroundDir() {
        const wf = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
        if (!wf)
            return undefined;
        const dir = path.join(wf, '.vitte', 'playground');
        try {
            await fs.promises.mkdir(dir, { recursive: true });
        }
        catch { /* ignore */ }
        return dir;
    }
    async saveSnippet(text) {
        const dir = await this.ensurePlaygroundDir();
        if (!dir)
            return undefined;
        const file = path.join(dir, 'main.vitte');
        await fs.promises.writeFile(file, text, 'utf8');
        this.lastFile = file;
        return file;
    }
    async formatCode(text) {
        try {
            const { fmt } = await this.getTooling();
            if (!fmt)
                return undefined;
            return await new Promise((resolve, reject) => {
                const p = cp.spawn(fmt, [], { stdio: ['pipe', 'pipe', 'pipe'] });
                let out = '';
                let err = '';
                p.stdout.on('data', (b) => { out += b.toString(); });
                p.stderr.on('data', (b) => { err += b.toString(); });
                p.on('error', reject);
                p.on('close', (code) => {
                    if (code === 0)
                        resolve(out);
                    else
                        reject(new Error(err || `vitte-fmt exited with ${code}`));
                });
                if (p.stdin) {
                    p.stdin.write(text);
                    p.stdin.end();
                }
                else {
                    reject(new Error('vitte-fmt stdin unavailable'));
                }
            });
        }
        catch (e) {
            const message = e instanceof Error ? e.message : String(e);
            void vscode.window.showWarningMessage(`Format: ${message}`);
            return undefined;
        }
    }
    async runSnippet(text, saveFirst) {
        const file = saveFirst ? (await this.saveSnippet(text)) : (this.lastFile ?? await this.saveSnippet(text));
        if (!file) {
            void vscode.window.showErrorMessage('Playground: impossible d’écrire le fichier.');
            return;
        }
        const { runtime } = await this.getTooling();
        const wf = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ?? path.dirname(file);
        const cmd = `${runtime} run ${quote(file)}`;
        // Ensure terminal
        this.terminal ?? (this.terminal = vscode.window.createTerminal({ name: 'Vitte Playground', cwd: wf }));
        this.terminal.show(true);
        this.terminal.sendText(cmd, true);
    }
    // ---- Render ----
    render() {
        const webview = this.panel.webview;
        const nonce = String(Date.now());
        const csp = `default-src 'none'; style-src 'unsafe-inline' ${webview.cspSource}; script-src 'nonce-${nonce}';`;
        const initial = `// Éditez votre code Vitte ici\nfn main() {\n    println("Hello from Playground!");\n}`;
        this.panel.webview.html = `<!DOCTYPE html>
      <html lang="fr">
      <head>
        <meta charset="UTF-8" />
        <meta http-equiv="Content-Security-Policy" content="${csp}">
        <meta name="viewport" content="width=device-width, initial-scale=1" />
        <title>Vitte • Playground</title>
        <style>
          :root { --pad: 12px; }
          body { font: 13px/1.5 -apple-system, Segoe UI, system-ui, sans-serif; padding: 0; margin: 0; }
          header { position: sticky; top: 0; backdrop-filter: blur(8px); background: var(--vscode-editor-background); border-bottom: 1px solid var(--vscode-panel-border); padding: 10px var(--pad); display:flex; gap:10px; align-items:center; }
          .btn { padding:6px 10px; border-radius:6px; border:1px solid var(--vscode-button-border, transparent); background: var(--vscode-button-secondaryBackground); color: var(--vscode-button-secondaryForeground); cursor:pointer; }
          .btn.primary { background: var(--vscode-button-background); color: var(--vscode-button-foreground); }
          textarea { width: 100%; height: calc(100vh - 120px); box-sizing: border-box; padding: var(--pad); border: none; outline: none; resize: none; font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", monospace; font-size: 12.5px; background: var(--vscode-editor-background); color: var(--vscode-editor-foreground); }
          footer { padding: 8px var(--pad); color: var(--vscode-descriptionForeground); display: flex; justify-content: space-between; align-items:center; }
        </style>
      </head>
      <body>
        <header>
          <button id="format" class="btn">Formater</button>
          <button id="run" class="btn primary">Exécuter ▶</button>
          <button id="save" class="btn">Enregistrer</button>
          <button id="open" class="btn">Ouvrir le dossier</button>
          <span id="status"></span>
        </header>
        <textarea id="code" spellcheck="false">${escapeHtml(initial)}</textarea>
        <footer>
          <span>Les sorties d’exécution s’affichent dans le terminal intégré “Vitte Playground”.</span>
          <span id="info"></span>
        </footer>
        <script nonce="${nonce}">
          const vscodeApi = acquireVsCodeApi();
          const $ = (sel) => document.querySelector(sel);
          const code = $('#code');
          const status = $('#status');
          const info = $('#info');

          function post(type, payload) { vscodeApi.postMessage({ type, ...payload }); }

          $('#format').addEventListener('click', () => { status.textContent = 'Formatage…'; post('format', { text: code.value }); });
          $('#run').addEventListener('click', () => { status.textContent = 'Exécution…'; post('run', { text: code.value, save: true }); });
          $('#save').addEventListener('click', () => { post('save', { text: code.value }); });
          $('#open').addEventListener('click', () => { post('openFolder', {}); });

          window.addEventListener('message', (e) => {
            const m = e.data || {};
            if (m.type === 'formatted') {
              code.value = m.text || code.value;
              status.textContent = 'Formatté.';
              setTimeout(() => status.textContent = '', 800);
            }
            if (m.type === 'saved') {
              info.textContent = m.file ? 'Enregistré: ' + m.file : '';
              status.textContent = m.file ? 'Enregistré.' : 'Erreur sauvegarde';
              setTimeout(() => status.textContent = '', 1000);
            }
          });
        </script>
      </body>
      </html>`;
    }
}
exports.PlaygroundPanel = PlaygroundPanel;
exports.default = PlaygroundPanel;
const HTML_ESCAPE = {
    '&': '&amp;',
    '<': '&lt;',
    '>': '&gt;',
    '"': '&quot;',
    '\'': '&#39;',
};
function escapeHtml(s) {
    return s.replace(/[&<>"']/g, (c) => HTML_ESCAPE[c] ?? c);
}
function quote(p) {
    if (process.platform === 'win32')
        return `"${p.replace(/"/g, '\\"')}"`;
    return `'${p.replace(/'/g, `'\\''`)}'`;
}
//# sourceMappingURL=playgroundPanel.js.map