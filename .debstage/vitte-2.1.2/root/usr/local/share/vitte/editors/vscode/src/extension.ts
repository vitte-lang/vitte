/*
 * extension.ts — VS Code client for Vitte/Vit
 * - Full LSP client (start/stop/restart, traces, status, watchers)
 * - Commands: logs, restart, actions (format/organize/fix), rename, debug demos
 * - Progress UI, output channel, status bar, config-sync, file watcher
 * - Hardened: no-op when no editor, consistent error handling, strict types
 */

import * as path from "node:path";
import * as fs from "node:fs";
import * as os from "node:os";
import * as vscode from "vscode";
import { registerDiagnosticsView } from "./diagnosticsView";
import { registerModuleExplorerView } from "./moduleExplorerView";
import { PlaygroundPanel } from "./providers/playgroundPanel";
import { registerOfflineView } from "./providers/offlineView";
import { registerBuildTasks } from "./tasks/buildTasks";
import { registerBenchTasks } from "./tasks/benchTasks";
import { registerRuntimeLocatorCommand } from "./debug/runtimeLocator";
import { registerDebugFactory } from "./debug/adapterFactory";
import { registerDebugConfigurationProvider } from "./debug/configurationProvider";
import { registerTelemetry } from "./utils/telemetry";
import { registerQuickActions } from "./commands/quickActions";
import { registerMetricsView } from "./providers/metricsView";
import { registerDocumentationView } from "./providers/documentationView";
import { registerDocsChaptersView } from "./providers/docsChaptersView";
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
let offlineReason: string | undefined;
let offlineBannerShown = false;
const recentStops: number[] = [];
const OFFLINE_STATUS_COLOR = new vscode.ThemeColor("statusBarItem.warningForeground");
const OFFLINE_STATUS_BG = new vscode.ThemeColor("statusBarItem.warningBackground");
let offlineRetryTimer: NodeJS.Timeout | undefined;
let offlineRetryMs = 60000;
let lastActivationContext: vscode.ExtensionContext | undefined;
let editorLintCollection: vscode.DiagnosticCollection | undefined;
let offlineSince: number | undefined;
let documentationViewsRegistered = false;
let documentationViewsRegistrationError: string | undefined;

interface ExtensionManifest {
  version?: string;
}

function getExtensionVersion(ext: vscode.Extension<unknown> | undefined): string {
  const manifest: unknown = ext?.packageJSON;
  if (!manifest || typeof manifest !== "object") return "unknown";
  const version = (manifest as ExtensionManifest).version;
  return typeof version === "string" && version.length > 0 ? version : "unknown";
}

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
  { label: "Documentation", description: "Open vitte.netlify.app", command: "vitte.openDocs", detail: "Online docs, auto-updated from website" },
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
  if (offlineReason) {
    statusItem.color = OFFLINE_STATUS_COLOR;
    statusItem.backgroundColor = OFFLINE_STATUS_BG;
  } else {
    statusItem.color = undefined;
    statusItem.backgroundColor = undefined;
  }
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

function isOfflineEnabled(): boolean {
  return vscode.workspace.getConfiguration("vitte").get<boolean>("server.offline", false);
}

function isOfflinePermanent(): boolean {
  return vscode.workspace.getConfiguration("vitte").get<boolean>("server.offlinePermanent", false);
}

function isOfflineEffective(): boolean {
  return isOfflineEnabled() || isOfflinePermanent();
}

function setOfflineStatus(reason: string): void {
  offlineReason = reason;
  offlineSince = offlineSince ?? Date.now();
  setStatusBase("$(circle-slash)", "Vitte LSP: offline");
  const forced = isOfflinePermanent() ? "Offline permanent (user-forced)" : "Offline";
  const since = offlineSince ? ` since ${new Date(offlineSince).toLocaleTimeString()}` : "";
  setStatusOverride(`$(circle-slash) Vitte ${forced.toUpperCase()}`, `${reason}${since}`);
  void setServerOnlineContext(false);
  logOfflineReason(reason);
  void showOfflineBanner(reason);
  scheduleOfflineRetry();
  void vscode.commands.executeCommand("vitte.offline.refresh");
}

