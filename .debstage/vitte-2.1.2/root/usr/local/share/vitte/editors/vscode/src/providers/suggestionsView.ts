import * as vscode from "vscode";

type Logger = (message: string) => void;

interface SuggestionState {
  cInput: string;
  outputVitte: string;
  history: { title: string; output: string; ts: number }[];
  cFlavor: "c" | "cpp";
}

function mapCTypeToVitte(typeRaw: string): string {
  const cleaned = typeRaw
    .replace(/\bconst\b/g, "")
    .replace(/\bunsigned\b/g, "")
    .replace(/\bsigned\b/g, "")
    .replace(/\bstd::/g, "")
    .trim();
  if (!cleaned) return "int";
  if (cleaned.includes("string")) return "string";
  if (cleaned.includes("bool")) return "bool";
  if (cleaned.includes("char*") || cleaned.includes("char *")) return "string";
  if (cleaned.includes("char")) return "int";
  if (/\b(short|int|long|size_t|ssize_t)\b/.exec(cleaned)) return "int";
  const vectorMatch = /vector<\s*([^>]+)\s*>/.exec(cleaned);
  if (vectorMatch) return `[${mapCTypeToVitte(vectorMatch[1] ?? "int")}]`;
  if (cleaned.includes("*")) return `*${mapCTypeToVitte(cleaned.replace(/\*/g, "").trim())}`;
  return cleaned;
}

function parseCParams(paramList: string): string {
  const params = paramList.trim();
  if (!params || params === "void") return "";
  return params
    .split(",")
    .map((chunk) => {
      const part = chunk.trim();
      if (!part) return "";
      const tokens = part.split(/\s+/);
      let name = tokens.pop() ?? "arg";
      let type = tokens.join(" ");
      if (name.includes("*")) {
        type = `${type} ${name.replace(/[A-Za-z_]\w*/, "")}`.trim();
        name = name.replace(/[^A-Za-z_0-9]/g, "");
      }
      return `${name}: ${mapCTypeToVitte(type)}`;
    })
    .filter(Boolean)
    .join(", ");
}

function convertCSignature(line: string): string | null {
  const match = /^\s*([A-Za-z_][\w:<>,\s\*&]+)\s+([A-Za-z_]\w*)\s*\(([^)]*)\)\s*\{?/.exec(line);
  if (!match) return null;
  const ret = mapCTypeToVitte(match[1] ?? "");
  const name = match[2] ?? "proc";
  const params = parseCParams(match[3] ?? "");
  return ret === "void" ? `proc ${name}(${params}) {` : `proc ${name}(${params}) -> ${ret} {`;
}

function convertCStruct(line: string): string | null {
  const match = /^\s*struct\s+([A-Za-z_]\w*)\s*\{/.exec(line);
  if (!match) return null;
  return `form ${match[1] ?? "Struct"} {`;
}

function convertCField(line: string): string | null {
  const match = /^\s*([A-Za-z_][\w:<>,\s\*&]+)\s+([A-Za-z_]\w*)\s*(\[[^\]]+\])?\s*;/.exec(line);
  if (!match) return null;
  const type = mapCTypeToVitte(match[1] ?? "");
  const name = match[2] ?? "field";
  return `  ${name}: ${type},`;
}

function convertCVar(line: string): string | null {
  const match = /^\s*(const\s+)?([A-Za-z_][\w:<>,\s\*&]+)\s+([A-Za-z_]\w*)\s*(=\s*(.+))?;/.exec(line);
  if (!match) return null;
  const isConst = Boolean(match[1]);
  const type = mapCTypeToVitte(match[2] ?? "");
  const name = match[3] ?? "var";
  const value = (match[5] ?? "").trim();
  const head = isConst ? "const" : "let";
  if (value) return `${head} ${name}: ${type} = ${value}`;
  return `${head} ${name}: ${type} = ${type === "string" ? "\"\"" : "0"}`;
}

