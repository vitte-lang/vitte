/*
 * extension.ts — VS Code client for Vitte/Vit
 * - Full LSP client (start/stop/restart, traces, status, watchers)
 * - Commands: logs, restart, actions (format/organize/fix), rename, debug demos
 * - Progress UI, output channel, status bar, config-sync, file watcher
 * - Hardened: no-op when no editor, consistent error handling, strict types
 */

import * as path from "node:path";
import * as fs from "node:fs";
import * as vscode from "vscode";
import { registerDiagnosticsView } from "./diagnosticsView";
import { registerModuleExplorerView } from "./moduleExplorerView";
import { VitteProjectTreeProvider } from "./providers/tree/projectTree";
import { PlaygroundPanel } from "./providers/playgroundPanel";
import { registerBuildTasks } from "./tasks/buildTasks";
import { registerBenchTasks } from "./tasks/benchTasks";
import { registerRuntimeLocatorCommand } from "./debug/runtimeLocator";
import { registerDebugFactory } from "./debug/adapterFactory";
import { registerDebugConfigurationProvider } from "./debug/configurationProvider";
import { registerTelemetry } from "./utils/telemetry";
import { registerQuickActions } from "./commands/quickActions";
import { registerMetricsView } from "./providers/metricsView";
import {
  LanguageClient,
  TransportKind,
  RevealOutputChannelOn,
  State as ClientState,
} from "vscode-languageclient/node";
import type {
  LanguageClientOptions,
  ServerOptions,
  DocumentSelector,
  ProvideDocumentFormattingEditsSignature,
} from "vscode-languageclient/node";
import {
  summarizeWorkspaceDiagnostics,
  diagnosticsLevel,
  formatDiagnosticsSummary,
} from "./utils/diagnostics";
import type { ServerMetricEntry } from "./types/metrics";

let client: LanguageClient | undefined;
let output: vscode.OutputChannel;
let statusItem: vscode.StatusBarItem;

let statusBaseIcon = "$(rocket)";
const STATUS_LABEL = "Vitte";
let statusLanguageSuffix = "";
let statusBaseTooltip = "Vitte Language Server";
let statusHealthIcon = "";
let statusHealthTooltip = "";
let statusOverrideText: string | undefined;
let statusOverrideTooltip: string | undefined;

export interface ExtensionApi {
  getStatusText(): string;
  getStatusTooltip(): string;
  getClientState(): ClientState | undefined;
  runAction(action: string): Promise<void>;
  restart(): Promise<void>;
  resolveServerModuleForTest(ctx: Pick<vscode.ExtensionContext, "asAbsolutePath">): string;
}

const LANGUAGES = ["vitte", "vit"] as const;
const WATCH_PATTERNS = [
  "**/*.{vitte,vit}",
  "**/vitte.toml",
  "**/.vitteconfig"
] as const;
const LANGUAGE_SET = new Set<string>(LANGUAGES);

interface CommandMenuEntry {
  label: string;
  description?: string;
  detail?: string;
  command: string;
}

interface CommandMenuItem extends vscode.QuickPickItem {
  command: string;
}

interface CommandShortcutConfig {
  label: string;
  command: string;
  icon?: string;
  tooltip?: string;
  statusBar?: boolean;
  startup?: boolean;
}

interface CommandMessageItem extends vscode.MessageItem {
  command?: string;
}

const DEFAULT_COMMAND_SHORTCUTS: readonly CommandShortcutConfig[] = [
  { label: "Build", command: "vitte.build", icon: "$(tools)", tooltip: "Build the workspace (vitte.build)", statusBar: true, startup: true },
  { label: "Run", command: "vitte.run", icon: "$(debug-start)", tooltip: "Build and run the entry point (vitte.run)", statusBar: true, startup: true },
  { label: "Test", command: "vitte.test", icon: "$(beaker)", tooltip: "Execute the test suite (vitte.test)", statusBar: true, startup: true },
] as const;