async function setServerOnlineContext(online: boolean): Promise<void> {
  try { await vscode.commands.executeCommand("setContext", "vitte.serverOnline", online); } catch { /* noop */ }
  try { await vscode.commands.executeCommand("setContext", "vitte.serverOffline", !online); } catch { /* noop */ }
}

function logOfflineReason(reason: string): void {
  try { output.appendLine(`[offline] ${reason}`); } catch { /* noop */ }
  void appendOfflineLog(reason);
}

async function showOfflineBanner(reason: string): Promise<void> {
  if (offlineBannerShown) return;
  const mute = vscode.workspace.getConfiguration("vitte").get<boolean>("server.offlineMuteBanner", false);
  if (mute) return;
  offlineBannerShown = true;
  const selection = await vscode.window.showWarningMessage(
    `Vitte server offline: ${reason}`,
    "Explain Offline",
    "Open Settings"
  );
  if (selection === "Explain Offline") {
    await vscode.commands.executeCommand("vitte.offline.explain");
  } else if (selection === "Open Settings") {
    void vscode.commands.executeCommand("workbench.action.openSettings", "vitte.server");
  }
}

async function setOfflineMode(enabled: boolean, reason?: string): Promise<void> {
  const config = vscode.workspace.getConfiguration("vitte");
  const hasWorkspace = Boolean(vscode.workspace.workspaceFolders?.length);
  const target = hasWorkspace ? vscode.ConfigurationTarget.Workspace : vscode.ConfigurationTarget.Global;
  await config.update("server.offline", enabled, target);
  if (enabled) {
    setOfflineStatus(reason ?? "Offline mode enabled.");
  } else {
    offlineReason = undefined;
    offlineBannerShown = false;
    offlineSince = undefined;
    cancelOfflineRetry();
    statusItem.color = undefined;
    statusItem.backgroundColor = undefined;
    setStatusBase("$(rocket)", "Vitte Language Server");
    await setServerOnlineContext(true);
  }
}

function scheduleOfflineRetry(): void {
  if (isOfflineEffective()) return;
  const cfg = vscode.workspace.getConfiguration("vitte");
  const enabled = cfg.get<boolean>("server.autoRetry", true);
  if (!enabled) return;
  const base = clampNumber(cfg.get<number>("server.autoRetryBaseMs", 60000), 10000, 300000, 60000);
  const max = clampNumber(cfg.get<number>("server.autoRetryMaxMs", 300000), base, 900000, 300000);
  if (!offlineRetryTimer) {
    offlineRetryMs = Math.max(base, offlineRetryMs);
    offlineRetryMs = Math.min(offlineRetryMs, max);
    offlineRetryTimer = setTimeout(() => {
      offlineRetryTimer = undefined;
      void (async () => {
        try {
          const ok = await restartClient(lastActivationContext ?? undefined);
          if (ok) {
            offlineRetryMs = base;
            cancelOfflineRetry();
            return;
          }
          throw new Error("restart failed");
        } catch {
          offlineRetryMs = Math.min(offlineRetryMs * 2, max);
          scheduleOfflineRetry();
        }
      })();
    }, offlineRetryMs);
  }
}

function cancelOfflineRetry(): void {
  if (offlineRetryTimer) clearTimeout(offlineRetryTimer);
  offlineRetryTimer = undefined;
  offlineRetryMs = 60000;
}

async function appendOfflineLog(reason: string): Promise<void> {
  try {
    const folder = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    const base = folder ?? os.tmpdir();
    const dir = path.join(base, ".vitte");
    await fs.promises.mkdir(dir, { recursive: true });
    const file = path.join(dir, "offline.log");
    const line = `${new Date().toISOString()} ${reason}\n`;
    await fs.promises.appendFile(file, line, "utf8");
  } catch { /* noop */ }
}

