import * as path from 'node:path';
import * as vscode from 'vscode';

interface QuickActionItem extends vscode.QuickPickItem {
  readonly command: string;
  readonly args?: unknown[] | undefined;
}

interface QuickActionStep {
  readonly command: string;
  readonly args?: unknown[] | undefined;
}

export interface QuickActionDefinition {
  readonly label: string;
  readonly description?: string | undefined;
  readonly id?: string;
  readonly command: string;
  readonly args?: unknown[] | undefined;
  readonly actions?: QuickActionStep[];
  readonly when?: (ctx: QuickActionsContext) => boolean | Promise<boolean>;
  readonly isSuggested?: (ctx: QuickActionsContext) => boolean;
  readonly detail?: (ctx: QuickActionsContext) => string | undefined;
  readonly highlightLastRun?: boolean;
}
type Mutable<T> = { -readonly [P in keyof T]: T[P] };

interface QuickActionOverrideConfig {
  label?: string;
  description?: string;
  detail?: string;
  command?: string;
  args?: unknown[];
  actions?: QuickActionStep[];
}

export interface QuickActionsContext {
  hasBenchConfig: boolean;
  activeIsTestFile: boolean;
  workspaceBuilt: boolean;
  diagnosticsLastRefresh?: number | undefined;
  diagnosticsStale: boolean;
  telemetryEnabled: boolean;
  usageStats: Record<string, UsageEntry>;
  buildProfile: string;
  buildIncremental: boolean;
}

const strings = {
  noActions: () => vscode.l10n.t('Vitte: no actions available for this workspace.'),
  placeholder: () => vscode.l10n.t('Choose a Vitte action'),
  neverRefreshed: () => vscode.l10n.t('Never refreshed'),
  lastRefresh: (prefix: string, relative: string) => vscode.l10n.t('{0} Last refresh: {1}', prefix, relative),
  lastRun: (relative: string) => vscode.l10n.t('🔁 Last run: {0}', relative),
  runAgain: () => vscode.l10n.t('⟳ Run again'),
  usageCount: (count: number) => vscode.l10n.t('{0}× run', count),
  favoriteDetail: (detail?: string) =>
    detail ? vscode.l10n.t('{0} • Favorite action', detail) : vscode.l10n.t('Favorite action'),
  runError: (message: string) => vscode.l10n.t('Vitte: unable to run the selected action ({0})', message),
};