const COMMAND_MENU_ENTRIES: readonly CommandMenuEntry[] = [
  { label: "Clean workspace", description: "Remove build outputs", command: "vitte.clean" },
  { label: "Bench workspace", description: "Run vitte.bench", command: "vitte.bench" },
  { label: "Open bench report", description: "Latest bench report", command: "vitte.benchReport" },
  { label: "Diagnostics ▸ Refresh", description: "Re-scan diagnostics", command: "vitte.diagnostics.refresh" },
  { label: "Diagnostics ▸ Next issue", description: "Jump to next diagnostic", command: "editor.action.marker.next" },
  { label: "Docs & Playground", description: "Open docs or playground", command: "vitte.openDocs", detail: "vitte.openDocs → playground" },
  { label: "Quick Actions", description: "Interactive menu", command: "vitte.quickActions" },
  { label: "Server log", description: "Open log output", command: "vitte.showServerLog" },
  { label: "Server metrics", description: "Show performance snapshot", command: "vitte.showServerMetrics" },
  { label: "Detect toolchain", description: "Scan for Vitte runtimes", command: "vitte.detectToolchain" },
];

let fileWatchers: vscode.FileSystemWatcher[] = [];
let commandButtonItems: vscode.StatusBarItem[] = [];
let commandMenuButton: vscode.StatusBarItem | undefined;
const DIAGNOSTICS_REFRESH_KEY = "vitte.quickActions.diagRefresh";

function logServerResolution(message: string): void {
  const text = `[vitte] ${message}`;
  output?.appendLine(text);
}

function readCommandShortcuts(): CommandShortcutConfig[] {
  const cfg = vscode.workspace.getConfiguration("vitte");
  const raw = cfg.get<unknown>("commandShortcuts");
  if (!Array.isArray(raw)) {
    return DEFAULT_COMMAND_SHORTCUTS.map((item) => ({ ...item }));
  }
  const sanitized: CommandShortcutConfig[] = [];
  for (const entry of raw) {
    if (!entry || typeof entry !== "object") continue;
    const data = entry as Partial<CommandShortcutConfig>;
    const label = typeof data.label === "string" && data.label.trim().length > 0 ? data.label.trim() : undefined;
    const command = typeof data.command === "string" && data.command.trim().length > 0 ? data.command.trim() : undefined;
    if (!label || !command) continue;
    const shortcut: CommandShortcutConfig = { label, command };
    const icon = typeof data.icon === "string" && data.icon.trim().length > 0 ? data.icon.trim() : undefined;
    if (icon) shortcut.icon = icon;
    const tooltip = typeof data.tooltip === "string" && data.tooltip.trim().length > 0 ? data.tooltip : undefined;
    if (tooltip) shortcut.tooltip = tooltip;
    if (typeof data.statusBar === "boolean") shortcut.statusBar = data.statusBar;
    if (typeof data.startup === "boolean") shortcut.startup = data.startup;
    sanitized.push(shortcut);
  }
  return sanitized.length > 0 ? sanitized : DEFAULT_COMMAND_SHORTCUTS.map((item) => ({ ...item }));
}

function getWorkspaceId(): string | undefined {
  return vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
}

function readDiagnosticsRefreshState(context: vscode.ExtensionContext): { lastRefresh?: number; stale: boolean } {
  const workspaceId = getWorkspaceId();
  if (!workspaceId) return { stale: true };
  const map = context.globalState.get<Record<string, number>>(DIAGNOSTICS_REFRESH_KEY, {});
  const ts = map[workspaceId];
  if (!ts) return { stale: true };
  const stale = Date.now() - ts > 10 * 60 * 1000;
  return { lastRefresh: ts, stale };
}

function labelForCommand(command: string): string {
  switch (command) {
    case "vitte.build": return "Build";
    case "vitte.run": return "Run";
    case "vitte.test": return "Run Tests";
    case "vitte.diagnostics.refresh": return "Refresh Diagnostics";
    default: return command.replace(/^vitte\./, "");
  }
}

function applyStatusBar(): void {
  if (!statusItem) return;
  let text: string;
  let tooltipParts: string[];

  if (statusOverrideText !== undefined) {
    text = statusOverrideText;
    tooltipParts = [statusOverrideTooltip ?? statusBaseTooltip];
  } else {
    const suffix = statusLanguageSuffix ? ` (${statusLanguageSuffix})` : "";
    text = `${statusBaseIcon} ${STATUS_LABEL}${suffix}`;
    tooltipParts = [statusBaseTooltip];
    if (statusOverrideTooltip) {
      tooltipParts.push(statusOverrideTooltip);
    }
  }

  if (statusHealthIcon) {
    text = `${text} ${statusHealthIcon}`;
  }

  if (statusHealthTooltip) {
    tooltipParts.push(statusHealthTooltip);
  }

  statusItem.text = text;
  statusItem.tooltip = tooltipParts.filter(Boolean).join("\n");
  statusItem.accessibilityInformation = {
    label: text.replace(/\$\([^)]+\)/g, "").trim(),
    role: "status"
  };
}

