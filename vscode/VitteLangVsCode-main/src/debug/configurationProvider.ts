import * as vscode from 'vscode';
import * as path from 'path';
import { expandToolPath, resolveProjectRoot } from '../utils/project';
import { requireTrustedWorkspace } from '../utils/workspaceTrust';
import { isNativeDebugCandidatePlatform, isSupportedVitteCompilerVersion, locateVitteRuntime, VITTE_COMPILER_EVIDENCE_VERSION } from './runtimeLocator';

interface VitteDebugSettings {
  program?: string;
  compiler?: string;
  nativeBackend?: boolean;
  adapterArgs?: string[];
  adapterTransport?: 'stdio' | 'tcp';
  args?: string[];
  compilerArgs?: string[];
  target?: string;
  manifestPath?: string;
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

async function readScopedJson(root: string, relativePath: string): Promise<VitteDebugJson | undefined> {
  const uri = vscode.Uri.file(path.join(root, relativePath));
  try { await vscode.workspace.fs.stat(uri); }
  catch { return undefined; }
  return readJsonFile<VitteDebugJson>(uri);
}

/** Read extension-only debug overlays from the selected folder/project, never another workspace root. */
async function readVitteProjectConfig(workspaceRoot: string, projectRoot: string): Promise<VitteDebugJson> {
  const results: VitteDebugJson[] = [];
  for (const root of [...new Set([workspaceRoot, projectRoot])]) {
    const rootConfig = await readScopedJson(root, 'vitte.config.json');
    if (rootConfig) results.push(rootConfig);
    const hiddenConfig = await readScopedJson(root, path.join('.vitte', 'config.json'));
    if (hiddenConfig) results.push(hiddenConfig);

    // Optional extension-specific overlay. The native CLI still reads its project file from cwd.
    const packageUri = vscode.Uri.file(path.join(root, 'package.json'));
    let pjson: Record<string, unknown> | undefined;
    try {
      await vscode.workspace.fs.stat(packageUri);
      pjson = await readJsonFile<Record<string, unknown>>(packageUri);
    } catch { /* package metadata is optional */ }
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

function ensureWorkspaceFolder(folder?: vscode.WorkspaceFolder): string | undefined {
  return folder?.uri.fsPath ?? vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
}

/** Expand VS Code variables and env vars in strings. */
function expand(str: string, fileUri?: vscode.Uri, workspaceRoot?: string): string {
  const wf = workspaceRoot ?? ensureWorkspaceFolder() ?? '';
  const active = fileUri ?? vscode.window.activeTextEditor?.document.uri;
  const filePath = active?.fsPath ?? '';
  const fileDir = filePath ? path.dirname(filePath) : '';
  const env = process.env;
  const table: Record<string, string> = {
    workspaceFolder: wf,
    file: filePath,
    fileDirname: fileDir,
    fileBasename: filePath ? path.basename(filePath) : '',
    fileBasenameNoExtension: filePath ? path.parse(filePath).name : '',
    fileExtname: filePath ? path.extname(filePath) : '',
    relativeFile: filePath ? path.relative(wf, filePath) : '',
    relativeFileDirname: filePath ? path.relative(wf, fileDir) : '',
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

function resolveRuntimePath(cfg: vscode.WorkspaceConfiguration, project: VitteDebugJson, workspaceRoot: string): string {
  // `debug.program` is retained as the setting name for compatibility; it is a DAP adapter, not the debuggee.
  const setProgram = cfg.get<string>('debug.program') ?? cfg.get<string>('runtime.path');
  if (setProgram?.trim()) return resolveToolchainPath(setProgram, cfg, project, workspaceRoot);

  const projectProg = project.debug?.program;
  if (!projectProg?.trim()) return '';
  return resolveToolchainPath(projectProg, cfg, project, workspaceRoot);
}

function resolveToolchainPath(candidate: string, cfg: vscode.WorkspaceConfiguration, project: VitteDebugJson, workspaceRoot: string): string {
  const expanded = expand(candidate, undefined, workspaceRoot);
  if (path.isAbsolute(expanded)) return expanded;
  if (!path.isAbsolute(candidate)) {
    const toolchainRoot = cfg.get<string>('toolchain.root')
      ?? project.toolchain?.root
      ?? cfg.get<string>('toolchainPath');
    if (toolchainRoot) return path.resolve(expand(toolchainRoot, undefined, workspaceRoot), expanded);
  }
  return path.resolve(workspaceRoot, expanded);
}

/**
 * Provides initial debug configurations and resolves them using Vitte settings and project config files.
 */
class VitteDebugConfigurationProvider implements vscode.DebugConfigurationProvider {
  async provideDebugConfigurations(folder: vscode.WorkspaceFolder | undefined): Promise<vscode.DebugConfiguration[]> {
    const settings = vscode.workspace.getConfiguration('vitte', folder?.uri);
    const workspaceRoot = ensureWorkspaceFolder(folder) ?? process.cwd();
    const projectRoot = resolveProjectRoot(settings.get<string>('project.root'), workspaceRoot) ?? workspaceRoot;
    const project = await readVitteProjectConfig(workspaceRoot, projectRoot);
    const configs: vscode.DebugConfiguration[] = [{
        name: 'Vitte: Attach to DAP server',
        type: 'vitte',
        request: 'attach',
        host: '127.0.0.1',
        port: 9333
    }];
    const externalAdapter = Boolean(settings.get<string>('debug.program', '').trim() || project.debug?.program?.trim());
    const nativeOptIn = settings.get<boolean>('debug.nativeBackend', false) || project.debug?.nativeBackend === true;
    if (externalAdapter) configs.unshift({
      name: 'Vitte: Launch current file (external DAP)',
      type: 'vitte',
      request: 'launch',
      program: '${file}',
      nativeBackend: false,
      args: [],
    });
    if (nativeOptIn && isNativeDebugCandidatePlatform()) {
      const folderSuffix = (vscode.workspace.workspaceFolders?.length ?? 0) > 1 ? ` — ${folder?.name ?? path.basename(workspaceRoot)}` : '';
      configs.unshift({
        name: 'Vitte: Debug built executable (DWARF)',
        type: 'vitte',
        request: 'launch',
        program: path.join(projectRoot, 'build', 'vscode', '${fileBasenameNoExtension}'),
        preLaunchTask: `Vitte Build${folderSuffix}`,
        nativeBackend: true,
        args: [],
      });
      configs.unshift({
        name: 'Vitte: Launch current file (native, experimental)',
        type: 'vitte',
        request: 'launch',
        program: '${file}',
        nativeBackend: true,
        args: [],
      });
    }
    return configs;
  }

  async resolveDebugConfiguration(folder: vscode.WorkspaceFolder | undefined, config: vscode.DebugConfiguration): Promise<vscode.DebugConfiguration | null | undefined> {
    if (!(await requireTrustedWorkspace('starting or attaching a Vitte debug session'))) return undefined;
    const activeUri = vscode.window.activeTextEditor?.document.uri;
    const activeFolder = activeUri ? vscode.workspace.getWorkspaceFolder(activeUri) : undefined;
    const selectedFolder = folder ?? activeFolder;
    const workspaceFolder = selectedFolder?.uri.fsPath ?? ensureWorkspaceFolder() ?? process.cwd();
    const settings = vscode.workspace.getConfiguration('vitte', selectedFolder?.uri ?? activeUri);
    const activeInFolder = activeFolder?.uri.toString() === selectedFolder?.uri.toString() ? activeUri?.fsPath : undefined;
    const projectRoot = resolveProjectRoot(settings.get<string>('project.root'), workspaceFolder, activeInFolder) ?? workspaceFolder;
    const project = await readVitteProjectConfig(workspaceFolder, projectRoot);

    const configRecord = config as Record<string, unknown>;
    const configEnvValue = configRecord.env;
    const configEnvObject = (typeof configEnvValue === 'object' && configEnvValue) ? configEnvValue : undefined;

    const mergedEnvRaw: unknown = {
      ...(settings.get<Record<string, string>>('build.env', {}) ?? {}),
      ...(project.debug?.env ?? {}),
      ...(configEnvObject as Record<string, unknown> | undefined ?? {}),
    };
    const mergedEnv = sanitizeEnv(mergedEnvRaw);
    const modulePaths = settings.get<unknown>('modulePaths', []);
    if (Array.isArray(modulePaths)) {
      const resolved = modulePaths
        .filter((value): value is string => typeof value === 'string' && value.trim().length > 0)
        .map((value) => expandToolPath(value.trim(), projectRoot));
      if (resolved.length > 0) {
        const inherited = mergedEnv.VITTE_MODULE_PATH?.split(path.delimiter).filter(Boolean) ?? [];
        mergedEnv.VITTE_MODULE_PATH = [...new Set([...resolved, ...inherited])].join(path.delimiter);
      }
    }

    const request = configRecord.request === 'attach' ? 'attach' : 'launch';
    const configuredNativeBackend = typeof configRecord.nativeBackend === 'boolean'
      ? configRecord.nativeBackend
      : project.debug?.nativeBackend
        ?? settings.get<boolean>('debug.nativeBackend', false);
    const nativeBackend = request === 'launch' && configuredNativeBackend;
    const explicitAdapter = typeof configRecord.adapter === 'string' && configRecord.adapter.trim().length > 0
      ? configRecord.adapter
      : undefined;
    if (nativeBackend && explicitAdapter) {
      void vscode.window.showErrorMessage('Choose either nativeBackend or an explicit adapter in this launch configuration. The extension will not switch debugger backends silently.');
      return undefined;
    }
    // `nativeBackend: true` is the explicit selector; do not let a saved
    // external-adapter setting silently replace the requested native path.
    const adapter = nativeBackend ? '' : explicitAdapter ?? resolveRuntimePath(settings, project, workspaceFolder);
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
      compilerArgs: [
        ...settings.get<string[]>('build.args', []),
        ...(coerceArgs(project.debug?.compilerArgs) ?? []),
        ...(coerceArgs(configRecord.compilerArgs) ?? []),
      ],
      target: typeof configRecord.target === 'string' && configRecord.target.trim()
        ? configRecord.target.trim()
        : project.debug?.target?.trim() || settings.get<string>('build.target', '').trim(),
      manifestPath: typeof configRecord.manifestPath === 'string' && configRecord.manifestPath.trim()
        ? configRecord.manifestPath
        : project.debug?.manifestPath?.trim() || settings.get<string>('project.manifestPath', ''),
      cwd: (() => {
        const cfgCwd = configRecord.cwd;
        if (typeof cfgCwd === 'string' && cfgCwd.length > 0) return cfgCwd;
        if (typeof project.debug?.cwd === 'string' && project.debug.cwd.length > 0) return project.debug.cwd;
        return projectRoot;
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
    if (programValue) base.program = expand(programValue, activeUri, workspaceFolder);
    if (typeof base.adapter === 'string') base.adapter = expand(base.adapter, activeUri, workspaceFolder);

    const cwdValue = typeof base.cwd === 'string' ? base.cwd : String(base.cwd ?? workspaceFolder);
    const expandedCwd = expand(cwdValue, activeUri, workspaceFolder);
    base.cwd = path.isAbsolute(expandedCwd) ? expandedCwd : path.resolve(projectRoot, expandedCwd);

    const argsValue = Array.isArray(base.args) ? base.args : [];
    base.args = argsValue.map((s) => expand(String(s), activeUri, workspaceFolder));
    const compilerArgsValue = Array.isArray(base.compilerArgs) ? base.compilerArgs : [];
    base.compilerArgs = compilerArgsValue.map((s) => expand(String(s), activeUri, workspaceFolder));
    const adapterArgsValue = Array.isArray(base.adapterArgs) ? base.adapterArgs : [];
    base.adapterArgs = adapterArgsValue.map((s) => expand(String(s), activeUri, workspaceFolder));
    if (typeof base.adapterCwd === 'string') base.adapterCwd = expand(base.adapterCwd, activeUri, workspaceFolder);
    if (typeof base.host === 'string') base.host = expand(base.host, activeUri, workspaceFolder);
    if (typeof base.manifestPath === 'string' && base.manifestPath.trim()) {
      base.manifestPath = expandToolPath(base.manifestPath.trim(), projectRoot);
    }

    if (base.env && typeof base.env === 'object') {
      const env: Record<string, string> = {};
      for (const [k, v] of Object.entries(base.env as Record<string, unknown>)) {
        if (typeof v === 'string') env[k] = expand(v, activeUri, workspaceFolder);
      }
      base.env = env;
    }

    if (request === 'launch' && (!base.program || typeof base.program !== 'string')) {
      void vscode.window.showErrorMessage('Vitte: set the source file or executable to launch in "program".');
      return undefined;
    }
    if (request === 'launch' && (!base.adapter || typeof base.adapter !== 'string') && !nativeBackend) {
      void vscode.window.showErrorMessage(
        'Vitte: no DAP adapter is configured. Set "vitte.debug.program" or "adapter", or enable the experimental native backend. Without a debugger, use Vitte: Build or Vitte: Run with the selected CLI.',
      );
      return undefined;
    }
    if (request === 'launch' && nativeBackend && !base.adapter) {
      if (!isNativeDebugCandidatePlatform()) {
      void vscode.window.showWarningMessage('The Vitte native backend is experimental; Linux x86_64 / Ubuntu 24.04 is the only platform candidate. Configure an external DAP adapter, or use Vitte: Build and Vitte: Run with the selected CLI.');
        return undefined;
      }
      const runtime = await locateVitteRuntime(undefined, selectedFolder?.uri ?? activeUri);
      if (!runtime.found || !isSupportedVitteCompilerVersion(runtime.versions?.cli)) {
        const version = runtime.versions?.cli ?? (runtime.found ? 'unknown' : 'not found');
        const choice = await vscode.window.showWarningMessage(
          `Vitte ${version} is not verified for native debugging. Version ${VITTE_COMPILER_EVIDENCE_VERSION} is required. Configure vitte.compiler.path or select a supported compiler. You can use Vitte: Build and Vitte: Run with the selected CLI while debugging is unavailable.`,
          'Select Compiler',
          'Open Settings',
        );
        if (choice === 'Select Compiler') await vscode.commands.executeCommand('vitte.selectRuntime', selectedFolder?.uri.toString());
        else if (choice === 'Open Settings') await vscode.commands.executeCommand('workbench.action.openSettings', '@ext:VitteStudio.vitte-studio vitte.compiler.path');
        return undefined;
      }
      const compiler = runtime.cliPath;
      if (!compiler) {
        void vscode.window.showErrorMessage('Vitte native debugging requires the selected workspace folder’s Vitte CLI. No alternate frontend will be substituted. Use Vitte: Build and Vitte: Run for the selected toolchain.');
        return undefined;
      }
      if (typeof base.program === 'string' && /\.(?:vit|vitl|vitte)$/i.test(base.program) && typeof base.target === 'string' && base.target.trim()) {
        void vscode.window.showWarningMessage('The native debugger has no verified mapping for a non-default target. Clear vitte.build.target to debug a host executable, or use Vitte: Build / Vitte: Run with the selected target.');
        return undefined;
      }
      base.compiler = compiler;
      base.env = { ...((base.env as Record<string, string> | undefined) ?? {}), VITTE_COMPILER: compiler };
    }
    if (request === 'attach' && (!Number.isInteger(base.port) || Number(base.port) < 1 || Number(base.port) > 65535)) {
      void vscode.window.showErrorMessage('Vitte: the DAP server port must be between 1 and 65535.');
      return undefined;
    }

    return base;
  }
}

export function registerDebugConfigurationProvider(ctx: vscode.ExtensionContext) {
  const provider = new VitteDebugConfigurationProvider();
  ctx.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('vitte', provider));
}