const BUILTIN_ACTION_DEFS: QuickActionDefinition[] = [
  {
    id: 'build',
    label: '⚙️ Build',
    description: 'Compile the entire workspace (vitte.build)',
    command: 'vitte.build',
    isSuggested: (ctx) => !ctx.workspaceBuilt,
    highlightLastRun: true,
  },
  {
    id: 'clean',
    label: '🧹 Clean',
    description: 'Remove build outputs (vitte.clean)',
    command: 'vitte.clean',
  },
  {
    id: 'run',
    label: '▶️ Run',
    description: 'Build and run the entry point (vitte.run)',
    command: 'vitte.run',
    highlightLastRun: true,
  },
  {
    id: 'tests',
    label: '🧪 Tests',
    description: 'Execute the toolchain test suite (vitte.test)',
    command: 'vitte.test',
    highlightLastRun: true,
  },
  {
    id: 'testFile',
    label: '🧪 Test file',
    description: 'Run vitte.testCurrent for *_test.vitte files',
    command: 'vitte.testCurrent',
    isSuggested: (ctx) => ctx.activeIsTestFile,
    highlightLastRun: true,
  },
  {
    id: 'bench',
    label: '📈 Bench',
    description: 'Run vitte.bench with current settings',
    command: 'vitte.bench',
    when: (ctx) => ctx.hasBenchConfig,
    highlightLastRun: true,
  },
  {
    id: 'benchReport',
    label: '📊 Report',
    description: 'Run vitte.bench and open the latest report',
    command: 'vitte.benchReport',
    when: (ctx) => ctx.hasBenchConfig,
  },
  {
    id: 'benchDir',
    label: '📁 Bench dir',
    description: 'Reveal .vitte/bench or configured bench.reportDir',
    command: 'vitte.openBenchDir',
    when: (ctx) => ctx.hasBenchConfig,
  },
  {
    id: 'diagnostics.refresh',
    label: '🩺 Refresh',
    description: 'Refresh the Vitte Diagnostics view',
    command: 'vitte.diagnostics.refresh',
    detail: (ctx) => {
      if (!ctx.diagnosticsLastRefresh) return strings.neverRefreshed();
      const prefix = ctx.diagnosticsStale ? '⚠️' : '⏱️';
      return strings.lastRefresh(prefix, formatRelativeTime(ctx.diagnosticsLastRefresh));
    },
  },
  {
    id: 'diagnostics.next',
    label: '➡️ Next issue',
    description: 'Jump to the next diagnostic',
    command: 'editor.action.marker.next',
  },
  {
    id: 'docs',
    label: '📚 Docs',
    description: 'Open the bundled docs/playground portal',
    command: 'vitte.openDocs',
  },
  {
    id: 'profile',
    label: '🔁 Profile',
    description: 'Cycle dev → test → release → bench (vitte.switchProfile)',
    command: 'vitte.switchProfile',
    detail: (ctx) => `Current profile: ${ctx.buildProfile}`,
  },
  {
    id: 'incremental',
    label: '⚡ Incremental',
    description: 'Toggle vitte.build.incremental',
    command: 'vitte.toggleIncremental',
    detail: (ctx) => `Incremental: ${ctx.buildIncremental ? 'ON' : 'OFF'}`,
  },
  {
    id: 'docs.combo',
    label: '📘 Docs+Play',
    description: 'Open docs then launch the playground',
    command: 'vitte.openDocs',
    actions: [
      { command: 'vitte.openDocs' },
      { command: 'vitte.openPlayground' },
    ],
    detail: () => 'Docs & playground chain',
  },
  {
    id: 'onboarding.setup',
    label: '🚀 Setup',
    description: 'Docs → toolchain detection → build for onboarding',
    command: 'vitte.openDocs',
    actions: [
      { command: 'vitte.openDocs' },
      { command: 'vitte.detectToolchain' },
      { command: 'vitte.build' },
    ],
    detail: () => 'Guided: docs + tooling + build',
  },
];

const RECENT_STORAGE_KEY = 'vitte.quickActions.recent';
const PIN_STORAGE_KEY = 'vitte.quickActions.pinned';
const BUILD_STATE_KEY = 'vitte.quickActions.buildState';
const DIAGNOSTICS_REFRESH_KEY = 'vitte.quickActions.diagRefresh';
const USAGE_STORAGE_KEY = 'vitte.quickActions.usage';
const RECENT_LIMIT = 3;
const RECENT_TTL_MS = 30 * 24 * 60 * 60 * 1000;
const DIAGNOSTICS_STALE_MS = 10 * 60 * 1000;

const PIN_BUTTON: vscode.QuickInputButton = { iconPath: new vscode.ThemeIcon('star-empty'), tooltip: 'Mark as favorite' };
const UNPIN_BUTTON: vscode.QuickInputButton = { iconPath: new vscode.ThemeIcon('star-full'), tooltip: 'Remove from favorites' };

function getVitteConfiguration(): vscode.WorkspaceConfiguration | undefined {
  try {
    return vscode.workspace.getConfiguration('vitte');
  } catch (error) {
    console.debug('[vitte.quickActions] Unable to read vitte configuration', error);
    return undefined;
  }
}

function getConfigValue<T>(config: vscode.WorkspaceConfiguration | undefined, section: string, fallback: T): T {
  if (!config) return fallback;
  try {
    const value = config.get<T>(section);
    return value ?? fallback;
  } catch {
    return fallback;
  }
}

function memoizeConfiguration(): () => vscode.WorkspaceConfiguration | undefined {
  let cached: vscode.WorkspaceConfiguration | undefined;
  let loaded = false;
  return () => {
    if (loaded) return cached;
    cached = getVitteConfiguration();
    loaded = true;
    return cached;
  };
}

