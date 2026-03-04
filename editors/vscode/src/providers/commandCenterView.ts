import * as vscode from "vscode";
import { summarizeWorkspaceDiagnostics, summarizeDiagnosticsByDirectory } from "../utils/diagnostics";
import type { LanguageClient } from "vscode-languageclient/node";
import type { ServerMetricEntry } from "../types/metrics";

class CommandCenterProvider implements vscode.WebviewViewProvider {
  static readonly viewId = "vitteCommandCenter";
  private view?: vscode.WebviewView;

  constructor(private readonly getClient: () => LanguageClient | undefined) {}

  resolveWebviewView(view: vscode.WebviewView): void {
    this.view = view;
    view.webview.options = { enableScripts: true };
    view.webview.onDidReceiveMessage(async (m: { cmd?: string }) => {
      if (!m?.cmd) return;
      await vscode.commands.executeCommand(m.cmd);
      void this.refresh();
    });
    void this.refresh();
  }

  async refresh(): Promise<void> {
    if (!this.view) return;
    const summary = summarizeWorkspaceDiagnostics();
    const byDir = summarizeDiagnosticsByDirectory().slice(0, 8);
    let metrics: ServerMetricEntry[] = [];
    const client = this.getClient();
    if (client) {
      try { metrics = await client.sendRequest<ServerMetricEntry[]>("vitte/metrics"); } catch { /* ignore */ }
    }
    const slow = [...metrics].sort((a, b) => (b.p95Ms ?? b.averageMs) - (a.p95Ms ?? a.averageMs)).slice(0, 10);
    this.view.webview.html = html(summary, byDir, slow);
  }
}

function html(
  summary: { errors: number; warnings: number; info: number; hints: number },
  byDir: Array<{ dir: string; errors: number; warnings: number }>,
  slow: Array<{ name: string; p95Ms?: number; averageMs: number }>
): string {
  const noisy = byDir.map((d) => `<li>${d.dir}: ${d.errors}e/${d.warnings}w</li>`).join("") || "<li>No noisy modules</li>";
  const slowRows = slow.map((s) => `<li>${s.name}: ${(s.p95Ms ?? s.averageMs).toFixed(1)} ms</li>`).join("") || "<li>No metric yet</li>";
  const buttons = [
    ["Doctor", "vitte.doctor"],
    ["Perf Session", "vitte.exportPerfSession"],
    ["Module Graph", "vitte.moduleGraph.refresh"],
    ["Problems by Package", "vitte.packageProblems.refresh"],
    ["Offline Docs Search", "vitte.docs.searchLocal"],
    ["Export Workspace State", "vitte.exportWorkspaceState"],
  ] as const;
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

export function registerCommandCenterView(context: vscode.ExtensionContext, getClient: () => LanguageClient | undefined): void {
  const p = new CommandCenterProvider(getClient);
  context.subscriptions.push(
    vscode.window.registerWebviewViewProvider(CommandCenterProvider.viewId, p, {
      webviewOptions: { retainContextWhenHidden: true },
    }),
    vscode.languages.onDidChangeDiagnostics(() => void p.refresh()),
    vscode.commands.registerCommand("vitte.commandCenter.refresh", async () => p.refresh()),
  );
}