function setStatusBase(icon: string, tooltip: string): void {
  statusBaseIcon = icon;
  statusBaseTooltip = tooltip;
  statusOverrideText = undefined;
  statusOverrideTooltip = undefined;
  applyStatusBar();
}

function setStatusLanguageSuffix(lang?: string): void {
  statusLanguageSuffix = lang ?? "";
  applyStatusBar();
}

function setStatusOverride(text?: string, tooltip?: string): void {
  statusOverrideText = text;
  statusOverrideTooltip = tooltip;
  applyStatusBar();
}

function refreshDiagnosticsStatus(): void {
  const summary = summarizeWorkspaceDiagnostics();
  const level = diagnosticsLevel(summary);
  switch (level) {
    case "error":
      statusHealthIcon = "$(error)";
      break;
    case "warning":
      statusHealthIcon = "$(warning)";
      break;
    default:
      statusHealthIcon = "$(pass-filled)";
  }
  statusHealthTooltip = formatDiagnosticsSummary(summary);
  applyStatusBar();
}

function ensureFileWatchers(context: vscode.ExtensionContext): vscode.FileSystemWatcher[] {
  if (fileWatchers.length === 0) {
    fileWatchers = WATCH_PATTERNS.map((pattern) => {
      const watcher = vscode.workspace.createFileSystemWatcher(pattern);
      context.subscriptions.push(watcher);
      return watcher;
    });
  }
  return fileWatchers;
}

function updateCommandButtons(context: vscode.ExtensionContext): void {
  for (const item of commandButtonItems) {
    try { item.dispose(); } catch { /* noop */ }
  }
  commandButtonItems = [];
  if (commandMenuButton) {
    try { commandMenuButton.dispose(); } catch { /* noop */ }
    commandMenuButton = undefined;
  }
  const shortcuts = readCommandShortcuts();
  const visible = shortcuts.filter((entry) => entry.statusBar !== false);
  let priority = 1000;
  for (const shortcut of visible) {
    const icon = shortcut.icon ?? "$(rocket)";
    const item = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, priority--);
    item.text = `${icon} ${shortcut.label}`;
    item.command = shortcut.command;
    item.tooltip = shortcut.tooltip ?? shortcut.command;
    item.name = `Vitte ${shortcut.label}`;
    item.accessibilityInformation = { label: `${shortcut.label} button`, role: "button" };
    item.show();
    commandButtonItems.push(item);
    context.subscriptions.push(item);
  }
  commandMenuButton = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, priority--);
  commandMenuButton.text = "$(list-selection) Vitte";
  commandMenuButton.command = "vitte.showCommandMenu";
  commandMenuButton.tooltip = "Show additional Vitte commands";
  commandMenuButton.name = "Vitte Command Menu";
  commandMenuButton.accessibilityInformation = { label: "Vitte command menu", role: "button" };
  commandMenuButton.show();
  context.subscriptions.push(commandMenuButton);
}

