import * as vscode from 'vscode';
import * as path from 'path';

interface VitteDebugSettings {
  program?: string;
  compiler?: string;
  nativeBackend?: boolean;
  adapterArgs?: string[];
  adapterTransport?: 'stdio' | 'tcp';
  args?: string[];
  cwd?: string;
  host?: string;
  port?: number;
  trace?: boolean;
  sourceMaps?: boolean;
  env?: Record<string, string>;
}

interface VitteDebugJson {
  debug?: VitteDebugSettings;
  toolchain?: { root?: string; runtime?: string };
}

/** Read and parse JSON file into object. */
async function readJsonFile<T = unknown>(uri: vscode.Uri): Promise<T | undefined> {
  try {
    const doc = await vscode.workspace.openTextDocument(uri);
    return JSON.parse(doc.getText()) as T;
  } catch {
    return undefined;
  }
}

/** Try multiple known config locations, first hit wins then shallow-merge. */
async function readVitteProjectConfig(): Promise<VitteDebugJson> {
  const results: VitteDebugJson[] = [];

  // 1) vitte.config.json at workspace root
  const rootCfg = await vscode.workspace.findFiles('vitte.config.json', '**/node_modules/**', 1);
  const rootUri = Array.isArray(rootCfg) ? rootCfg[0] : undefined;
  if (rootUri) {
    const j = await readJsonFile<VitteDebugJson>(rootUri);
    if (j) results.push(j);
  }

  // 2) .vitte/config.json fallback
  const hiddenCfg = await vscode.workspace.findFiles('.vitte/config.json', '**/node_modules/**', 1);
  const hiddenUri = Array.isArray(hiddenCfg) ? hiddenCfg[0] : undefined;
  if (hiddenUri) {
    const j = await readJsonFile<VitteDebugJson>(hiddenUri);
    if (j) results.push(j);
  }

  // 3) package.json { vitte: { debug: {...} } }
  const pkg = await vscode.workspace.findFiles('package.json', '**/node_modules/**', 1);
  const pkgUri = Array.isArray(pkg) ? pkg[0] : undefined;
  if (pkgUri) {
    const pjson = await readJsonFile<Record<string, unknown>>(pkgUri);
    const vitteSection = pjson?.vitte;
    if (vitteSection && typeof vitteSection === 'object') {
      const pick: VitteDebugJson = {};
      const debugSection = (vitteSection as Record<string, unknown>).debug;
      const toolchainSection = (vitteSection as Record<string, unknown>).toolchain;
      if (debugSection && typeof debugSection === 'object') pick.debug = { ...(debugSection as VitteDebugSettings) };
      if (toolchainSection && typeof toolchainSection === 'object') pick.toolchain = { ...(toolchainSection as NonNullable<VitteDebugJson['toolchain']>) };
      results.push(pick);
    }
  }

  // Merge shallowly from first to last, later wins
  const merged: VitteDebugJson = {};
  for (const r of results) {
    if (r.toolchain) merged.toolchain = { ...(merged.toolchain ?? {}), ...r.toolchain };
    if (r.debug) merged.debug = { ...(merged.debug ?? {}), ...r.debug };
  }
  return merged;
}

function ensureWorkspaceFolder(): string | undefined {
  return vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
}

/** Expand VS Code variables and env vars in strings. */
function expand(str: string, fileUri?: vscode.Uri): string {
  const wf = ensureWorkspaceFolder() ?? '';
  const active = fileUri ?? vscode.window.activeTextEditor?.document.uri;
  const filePath = active?.fsPath ?? '';
  const fileDir = filePath ? path.dirname(filePath) : '';
  const env = process.env;
  const table: Record<string, string> = {
    workspaceFolder: wf,
    file: filePath,
    fileDirname: fileDir,
  };
  return str.replace(/\$\{(env:)?([^}]+)}/g, function (_m: string, envPrefix: string | undefined, key: string): string {
    const k = String(key);
    if (envPrefix) return env[k] ?? '';
    return table[k] ?? '';
  });
}