function deepCloneArgs<T extends unknown[] | undefined>(args: T): T {
  if (args === undefined) return args;
  const cloneFn = (globalThis as { structuredClone?: <T>(value: T) => T }).structuredClone;
  if (typeof cloneFn === 'function') {
    try {
      return cloneFn(args);
    } catch {
      // ignore and fall through to simpler clones
    }
  }
  try {
    return JSON.parse(JSON.stringify(args)) as T;
  } catch {
    return [...args] as T;
  }
}

interface UserSequenceConfig {
  id?: string;
  label?: string;
  description?: string;
  detail?: string;
  steps?: UserSequenceStep[];
}

interface UserSequenceStep {
  command?: string;
  args?: unknown[];
}

interface RecentCommandEntry {
  command: string;
  actionId?: string;
  workspace?: string;
  timestamp: number;
}

interface UsageEntry {
  count: number;
  lastUsed: number;
}

export function registerQuickActions(context: vscode.ExtensionContext): void {
  const disposable = vscode.commands.registerCommand('vitte.quickActions', async () => {
    const loadConfig = memoizeConfiguration();
    const recentEntries = getRecentCommands(context);
    const cfg = loadConfig();
    const ctx = await resolveContext(context, cfg);
    const definitions = getActionDefinitions(cfg);
    const buildWithDefinitions = (pinned?: string) => buildItems(definitions, ctx, recentEntries, pinned);
    let pinnedCommand = getPinnedCommand(context);
    let items = await buildWithDefinitions(pinnedCommand);
    if (items.length === 0) {
      void vscode.window.showWarningMessage(strings.noActions());
      return;
    }

    const quickPick = vscode.window.createQuickPick<QuickPickWithMeta>();
    quickPick.items = items;
    quickPick.matchOnDescription = true;
    quickPick.placeholder = strings.placeholder();
    quickPick.ignoreFocusOut = true;
    const initialSelection = items.filter((item) => item.__isSuggested);
    if (initialSelection.length > 0) {
      quickPick.selectedItems = initialSelection.slice(0, 1) as readonly QuickPickWithMeta[];
    }

    quickPick.onDidTriggerItemButton(async ({ item, button }) => {
      if (button === PIN_BUTTON) {
        const activeSelectionId = quickPick.selectedItems[0]?.actionId ?? item.actionId;
        pinnedCommand = item.actionId;
        await setPinnedCommand(context, pinnedCommand);
        items = await buildWithDefinitions(pinnedCommand);
        updateQuickPickItems(quickPick, items, activeSelectionId);
      } else if (button === UNPIN_BUTTON && pinnedCommand) {
        const activeSelectionId = quickPick.selectedItems[0]?.actionId ?? item.actionId;
        pinnedCommand = undefined;
        await setPinnedCommand(context, undefined);
        items = await buildWithDefinitions(pinnedCommand);
        updateQuickPickItems(quickPick, items, activeSelectionId);
      }
    });

    const selection = await new Promise<QuickPickWithMeta | undefined>((resolve) => {
      quickPick.onDidAccept(() => {
        resolve(quickPick.selectedItems[0]);
        quickPick.hide();
      });
      quickPick.onDidHide(() => resolve(undefined));
      quickPick.show();
    });
    quickPick.dispose();

    if (!selection) return;
    try {
      for (const step of selection.steps) {
        await vscode.commands.executeCommand(step.command, ...(step.args ?? []));
      }
      await rememberSelection(context, selection);
      if (ctx.telemetryEnabled && !selection.__isRecent) {
        await recordUsage(context, selection.actionId);
      }
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      void vscode.window.showErrorMessage(strings.runError(message));
    }
  });
  context.subscriptions.push(disposable);

  const exportStatsDisposable = vscode.commands.registerCommand('vitte.quickActions.exportStats', async () => {
    const cfg = getVitteConfiguration();
    const stats = getUsageStats(context);
    const definitions = getActionDefinitions(cfg);
    const actionEntries = Object.entries(stats ?? {});
    if (actionEntries.length === 0) {
      void vscode.window.showInformationMessage('Quick Actions: no usage data recorded yet.');
      return;
    }
    const report = buildUsageReport(definitions, stats);
    const defaultUri = buildDefaultExportUri(report);
    const saveOptions: vscode.SaveDialogOptions = {
      filters: { 'JSON Files': ['json'] },
      saveLabel: 'Export Quick Action stats',
    };
    if (defaultUri) saveOptions.defaultUri = defaultUri;
    const targetUri = await vscode.window.showSaveDialog(saveOptions);
    if (!targetUri) return;
    const encoder = new TextEncoder();
    const payload = encoder.encode(JSON.stringify(report, null, 2));
    await vscode.workspace.fs.writeFile(targetUri, payload);
    void vscode.window.showInformationMessage(`Quick Actions: exported stats to ${targetUri.fsPath}`);
  });
  context.subscriptions.push(exportStatsDisposable);
}

