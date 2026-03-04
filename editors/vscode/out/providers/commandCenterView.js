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
exports.registerCommandCenterView = registerCommandCenterView;
const vscode = __importStar(require("vscode"));
const diagnostics_1 = require("../utils/diagnostics");
class CommandCenterProvider {
    constructor(getClient) {
        this.getClient = getClient;
    }
    resolveWebviewView(view) {
        this.view = view;
        view.webview.options = { enableScripts: true };
        view.webview.onDidReceiveMessage(async (m) => {
            if (!m?.cmd)
                return;
            await vscode.commands.executeCommand(m.cmd);
            void this.refresh();
        });
        void this.refresh();
    }
    async refresh() {
        if (!this.view)
            return;
        const summary = (0, diagnostics_1.summarizeWorkspaceDiagnostics)();
        const byDir = (0, diagnostics_1.summarizeDiagnosticsByDirectory)().slice(0, 8);
        let metrics = [];
        const client = this.getClient();
        if (client) {
            try {
                metrics = await client.sendRequest("vitte/metrics");
            }
            catch { /* ignore */ }
        }
        const slow = [...metrics].sort((a, b) => (b.p95Ms ?? b.averageMs) - (a.p95Ms ?? a.averageMs)).slice(0, 10);
        this.view.webview.html = html(summary, byDir, slow);
    }
}
CommandCenterProvider.viewId = "vitteCommandCenter";
function html(summary, byDir, slow) {
    const noisy = byDir.map((d) => `<li>${d.dir}: ${d.errors}e/${d.warnings}w</li>`).join("") || "<li>No noisy modules</li>";
    const slowRows = slow.map((s) => `<li>${s.name}: ${(s.p95Ms ?? s.averageMs).toFixed(1)} ms</li>`).join("") || "<li>No metric yet</li>";
    const buttons = [
        ["Doctor", "vitte.doctor"],
        ["Perf Session", "vitte.exportPerfSession"],
        ["Module Graph", "vitte.moduleGraph.refresh"],
        ["Problems by Package", "vitte.packageProblems.refresh"],
        ["Offline Docs Search", "vitte.docs.searchLocal"],
        ["Export Workspace State", "vitte.exportWorkspaceState"],
    ];
    const btn = buttons.map(([label, cmd]) => `<button data-cmd="${cmd}">${label}</button>`).join("");
    return `<!doctype html><html><head><meta charset="utf-8"/><style>
    body{font-family:var(--vscode-font-family);padding:10px}
    .grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}
    button{margin:4px;padding:6px 10px}
    h3{margin:8px 0}
    ul{padding-left:18px}
  </style></head><body>
    <h3>Vitte Command Center</h3>
    <div>Diagnostics: ${summary.errors}e / ${summary.warnings}w / ${summary.info}i / ${summary.hints}h</div>
    <div>${btn}</div>
    <div class="grid">
      <section><h3>Top 10 noisy modules</h3><ul>${noisy}</ul></section>
      <section><h3>Top 10 slow files/features</h3><ul>${slowRows}</ul></section>
    </div>
    <script>
      const vscode = acquireVsCodeApi();
      document.querySelectorAll("button[data-cmd]").forEach((b)=>b.addEventListener("click",()=>vscode.postMessage({cmd:b.getAttribute("data-cmd")})));
    </script>
  </body></html>`;
}
function registerCommandCenterView(context, getClient) {
    const p = new CommandCenterProvider(getClient);
    context.subscriptions.push(vscode.window.registerWebviewViewProvider(CommandCenterProvider.viewId, p, {
        webviewOptions: { retainContextWhenHidden: true },
    }), vscode.languages.onDidChangeDiagnostics(() => void p.refresh()), vscode.commands.registerCommand("vitte.commandCenter.refresh", async () => p.refresh()));
}
//# sourceMappingURL=commandCenterView.js.map