function convertCIf(line: string): string | null {
  const match = /^\s*if\s*\((.+)\)\s*\{/.exec(line);
  if (!match) return null;
  return `if ${match[1] ?? ""} {`;
}

function convertCElse(line: string): string | null {
  if (/^\s*else\s*\{/.test(line)) return "else {";
  return null;
}

function convertCWhile(line: string): string | null {
  const match = /^\s*while\s*\((.+)\)\s*\{/.exec(line);
  if (!match) return null;
  return `loop { // while ${match[1] ?? ""}`;
}

function convertCFor(line: string): string | null {
  if (!/^\s*for\s*\(/.test(line)) return null;
  return "loop { // for (...)";
}

function convertCReturn(line: string): string | null {
  const match = /^\s*return\s+(.+)\s*;/.exec(line);
  if (!match) return null;
  return `return ${match[1] ?? ""}`;
}

function convertCStatements(line: string): string {
  return line
    .replace(/\btrue\b/g, "true")
    .replace(/\bfalse\b/g, "false")
    .replace(/\bprintf\s*\(/g, "emit ");
}

function convertCToVitte(input: string): string {
  const lines = input.split(/\r?\n/);
  const out: string[] = [];
  for (const raw of lines) {
    const line = raw.trimEnd();
    if (!line) {
      out.push("");
      continue;
    }
    const sig = convertCSignature(line);
    if (sig) {
      out.push(sig);
      continue;
    }
    const form = convertCStruct(line);
    if (form) {
      out.push(form);
      continue;
    }
    if (line.startsWith("}")) {
      out.push("}");
      continue;
    }
    const field = convertCField(line);
    if (field) {
      out.push(field);
      continue;
    }
    const ifLine = convertCIf(line);
    if (ifLine) {
      out.push(ifLine);
      continue;
    }
    const elseLine = convertCElse(line);
    if (elseLine) {
      out.push(elseLine);
      continue;
    }
    const whileLine = convertCWhile(line);
    if (whileLine) {
      out.push(whileLine);
      continue;
    }
    const forLine = convertCFor(line);
    if (forLine) {
      out.push(forLine);
      continue;
    }
    const retLine = convertCReturn(line);
    if (retLine) {
      out.push(retLine);
      continue;
    }
    const decl = convertCVar(line);
    if (decl) {
      out.push(decl);
      continue;
    }
    out.push(convertCStatements(line));
  }
  return out.join("\n");
}

function createConverterSession(webview: vscode.Webview, title: string, log?: Logger): void {
  webview.options = { enableScripts: true };
  const state: SuggestionState = {
    cInput: "",
    outputVitte: "",
    history: [],
    cFlavor: "cpp"
  };
  const update = () => {
    try {
      webview.html = getHtml(state, title);
    } catch (err) {
      const message = err instanceof Error ? err.message : String(err);
      log?.(`Webview render error: ${message}`);
      webview.html = `<!doctype html><html><body><pre>Vitte converter failed to render: ${message}</pre></body></html>`;
    }
  };
  webview.html = `<!doctype html><html><body><p>Loading Vitte converter…</p></body></html>`;
  update();
  log?.("Converter session initialized.");

  webview.onDidReceiveMessage(async (msg) => {
    const data = asRecord(msg);
    if (!data) return;
    const type = typeof data.type === "string" ? data.type : "";
    if (type === "convertFromC") {
      const input = typeof data.input === "string" ? data.input : "";
      const flavor = data.flavor === "c" ? "c" : "cpp";
      state.cInput = input;
      state.cFlavor = flavor;
      state.outputVitte = convertCToVitte(state.cInput);
      state.history.unshift({ title: "Convert: C/C++ -> Vitte", output: state.outputVitte, ts: Date.now() });
      state.history = state.history.slice(0, 12);
      update();
      return;
    }
    if (type === "insert") {
      const editor = vscode.window.activeTextEditor;
      if (!editor) {
        void vscode.window.showWarningMessage("Open a Vitte/Vit file to insert code.");
        return;
      }
      await editor.edit((edit) => {
        edit.insert(editor.selection.active, state.outputVitte + "\n");
      });
      return;
    }
    if (type === "copy") {
      await vscode.env.clipboard.writeText(state.outputVitte);
      void vscode.window.showInformationMessage("Vitte code copied.");
      return;
    }
    if (type === "clear") {
      state.cInput = "";
      state.outputVitte = "";
      state.history = [];
      update();
      return;
    }
  });
}

function asRecord(value: unknown): Record<string, unknown> | null {
  if (!value || typeof value !== "object") return null;
  return value as Record<string, unknown>;
}

export function registerSuggestionsView(
  context: vscode.ExtensionContext,
  viewId = "vitteSuggestions",
  title = "Vitte C/C++ Converter",
  log?: Logger
): void {
  const provider = {
    resolveWebviewView(view: vscode.WebviewView) {
      log?.(`Resolve view: ${viewId}`);
      createConverterSession(view.webview, title, log);
    },
  } satisfies vscode.WebviewViewProvider;

  context.subscriptions.push(
    vscode.window.registerWebviewViewProvider(viewId, provider)
  );
}

export function openSuggestionsPanel(
  context: vscode.ExtensionContext,
  title = "Vitte C/C++ Converter",
  log?: Logger
): void {
  const panel = vscode.window.createWebviewPanel(
    "vitteCodeGeneratorPanel",
    title,
    vscode.ViewColumn.One,
    { enableScripts: true, retainContextWhenHidden: true }
  );
  log?.("Open converter panel.");
  createConverterSession(panel.webview, title, log);
  context.subscriptions.push(panel);
}

function getHtml(state: SuggestionState, title: string): string {
  const cInput = escapeHtml(state.cInput);
  const outputVitte = escapeHtml(state.outputVitte);
  const history = state.history
    .map((entry) => {
      const when = new Date(entry.ts).toLocaleTimeString();
      const entryPrompt = escapeHtml(entry.title || "Prompt");
      const entryOutput = escapeHtml(entry.output || "");
      return `<div class="bubble">\n  <div class="bubble-header">${entryPrompt} <span class="time">${when}</span></div>\n  <pre>${entryOutput}</pre>\n</div>`;
    })
    .join("");
  return `<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<style>
  :root { color-scheme: light dark; }
  body { font-family: -apple-system, BlinkMacSystemFont, Segoe UI, sans-serif; padding: 12px; }
  h3 { margin: 0 0 8px; }
  textarea { width: 100%; box-sizing: border-box; min-height: 160px; }
  .row { display: flex; gap: 8px; margin-top: 8px; }
  button { flex: 1; padding: 6px 10px; }
  pre { white-space: pre-wrap; background: rgba(127,127,127,0.15); padding: 8px; border-radius: 6px; margin: 0; }
  .bubble { border: 1px solid rgba(127,127,127,0.35); border-radius: 8px; padding: 8px; margin-top: 8px; }
  .bubble-header { font-weight: 600; margin-bottom: 6px; display: flex; justify-content: space-between; gap: 8px; }
  .time { font-weight: 400; opacity: 0.7; font-size: 11px; }
  .muted { opacity: 0.7; font-size: 12px; }
</style>
</head>
<body>
  <h3>${escapeHtml(title)}</h3>
  <div class="muted">Paste C/C++ code and convert to Vitte (offline).</div>
  <label>C / C++ input</label>
  <textarea id="c-input" placeholder="Paste C or C++ code here">${cInput}</textarea>
  <div class="row">
    <select id="c-flavor">
      <option value="cpp">C++</option>
      <option value="c">C</option>
    </select>
    <button id="convert">Convert to Vitte</button>
  </div>
  <div class="row">
    <button id="insert">Insert Vitte</button>
    <button id="copy">Copy Vitte</button>
    <button id="clear">Clear</button>
  </div>
  <label style="margin-top:8px; display:block;">Vitte output</label>
  <pre id="out-vitte">${outputVitte || "(no Vitte output yet)"}</pre>
  <div style="margin-top:10px;">
    <label>History</label>
    ${history || `<div class="muted">(no history yet)</div>`}
  </div>

<script>
  const vscode = acquireVsCodeApi();
  document.getElementById('convert').addEventListener('click', () => {
    const input = document.getElementById('c-input').value || '';
    const flavor = document.getElementById('c-flavor').value || 'cpp';
    vscode.postMessage({ type: 'convertFromC', input, flavor });
  });
  document.getElementById('insert').addEventListener('click', () => {
    vscode.postMessage({ type: 'insert' });
  });
  document.getElementById('copy').addEventListener('click', () => {
    vscode.postMessage({ type: 'copy' });
  });
  document.getElementById('clear').addEventListener('click', () => {
    vscode.postMessage({ type: 'clear' });
  });
</script>
</body>
</html>`;
}

function escapeHtml(value: string): string {
  return value
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/\"/g, "&quot;")
    .replace(/'/g, "&#39;");
}