export interface QuickPickWithMeta extends QuickActionItem {
  readonly __isSuggested: boolean;
  readonly __isRecent: boolean;
  readonly __isPinned: boolean;
  readonly actionId: string;
  readonly steps: QuickActionStep[];
  readonly baseCommand: string;
}

async function buildItems(
  defs: QuickActionDefinition[],
  ctx: QuickActionsContext,
  recentEntries: RecentCommandEntry[],
  pinnedCommand?: string
): Promise<QuickPickWithMeta[]> {
  const available: QuickPickWithMeta[] = [];
  for (const def of defs) {
    if (def.when) {
      try {
        const ok = await def.when(ctx);
        if (!ok) continue;
      } catch {
        continue;
      }
    }
    const actionId = def.id ?? def.command;
    const steps = (def.actions ?? [{ command: def.command, args: def.args }]).map((step) => ({
      command: step.command,
      args: deepCloneArgs(step.args),
    }));
    const usageInfo = ctx.telemetryEnabled ? ctx.usageStats[actionId] : undefined;
    const lastRunBadge =
      ctx.telemetryEnabled && def.highlightLastRun && usageInfo?.lastUsed
        ? strings.lastRun(formatRelativeTime(usageInfo.lastUsed))
        : undefined;
    const runAgainBadge =
      ctx.telemetryEnabled && def.highlightLastRun && usageInfo
        ? strings.runAgain()
        : undefined;
    const usageCountText = usageInfo?.count ? strings.usageCount(usageInfo.count) : undefined;
    const detailParts = [def.detail?.(ctx), lastRunBadge, usageCountText].filter(
      (value): value is string => typeof value === 'string'
    );
    const suggestedFlag = Boolean(def.isSuggested?.(ctx));

    const baseItem: QuickPickWithMeta = {
      label: def.label,
      command: def.command,
      args: deepCloneArgs(def.args),
      actionId,
      steps,
      baseCommand: def.command,
      __isSuggested: suggestedFlag,
      __isRecent: false,
      __isPinned: false,
      buttons: [PIN_BUTTON],
    };
    if (def.description !== undefined || runAgainBadge) {
      const baseDesc = def.description ?? '';
      baseItem.description =
        runAgainBadge && baseDesc.length > 0 ? `${baseDesc} — ${runAgainBadge}` : runAgainBadge ?? baseDesc;
      if (!baseItem.description || baseItem.description.trim().length === 0) {
        delete baseItem.description;
      }
    }
    if (detailParts.length > 0) {
      baseItem.detail = detailParts.join(' • ');
    }
    available.push(baseItem);
  }

  const items: QuickPickWithMeta[] = [];

  if (pinnedCommand) {
    const idx = available.findIndex((item) => item.actionId === pinnedCommand);
    if (idx >= 0) {
      const removed = available.splice(idx, 1);
      const match = removed[0];
      if (match) {
        items.push({
          ...match,
          label: `★ ${match.label.replace(/^★\s*/u, '').replace(/^↺\s*/u, '')}`,
          detail: strings.favoriteDetail(match.detail),
          __isPinned: true,
          __isRecent: false,
          buttons: [UNPIN_BUTTON],
        });
      }
    }
  }

  const recentItems: QuickPickWithMeta[] = [];
  for (const entry of recentEntries) {
    const entryId = entry.actionId ?? entry.command;
    const idx = available.findIndex((item) => item.actionId === entryId);
    if (idx === -1) continue;
    const removed = available.splice(idx, 1);
    const match = removed[0];
    if (!match) continue;
    const detailParts: string[] = [];
    if (match.detail) detailParts.push(match.detail);
    if (entry.workspace) detailParts.push(entry.workspace);
    detailParts.push(formatRelativeTime(entry.timestamp));
    recentItems.push({
      ...match,
      label: `↺ ${match.label}`,
      detail: detailParts.join(' • '),
      __isRecent: true,
      __isPinned: false,
      buttons: [PIN_BUTTON],
    });
  }

  items.push(...recentItems, ...available);
  return items;
}

