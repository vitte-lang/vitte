import { workspaceBinaryRoots, expandToolPath, resolveProjectRoot } from "../utils/project";
import { redactLogText } from "../utils/logRedaction";
import { uiText } from "../utils/uiText";
import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import * as os from 'os';

import { spawnManaged } from '../utils/managedProcesses';

interface VitteVersions {
  cli?: string;
  runtime?: string;
  lsp?: string;
  build?: string;
  fmt?: string;
  bench?: string;
}

/**
 * Utility for detecting the Vitte toolchain root and runtime binaries.
 * Used by DebugConfigurationProvider and factory.
 */
export interface VitteRuntimePaths {
  toolchainRoot?: string;
  cliPath?: string;
  runtimePath: string;
  lspPath?: string;
  buildPath?: string;
  fmtPath?: string;
  benchPath?: string;
  found: boolean;
  message?: string;
  versions?: VitteVersions;
}


// -----------------------------
// fs/helpers (cross-platform)
// -----------------------------
function isWindows(): boolean { return os.platform() === 'win32'; }

function isExecutableSync(p: string | undefined): boolean {
  if (!p) return false;
  try {
    if (isWindows()) { fs.accessSync(p, fs.constants.F_OK); return true; }
    fs.accessSync(p, fs.constants.X_OK); return true;
  } catch { return false; }
}

function real(p: string | undefined): string | undefined {
  if (!p) return undefined;
  try {
    const rs = (fs.realpathSync as unknown) as { native?: (path: string) => string };
    if (typeof rs.native === 'function') {
      return rs.native(p);
    }
    return fs.realpathSync(p);
  } catch {
    return p;
  }
}

function firstNonEmpty(...values: (string | undefined)[]): string | undefined {
  for (const value of values) {
    if (value && value.length > 0) return value;
  }
  return undefined;
}

/** In-memory cache for lookups to avoid excessive fs access */
const WHICH_CACHE = new Map<string, string | undefined>();

/** Try resolve binary with PATH and common install roots. */
function which(bin: string): string | undefined {
  if (WHICH_CACHE.has(bin)) return WHICH_CACHE.get(bin)!;

  const envPath = process.env.PATH ?? '';
  const exts = isWindows() ? (process.env.PATHEXT?.split(';').filter(Boolean) ?? ['.exe', '.cmd', '.bat']) : [''];
  const parts = envPath.split(path.delimiter).filter(Boolean);

  for (const p of parts) {
    for (const ext of exts) {
      const full = path.join(p, bin + ext);
      if (isExecutableSync(full)) { const r = real(full) ?? full; WHICH_CACHE.set(bin, r); return r; }
    }
  }

  // The extension-host regression suite uses this opt-in switch to assert the
  // missing-toolchain path even on maintainer machines with a global install.
  // It is ignored unless the VS Code test harness is active.
  if (process.env.VSCODE_TESTING === '1' && process.env.VITTE_TEST_NO_SYSTEM_DISCOVERY === '1') {
    WHICH_CACHE.set(bin, undefined);
    return undefined;
  }

  // Common extra roots: Homebrew, /usr/local, Nix, ~/.local/bin
  const candidates: string[] = [];
  const home = os.homedir();
  if (isWindows()) {
    candidates.push(path.join(process.env.ProgramFiles ?? 'C:/Program Files', 'Vitte', 'bin', `${bin}.exe`));
  } else {
    candidates.push('/opt/homebrew/bin/' + bin);
    candidates.push('/usr/local/bin/' + bin);
    candidates.push('/usr/bin/' + bin);
    candidates.push(path.join(home, '.local', 'bin', bin));
    // Nix store symlink path
    candidates.push('/run/current-system/sw/bin/' + bin);
    candidates.push('/nix/var/nix/profiles/default/bin/' + bin);
  }
  for (const c of candidates) {
    if (isExecutableSync(c)) { const r = real(c) ?? c; WHICH_CACHE.set(bin, r); return r; }
  }

  WHICH_CACHE.set(bin, undefined);
  return undefined;
}

// -----------------------------
// semver helpers
// -----------------------------
interface VSemVer { major: number; minor: number; patch: number; prerelease?: string; }

export const VITTE_COMPILER_EVIDENCE_VERSION = "0.1.1";

