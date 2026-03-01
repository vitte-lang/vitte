/* ----------------------------------------------------------------------------
 * extension.ts — Extension VS Code pour Vitte / Vit
 * SPDX-License-Identifier: MIT
 * --------------------------------------------------------------------------- */

import * as vscode from "vscode";
import * as path from "path";
import * as fs from "fs";
import { spawn, SpawnOptionsWithoutStdio } from "child_process";

// ----------------------------- Constantes ----------------------------------
const LANGS = [
  { id: "vit",  exts: [".vit"],  aliases: ["Vitte", "Vit"] },
  { id: "vitte", exts: [".vitte"],aliases: ["Vitte Canonical"] },
];

const CMD = {
  RUN: "vitte.runFile",
  BUILD: "vitte.buildWorkspace",
  FMT: "vitte.formatFile",
  TEST: "vitte.testWorkspace",
  DOCS: "vitte.openDocs",
  CREATE_SAMPLE: "vitte.createSample",
};

// ---------------------------- Utilitaires ----------------------------------
function getCfg() {
  return vscode.workspace.getConfiguration("vitte");
}

function toolPath(name: string): string {
  const cfg = getCfg();
  const configured: string | undefined = cfg.get("paths." + name);
  if (configured && configured.trim().length > 0) return configured;
  return name; // cherche dans PATH
}

function workspaceFolderFor(doc?: vscode.TextDocument): vscode.WorkspaceFolder | undefined {
  if (doc) return vscode.workspace.getWorkspaceFolder(doc.uri) ?? undefined;
  const folders = vscode.workspace.workspaceFolders;
  return folders && folders.length > 0 ? folders[0] : undefined;
}

function runInTerminal(cmd: string, args: string[], cwd?: string) {
  const term = vscode.window.createTerminal({ name: "Vitte", cwd });
  term.show();
  const line = [cmd, ...args.map(a => shellEscape(a))].join(" ");
  term.sendText(line);
}

function shellEscape(s: string): string {
  if (/^[A-Za-z0-9_@%+=:,./-]+$/.test(s)) return s;
  return `'${s.replace(/'/g, "'\\''")}'`;
}

async function fileExists(p: string): Promise<boolean> {
  try { await fs.promises.access(p, fs.constants.F_OK); return true; } catch { return false; }
}

function asExecutableMaybe(p: string): string {
  if (process.platform === "win32" && !/[.]exe$/i.test(p)) return p + ".exe";
  return p;
}

function pickActiveDoc(): vscode.TextDocument | undefined {
  const ed = vscode.window.activeTextEditor;
  return ed?.document;
}

// --------------------- Diagnostics et Linting léger ------------------------
const diag = vscode.languages.createDiagnosticCollection("vitte");

async function lintDocument(doc: vscode.TextDocument) {
  if (!LANGS.some(l => l.exts.some(e => doc.fileName.endsWith(e)))) return;

  const cfg = getCfg();
  const linter = asExecutableMaybe(toolPath("vitte"));
  const args = ["fmt", "--check", "--quiet", doc.fileName];

  const spawnOpts: SpawnOptionsWithoutStdio = {
    cwd: workspaceFolderFor(doc)?.uri.fsPath,
    env: { ...process.env },
  };

  return new Promise<void>((resolve) => {
    const p = spawn(linter, args, spawnOpts);
    const output: string[] = [];
    const errs: string[] = [];

    p.stdout.on("data", d => output.push(String(d)));
    p.stderr.on("data", d => errs.push(String(d)));

    p.on("error", () => {
      diag.set(doc.uri, []); // pas de linter disponible
      resolve();
    });

    p.on("close", () => {
      const text = output.join("") + errs.join("");
      const diags: vscode.Diagnostic[] = [];

      // Parse très basique: "path:line:col: message"
      const re = /^(.*?):(\d+):(\d+):\s*(.*)$/gm;
      for (let m; (m = re.exec(text)); ) {
        const line = Math.max(0, parseInt(m[2], 10) - 1);
        const col = Math.max(0, parseInt(m[3], 10) - 1);
        const range = new vscode.Range(line, col, line, col + 1);
        diags.push(new vscode.Diagnostic(range, m[4], vscode.DiagnosticSeverity.Warning));
      }
      diag.set(doc.uri, diags);
      resolve();
    });
  });
}

