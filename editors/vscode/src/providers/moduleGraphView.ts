import * as vscode from "vscode";

interface NodeInfo {
  id: string;
  file: string;
  errors: number;
}

interface EdgeInfo {
  source: string;
  target: string;
}

interface GraphData {
  nodes: NodeInfo[];
  edges: EdgeInfo[];
  focus?: string;
}

function esc(s: string): string {
  return s.replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", "\"": "&quot;" }[c] ?? c));
}

function rxEscape(s: string): string {
  return s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function activeModule(): string | undefined {
  const editor = vscode.window.activeTextEditor;
  if (!editor) return undefined;
  const m = /^\s*(?:space|module)\s+([A-Za-z_][\w./:]*)/m.exec(editor.document.getText());
  return m?.[1];
}

async function buildGraph(): Promise<GraphData> {
  const files = await vscode.workspace.findFiles("**/*.{vit,vitte}", "**/{node_modules,.git,out,dist,build}/**", 4000);
  const nodeMap = new Map<string, NodeInfo>();
  const edges: EdgeInfo[] = [];
  const diagMap = new Map<string, number>();
  for (const [uri, diags] of vscode.languages.getDiagnostics()) {
    diagMap.set(uri.toString(), diags.filter((d) => (d.severity ?? vscode.DiagnosticSeverity.Hint) <= vscode.DiagnosticSeverity.Warning).length);
  }

  for (const uri of files) {
    let text = "";
    try {
      text = (await vscode.workspace.openTextDocument(uri)).getText();
    } catch {
      continue;
    }
    const from = /^\s*(?:space|module)\s+([A-Za-z_][\w./:]*)/m.exec(text)?.[1]
      ?? vscode.workspace.asRelativePath(uri, false).replace(/\\/g, "/");
    if (!nodeMap.has(from)) {
      nodeMap.set(from, {
        id: from,
        file: vscode.workspace.asRelativePath(uri, false).replace(/\\/g, "/"),
        errors: diagMap.get(uri.toString()) ?? 0,
      });
    }
    const depRx = /^\s*(?:import|use|pull)\s+([A-Za-z_][\w./:]*)/gm;
    let m: RegExpExecArray | null;
    while ((m = depRx.exec(text))) {
      const target = m[1] ?? "";
      if (!target) continue;
      edges.push({ source: from, target });
      if (!nodeMap.has(target)) {
        nodeMap.set(target, { id: target, file: "(virtual)", errors: 0 });
      }
      if (m[0].length === 0) depRx.lastIndex++;
    }
  }
  const focus = activeModule();
  return focus
    ? { nodes: Array.from(nodeMap.values()), edges, focus }
    : { nodes: Array.from(nodeMap.values()), edges };
}

class ModuleGraphProvider implements vscode.WebviewViewProvider {
  public static readonly viewType = "vitteModuleGraph";
  private view?: vscode.WebviewView;

  resolveWebviewView(view: vscode.WebviewView): void {
    this.view = view;
    view.webview.options = { enableScripts: true };
    view.webview.onDidReceiveMessage(async (msg: { type?: string; module?: string }) => {
      if (msg?.type === "open" && msg.module) {
        await vscode.commands.executeCommand("vitte.moduleGraph.openModule", msg.module);
      }
    });
    void this.refresh();
  }

  async refresh(): Promise<void> {
    if (!this.view) return;
    const data = await buildGraph();
    this.view.webview.html = html(data);
  }
}

function html(data: GraphData): string {
  const payload = JSON.stringify(data);
  return `<!doctype html>
<html>
<head>
  <meta charset="utf-8"/>
  <style>
    body { font-family: var(--vscode-font-family); margin:0; padding:8px; }
    .toolbar { display:flex; gap:8px; align-items:center; margin-bottom:6px; flex-wrap: wrap; }
    .toolbar input[type="text"] { min-width: 180px; }
    .hint { opacity:.8; font-size:12px; }
    svg { width:100%; height:460px; border:1px solid var(--vscode-panel-border); border-radius:6px; background: var(--vscode-editor-background); }
    .edge { stroke: var(--vscode-editorLineNumber-foreground); stroke-opacity:.45; stroke-width:1.4; }
    .node { cursor:pointer; }
    .label { font-size:10px; fill: var(--vscode-editor-foreground); pointer-events:none; }
  </style>
</head>
<body>
  <div class="toolbar">
    <label>Package <input id="packageFilter" type="text" placeholder="std/net or app/core"/></label>
    <label><input id="errorsOnly" type="checkbox"/> errors only</label>
    <label><input id="orphansOnly" type="checkbox"/> orphans only</label>
    <button id="focusActive">focus active module</button>
  </div>
  <div class="hint">Click node to open module. Red = module with diagnostics.</div>
  <svg id="g" viewBox="0 0 1000 460"></svg>
  <script>
    const vscode = acquireVsCodeApi();
    const raw = ${payload};
    const svg = document.getElementById("g");
    const pkgInput = document.getElementById("packageFilter");
    const errorsOnly = document.getElementById("errorsOnly");
    const orphansOnly = document.getElementById("orphansOnly");
    const focusBtn = document.getElementById("focusActive");

    const W = 1000, H = 460;
    const state = {
      nodes: raw.nodes.map(n => ({...n, x: Math.random()*W, y: Math.random()*H, vx:0, vy:0})),
      edges: raw.edges.slice(),
      focus: raw.focus || null,
    };

    function filtered() {
      const pkg = (pkgInput.value || "").trim().toLowerCase();
      let nodes = state.nodes.filter(n => !pkg || n.id.toLowerCase().includes(pkg));
      const set = new Set(nodes.map(n => n.id));
      let edges = state.edges.filter(e => set.has(e.source) && set.has(e.target));
      const indeg = new Map(), outdeg = new Map();
      for (const n of nodes) { indeg.set(n.id, 0); outdeg.set(n.id, 0); }
      for (const e of edges) {
        indeg.set(e.target, (indeg.get(e.target) || 0) + 1);
        outdeg.set(e.source, (outdeg.get(e.source) || 0) + 1);
      }
      if (errorsOnly.checked) {
        nodes = nodes.filter(n => n.errors > 0);
      }
      if (orphansOnly.checked) {
        nodes = nodes.filter(n => (indeg.get(n.id) || 0) === 0 && (outdeg.get(n.id) || 0) === 0);
      }
      const nodeIds = new Set(nodes.map(n => n.id));
      edges = edges.filter(e => nodeIds.has(e.source) && nodeIds.has(e.target));
      return { nodes, edges };
    }

    function step(nodes, edges) {
      const kRep = 12000, kLink = 0.015, damping = 0.86;
      for (let i=0; i<nodes.length; i++) {
        const a = nodes[i];
        for (let j=i+1; j<nodes.length; j++) {
          const b = nodes[j];
          const dx = a.x - b.x, dy = a.y - b.y;
          const d2 = Math.max(12, dx*dx + dy*dy);
          const f = kRep / d2;
          const fx = f * dx, fy = f * dy;
          a.vx += fx; a.vy += fy;
          b.vx -= fx; b.vy -= fy;
        }
      }
      const byId = new Map(nodes.map(n => [n.id, n]));
      for (const e of edges) {
        const s = byId.get(e.source), t = byId.get(e.target);
        if (!s || !t) continue;
        const dx = t.x - s.x, dy = t.y - s.y;
        const fx = dx * kLink, fy = dy * kLink;
        s.vx += fx; s.vy += fy;
        t.vx -= fx; t.vy -= fy;
      }
      for (const n of nodes) {
        const cx = W/2, cy = H/2;
        n.vx += (cx - n.x) * 0.0009;
        n.vy += (cy - n.y) * 0.0009;
        n.vx *= damping; n.vy *= damping;
        n.x = Math.max(20, Math.min(W-20, n.x + n.vx));
        n.y = Math.max(20, Math.min(H-20, n.y + n.vy));
      }
    }

    function render() {
      const f = filtered();
      for (let i = 0; i < 35; i++) step(f.nodes, f.edges);
      const byId = new Map(f.nodes.map(n => [n.id, n]));
      const edgesSvg = f.edges.map(e => {
        const s = byId.get(e.source), t = byId.get(e.target);
        if (!s || !t) return "";
        return '<line class="edge" x1="'+s.x+'" y1="'+s.y+'" x2="'+t.x+'" y2="'+t.y+'"/>';
      }).join("");
      const nodesSvg = f.nodes.map(n => {
        const focus = state.focus && n.id === state.focus;
        const r = focus ? 8 : 6;
        const fill = n.errors > 0 ? "#d9534f" : (focus ? "#5bc0de" : "#7aa2f7");
        return '<g class="node" data-id="'+n.id+'">'+
          '<circle cx="'+n.x+'" cy="'+n.y+'" r="'+r+'" fill="'+fill+'"/>'+
          '<text class="label" x="'+(n.x+8)+'" y="'+(n.y+3)+'">'+n.id+'</text>'+
          '</g>';
      }).join("");
      svg.innerHTML = edgesSvg + nodesSvg;
      svg.querySelectorAll(".node").forEach(el => {
        el.addEventListener("click", () => {
          const id = el.getAttribute("data-id");
          if (id) vscode.postMessage({ type: "open", module: id });
        });
      });
    }

    [pkgInput, errorsOnly, orphansOnly].forEach(el => el.addEventListener("input", render));
    focusBtn.addEventListener("click", () => { if (raw.focus) state.focus = raw.focus; render(); });
    render();
  </script>
</body>
</html>`;
}

export function registerModuleGraphView(context: vscode.ExtensionContext): void {
  const provider = new ModuleGraphProvider();
  context.subscriptions.push(
    vscode.window.registerWebviewViewProvider(ModuleGraphProvider.viewType, provider, {
      webviewOptions: { retainContextWhenHidden: true },
    }),
    vscode.workspace.onDidSaveTextDocument((doc) => {
      if (doc.languageId === "vitte" || doc.languageId === "vit") void provider.refresh();
    }),
    vscode.workspace.onDidOpenTextDocument((doc) => {
      if (doc.languageId === "vitte" || doc.languageId === "vit") void provider.refresh();
    }),
    vscode.languages.onDidChangeDiagnostics(() => void provider.refresh()),
    vscode.window.onDidChangeActiveTextEditor(() => void provider.refresh()),
    vscode.commands.registerCommand("vitte.moduleGraph.refresh", async () => provider.refresh()),
    vscode.commands.registerCommand("vitte.moduleGraph.openModule", async (moduleName: string) => {
      if (!moduleName) return;
      const files = await vscode.workspace.findFiles("**/*.{vit,vitte}", "**/{node_modules,.git,out,dist,build}/**", 4000);
      const modRx = new RegExp(`^\\s*(?:space|module)\\s+${rxEscape(moduleName)}\\b`, "m");
      for (const uri of files) {
        const doc = await vscode.workspace.openTextDocument(uri);
        if (modRx.test(doc.getText())) {
          await vscode.window.showTextDocument(doc, { preview: true });
          return;
        }
      }
    }),
  );
}