/** Only supported-platform candidate; the current extension revision still needs a matching CI gate. */
export function isNativeDebugCandidatePlatform(): boolean {
  if (process.platform !== 'linux' || process.arch !== 'x64') return false;
  try {
    const release = fs.readFileSync('/etc/os-release', 'utf8');
    return /^ID=ubuntu$/m.test(release) && /^VERSION_ID="?24\.04"?$/m.test(release);
  } catch {
    return false;
  }
}

export function isSupportedVitteCompilerVersion(version: string | undefined): boolean {
  return version?.replace(/^v/i, "") === VITTE_COMPILER_EVIDENCE_VERSION;
}

function parseSemver(input: string | undefined): VSemVer | undefined {
  if (!input) return undefined;
  const match = /v?(\d+)\.(\d+)\.(\d+)(?:[-+]([^\s]+))?/.exec(input.trim());
  if (!match) return undefined;
  const [, sMaj, sMin, sPatch, pre] = match;
  if (!sMaj || !sMin || !sPatch) return undefined;
  const ver: VSemVer = { major: Number.parseInt(sMaj, 10), minor: Number.parseInt(sMin, 10), patch: Number.parseInt(sPatch, 10) };
  if (pre !== undefined) ver.prerelease = pre;
  return ver;
}

function cmpSemver(a: VSemVer, b: VSemVer): number {
  if (a.major !== b.major) return a.major - b.major;
  if (a.minor !== b.minor) return a.minor - b.minor;
  if (a.patch !== b.patch) return a.patch - b.patch;
  // prereleases sort before stable
  if (a.prerelease && !b.prerelease) return -1;
  if (!a.prerelease && b.prerelease) return 1;
  return 0;
}

// -----------------------------
// discovery
// -----------------------------
function scopedWorkspaceRoot(scope?: vscode.Uri): string {
  if (scope) return vscode.workspace.getWorkspaceFolder(scope)?.uri.fsPath ?? scope.fsPath;
  return vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ?? process.cwd();
}

function explicitToolchainRoot(scope?: vscode.Uri): string | undefined {
  const cfg = vscode.workspace.getConfiguration('vitte', scope);
  const raw = firstNonEmpty(cfg.get<string>('toolchain.root'), cfg.get<string>('toolchainPath'));
  if (!raw) return undefined;
  const workspaceRoot = scopedWorkspaceRoot(scope);
  return expandToolPath(raw, workspaceRoot);
}

function projectRoots(scope?: vscode.Uri): string[] {
  const folder = scopedWorkspaceRoot(scope);
  const cfg = vscode.workspace.getConfiguration('vitte', scope);
  const root = resolveProjectRoot(cfg.get<string>('project.root'), folder, scope?.fsPath);
  if (scope) return [...new Set([...(root ? [root] : []), folder])];
  const folders = (vscode.workspace.workspaceFolders ?? []).map(item => item.uri.fsPath);
  return [...new Set([...folders.flatMap(folderRoot => {
    const folderCfg = vscode.workspace.getConfiguration('vitte', vscode.Uri.file(folderRoot));
    const projectRoot = resolveProjectRoot(folderCfg.get<string>('project.root'), folderRoot);
    return projectRoot ? [projectRoot, folderRoot] : [folderRoot];
  })])];
}

function discoverWorkspaceBinRoots(scope?: vscode.Uri): string[] {
  return workspaceBinaryRoots(projectRoots(scope));
}

function environmentToolchainRoots(): string[] {
  const roots = [process.env.VITTE_TOOLCHAIN_ROOT, process.env.VITTE_ROOT, process.env.VITTE_HOME]
    .filter((value): value is string => Boolean(value));
  return [...new Set(roots)];
}

function environmentBinary(name: string): string | undefined {
  const exe = name + (isWindows() ? '.exe' : '');
  const binDir = process.env.VITTE_BIN_DIR;
  if (binDir) {
    const candidate = path.join(binDir, exe);
    if (isExecutableSync(candidate)) return real(candidate) ?? candidate;
  }
  for (const root of environmentToolchainRoots()) {
    const candidate = path.join(root, 'bin', exe);
    if (isExecutableSync(candidate)) return real(candidate) ?? candidate;
  }
  return undefined;
}