function updateQuickPickItems(
  quickPick: vscode.QuickPick<QuickPickWithMeta>,
  items: QuickPickWithMeta[],
  preferredActionId?: string
): void {
  const currentSelectionId = preferredActionId ?? quickPick.selectedItems?.[0]?.actionId;
  const currentActiveId = quickPick.activeItems?.[0]?.actionId ?? currentSelectionId;
  quickPick.items = items;
  const nextSelection = currentSelectionId
    ? items.find((candidate) => candidate.actionId === currentSelectionId)
    : undefined;
  const nextActive = currentActiveId ? items.find((candidate) => candidate.actionId === currentActiveId) : undefined;
  if (nextSelection) {
    quickPick.selectedItems = [nextSelection];
  } else if (items.length > 0) {
    quickPick.selectedItems = [items[0]!];
  }
  if (nextActive) {
    quickPick.activeItems = [nextActive];
  } else if (quickPick.selectedItems.length > 0) {
    quickPick.activeItems = quickPick.selectedItems;
  }
}

function getActionDefinitions(config?: vscode.WorkspaceConfiguration): QuickActionDefinition[] {
  const cfg = config ?? getVitteConfiguration();
  return applyQuickActionOverrides([...BUILTIN_ACTION_DEFS, ...readConfiguredSequences(cfg)], cfg);
}

async function resolveContext(
  extCtx: vscode.ExtensionContext,
  config?: vscode.WorkspaceConfiguration,
): Promise<QuickActionsContext> {
  const active = vscode.window.activeTextEditor;
  const activePath = active?.document?.uri?.fsPath ?? '';
  const activeLanguage = active?.document?.languageId;
  const isVitteLanguage = activeLanguage ? ['vitte', 'vit'].includes(activeLanguage) : false;
  const activeIsTestFile = Boolean(isVitteLanguage && activePath && /(_test\.vitte|\.(vitte|vit))$/i.test(activePath));

  const hasBenchConfig = await workspaceHasBenchConfig();

  const workspaceBuilt = hasWorkspaceBeenBuilt(extCtx);
  const diagnosticsState = getDiagnosticsRefreshState(extCtx);
  const cfg = config ?? getVitteConfiguration();
  const telemetryEnabled = Boolean(getConfigValue(cfg, 'telemetry.enabled', false));
  const buildProfile = getConfigValue(cfg, 'build.profile', 'dev').toLowerCase();
  const buildIncremental = Boolean(getConfigValue(cfg, 'build.incremental', false));
  const usageStats: Record<string, UsageEntry> = telemetryEnabled ? getUsageStats(extCtx) : createEmptyUsageStats();

  return {
    hasBenchConfig,
    activeIsTestFile,
    workspaceBuilt,
    diagnosticsLastRefresh: diagnosticsState.lastRefresh,
    diagnosticsStale: diagnosticsState.stale,
    telemetryEnabled,
    usageStats,
    buildProfile,
    buildIncremental,
  };
}

async function workspaceHasBenchConfig(): Promise<boolean> {
  const folders = vscode.workspace.workspaceFolders;
  if (!folders || folders.length === 0) return false;

  try {
    const files = await vscode.workspace.findFiles('vitte.config.json', '**/node_modules/**', 1);
    if (files.length > 0) return true;

    // fallback: check hidden folder `.vitte/config.json`
    for (const folder of folders) {
      const candidate = vscode.Uri.file(path.join(folder.uri.fsPath, '.vitte', 'config.json'));
      try {
        await vscode.workspace.fs.stat(candidate);
        return true;
      } catch {
        // ignore
      }
    }
  } catch {
    // ignore lookup errors
  }

  return false;
}