async function showStartupCommandPrompt(context: vscode.ExtensionContext): Promise<void> {
  if (process.env.VSCODE_TESTING === "1") return;
  const shortcuts = readCommandShortcuts().filter((entry) => entry.startup !== false);
  if (shortcuts.length === 0) return;
  const diagSummary = summarizeWorkspaceDiagnostics();
  const diagState = readDiagnosticsRefreshState(context);
  const recommendedCommands = new Set<string>();
  if (diagSummary.errors > 0) {
    recommendedCommands.add("vitte.test");
  }
  if (diagState.stale || diagSummary.warnings > 0) {
    recommendedCommands.add("vitte.diagnostics.refresh");
  }
  const items: CommandMessageItem[] = [];
  const seen = new Set<string>();
  const ordered = [...shortcuts].sort((a, b) => {
    const aRec = recommendedCommands.has(a.command);
    const bRec = recommendedCommands.has(b.command);
    if (aRec && !bRec) return -1;
    if (!aRec && bRec) return 1;
    return a.label.localeCompare(b.label);
  });
  for (const shortcut of ordered) {
    if (seen.has(shortcut.command)) continue;
    const recommended = recommendedCommands.has(shortcut.command);
    items.push({
      title: recommended ? `⭐ ${shortcut.label}` : shortcut.label,
      command: shortcut.command,
    });
    seen.add(shortcut.command);
  }
  for (const command of recommendedCommands) {
    if (seen.has(command)) continue;
    items.unshift({
      title: `⭐ ${labelForCommand(command)}`,
      command,
    });
    seen.add(command);
  }
  if (items.length === 0) return;

  const moreItem: CommandMessageItem = { title: "More…" };
  const dismissItem: CommandMessageItem = { title: "Dismiss", isCloseAffordance: true };
  const selection = await vscode.window.showInformationMessage<CommandMessageItem>(
    "Vitte is ready — run a command:",
    ...items,
    moreItem,
    dismissItem
  );
  if (!selection || selection === dismissItem) return;
  if (selection === moreItem) {
    await vscode.commands.executeCommand("vitte.showCommandMenu");
    return;
  }
  if (selection.command) {
    await vscode.commands.executeCommand(selection.command);
  }
}
export async function activate(context: vscode.ExtensionContext): Promise<ExtensionApi | undefined> {
  output = vscode.window.createOutputChannel("Vitte Language Server", { log: true });
  statusItem = vscode.window.createStatusBarItem("vitte.status", vscode.StatusBarAlignment.Right, 100);
  statusItem.name = "Vitte LSP";
  statusItem.command = "vitte.showServerLog";
  context.subscriptions.push(output, statusItem);
  setStatusBase("$(rocket)", "Vitte Language Server");
  refreshDiagnosticsStatus();
  statusItem.show();
  updateCommandButtons(context);
  void showStartupCommandPrompt(context);

  await startClient(context);

  // Debug & runtime tooling
  registerDebugConfigurationProvider(context);
  registerDebugFactory(context);
  registerRuntimeLocatorCommand(context);
  registerBuildTasks(context);
  registerBenchTasks(context);
  registerQuickActions(context);
  await registerTelemetry(context);

  // Sidebar: Explorateur Vitte (activity bar)
  const vitteTree = new VitteProjectTreeProvider(context);
  context.subscriptions.push(
    vscode.window.registerTreeDataProvider('vitteExplorer', vitteTree)
  );

  // Toolbar + palette commands for the view
  context.subscriptions.push(
    vscode.commands.registerCommand('vitte.refreshExplorer', () => vitteTree.refresh()),
    vscode.commands.registerCommand('vitte.openDocs', () => {
      const uri = vscode.Uri.file(path.join(context.extensionPath, 'media', 'docs.html'));
      return vscode.commands.executeCommand('vscode.open', uri);
    }),
    vscode.commands.registerCommand('vitte.openPlayground', () => PlaygroundPanel.createOrShow(context))
  );

  // Commandes
  context.subscriptions.push(
    vscode.commands.registerCommand("vitte.showServerLog", () => {
      output.show(true);
    }),
    vscode.commands.registerCommand("vitte.showServerMetrics", async () => {
      if (!client) {
        void vscode.window.showWarningMessage("Vitte server is not running.");
        return;
      }
      try {
        const stats = await client.sendRequest<ServerMetricEntry[]>("vitte/metrics");
        if (!stats || stats.length === 0) {
          void vscode.window.showInformationMessage("Vitte: no metrics available yet.");
          return;
        }
        const timestamp = new Date().toLocaleTimeString();
        output.appendLine(`[metrics] Snapshot ${timestamp}`);
        for (const entry of stats) {
          const avg = entry.averageMs.toFixed(2);
          const last = entry.lastMs.toFixed(2);
          const max = entry.maxMs.toFixed(2);
          const when = entry.lastAt ? new Date(entry.lastAt).toLocaleTimeString() : "n/a";
          const countInfo = typeof entry.lastCount === "number" ? ` n=${entry.lastCount}` : "";
          output.appendLine(
            `  ${entry.name.padEnd(18)} avg=${avg}ms last=${last}ms max=${max}ms count=${entry.count}${countInfo} last=${when} uri=${entry.lastUri}`
          );
        }
        output.show(true);
      } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        void vscode.window.showErrorMessage(`Vitte: unable to fetch server metrics (${message})`);
      }
    }),
    vscode.commands.registerCommand("vitte.showCommandMenu", async () => {
      const pickItems: CommandMenuItem[] = COMMAND_MENU_ENTRIES.map((entry) => {
        const item: CommandMenuItem = {
          label: entry.label,
          command: entry.command,
          detail: entry.detail ?? entry.command,
        };
        if (entry.description) {
          item.description = entry.description;
        }
        return item;
      });
      const pick = await vscode.window.showQuickPick<CommandMenuItem>(pickItems, {
        title: "Vitte commands",
        placeHolder: "Select a command to run",
        matchOnDetail: true,
      });
      if (!pick) return;
      await vscode.commands.executeCommand(pick.command);
    }),
    vscode.commands.registerCommand("vitte.restartServer", async () => {
      await vscode.window.withProgress(
        {
          location: vscode.ProgressLocation.Notification,
          title: "Vitte: restarting language server…",
        },
        async () => {
          await restartClient(context);
        }
      );
      vscode.window.setStatusBarMessage("Vitte server restarted successfully.", 3000);
    }),
    vscode.commands.registerCommand("vitte.runAction", async () => {
      const pick = await vscode.window.showQuickPick([
        {
          label: "Format document",
          description: "editor.action.formatDocument",
          detail: "Apply the configured formatter to the active file.",
          action: "format",
        },
        {
          label: "Organize imports",
          description: "editor.action.organizeImports",
          detail: "Trie et nettoie les imports du document courant.",
          action: "organizeImports",
        },
        {
          label: "Fix all",
          description: "source.fixAll",
          detail: "Run available quick fixes.",
          action: "fixAll",
        }
      ], { title: "Vitte: run a quick action" });
      if (!pick) return;
      await runBuiltinAction(pick.action);
    }),
    vscode.commands.registerCommand("vitte.runActionWithArgs", async () => {
      const action = await vscode.window.showInputBox({ prompt: "Action (format | organizeImports | fixAll)", value: "format" });
      if (!action) return;
      await runBuiltinAction(action.trim());
    }),
    vscode.commands.registerCommand("vitte.formatDocument", async () => runBuiltinAction("format")),
    vscode.commands.registerCommand("vitte.organizeImports", async () => runBuiltinAction("organizeImports")),
    vscode.commands.registerCommand("vitte.fixAll", async () => runBuiltinAction("fixAll")),
    vscode.commands.registerCommand("vitte.renameSymbol", async () => {
      if (!vscode.window.activeTextEditor) return;
      await vscode.commands.executeCommand("editor.action.rename");
    }),
    vscode.commands.registerCommand("vitte.applyEditSample", async () => {
      const editor = vscode.window.activeTextEditor; if (!editor) return;
      const edit = new vscode.WorkspaceEdit();
      edit.insert(editor.document.uri, new vscode.Position(0, 0), "// Edited by Vitte sample\n");
      await vscode.workspace.applyEdit(edit);
    }),
    vscode.commands.registerCommand("vitte.progressSample", async () => {
      await vscode.window.withProgress({ location: vscode.ProgressLocation.Window, title: "Vitte: Running task" }, async (p) => {
        p.report({ message: "Step 1/3" }); await sleep(250);
        p.report({ message: "Step 2/3" }); await sleep(250);
        p.report({ message: "Step 3/3" }); await sleep(250);
      });
    }),
    vscode.commands.registerCommand("vitte.showInfo", async () => {
      const cfg = vscode.workspace.getConfiguration("vitte");
      const trace = cfg.get<string>("trace.server", "off");
      await vscode.window.showInformationMessage(`Vitte LSP — trace: ${trace}`);
    }),
    vscode.commands.registerCommand("vitte.debug.runFile", async () => { await runDebugCurrentFile(); }),
    vscode.commands.registerCommand("vitte.debug.attachServer", async () => { await attachDebugServer(); }),
  );

  // Refresh status depending on the active editor
  context.subscriptions.push(vscode.window.onDidChangeActiveTextEditor(updateStatusText));
  updateStatusText(vscode.window.activeTextEditor ?? undefined);

  // Relance si config Vitte change
  context.subscriptions.push(vscode.workspace.onDidChangeConfiguration(async (e) => {
    if (e.affectsConfiguration("vitte.commandShortcuts")) {
      updateCommandButtons(context);
    }
    if (e.affectsConfiguration("vitte")) {
      await restartClient(context);
    }
  }));

  // Diagnostics view for both beginners and power users
  registerDiagnosticsView(context);
  registerModuleExplorerView(context);
  registerMetricsView(context, () => client);
  context.subscriptions.push(vscode.languages.onDidChangeDiagnostics(() => refreshDiagnosticsStatus()));

  if (process.env.VSCODE_TESTING === "1") {
    const api: ExtensionApi = {
      getStatusText: () => statusItem?.text ?? "",
      getStatusTooltip: () => {
        const tip = statusItem?.tooltip;
        if (typeof tip === "string") return tip;
        if (tip instanceof vscode.MarkdownString) {
          return tip.value ?? "";
        }
        return "";
      },
      getClientState: () => client?.state,
      runAction: async (action: string) => {
        await runBuiltinAction(action);
      },
      restart: async () => {
        await restartClient(context);
      },
      resolveServerModuleForTest: (ctx) => resolveServerModule(ctx as vscode.ExtensionContext),
    };
    return api;
  }

  return undefined;
}

