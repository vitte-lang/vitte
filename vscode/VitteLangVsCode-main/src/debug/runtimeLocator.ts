import { workspaceBinaryRoots, expandToolPath, resolveProjectRoot } from "../utils/project";
import { redactLogText } from "../utils/logRedaction";
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
function explicitToolchainRoot(): string | undefined {
  const cfg = vscode.workspace.getConfiguration('vitte');
  const raw = firstNonEmpty(cfg.get<string>('toolchain.root'), cfg.get<string>('toolchainPath'));
  if (!raw) return undefined;
  const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ?? process.cwd();
  return expandToolPath(raw, workspaceRoot);
}

function projectRoots(): string[] {
  const folders = (vscode.workspace.workspaceFolders ?? []).map(folder => folder.uri.fsPath);
  const configured = vscode.workspace.getConfiguration('vitte').get<string>('project.root');
  const root = resolveProjectRoot(configured, folders[0]);
  return [...new Set([...(root ? [root] : []), ...folders])];
}

function discoverWorkspaceBinRoots(): string[] {
  return workspaceBinaryRoots(projectRoots());
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
export function detectVitteToolchainRoot(): string | undefined {
  const explicit = explicitToolchainRoot();
  if (explicit && [
    path.join(explicit, 'bin', isWindows() ? 'vitte.exe' : 'vitte'),
    path.join(explicit, 'bin', isWindows() ? 'vitte-runtime.exe' : 'vitte-runtime'),
  ].some(isExecutableSync)) return real(explicit) ?? explicit;

  // Prefer project-local tools over ambient user PATH/environment installs.
  for (const root of discoverWorkspaceBinRoots()) {
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
let MEMO_RESULT: VitteRuntimePaths | undefined;
let MEMO_PROMISE: Promise<VitteRuntimePaths> | undefined;
let MEMO_GENERATION = 0;

export function clearRuntimeLocatorCache(): void {
  MEMO_RESULT = undefined;
  MEMO_PROMISE = undefined;
  MEMO_GENERATION += 1;
  WHICH_CACHE.clear();
}

function resolveFromSettingOrRoot(cfg: vscode.WorkspaceConfiguration, name: string, settingKey: string): string | undefined {
  const raw = cfg.get<string>(settingKey);
  const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ?? process.cwd();
  const fromSetting = raw ? expandToolPath(raw, workspaceRoot) : undefined;
  if (fromSetting && isExecutableSync(fromSetting)) return real(fromSetting) ?? fromSetting;
  const configured = explicitToolchainRoot();
  if (configured) {
    const candidate = path.join(configured, 'bin', name + (isWindows() ? '.exe' : ''));
    if (isExecutableSync(candidate)) return real(candidate) ?? candidate;
  }
  for (const r of discoverWorkspaceBinRoots()) {
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
export function locateVitteRuntime(minRequired?: string): Promise<VitteRuntimePaths> {
  if (MEMO_RESULT) return Promise.resolve(MEMO_RESULT);
  if (MEMO_PROMISE) return MEMO_PROMISE;

  const generation = MEMO_GENERATION;
  const pending = locateVitteRuntimeUncached(minRequired).then((result) => {
    if (generation === MEMO_GENERATION) MEMO_RESULT = result;
    return result;
  }).finally(() => {
    if (MEMO_PROMISE === pending) MEMO_PROMISE = undefined;
  });
  MEMO_PROMISE = pending;
  return pending;
}

async function locateVitteRuntimeUncached(minRequired?: string): Promise<VitteRuntimePaths> {
  const cfg = vscode.workspace.getConfiguration('vitte');
  const toolchainRoot = detectVitteToolchainRoot();

  const hasCompilerSetting = Boolean(cfg.get<string>('compiler.path')?.trim());
  const cliPath = resolveFromSettingOrRoot(cfg, 'vitte', 'compiler.path')
    ?? which('vitte');
  const runtimePath = cliPath ?? 'vitte';
  const lspPath   = resolveFromSettingOrRoot(cfg, 'vitte-lsp', 'lsp.path');
  const buildPath = hasCompilerSetting ? cliPath : resolveFromSettingOrRoot(cfg, 'vitte', 'build.path') ?? cliPath;
  const fmtPath   = resolveFromSettingOrRoot(cfg, 'vitte-fmt', 'fmt.path');
  const benchPath = resolveFromSettingOrRoot(cfg, 'vitte-bench', 'bench.path');

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
export async function detectAndShowRuntimeInfo(output?: vscode.OutputChannel): Promise<void> {
  const result = await locateVitteRuntime();
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
    await detectAndShowRuntimeInfo(output);
  });

  const clearCmd = vscode.commands.registerCommand('vitte.clearRuntimeCache', () => {
    clearRuntimeLocatorCache();
    vscode.window.showInformationMessage('Vitte: runtime cache cleared. Restart the language server to apply toolchain changes.');
  });

  const pickCmd = vscode.commands.registerCommand('vitte.selectRuntime', async () => {
    const opts: vscode.OpenDialogOptions = {
      canSelectMany: false,
      canSelectFiles: true,
      canSelectFolders: false,
      openLabel: 'Sélectionner vitte',
      title: 'Sélectionner le binaire vitte',
    };
    if (isWindows()) {
      opts.filters = { Exécutables: ['exe', 'cmd', 'bat'] };
    }
    const picked = await vscode.window.showOpenDialog(opts);
    const uri = picked?.[0];
    if (!uri) return;
    const fsPath = uri.fsPath;
    if (!isExecutableSync(fsPath)) {
      vscode.window.showErrorMessage('Le fichier sélectionné ne semble pas exécutable.');
      return;
    }
    const activeFolder = vscode.window.activeTextEditor
      ? vscode.workspace.getWorkspaceFolder(vscode.window.activeTextEditor.document.uri)
      : undefined;
    const target = activeFolder
      ? vscode.ConfigurationTarget.WorkspaceFolder
      : vscode.workspace.workspaceFolders?.length
        ? vscode.ConfigurationTarget.Workspace
        : vscode.ConfigurationTarget.Global;
    await vscode.workspace.getConfiguration('vitte', activeFolder?.uri).update('compiler.path', fsPath, target);
    clearRuntimeLocatorCache();
    vscode.window.showInformationMessage(`Vitte: CLI défini sur ${fsPath}`);
  });

  ctx.subscriptions.push(detectCmd, clearCmd, pickCmd);
}