// ---------------------- Formateur minimal intégré --------------------------
const formatter: vscode.DocumentFormattingEditProvider = {
  provideDocumentFormattingEdits(doc) {
    const edits: vscode.TextEdit[] = [];
    const cfg = getCfg();
    const useExternal = cfg.get<boolean>("format.useExternal", true);

    if (useExternal) {
      // Déclenche le formateur externe via une commande shell
      const exec = asExecutableMaybe(toolPath("vitte"));
      const cwd = workspaceFolderFor(doc)?.uri.fsPath;
      runInTerminal(exec, ["fmt", doc.fileName], cwd);
      return edits; // pas de remplacement en place
    }

    // Fallback naïf: normalise indent en 2 espaces et trim trailing spaces
    const indentUnit = "  ";
    let level = 0;
    const lines = Array.from({ length: doc.lineCount }, (_, i) => doc.lineAt(i).text);

    const reOpen = /\{\s*$/;
    const reClose = /^\s*\}/;

    const newLines = lines.map((raw) => {
      let s = raw.replace(/\s+$/g, "");
      if (reClose.test(s)) level = Math.max(0, level - 1);
      const pref = indentUnit.repeat(level);
      const out = pref + s.trimStart();
      if (reOpen.test(s)) level += 1;
      return out;
    });

    const full = new vscode.Range(0, 0, doc.lineCount, 0);
    edits.push(vscode.TextEdit.replace(full, newLines.join("\n")));
    return edits;
  },
};

// ---------------------- Hover Provider simple ------------------------------
const hoverProvider: vscode.HoverProvider = {
  provideHover(doc, pos) {
    const word = doc.getText(doc.getWordRangeAtPosition(pos));
    if (!word) return undefined;
    const md = new vscode.MarkdownString();
    // fix: compat globale sans appendCodeblock
    md.appendMarkdown("```" + doc.languageId + "\n" + word + "\n```");
    md.appendMarkdown("\nSymbole détecté. Aucun index de symboles disponible.");
    md.isTrusted = true;
    return new vscode.Hover(md);
  },
};

// ---------------------- Debug Adapter via process externe ------------------
class VitteInlineDebugAdapter implements vscode.DebugAdapter {
  private readonly emitter = new vscode.EventEmitter<vscode.DebugProtocolMessage>();
  public readonly onDidSendMessage: vscode.Event<vscode.DebugProtocolMessage>;
  private proc: ReturnType<typeof spawn> | undefined;

  constructor(private command: string, private args: string[], private cwd?: string) {
    // fix: non optionnel
    this.onDidSendMessage = this.emitter.event;
  }

  start(): void {
    this.proc = spawn(this.command, this.args, { cwd: this.cwd, env: { ...process.env } });
    this.proc.stdout?.on("data", d => this.forward(d));
    this.proc.stderr?.on("data", d => this.forward(d));
    this.proc.on("error", err => {
      this.emitter.fire({ type: "event", event: "output", body: { category: "stderr", output: String(err) + "\n" } } as any);
    });
  }

  handleMessage(message: vscode.DebugProtocolMessage): void {
    if (!this.proc || !this.proc.stdin) return;
    try { this.proc.stdin.write(JSON.stringify(message) + "\n"); } catch { /* ignore */ }
  }

  dispose(): void {
    try { this.proc?.kill(); } catch { /* ignore */ }
    this.emitter.dispose();
  }

  private forward(d: Buffer) {
    // Suppose que la cible émet une ligne JSON DAP par ligne
    const lines = String(d).split(/\r?\n/).filter(Boolean);
    for (const line of lines) {
      try { this.emitter.fire(JSON.parse(line)); } catch { /* ignore output non DAP */ }
    }
  }
}

class VitteDebugAdapterFactory implements vscode.DebugAdapterDescriptorFactory, vscode.Disposable {
  private subs: vscode.Disposable[] = [];

  createDebugAdapterDescriptor(_session: vscode.DebugSession): vscode.ProviderResult<vscode.DebugAdapterDescriptor> {
    const exe = asExecutableMaybe(toolPath("vitte-runtime"));
    const args = ["--dap"]; // le runtime doit exposer un serveur DAP en ligne de commande
    const cwd = workspaceFolderFor()?.uri.fsPath;
    const impl = new VitteInlineDebugAdapter(exe, args, cwd);
    impl.start();
    // fix: cast explicite vers DebugAdapterDescriptor
    const inline = new (vscode as any).DebugAdapterInlineImplementation(impl);
    return inline as vscode.DebugAdapterDescriptor;
  }

  dispose(): void {
    this.subs.forEach(d => d.dispose());
    this.subs = [];
  }
}