function readConfiguredSequences(config?: vscode.WorkspaceConfiguration): QuickActionDefinition[] {
  const cfg = config ?? getVitteConfiguration();
  const raw = cfg?.get<unknown>('quickActions.sequences');
  if (!Array.isArray(raw)) return [];

  const defs: QuickActionDefinition[] = [];
  raw.forEach((entry, index) => {
    if (!entry || typeof entry !== 'object') return;
    const data = entry as UserSequenceConfig;
    const label = typeof data.label === 'string' ? data.label.trim() : '';
    if (!label) return;
    const rawSteps = Array.isArray(data.steps) ? data.steps : [];
    const steps: QuickActionStep[] = [];
    for (const step of rawSteps) {
      if (!step || typeof step !== 'object') continue;
      const cmd = typeof step.command === 'string' ? step.command.trim() : '';
      if (!cmd) continue;
      const args = Array.isArray(step.args) ? deepCloneArgs(step.args) : undefined;
      steps.push({ command: cmd, args });
    }
    if (steps.length === 0) return;
    const firstStep = steps[0];
    if (!firstStep) return;
    const actionId =
      typeof data.id === 'string' && data.id.trim().length > 0
        ? data.id.trim()
        : `custom.sequence.${index}`;
    const description = typeof data.description === 'string' ? data.description : undefined;
    const detailText =
      typeof data.detail === 'string' && data.detail.trim().length > 0
        ? data.detail
        : `Steps: ${steps.map((s) => summarizeCommandLabel(s.command)).join(' → ')}`;
    defs.push({
      id: actionId,
      label,
      description,
      command: firstStep.command,
      actions: steps,
      detail: () => detailText,
    });
  });
  return defs;
}

function readQuickActionOverrides(config?: vscode.WorkspaceConfiguration): Record<string, QuickActionOverrideConfig> {
  const cfg = config ?? getVitteConfiguration();
  const raw = cfg?.get<unknown>('quickActions.defaults');
  if (!raw || typeof raw !== 'object' || Array.isArray(raw)) {
    return {};
  }
  const overrides: Record<string, QuickActionOverrideConfig> = {};
  for (const [id, value] of Object.entries(raw)) {
    if (!value || typeof value !== 'object') continue;
    const data = value as QuickActionOverrideConfig;
    const sanitized: QuickActionOverrideConfig = {};
    if (typeof data.label === 'string' && data.label.trim().length > 0) sanitized.label = data.label.trim();
    if (typeof data.description === 'string' && data.description.trim().length > 0) {
      sanitized.description = data.description;
    }
    if (typeof data.detail === 'string' && data.detail.trim().length > 0) {
      sanitized.detail = data.detail;
    }
    if (typeof data.command === 'string' && data.command.trim().length > 0) {
      sanitized.command = data.command.trim();
    }
    if (Array.isArray(data.args)) {
      sanitized.args = deepCloneArgs(data.args);
    }
    if (Array.isArray(data.actions)) {
      const steps: QuickActionStep[] = [];
      for (const step of data.actions) {
        if (!step || typeof step !== 'object') continue;
        const cmd = typeof step.command === 'string' ? step.command.trim() : '';
        if (!cmd) continue;
        const args = Array.isArray(step.args) ? deepCloneArgs(step.args) : undefined;
        steps.push({ command: cmd, args });
      }
      if (steps.length > 0) {
        sanitized.actions = steps;
      }
    }
    if (Object.keys(sanitized).length > 0) {
      overrides[id] = sanitized;
    }
  }
  return overrides;
}