function clampNumber(value: number, min: number, max: number, fallback: number): number {
  if (!Number.isFinite(value)) return fallback;
  return Math.min(max, Math.max(min, value));
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
  lastActivationContext = context;
  try {
    output = vscode.window.createOutputChannel("Vitte Language Server", { log: true });
  } catch {
    output = vscode.window.createOutputChannel("Vitte Language Server");
  }
  output.appendLine("[activate] begin");
  output.appendLine("[activate] Vitte extension activated");
  try {
    statusItem = vscode.window.createStatusBarItem("vitte.status", vscode.StatusBarAlignment.Right, 100);
  } catch {
    statusItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
  }
  statusItem.name = "Vitte LSP";
  statusItem.command = "vitte.showServerLog";
  context.subscriptions.push(output, statusItem);
  setStatusBase("$(rocket)", "Vitte Language Server");
  refreshDiagnosticsStatus();
  statusItem.show();
  updateCommandButtons(context);
  void showStartupCommandPrompt(context);
  void setServerOnlineContext(false);

  // Register command shortcuts early so they're available even if later init fails.
  context.subscriptions.push(
    vscode.commands.registerCommand('vitte.openDocs', () => vscode.env.openExternal(vscode.Uri.parse("https://vitte.netlify.app"))),
    vscode.commands.registerCommand("vitte.docs.openChapter", async (url?: string) => {
      if (typeof url !== "string" || url.length === 0) return;
      await vscode.env.openExternal(vscode.Uri.parse(url));
    }),
    vscode.commands.registerCommand('vitte.openPlayground', () => PlaygroundPanel.createOrShow(context)),
    vscode.commands.registerCommand("vitte.debugActivationStatus", async () => {
      const ext =
        vscode.extensions.getExtension("vittestudio.vitte-studio")
        ?? vscode.extensions.getExtension("VitteStudio.vitte-studio");
      const viewsRegistered = documentationViewsRegistered ? "yes" : "no";
      const viewErr = documentationViewsRegistrationError ?? "none";
      const clientState = client ? ClientState[client.state] : "none";
      const details = [
        `id=${ext?.id ?? "unknown"}`,
        `version=${getExtensionVersion(ext)}`,
        `isActive=${String(ext?.isActive ?? false)}`,
        `clientState=${clientState}`,
        `documentationViewsRegistered=${viewsRegistered}`,
        `documentationViewsRegistrationError=${viewErr}`,
      ].join("\n");
      output.appendLine(`[activate-debug]\n${details}`);
      output.show(true);
      await vscode.window.showInformationMessage(`Vitte activation status:\n${details}`);
    })
  );
  output.appendLine("[activate] command registered: vitte.openDocs");

  // Register documentation views early (do not depend on server start)
  const viewRegistrationErrors: string[] = [];
  try {
    registerDocumentationView(context, "vitteExplorer", "Vitte Documentation");
  } catch (err) {
    const message = err instanceof Error ? err.message : String(err);
    viewRegistrationErrors.push(`vitteExplorer: ${message}`);
    output.appendLine(`[activate] view registration failed (vitteExplorer): ${message}`);
  }
  try {
    registerDocsChaptersView(context, "vitteSuggestions");
  } catch (err) {
    const message = err instanceof Error ? err.message : String(err);
    viewRegistrationErrors.push(`vitteSuggestions: ${message}`);
    output.appendLine(`[activate] view registration failed (vitteSuggestions): ${message}`);
  }
  documentationViewsRegistered = viewRegistrationErrors.length === 0;
  documentationViewsRegistrationError = viewRegistrationErrors.length > 0
    ? viewRegistrationErrors.join(" | ")
    : undefined;

  void startClient(context).catch((err) => {
    const message = err instanceof Error ? err.message : String(err);
    output.appendLine(`[activate] startClient failed: ${message}`);
  });
  if (isOfflineEffective()) {
    void showOfflineBanner(offlineReason ?? (isOfflinePermanent()
      ? "Offline permanent (user-forced)."
      : "Offline mode is enabled (vitte.server.offline)."));
  }

  editorLintCollection = vscode.languages.createDiagnosticCollection("vitte-lint");
  context.subscriptions.push(editorLintCollection);
  context.subscriptions.push(vscode.workspace.onDidOpenTextDocument((doc) => updateEditorLint(doc)));
  context.subscriptions.push(vscode.workspace.onDidChangeTextDocument((e) => updateEditorLint(e.document)));
  context.subscriptions.push(vscode.workspace.onDidCloseTextDocument((doc) => editorLintCollection?.delete(doc.uri)));
  for (const doc of vscode.workspace.textDocuments) updateEditorLint(doc);

  // Debug & runtime tooling
  registerDebugConfigurationProvider(context);
  registerDebugFactory(context);
  registerRuntimeLocatorCommand(context);
  registerBuildTasks(context);
  registerBenchTasks(context);
  registerQuickActions(context);
  try {
    await registerTelemetry(context);
  } catch (err) {
    const message = err instanceof Error ? err.message : String(err);
    output.appendLine(`[activate] telemetry init failed: ${message}`);
  }

  // Commandes
  context.subscriptions.push(
    vscode.commands.registerCommand("vitte.showServerLog", () => {
      output.show(true);
    }),
    vscode.commands.registerCommand("vitte.showServerMetrics", async () => {
      if (isOfflineEffective()) return showOfflineNoop("metrics");
      if (!client) {
        const reason = offlineReason ? ` (${offlineReason})` : "";
        void vscode.window.showWarningMessage(`Vitte server is not running.${reason}`);
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
          const p99 = typeof entry.p99Ms === "number" ? entry.p99Ms.toFixed(2) : null;
          const when = entry.lastAt ? new Date(entry.lastAt).toLocaleTimeString() : "n/a";
          const countInfo = typeof entry.lastCount === "number" ? ` n=${entry.lastCount}` : "";
          const errInfo = entry.errorCount ? ` errors=${entry.errorCount}` : "";
          const p99Info = p99 ? ` p99=${p99}ms` : "";
          const lastErr = entry.lastError ? ` lastErr="${entry.lastError}"` : "";
          output.appendLine(
            `  ${entry.name.padEnd(18)} avg=${avg}ms last=${last}ms max=${max}ms${p99Info} count=${entry.count}${countInfo}${errInfo} last=${when} uri=${entry.lastUri}${lastErr}`
          );
        }
        output.show(true);
      } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        void vscode.window.showErrorMessage(`Vitte: unable to fetch server metrics (${message})`);
      }
    }),
    vscode.commands.registerCommand("vitte.metrics.reset", async () => {
      if (isOfflineEffective()) return showOfflineNoop("metrics reset");
      if (!client) {
        void vscode.window.showWarningMessage("Vitte server is not running.");
        return;
      }
      try {
        await client.sendRequest("vitte/metrics.reset");
        void vscode.window.showInformationMessage("Vitte: metrics reset.");
        void vscode.commands.executeCommand("vitte.metrics.refresh");
      } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        void vscode.window.showErrorMessage(`Vitte: unable to reset metrics (${message})`);
      }
    }),
    vscode.commands.registerCommand("vitte.pingServer", async () => {
      if (isOfflineEffective()) return showOfflineNoop("ping");
      if (!client) {
        void vscode.window.showWarningMessage("Vitte server is not running.");
        return;
      }
      try {
        const res = await client.sendRequest<{ ok: boolean; ts: number }>("vitte/ping");
        void vscode.window.showInformationMessage(`Vitte: pong (${res.ok ? "ok" : "fail"}) at ${new Date(res.ts).toLocaleTimeString()}`);
      } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        void vscode.window.showErrorMessage(`Vitte: ping failed (${message})`);
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
      if (isOfflinePermanent()) {
        return showOfflineNoop("restart");
      }
      await vscode.window.withProgress(
        {
          location: vscode.ProgressLocation.Notification,
          title: "Vitte: restarting language server…",
        },
        async () => {
          const ok = await restartClient(context);
          if (!ok) {
            showOfflineNoop("restart");
          }
        }
      );
      vscode.window.setStatusBarMessage("Vitte server restart attempted.", 3000);
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
      const offline = cfg.get<boolean>("server.offline", false);
      const offlinePermanent = cfg.get<boolean>("server.offlinePermanent", false);
      const offlineMsg = offlinePermanent ? " (offline permanent)" : (offline ? " (offline)" : "");
      await vscode.window.showInformationMessage(`Vitte LSP — trace: ${trace}${offlineMsg}`);
    }),
    vscode.commands.registerCommand("vitte.offline.explain", async () => {
      const uri = vscode.Uri.file(path.join(context.extensionPath, "media", "offline.md"));
      await vscode.commands.executeCommand("vscode.open", uri);
      void vscode.commands.executeCommand("vitte.offline.openLog");
    }),
    vscode.commands.registerCommand("vitte.offline.copyReport", async () => {
      const report = await readOfflineReport();
      await vscode.env.clipboard.writeText(report);
      void vscode.window.showInformationMessage("Vitte: offline report copied to clipboard.");
    }),
    vscode.commands.registerCommand("vitte.goOffline", async () => {
      await setOfflineMode(true, "Manual offline mode enabled.");
      if (client) {
        try { await client.stop(); } catch { /* noop */ }
        client = undefined;
      }
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
    if (e.affectsConfiguration("vitte.server.offlinePermanent")) {
      if (isOfflinePermanent()) {
        setOfflineStatus("Offline permanent (user-forced).");
      }
    }
    if (e.affectsConfiguration("vitte.lint") || e.affectsConfiguration("vitte.features.lint") || e.affectsConfiguration("vitte.server.offline") || e.affectsConfiguration("vitte.server.offlinePermanent")) {
      for (const doc of vscode.workspace.textDocuments) updateEditorLint(doc);
    }
  }));

  // Diagnostics view for both beginners and power users
  registerDiagnosticsView(context);
  registerModuleExplorerView(context);
  registerMetricsView(context, () => client);
  registerOfflineView(
    context,
    () => offlineReason,
    () => output,
    () => formatOfflineSince(),
    () => {
      const summary = summarizeWorkspaceDiagnostics();
      const total = summary.errors + summary.warnings + summary.info + summary.hints;
      if (total === 0) return "No local diagnostics";
      return `${summary.errors} errors, ${summary.warnings} warnings`;
    }
  );
  context.subscriptions.push(vscode.languages.onDidChangeDiagnostics(() => refreshDiagnosticsStatus()));

  if (isOfflineEnabled()) {
    setOfflineStatus("Offline mode is enabled (vitte.server.offline).");
  }
  if (isOfflinePermanent()) {
    setOfflineStatus("Offline permanent (user-forced).");
  }

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
  recentStops.length = 0;
  offlineBannerShown = false;
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
  const nested = context.asAbsolutePath(path.join("server", "out", "src", "server.js"));
  if (fs.existsSync(nested)) {
    logServerResolution(`Using packaged server (server/out/src): ${nested}`);
    return nested;
  }
  const legacyNested = context.asAbsolutePath(path.join("server", "out", "server.js"));
  if (fs.existsSync(legacyNested)) {
    logServerResolution(`Using legacy server (server/out): ${legacyNested}`);
    return legacyNested;
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

async function startClient(context: vscode.ExtensionContext | undefined): Promise<boolean> {
  if (client) return true; // already running
  if (!context) return false;

  if (isOfflineEffective()) {
    const reason = isOfflinePermanent()
      ? "Offline permanent (user-forced)."
      : "Offline mode is enabled (vitte.server.offline).";
    setOfflineStatus(reason);
    return false;
  }

  let serverModule: string;
  try {
    serverModule = resolveServerModule(context);
  } catch (err) {
    const msg = err instanceof Error ? err.message : String(err);
    setOfflineStatus(msg);
    void vscode.window.showWarningMessage(`Vitte: server offline — ${msg}`);
    return false;
  }
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
  return true;
}

async function restartClient(context: vscode.ExtensionContext | undefined): Promise<boolean> {
  if (client) {
    setStatusBase("$(sync)", "Vitte LSP: restarting…");
    try { await client.stop(); } catch { /* noop */ }
    client = undefined;
  }
  if (isOfflineEffective()) {
    const reason = isOfflinePermanent()
      ? "Offline permanent (user-forced)."
      : "Offline mode is enabled (vitte.server.offline).";
    setOfflineStatus(reason);
    return false;
  }
  return startClient(context);
}

function wireClientState(c: LanguageClient): void {
  c.onDidChangeState((e: { oldState: ClientState; newState: ClientState }) => {
    if (e.newState === ClientState.Starting) {
      setStatusBase("$(gear)", "Vitte LSP: starting");
      void setServerOnlineContext(false);
    } else if (e.newState === ClientState.Running) {
      offlineReason = undefined;
      setStatusBase("$(check)", "Vitte LSP: running");
      void setServerOnlineContext(true);
    } else if (e.newState === ClientState.Stopped) {
      setStatusBase("$(debug-stop)", "Vitte LSP: stopped");
      void setServerOnlineContext(false);
      const now = Date.now();
      recentStops.push(now);
      while (recentStops.length) {
        const first = recentStops[0];
        if (first === undefined) break;
        if ((now - first) <= 120000) break;
        recentStops.shift();
      }
      if (!isOfflineEnabled() && recentStops.length >= 3) {
        setOfflineStatus("Server stopped repeatedly (3x in 2 minutes).");
      }
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

function showOfflineNoop(action: string): void {
  const reason = offlineReason ?? (isOfflinePermanent()
    ? "Offline permanent (user-forced)."
    : "Offline mode is enabled (vitte.server.offline).");
  void vscode.window.showWarningMessage(`Vitte: ${action} unavailable while offline — ${reason}`);
}

function formatOfflineSince(): string {
  if (!offlineSince) return "unknown";
  const seconds = Math.floor((Date.now() - offlineSince) / 1000);
  if (seconds < 60) return `${seconds}s`;
  const minutes = Math.floor(seconds / 60);
  if (minutes < 60) return `${minutes}m`;
  const hours = Math.floor(minutes / 60);
  if (hours < 24) return `${hours}h`;
  const days = Math.floor(hours / 24);
  return `${days}d`;
}

async function readOfflineReport(): Promise<string> {
  const cfg = vscode.workspace.getConfiguration("vitte");
  const report: string[] = [];
  report.push(`# Vitte Offline Report`);
  report.push(`- offline: ${isOfflineEffective()}`);
  report.push(`- offlinePermanent: ${isOfflinePermanent()}`);
  report.push(`- offlineSince: ${offlineSince ? new Date(offlineSince).toISOString() : "unknown"}`);
  report.push(`- offlineReason: ${offlineReason ?? "unknown"}`);
  report.push(`- autoRetry: ${cfg.get<boolean>("server.autoRetry", true)}`);
  report.push(`- autoRetryBaseMs: ${cfg.get<number>("server.autoRetryBaseMs", 60000)}`);
  report.push(`- autoRetryMaxMs: ${cfg.get<number>("server.autoRetryMaxMs", 300000)}`);
  report.push(`- workspaceFolders: ${(vscode.workspace.workspaceFolders ?? []).length}`);
  report.push(`- openDocs: ${vscode.workspace.textDocuments.length}`);
  report.push(`- diagnostics (local): ${summarizeWorkspaceDiagnostics().errors} errors, ${summarizeWorkspaceDiagnostics().warnings} warnings`);
  report.push(`- offlineLog: ${getOfflineLogPathSafe()}`);
  const tail = await readOfflineLogTail(30);
  if (tail) {
    report.push(`\n## offline.log (last 30 lines)\n${tail}`);
  }
  return report.join("\n");
}

function getOfflineLogPathSafe(): string {
  try {
    const folder = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    const base = folder ?? os.tmpdir();
    return path.join(base, ".vitte", "offline.log");
  } catch {
    return "unknown";
  }
}

async function readOfflineLogTail(lines: number): Promise<string> {
  try {
    const filePath = getOfflineLogPathSafe();
    const content = await fs.promises.readFile(filePath, "utf8");
    const rows = content.trim().split(/\r?\n/);
    return rows.slice(-lines).join("\n");
  } catch {
    return "";
  }
}

function isVitteDocument(doc: vscode.TextDocument): boolean {
  return LANGUAGE_SET.has(doc.languageId as typeof LANGUAGES[number]);
}

function updateEditorLint(doc: vscode.TextDocument): void {
  if (!editorLintCollection) return;
  const cfg = vscode.workspace.getConfiguration("vitte");
  if (!cfg.get<boolean>("features.lint", true)) {
    editorLintCollection.delete(doc.uri);
    return;
  }
  if (!isVitteDocument(doc)) {
    editorLintCollection.delete(doc.uri);
    return;
  }
  const lintCfg = cfg.get<{ maxLineLength?: number; allowTabs?: boolean; allowTrailingWhitespace?: boolean }>("lint") ?? {};
  const maxLineLength = typeof lintCfg.maxLineLength === "number" ? lintCfg.maxLineLength : 120;
  const allowTabs = lintCfg.allowTabs === true;
  const allowTrailing = lintCfg.allowTrailingWhitespace === true;

  const diagnostics: vscode.Diagnostic[] = [];
  const lines = doc.getText().split(/\r?\n/);
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    if (line === undefined) continue;
    if (!allowTabs && line.includes("\t")) {
      const idx = line.indexOf("\t");
      diagnostics.push(new vscode.Diagnostic(
        new vscode.Range(i, idx, i, idx + 1),
        "Tabulation détectée. Utiliser des espaces.",
        vscode.DiagnosticSeverity.Warning
      ));
    }
    if (!allowTrailing) {
      const m = /[ \t]+$/.exec(line);
      if (m) {
        const start = m.index ?? Math.max(0, line.length - m[0].length);
        const trailing = new vscode.Diagnostic(
          new vscode.Range(i, start, i, line.length),
          "Espaces en fin de ligne.",
          vscode.DiagnosticSeverity.Hint
        );
        trailing.tags = [vscode.DiagnosticTag.Unnecessary];
        diagnostics.push(trailing);
      }
    }
    if (maxLineLength > 0 && line.length > maxLineLength) {
      diagnostics.push(new vscode.Diagnostic(
        new vscode.Range(i, maxLineLength, i, line.length),
        `Ligne trop longue (${line.length} > ${maxLineLength}).`,
        vscode.DiagnosticSeverity.Hint
      ));
    }
  }
  diagnostics.push(...buildBracketDiagnostics(lines));
  editorLintCollection.set(doc.uri, diagnostics);
}

function buildBracketDiagnostics(lines: string[]): vscode.Diagnostic[] {
  const diagnostics: vscode.Diagnostic[] = [];
  const stack: Array<{ char: "(" | "[" | "{"; line: number; col: number }> = [];
  const pairs: Record<string, "(" | "[" | "{"> = { ")": "(", "]": "[", "}": "{" };
  const openers = new Set(["(", "[", "{"]);
  const closers = new Set([")", "]", "}"]);

  for (let lineIndex = 0; lineIndex < lines.length; lineIndex++) {
    const rawLine = lines[lineIndex] ?? "";
    const codeLine = normalizeForBracketScan(rawLine);
    for (let col = 0; col < codeLine.length; col++) {
      const char = codeLine[col];
      if (!char) continue;
      if (openers.has(char)) {
        stack.push({ char: char as "(" | "[" | "{", line: lineIndex, col });
        continue;
      }
      if (!closers.has(char)) continue;
      const expected = pairs[char];
      const top = stack[stack.length - 1];
      if (!top || top.char !== expected) {
        diagnostics.push(new vscode.Diagnostic(
          new vscode.Range(lineIndex, col, lineIndex, col + 1),
          `Parenthèse/accolade fermante inattendue "${char}".`,
          vscode.DiagnosticSeverity.Error
        ));
        continue;
      }
      stack.pop();
    }
  }

  for (const entry of stack) {
    const expected = entry.char === "(" ? ")" : entry.char === "[" ? "]" : "}";
    diagnostics.push(new vscode.Diagnostic(
      new vscode.Range(entry.line, entry.col, entry.line, entry.col + 1),
      `Parenthèse/accolade ouvrante "${entry.char}" non fermée (attendu: "${expected}").`,
      vscode.DiagnosticSeverity.Error
    ));
  }
  return diagnostics;
}

function normalizeForBracketScan(line: string): string {
  let result = "";
  let inSingle = false;
  let inDouble = false;
  let inTemplate = false;
  let escaped = false;

  for (let i = 0; i < line.length; i++) {
    const ch = line[i] ?? "";
    const next = i + 1 < line.length ? (line[i + 1] ?? "") : "";

    if (!inSingle && !inDouble && !inTemplate && ch === "/" && next === "/") {
      break;
    }
    if (escaped) {
      escaped = false;
      result += " ";
      continue;
    }
    if (ch === "\\") {
      escaped = true;
      result += " ";
      continue;
    }
    if (!inDouble && !inTemplate && ch === "'") {
      inSingle = !inSingle;
      result += " ";
      continue;
    }
    if (!inSingle && !inTemplate && ch === "\"") {
      inDouble = !inDouble;
      result += " ";
      continue;
    }
    if (!inSingle && !inDouble && ch === "`") {
      inTemplate = !inTemplate;
      result += " ";
      continue;
    }
    result += inSingle || inDouble || inTemplate ? " " : ch;
  }

  return result;
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