/** Detect toolchain root using explicit settings, project bins, environment roots, then user PATH installs. */
export function detectVitteToolchainRoot(scope?: vscode.Uri): string | undefined {
  const explicit = explicitToolchainRoot(scope);
  if (explicit && [
    path.join(explicit, 'bin', isWindows() ? 'vitte.exe' : 'vitte'),
    path.join(explicit, 'bin', isWindows() ? 'vitte-runtime.exe' : 'vitte-runtime'),
  ].some(isExecutableSync)) return real(explicit) ?? explicit;

  // Prefer project-local tools over ambient user PATH/environment installs.
  for (const root of discoverWorkspaceBinRoots(scope)) {
    const candidate = path.join(root, isWindows() ? 'vitte.exe' : 'vitte');
    if (isExecutableSync(candidate)) return real(path.resolve(root, '..')) ?? path.resolve(root, '..');
  }

  for (const root of environmentToolchainRoots()) {
    const candidate = path.join(root, 'bin', isWindows() ? 'vitte.exe' : 'vitte');
    if (isExecutableSync(candidate)) return real(root) ?? root;
  }

  const binDir = process.env.VITTE_BIN_DIR;
  if (binDir && isExecutableSync(path.join(binDir, isWindows() ? 'vitte.exe' : 'vitte'))) {
    return real(path.dirname(binDir)) ?? path.dirname(binDir);
  }

  const cli = which('vitte');
  if (cli) return path.resolve(path.dirname(cli), '..');

  const lsp = which('vitte-lsp');
  if (lsp) return path.resolve(path.dirname(lsp), '..');

  const runtime = which('vitte-runtime');
  if (runtime) return path.resolve(path.dirname(runtime), '..');

  return undefined;
}

function getVersion(bin: string | undefined): Promise<string | undefined> {
  if (!bin) return Promise.resolve(undefined);
  return new Promise((resolve) => {
    let stdout = '';
    let stderr = '';
    let finished = false;
    const child = spawnManaged(bin, ['--version'], {
      stdio: ['ignore', 'pipe', 'pipe'],
      windowsHide: true,
    });
    const finish = (version: string | undefined): void => {
      if (finished) return;
      finished = true;
      clearTimeout(timeout);
      resolve(version);
    };
    const timeout = setTimeout(() => {
      try { child.kill(); } catch { /* process may already have exited */ }
      finish(undefined);
    }, 700);
    child.stdout?.on('data', (chunk: Buffer | string) => { stdout += String(chunk); });
    child.stderr?.on('data', (chunk: Buffer | string) => { stderr += String(chunk); });
    child.once('error', () => finish(undefined));
    child.once('close', () => {
      const output = stdout.trim() || stderr.trim();
      const match = /\b(v?\d+\.\d+\.\d+(?:[-+].*)?)\b/.exec(output);
      finish(match?.[1] ?? (output || undefined));
    });
  });
}

// -----------------------------
// core location with memoization
// -----------------------------
const MEMO_RESULTS = new Map<string, VitteRuntimePaths>();
const MEMO_PROMISES = new Map<string, Promise<VitteRuntimePaths>>();
let MEMO_GENERATION = 0;

export function clearRuntimeLocatorCache(): void {
  MEMO_RESULTS.clear();
  MEMO_PROMISES.clear();
  MEMO_GENERATION += 1;
  WHICH_CACHE.clear();
}

function resolveFromSettingOrRoot(cfg: vscode.WorkspaceConfiguration, name: string, settingKey: string, scope?: vscode.Uri): string | undefined {
  const raw = cfg.get<string>(settingKey);
  const workspaceRoot = scopedWorkspaceRoot(scope);
  const fromSetting = raw ? expandToolPath(raw, workspaceRoot) : undefined;
  if (fromSetting && isExecutableSync(fromSetting)) return real(fromSetting) ?? fromSetting;
  const configured = explicitToolchainRoot(scope);
  if (configured) {
    const candidate = path.join(configured, 'bin', name + (isWindows() ? '.exe' : ''));
    if (isExecutableSync(candidate)) return real(candidate) ?? candidate;
  }
  for (const r of discoverWorkspaceBinRoots(scope)) {
    const candidate = path.join(r, name + (isWindows() ? '.exe' : ''));
    if (isExecutableSync(candidate)) return real(candidate) ?? candidate;
  }
  const fromEnvironment = environmentBinary(name);
  if (fromEnvironment) return fromEnvironment;
  return which(name);
}