export async function deactivate(): Promise<void> {
  try { await client?.stop(); } catch { /* noop */ }
  client = undefined;
  for (const watcher of fileWatchers) {
    try { watcher.dispose(); } catch { /* noop */ }
  }
  fileWatchers = [];
}

/* --------------------------------- LSP ----------------------------------- */

function resolveServerModule(context: vscode.ExtensionContext): string {
  // Permet d’overrider via settings: vitte.serverPath
  const cfgPath = vscode.workspace.getConfiguration("vitte").get<string>("serverPath");
  if (cfgPath) {
    if (fs.existsSync(cfgPath)) {
      logServerResolution(`Using custom server: ${cfgPath}`);
      return cfgPath;
    }
    logServerResolution(`Custom server path not found: ${cfgPath}`);
  }
  const nested = context.asAbsolutePath(path.join("server", "out", "server.js"));
  if (fs.existsSync(nested)) {
    logServerResolution(`Using packaged server (server/out): ${nested}`);
    return nested;
  }
  const bundled = context.asAbsolutePath(path.join("out", "server.js"));
  if (fs.existsSync(bundled)) {
    logServerResolution(`Using embedded server: ${bundled}`);
    return bundled;
  }
  const message = "Module serveur Vitte introuvable (out/server.js ou server/out/server.js)";
  logServerResolution(message);
  throw new Error(message);
}