function coerceArgs(v: unknown): string[] | undefined {
  if (Array.isArray(v)) return v.filter((x): x is string => typeof x === 'string' && x.length > 0);
  if (typeof v === 'string' && v.length) return [v];
  return undefined;
}

function sanitizeEnv(obj: unknown): Record<string, string> {
  const out: Record<string, string> = {};
  if (obj && typeof obj === 'object') {
    for (const [k, v] of Object.entries(obj as Record<string, unknown>)) {
      if (typeof v === 'string') out[k] = v;
    }
  }
  return out;
}

function resolveRuntimePath(cfg: vscode.WorkspaceConfiguration, project: VitteDebugJson): string {
  // `debug.program` is retained as the setting name for compatibility; it is a DAP adapter, not the debuggee.
  const setProgram = cfg.get<string>('debug.program') ?? cfg.get<string>('runtime.path');
  if (setProgram?.trim()) return resolveToolchainPath(setProgram, cfg, project);

  const projectProg = project.debug?.program;
  if (!projectProg?.trim()) return '';
  return resolveToolchainPath(projectProg, cfg, project);
}

function resolveToolchainPath(candidate: string, cfg: vscode.WorkspaceConfiguration, project: VitteDebugJson): string {
  if (!path.isAbsolute(candidate)) {
    const toolchainRoot = cfg.get<string>('toolchain.root')
      ?? project.toolchain?.root
      ?? cfg.get<string>('toolchainPath');
    if (toolchainRoot) return path.join(toolchainRoot, candidate);
  }
  return candidate;
}

/**
 * Provides initial debug configurations and resolves them using Vitte settings and project config files.
 */
class VitteDebugConfigurationProvider implements vscode.DebugConfigurationProvider {
  provideDebugConfigurations(_folder: vscode.WorkspaceFolder | undefined): vscode.ProviderResult<vscode.DebugConfiguration[]> {
    const workspaceFolder = ensureWorkspaceFolder() ?? '${workspaceFolder}';
    return [
      {
        name: 'Vitte: Launch current file',
        type: 'vitte',
        request: 'launch',
        program: '${file}',
        nativeBackend: true,
        args: [],
        cwd: workspaceFolder
      },
      {
        name: 'Vitte: Attach to DAP server',
        type: 'vitte',
        request: 'attach',
        host: '127.0.0.1',
        port: 9333
      }
    ];
  }