/**
 * Locate runtime and companion binaries. Memoized per session.
 * Precedence: compiler.path, toolchain.root, project-local bins, environment roots, then PATH/user installs.
 */
export function locateVitteRuntime(minRequired?: string, scope?: vscode.Uri): Promise<VitteRuntimePaths> {
  const key = scope?.toString() ?? '<global>';
  const cached = MEMO_RESULTS.get(key);
  if (cached) return Promise.resolve(cached);
  const inFlight = MEMO_PROMISES.get(key);
  if (inFlight) return inFlight;

  const generation = MEMO_GENERATION;
  const pending = locateVitteRuntimeUncached(minRequired, scope).then((result) => {
    if (generation === MEMO_GENERATION) MEMO_RESULTS.set(key, result);
    return result;
  }).finally(() => {
    if (MEMO_PROMISES.get(key) === pending) MEMO_PROMISES.delete(key);
  });
  MEMO_PROMISES.set(key, pending);
  return pending;
}

async function locateVitteRuntimeUncached(minRequired?: string, scope?: vscode.Uri): Promise<VitteRuntimePaths> {
  const cfg = vscode.workspace.getConfiguration('vitte', scope);
  const toolchainRoot = detectVitteToolchainRoot(scope);

  const hasCompilerSetting = Boolean(cfg.get<string>('compiler.path')?.trim());
  const cliPath = resolveFromSettingOrRoot(cfg, 'vitte', 'compiler.path', scope)
    ?? which('vitte');
  const runtimePath = cliPath ?? 'vitte';
  const lspPath   = resolveFromSettingOrRoot(cfg, 'vitte-lsp', 'lsp.path', scope);
  const buildPath = hasCompilerSetting ? cliPath : resolveFromSettingOrRoot(cfg, 'vitte', 'build.path', scope) ?? cliPath;
  const fmtPath   = resolveFromSettingOrRoot(cfg, 'vitte-fmt', 'fmt.path', scope);
  const benchPath = resolveFromSettingOrRoot(cfg, 'vitte-bench', 'bench.path', scope);

  const found = isExecutableSync(cliPath) || isExecutableSync(runtimePath);

  const result: VitteRuntimePaths = { runtimePath, found };
  if (cliPath) result.cliPath = cliPath;
  if (toolchainRoot) result.toolchainRoot = toolchainRoot;
  if (lspPath) result.lspPath = lspPath;
  if (buildPath) result.buildPath = buildPath;
  if (fmtPath) result.fmtPath = fmtPath;
  if (benchPath) result.benchPath = benchPath;

  // Versions
  const versions: VitteVersions = {};
  const trusted = vscode.workspace.isTrusted;
  const versionCache = new Map<string, Promise<string | undefined>>();
  const readVersion = (bin: string | undefined): Promise<string | undefined> => {
    if (!trusted || !bin) return Promise.resolve(undefined);
    let version = versionCache.get(bin);
    if (!version) {
      version = getVersion(bin);
      versionCache.set(bin, version);
    }
    return version;
  };
  const [vc, vr, vl, vb, vf, vn] = await Promise.all([
    readVersion(found ? cliPath : undefined),
    readVersion(found ? runtimePath : undefined),
    readVersion(lspPath),
    readVersion(buildPath),
    readVersion(fmtPath),
    readVersion(benchPath),
  ]);
  if (vc) {
    versions.cli = vc;
  }
  if (vr) {
    versions.runtime = vr;
  }
  if (vl) {
    versions.lsp = vl;
  }
  if (vb) {
    versions.build = vb;
  }
  if (vf) {
    versions.fmt = vf;
  }
  if (vn) {
    versions.bench = vn;
  }
  if (Object.keys(versions).length > 0) {
    result.versions = versions;
  }

  // Optional min version check
  if (minRequired && (versions.cli ?? versions.runtime)) {
    const have = parseSemver(versions.cli ?? versions.runtime);
    const need = parseSemver(minRequired);
    if (have && need && cmpSemver(have, need) < 0) {
      result.message = `vitte ${versions.cli ?? versions.runtime} is older than required ${minRequired}`;
    }
  }

  if (found && !trusted) {
    result.message = 'Vitte CLI detected; workspace trust is required before checking its version or running it.';
  } else if (found && !versions.cli) {
    result.message = 'Vitte CLI version could not be determined; compiler-dependent behavior is not verified.';
  } else if (found && !isSupportedVitteCompilerVersion(versions.cli)) {
    result.message = `Vitte CLI ${versions.cli} is outside the tested compiler evidence version ${VITTE_COMPILER_EVIDENCE_VERSION}.`;
  }

  result.message ??= found
    ? `vitte detected at ${cliPath ?? runtimePath}`
    : 'vitte not found in PATH or toolchain root';

  return result;
}