async function startClient(context: vscode.ExtensionContext): Promise<void> {
  if (client) return; // already running

  const serverModule = resolveServerModule(context);
  const debugOptions = { execArgv: ["--nolazy", "--inspect=6009"] };
  const serverOptions: ServerOptions = {
    run:   { module: serverModule, transport: TransportKind.ipc },
    debug: { module: serverModule, transport: TransportKind.ipc, options: debugOptions },
  };

  const documentSelector: DocumentSelector = LANGUAGES.flatMap((id) => ([
    { scheme: "file", language: id },
    { scheme: "untitled", language: id },
    { scheme: "vscode-notebook-cell", language: id }
  ]));
  const watchers = ensureFileWatchers(context);

  const clientOptions: LanguageClientOptions = {
    documentSelector,
    outputChannel: output,
    revealOutputChannelOn: RevealOutputChannelOn.Never,
    synchronize: {
      configurationSection: "vitte",
      fileEvents: watchers
    },
    middleware: {
      provideDocumentFormattingEdits: async (
        doc: vscode.TextDocument,
        options: vscode.FormattingOptions,
        token: vscode.CancellationToken,
        next: ProvideDocumentFormattingEditsSignature
      ) => {
        try { return await next(doc, options, token); } catch {
          await vscode.commands.executeCommand("editor.action.formatDocument");
          return [];
        }
      },
    },
    initializationOptions: {
      // Extension → Serveur: options d’init (libre)
    },
  };

  client = new LanguageClient("vitte-lsp", "Vitte Language Server", serverOptions, clientOptions);

  client.onTelemetry((e: unknown) => {
    output.appendLine(`[telemetry] ${JSON.stringify(e)}`);
  });

  wireClientState(client);

  await client.start();
}