class VitteDebugConfigProvider implements vscode.DebugConfigurationProvider {
  resolveDebugConfiguration(_folder: vscode.WorkspaceFolder | undefined, config: vscode.DebugConfiguration): vscode.ProviderResult<vscode.DebugConfiguration> {
    config.type ??= "vitte";
    config.request ??= "launch";
    config.name ??= "Vitte: Launch current file";
    config.program ??= "${file}";
    config.cwd ??= "${workspaceFolder}";
    config.stopOnEntry ??= true;
    config.args ??= [];
    return config;
  }
}

// ---------------------- Status Bar -----------------------------------------
let status: vscode.StatusBarItem | undefined;
function ensureStatusBar(context: vscode.ExtensionContext) {
  if (!status) {
    status = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 100);
    status.text = "$(play) Vitte Run";
    status.tooltip = "Exécuter le fichier actif avec vitte run";
    status.command = CMD.RUN;
    context.subscriptions.push(status);
  }
  status.show();
}

// ---------------------- Commands -------------------------------------------
function registerCommands(context: vscode.ExtensionContext) {
  context.subscriptions.push(
    vscode.commands.registerCommand(CMD.RUN, async () => {
      const doc = pickActiveDoc();
      if (!doc) { vscode.window.showWarningMessage("Aucun fichier actif."); return; }
      await doc.save();
      const exe = asExecutableMaybe(toolPath("vitte"));
      runInTerminal(exe, ["run", doc.fileName], workspaceFolderFor(doc)?.uri.fsPath);
    }),

    vscode.commands.registerCommand(CMD.BUILD, async () => {
      const exe = asExecutableMaybe(toolPath("vitte"));
      runInTerminal(exe, ["build"], workspaceFolderFor()?.uri.fsPath);
    }),

    vscode.commands.registerCommand(CMD.FMT, async () => {
      const doc = pickActiveDoc();
      if (!doc) { vscode.window.showWarningMessage("Aucun fichier actif."); return; }
      const exe = asExecutableMaybe(toolPath("vitte"));
      runInTerminal(exe, ["fmt", doc.fileName], workspaceFolderFor(doc)?.uri.fsPath);
    }),

    vscode.commands.registerCommand(CMD.TEST, async () => {
      const exe = asExecutableMaybe(toolPath("vitte"));
      runInTerminal(exe, ["test"], workspaceFolderFor()?.uri.fsPath);
    }),

    vscode.commands.registerCommand(CMD.DOCS, async () => {
      const url = getCfg().get<string>("docs.url", "https://vitte-lang.example/docs");
      vscode.env.openExternal(vscode.Uri.parse(url));
    }),

    vscode.commands.registerCommand(CMD.CREATE_SAMPLE, async () => {
      const ws = workspaceFolderFor();
      if (!ws) { vscode.window.showWarningMessage("Ouvrez un dossier."); return; }
      const p = path.join(ws.uri.fsPath, "hello.vitte");
      if (!(await fileExists(p))) await fs.promises.writeFile(p, "print(\"Hello, Vitte!\")\n");
      const doc = await vscode.workspace.openTextDocument(p);
      await vscode.window.showTextDocument(doc);
    }),
  );
}

// ---------------------- Activation -----------------------------------------
export function activate(context: vscode.ExtensionContext) {
  // Diagnostics on open and change
  context.subscriptions.push(diag);
  vscode.workspace.onDidOpenTextDocument(lintDocument, null, context.subscriptions);
  vscode.workspace.onDidSaveTextDocument(lintDocument, null, context.subscriptions);
  if (vscode.window.activeTextEditor) lintDocument(vscode.window.activeTextEditor.document);

  // Languages registration
  for (const l of LANGS) {
    context.subscriptions.push(
      vscode.languages.registerDocumentFormattingEditProvider(l.id, formatter),
      vscode.languages.registerHoverProvider(l.id, hoverProvider),
    );
  }

  // Debug registration
  const dbgFactory = new VitteDebugAdapterFactory();
  context.subscriptions.push(
    vscode.debug.registerDebugAdapterDescriptorFactory("vitte", dbgFactory),
    vscode.debug.registerDebugConfigurationProvider("vitte", new VitteDebugConfigProvider()),
    dbgFactory,
  );

  // Commands and status bar
  registerCommands(context);
  ensureStatusBar(context);

  // Info
  vscode.commands.executeCommand("setContext", "vitte.active", true);
}

export function deactivate() {
  // VS Code dispose automatiquement via subscriptions
}

// ---------------------- Configuration conseillée (settings.json) ------------
// "vitte.paths.vitte": "/usr/local/bin/vitte",
// "vitte.paths.runtime": "/usr/local/bin/vitte-runtime",
// "vitte.format.useExternal": true,