/** Quick command for users: Detect and show runtime info in VSCode output */
export async function detectAndShowRuntimeInfo(output?: vscode.OutputChannel, scope?: vscode.Uri): Promise<void> {
  const result = await locateVitteRuntime(undefined, scope);
  const out = output ?? vscode.window.createOutputChannel('Vitte');
  out.appendLine('[Vitte] Toolchain detection');
  if (result.toolchainRoot) out.appendLine(redactLogText(`Toolchain root: ${result.toolchainRoot}`));
  if (result.cliPath) out.appendLine(redactLogText(`CLI: ${result.cliPath}`));
  out.appendLine(redactLogText(`Runtime: ${result.runtimePath}`));
  if (result.lspPath) out.appendLine(redactLogText(`LSP: ${result.lspPath}`));
  if (result.buildPath) out.appendLine(redactLogText(`Build: ${result.buildPath}`));
  if (result.fmtPath) out.appendLine(redactLogText(`Fmt: ${result.fmtPath}`));
  if (result.benchPath) out.appendLine(redactLogText(`Bench: ${result.benchPath}`));
  if (result.versions) {
    const v = result.versions;
    out.appendLine('Versions:');
    if (v?.cli) out.appendLine(redactLogText(`  cli:     ${v.cli}`));
    if (v?.runtime) out.appendLine(redactLogText(`  runtime: ${v.runtime}`));
    if (v?.lsp) out.appendLine(redactLogText(`  lsp:     ${v.lsp}`));
    if (v?.build) out.appendLine(redactLogText(`  build:   ${v.build}`));
    if (v?.fmt) out.appendLine(redactLogText(`  fmt:     ${v.fmt}`));
    if (v?.bench) out.appendLine(redactLogText(`  bench:   ${v.bench}`));
  }
  out.appendLine(`Found: ${String(result.found)}`);
  if (result.message) out.appendLine(redactLogText(result.message));
  out.show(true);
}

/**
 * Register commands:
 *  - vitte.detectToolchain: show detected paths/versions
 *  - vitte.selectRuntime: let user pick a binary and persist to settings
 *  - vitte.clearRuntimeCache: invalidate memoized result
 */