function applyQuickActionOverrides(
  defs: QuickActionDefinition[],
  config?: vscode.WorkspaceConfiguration,
): QuickActionDefinition[] {
  const overrides = readQuickActionOverrides(config);
  if (Object.keys(overrides).length === 0) {
    return defs;
  }
  return defs.map((def) => {
    if (!def.id) return def;
    const override = overrides[def.id];
    if (!override) return def;
    const next: Mutable<QuickActionDefinition> = { ...def };
    if (override.label !== undefined) next.label = override.label;
    if (override.description !== undefined) next.description = override.description;
    if (override.detail !== undefined) {
      const detailText = override.detail;
      next.detail = detailText ? () => detailText : () => "";
    }
    if (override.command !== undefined) next.command = override.command;
    if (override.args !== undefined) next.args = deepCloneArgs(override.args);
    if (override.actions) {
      next.actions = override.actions.map((step) => ({
        ...step,
        args: deepCloneArgs(step.args),
      }));
    }
    return next;
  });
}

function getRecentCommands(context: vscode.ExtensionContext): RecentCommandEntry[] {
  const stored = context.globalState.get<RecentCommandEntry[] | string[]>(RECENT_STORAGE_KEY, []);
  if (stored.length === 0) return [];
  // Migration: older versions stored plain command strings
  if (typeof stored[0] === 'string') {
    const migrated = (stored as string[]).map((command) => ({
      command,
      timestamp: Date.now(),
    }));
    void context.globalState.update(RECENT_STORAGE_KEY, migrated);
    return migrated;
  }
  const now = Date.now();
  const entries = (stored as RecentCommandEntry[]).filter((entry) => isRecentEntry(entry, now));
  if (entries.length !== stored.length) {
    void context.globalState.update(RECENT_STORAGE_KEY, entries);
  }
  return entries;
}

function getPinnedCommand(context: vscode.ExtensionContext): string | undefined {
  return context.globalState.get<string>(PIN_STORAGE_KEY);
}

async function setPinnedCommand(context: vscode.ExtensionContext, command: string | undefined): Promise<void> {
  await context.globalState.update(PIN_STORAGE_KEY, command);
}

async function rememberSelection(context: vscode.ExtensionContext, selection: QuickPickWithMeta): Promise<void> {
  const now = Date.now();
  const workspace = vscode.workspace.workspaceFolders?.[0]?.name;
  const current = getRecentCommands(context).filter((entry) => isRecentEntry(entry, now));
  const next = [
    {
      command: selection.baseCommand,
      actionId: selection.actionId,
      workspace,
      timestamp: now,
    },
    ...current.filter((entry) => (entry.actionId ?? entry.command) !== selection.actionId),
  ].slice(0, RECENT_LIMIT);
  await context.globalState.update(RECENT_STORAGE_KEY, next);

  const containsBuild = selection.steps.some((step) => step.command === 'vitte.build');
  const containsDiagRefresh = selection.steps.some((step) => step.command === 'vitte.diagnostics.refresh');
  if (containsBuild) await markWorkspaceBuilt(context);
  if (containsDiagRefresh) await markDiagnosticsRefreshed(context);
}

function isRecentEntry(entry: RecentCommandEntry, now: number): boolean {
  const timestamp = typeof entry.timestamp === 'number' ? entry.timestamp : now;
  return now - timestamp <= RECENT_TTL_MS;
}

function formatRelativeTime(epochMs: number): string {
  const diff = Date.now() - epochMs;
  if (diff < 0) return 'just now';
  const sec = Math.floor(diff / 1000);
  if (sec < 60) return sec <= 1 ? '1s ago' : `${sec}s ago`;
  const min = Math.floor(sec / 60);
  if (min < 60) return min === 1 ? '1 min ago' : `${min} min ago`;
  const hours = Math.floor(min / 60);
  if (hours < 24) return hours === 1 ? '1 h ago' : `${hours} h ago`;
  const days = Math.floor(hours / 24);
  return days === 1 ? '1 day ago' : `${days} days ago`;
}

function getWorkspaceId(): string | undefined {
  const folder = vscode.workspace.workspaceFolders?.[0];
  return folder?.uri.fsPath;
}

function hasWorkspaceBeenBuilt(context: vscode.ExtensionContext): boolean {
  const workspaceId = getWorkspaceId();
  if (!workspaceId) return false;
  const map = context.globalState.get<Record<string, number>>(BUILD_STATE_KEY, {});
  return Boolean(map[workspaceId]);
}