async function restartClient(context: vscode.ExtensionContext): Promise<void> {
  if (client) {
    setStatusBase("$(sync)", "Vitte LSP: restarting…");
    try { await client.stop(); } catch { /* noop */ }
    client = undefined;
  }
  await startClient(context);
}

function wireClientState(c: LanguageClient): void {
  c.onDidChangeState((e: { oldState: ClientState; newState: ClientState }) => {
    if (e.newState === ClientState.Starting) {
      setStatusBase("$(gear)", "Vitte LSP: starting");
    } else if (e.newState === ClientState.Running) {
      setStatusBase("$(check)", "Vitte LSP: running");
    } else if (e.newState === ClientState.Stopped) {
      setStatusBase("$(debug-stop)", "Vitte LSP: stopped");
    }
  });

  c.onNotification("vitte/status", (msg: { text?: string; tooltip?: string }) => {
    const text = typeof msg?.text === "string" ? msg.text : undefined;
    const tooltip = typeof msg?.tooltip === "string" ? msg.tooltip : undefined;
    if (text !== undefined || tooltip !== undefined) {
      setStatusOverride(text, tooltip);
    }
  });

  c.onNotification("vitte/log", (msg: unknown) => {
    output.appendLine(typeof msg === "string" ? msg : JSON.stringify(msg));
  });
}

/* ----------------------------- Actions utilitaires ------------------------ */

async function runBuiltinAction(action: string): Promise<void> {
  const editor = vscode.window.activeTextEditor;
  if (!editor) {
    void vscode.window.showWarningMessage("Open a Vitte/Vit document before running this action.");
    return;
  }
  const languageId = editor.document.languageId;
  if (!LANGUAGE_SET.has(languageId)) {
    void vscode.window.showWarningMessage("Les actions Vitte ne sont disponibles que pour les fichiers Vitte/Vit.");
    return;
  }
  switch (action) {
    case "format":
      await vscode.commands.executeCommand("editor.action.formatDocument");
      return;
    case "organizeImports":
      await vscode.commands.executeCommand("editor.action.organizeImports");
      return;
    case "fixAll":
      await vscode.commands.executeCommand("editor.action.codeAction", {
        kind: vscode.CodeActionKind.SourceFixAll.value,
        apply: "first"
      });
      return;
    default:
      void vscode.window.showWarningMessage(`Action inconnue: ${action}`);
      return;
  }
}

function sleep(ms: number): Promise<void> { return new Promise(res => setTimeout(res, ms)); }

function updateStatusText(editor?: vscode.TextEditor): void {
  const lang = editor?.document?.languageId;
  if (lang && LANGUAGE_SET.has(lang)) {
    setStatusLanguageSuffix(lang);
    return;
  }
  setStatusLanguageSuffix("");
}

/* -------------------------------- Debug demo ------------------------------ */

async function runDebugCurrentFile(): Promise<void> {
  const editor = vscode.window.activeTextEditor; if (!editor) return;
  const folder = vscode.workspace.workspaceFolders?.[0];
  const cfg: vscode.DebugConfiguration = {
    type: "vitte",
    name: "Vitte: Launch current file",
    request: "launch",
    program: editor.document.fileName,
    cwd: folder?.uri.fsPath ?? path.dirname(editor.document.fileName),
    stopOnEntry: true,
    args: []
  };
  await vscode.debug.startDebugging(folder, cfg);
}

async function attachDebugServer(): Promise<void> {
  const portStr = await vscode.window.showInputBox({ prompt: "Port du serveur Vitte", value: "9333" });
  if (!portStr) return;
  const folder = vscode.workspace.workspaceFolders?.[0];
  const cfg: vscode.DebugConfiguration = {
    type: "vitte",
    name: "Vitte: Attach",
    request: "attach",
    port: Number.parseInt(portStr, 10),
  };
  if (!Number.isInteger(cfg.port) || (cfg.port as number) <= 0) {
    void vscode.window.showErrorMessage("Port Vitte invalide.");
    return;
  }
  await vscode.debug.startDebugging(folder, cfg);
}