export function registerRuntimeLocatorCommand(ctx: vscode.ExtensionContext, output?: vscode.OutputChannel): void {
  const detectCmd = vscode.commands.registerCommand('vitte.detectToolchain', async () => {
    const folder = await selectToolchainFolder();
    await detectAndShowRuntimeInfo(output, folder?.uri);
  });

  const clearCmd = vscode.commands.registerCommand('vitte.clearRuntimeCache', () => {
    clearRuntimeLocatorCache();
    vscode.window.setStatusBarMessage(uiText('Vitte runtime cache cleared. Restart the language server to apply toolchain changes.', 'Cache du runtime Vitte effacé. Redémarrez le serveur de langage pour appliquer les changements.'), 4000);
  });

  const pickCmd = vscode.commands.registerCommand('vitte.selectRuntime', async (requestedFolderUri?: unknown) => {
    const requestedFolder = typeof requestedFolderUri === 'string'
      ? (vscode.workspace.workspaceFolders ?? []).find(folder => folder.uri.toString() === requestedFolderUri)
      : undefined;
    const activeFolder = requestedFolder ?? (vscode.window.activeTextEditor
      ? vscode.workspace.getWorkspaceFolder(vscode.window.activeTextEditor.document.uri)
      : await selectToolchainFolder());
    const runtime = await locateVitteRuntime(undefined, activeFolder?.uri);
    const config = vscode.workspace.getConfiguration('vitte', activeFolder?.uri);
    const currentSetting = config.get<string>('compiler.path', '').trim();
    const choices: (vscode.QuickPickItem & { action: 'detected' | 'browse' | 'automatic' })[] = [];
    if (runtime.found && runtime.cliPath) {
      choices.push({
        label: uiText(`Use detected compiler: ${path.basename(runtime.cliPath)}`, `Utiliser le compilateur détecté : ${path.basename(runtime.cliPath)}`),
        description: runtime.versions?.cli ? `Vitte CLI ${runtime.versions.cli}` : runtime.cliPath,
        detail: runtime.cliPath,
        action: 'detected',
      });
    }
    choices.push({ label: uiText('Browse for compiler…', 'Parcourir pour choisir un compilateur…'), description: uiText('Select a Vitte executable from disk', 'Sélectionner un exécutable Vitte sur le disque'), action: 'browse' });
    if (currentSetting) {
      choices.push({ label: uiText('Use automatic discovery', 'Utiliser la détection automatique'), description: uiText('Clear vitte.compiler.path and resume discovery', 'Effacer vitte.compiler.path et reprendre la détection'), action: 'automatic' });
    }
    const selection = await vscode.window.showQuickPick(choices, {
      title: activeFolder ? uiText(`Select Vitte compiler — ${activeFolder.name}`, `Sélectionner le compilateur Vitte — ${activeFolder.name}`) : uiText('Select Vitte compiler', 'Sélectionner le compilateur Vitte'),
      placeHolder: uiText('Choose a discovered compiler, browse for one or restore automatic discovery', 'Choisissez un compilateur détecté, parcourez les fichiers ou réactivez la détection automatique'),
      matchOnDescription: true,
    });
    if (!selection) return;
    const target = activeFolder
      ? vscode.ConfigurationTarget.WorkspaceFolder
      : vscode.workspace.workspaceFolders?.length
        ? vscode.ConfigurationTarget.Workspace
        : vscode.ConfigurationTarget.Global;
    if (selection.action === 'automatic') {
      await config.update('compiler.path', '', target);
      clearRuntimeLocatorCache();
      vscode.window.setStatusBarMessage(uiText('Vitte compiler override cleared; automatic discovery will be used.', 'Le choix du compilateur Vitte est effacé ; la détection automatique sera utilisée.'), 4000);
      return;
    }

    let fsPath = selection.action === 'detected' ? runtime.cliPath : undefined;
    if (selection.action === 'browse') {
      const opts: vscode.OpenDialogOptions = {
        canSelectMany: false,
        canSelectFiles: true,
        canSelectFolders: false,
        openLabel: uiText('Select Vitte CLI', 'Sélectionner le CLI Vitte'),
        title: uiText('Select Vitte compiler executable', 'Sélectionner l’exécutable du compilateur Vitte'),
      };
      if (isWindows()) opts.filters = { [uiText('Executables', 'Exécutables')]: ['exe', 'cmd', 'bat'] };
      fsPath = (await vscode.window.showOpenDialog(opts))?.[0]?.fsPath;
    }
    if (!fsPath) return;
    if (!isExecutableSync(fsPath)) {
      void vscode.window.showErrorMessage(uiText('The selected file is not executable. Choose the Vitte CLI binary.', 'Le fichier sélectionné n’est pas exécutable. Choisissez le binaire CLI de Vitte.'));
      return;
    }
    await config.update('compiler.path', fsPath, target);
    clearRuntimeLocatorCache();
    vscode.window.setStatusBarMessage(uiText(`Vitte compiler selected: ${path.basename(fsPath)}`, `Compilateur Vitte sélectionné : ${path.basename(fsPath)}`), 4000);
  });

  ctx.subscriptions.push(detectCmd, clearCmd, pickCmd);
}

async function selectToolchainFolder(): Promise<vscode.WorkspaceFolder | undefined> {
  const activeUri = vscode.window.activeTextEditor?.document.uri;
  const activeFolder = activeUri ? vscode.workspace.getWorkspaceFolder(activeUri) : undefined;
  if (activeFolder) return activeFolder;
  const folders = vscode.workspace.workspaceFolders ?? [];
  if (folders.length === 1) return folders[0];
  if (folders.length === 0) return undefined;
  const picked = await vscode.window.showQuickPick(folders.map(folder => ({
    label: folder.name,
    description: folder.uri.fsPath,
    folder,
  })), { placeHolder: uiText('Choose the workspace folder whose Vitte toolchain to inspect', 'Choisissez le dossier de travail dont vous voulez examiner la chaîne Vitte') });
  return picked?.folder;
}