async function markWorkspaceBuilt(context: vscode.ExtensionContext): Promise<void> {
  const workspaceId = getWorkspaceId();
  if (!workspaceId) return;
  const map = { ...context.globalState.get<Record<string, number>>(BUILD_STATE_KEY, {}) };
  map[workspaceId] = Date.now();
  await context.globalState.update(BUILD_STATE_KEY, map);
}

function getDiagnosticsRefreshState(context: vscode.ExtensionContext): { lastRefresh?: number; stale: boolean } {
  const workspaceId = getWorkspaceId();
  if (!workspaceId) return { stale: true };
  const map = context.globalState.get<Record<string, number>>(DIAGNOSTICS_REFRESH_KEY, {});
  const ts = map[workspaceId];
  if (!ts) return { stale: true };
  return { lastRefresh: ts, stale: Date.now() - ts > DIAGNOSTICS_STALE_MS };
}

async function markDiagnosticsRefreshed(context: vscode.ExtensionContext): Promise<void> {
  const workspaceId = getWorkspaceId();
  if (!workspaceId) return;
  const map = { ...context.globalState.get<Record<string, number>>(DIAGNOSTICS_REFRESH_KEY, {}) };
  map[workspaceId] = Date.now();
  await context.globalState.update(DIAGNOSTICS_REFRESH_KEY, map);
}

function getUsageStats(context: vscode.ExtensionContext): Record<string, UsageEntry> {
  return context.globalState.get<Record<string, UsageEntry>>(USAGE_STORAGE_KEY, {});
}

async function recordUsage(context: vscode.ExtensionContext, actionId: string): Promise<void> {
  const map = { ...getUsageStats(context) };
  const entry = map[actionId] ?? { count: 0, lastUsed: 0 };
  map[actionId] = { count: entry.count + 1, lastUsed: Date.now() };
  await context.globalState.update(USAGE_STORAGE_KEY, map);
}

function createEmptyUsageStats(): Record<string, UsageEntry> {
  return {};
}

function summarizeCommandLabel(command: string): string {
  if (command.startsWith('vitte.')) {
    return command.slice('vitte.'.length);
  }
  if (command.startsWith('workbench.action.')) {
    return command.slice('workbench.action.'.length);
  }
  return command;
}

function buildUsageReport(
  defs: QuickActionDefinition[],
  stats: Record<string, UsageEntry>,
): { exportedAt: string; actions: { id: string; label: string; totalRuns: number; lastUsed?: string; commands: string[] }[]; totalActions: number } {
  const map = new Map<string, QuickActionDefinition>();
  for (const def of defs) {
    map.set(def.id ?? def.command, def);
  }
  const actions = Object.entries(stats).map(([id, entry]) => {
    const def = map.get(id);
    const commands = def?.actions?.map((step) => step.command) ?? (def?.command ? [def.command] : []);
    const record: { id: string; label: string; totalRuns: number; commands: string[]; lastUsed?: string } = {
      id,
      label: def?.label ?? id,
      totalRuns: entry.count,
      commands,
    };
    if (entry.lastUsed) {
      record.lastUsed = new Date(entry.lastUsed).toISOString();
    }
    return record;
  });
  actions.sort((a, b) => b.totalRuns - a.totalRuns);
  return {
    exportedAt: new Date().toISOString(),
    totalActions: actions.length,
    actions,
  };
}

function buildDefaultExportUri(report: { exportedAt: string }): vscode.Uri | undefined {
  const fileName = `vitte-quick-actions-stats-${report.exportedAt.replace(/[:.]/g, '-')}.json`;
  const root = vscode.workspace.workspaceFolders?.[0]?.uri;
  if (root) {
    const joined = root.with({ path: path.posix.join(root.path, fileName) });
    return joined;
  }
  if (typeof process?.env?.HOME === 'string' && process.env.HOME.length > 0) {
    return vscode.Uri.file(path.join(process.env.HOME, fileName));
  }
  return undefined;
}

// Exposed for unit tests
export const __test = {
  readConfiguredSequences,
  readQuickActionOverrides,
  applyQuickActionOverrides,
  buildItems,
  updateQuickPickItems,
};