  async resolveDebugConfiguration(folder: vscode.WorkspaceFolder | undefined, config: vscode.DebugConfiguration): Promise<vscode.DebugConfiguration | null | undefined> {
    const settings = vscode.workspace.getConfiguration('vitte');
    const project = await readVitteProjectConfig();

    const workspaceFolder = folder?.uri.fsPath ?? ensureWorkspaceFolder() ?? process.cwd();
    const activeUri = vscode.window.activeTextEditor?.document.uri;

    const configRecord = config as Record<string, unknown>;
    const configEnvValue = configRecord.env;
    const configEnvObject = (typeof configEnvValue === 'object' && configEnvValue) ? configEnvValue : undefined;

    const mergedEnvRaw: unknown = {
      ...(project.debug?.env ?? {}),
      ...(configEnvObject as Record<string, unknown> | undefined ?? {}),
    };
    const mergedEnv = sanitizeEnv(mergedEnvRaw);

    const request = configRecord.request === 'attach' ? 'attach' : 'launch';
    const adapter = typeof configRecord.adapter === 'string' && configRecord.adapter.trim().length > 0
      ? configRecord.adapter
      : resolveRuntimePath(settings, project);
    const nativeBackend = typeof configRecord.nativeBackend === 'boolean'
      ? configRecord.nativeBackend
      : project.debug?.nativeBackend
        ?? settings.get<boolean>('debug.nativeBackend', true);
    const configuredTransport = configRecord.adapterTransport
      ?? project.debug?.adapterTransport
      ?? settings.get<string>('debug.adapterTransport', 'stdio');
    const base: vscode.DebugConfiguration = {
      type: 'vitte',
      request,
      name: typeof configRecord.name === 'string' && configRecord.name.length > 0
        ? configRecord.name
        : request === 'attach' ? 'Vitte: Attach' : 'Vitte: Launch',
      adapter,
      nativeBackend,
      adapterTransport: configuredTransport,
      adapterArgs: coerceArgs(configRecord.adapterArgs)
        ?? coerceArgs(project.debug?.adapterArgs)
        ?? settings.get<string[]>('debug.adapterArgs', []),
      args: coerceArgs(configRecord.args) ?? coerceArgs(project.debug?.args) ?? [],
      cwd: (() => {
        const cfgCwd = configRecord.cwd;
        if (typeof cfgCwd === 'string' && cfgCwd.length > 0) return cfgCwd;
        if (typeof project.debug?.cwd === 'string' && project.debug.cwd.length > 0) return project.debug.cwd;
        return workspaceFolder;
      })(),
      env: mergedEnv,
    };
    if (request === 'launch') {
      const program = typeof configRecord.program === 'string' ? configRecord.program : '${file}';
      base.program = program;
    } else {
      base.host = typeof configRecord.host === 'string' ? configRecord.host : '127.0.0.1';
      base.port = typeof configRecord.port === 'number' ? configRecord.port : undefined;
    }

    // Optional flags
    const configTraceRaw = configRecord.trace;
    if (typeof project.debug?.trace === 'boolean' && typeof configTraceRaw !== 'boolean') {
      base.trace = project.debug.trace;
    }
    const configSourceMapsRaw = configRecord.sourceMaps;
    if (typeof project.debug?.sourceMaps === 'boolean' && typeof configSourceMapsRaw !== 'boolean') {
      base.sourceMaps = project.debug.sourceMaps;
    }

    // VS Code variable expansion
    const programValue = typeof base.program === 'string' ? base.program : undefined;
    if (programValue) base.program = expand(programValue, activeUri);
    if (typeof base.adapter === 'string') base.adapter = expand(base.adapter, activeUri);

    const cwdValue = typeof base.cwd === 'string' ? base.cwd : String(base.cwd ?? workspaceFolder);
    base.cwd = expand(cwdValue, activeUri);

    const argsValue = Array.isArray(base.args) ? base.args : [];
    base.args = argsValue.map((s) => expand(String(s), activeUri));
    const adapterArgsValue = Array.isArray(base.adapterArgs) ? base.adapterArgs : [];
    base.adapterArgs = adapterArgsValue.map((s) => expand(String(s), activeUri));
    if (typeof base.adapterCwd === 'string') base.adapterCwd = expand(base.adapterCwd, activeUri);
    if (typeof base.host === 'string') base.host = expand(base.host, activeUri);

    if (base.env && typeof base.env === 'object') {
      const env: Record<string, string> = {};
      for (const [k, v] of Object.entries(base.env as Record<string, unknown>)) {
        if (typeof v === 'string') env[k] = expand(v, activeUri);
      }
      base.env = env;
    }

    if (request === 'launch' && (!base.program || typeof base.program !== 'string')) {
      void vscode.window.showErrorMessage('Vitte: indiquez le fichier ou le binaire à lancer dans "program".');
      return undefined;
    }
    if (request === 'launch' && (!base.adapter || typeof base.adapter !== 'string') && !nativeBackend) {
      void vscode.window.showErrorMessage(
        'Vitte: aucun adaptateur DAP configuré. Définissez "vitte.debug.program", "adapter" ou activez "vitte.debug.nativeBackend".',
      );
      return undefined;
    }
    if (request === 'attach' && (!Number.isInteger(base.port) || Number(base.port) < 1 || Number(base.port) > 65535)) {
      void vscode.window.showErrorMessage('Vitte: le port du serveur DAP doit être compris entre 1 et 65535.');
      return undefined;
    }

    return base;
  }
}

export function registerDebugConfigurationProvider(ctx: vscode.ExtensionContext) {
  const provider = new VitteDebugConfigurationProvider();
  ctx.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('vitte', provider));
}
