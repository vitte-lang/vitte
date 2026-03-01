"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.detectVitteToolchainRoot = detectVitteToolchainRoot;
exports.clearRuntimeLocatorCache = clearRuntimeLocatorCache;
exports.locateVitteRuntime = locateVitteRuntime;
exports.detectAndShowRuntimeInfo = detectAndShowRuntimeInfo;
exports.registerRuntimeLocatorCommand = registerRuntimeLocatorCommand;
const vscode = __importStar(require("vscode"));
const path = __importStar(require("path"));
const fs = __importStar(require("fs"));
const os = __importStar(require("os"));
const cp = __importStar(require("child_process"));
// -----------------------------
// fs/helpers (cross-platform)
// -----------------------------
function isWindows() { return os.platform() === 'win32'; }
function exists(p) {
    if (!p)
        return false;
    try {
        fs.accessSync(p, fs.constants.F_OK);
        return true;
    }
    catch {
        return false;
    }
}
function isExecutableSync(p) {
    if (!p)
        return false;
    try {
        if (isWindows()) {
            fs.accessSync(p, fs.constants.F_OK);
            return true;
        }
        fs.accessSync(p, fs.constants.X_OK);
        return true;
    }
    catch {
        return false;
    }
}
function real(p) {
    if (!p)
        return undefined;
    try {
        const rs = fs.realpathSync;
        if (typeof rs.native === 'function') {
            return rs.native(p);
        }
        return fs.realpathSync(p);
    }
    catch {
        return p;
    }
}
function firstNonEmpty(...values) {
    for (const value of values) {
        if (value && value.length > 0)
            return value;
    }
    return undefined;
}
/** In-memory cache for lookups to avoid excessive fs access */
const WHICH_CACHE = new Map();
/** Try resolve binary with PATH and common install roots. */
function which(bin) {
    if (WHICH_CACHE.has(bin))
        return WHICH_CACHE.get(bin);
    const envPath = process.env.PATH ?? '';
    const exts = isWindows() ? (process.env.PATHEXT?.split(';').filter(Boolean) ?? ['.exe', '.cmd', '.bat']) : [''];
    const parts = envPath.split(path.delimiter).filter(Boolean);
    for (const p of parts) {
        for (const ext of exts) {
            const full = path.join(p, bin + ext);
            if (isExecutableSync(full)) {
                const r = real(full) ?? full;
                WHICH_CACHE.set(bin, r);
                return r;
            }
        }
    }
    // Common extra roots: Homebrew, /usr/local, Nix, ~/.local/bin
    const candidates = [];
    const home = os.homedir();
    if (isWindows()) {
        candidates.push(path.join(process.env.ProgramFiles ?? 'C:/Program Files', 'Vitte', 'bin', `${bin}.exe`));
    }
    else {
        candidates.push('/opt/homebrew/bin/' + bin);
        candidates.push('/usr/local/bin/' + bin);
        candidates.push('/usr/bin/' + bin);
        candidates.push(path.join(home, '.local', 'bin', bin));
        // Nix store symlink path
        candidates.push('/run/current-system/sw/bin/' + bin);
        candidates.push('/nix/var/nix/profiles/default/bin/' + bin);
    }
    for (const c of candidates) {
        if (isExecutableSync(c)) {
            const r = real(c) ?? c;
            WHICH_CACHE.set(bin, r);
            return r;
        }
    }
    // As a last resort, try Homebrew prefix discovery (guarded)
    if (!isWindows()) {
        try {
            const out = cp.spawnSync('brew', ['--prefix'], { encoding: 'utf8', timeout: 350 });
            const prefix = (out.status === 0 && typeof out.stdout === 'string') ? out.stdout.trim() : '';
            if (prefix) {
                const full = path.join(prefix, 'bin', bin);
                if (isExecutableSync(full)) {
                    const r = real(full) ?? full;
                    WHICH_CACHE.set(bin, r);
                    return r;
                }
            }
        }
        catch { /* ignore */ }
    }
    WHICH_CACHE.set(bin, undefined);
    return undefined;
}
function parseSemver(input) {
    if (!input)
        return undefined;
    const match = /v?(\d+)\.(\d+)\.(\d+)(?:[-+]([^\s]+))?/.exec(input.trim());
    if (!match)
        return undefined;
    const [, sMaj, sMin, sPatch, pre] = match;
    if (!sMaj || !sMin || !sPatch)
        return undefined;
    const ver = { major: Number.parseInt(sMaj, 10), minor: Number.parseInt(sMin, 10), patch: Number.parseInt(sPatch, 10) };
    if (pre !== undefined)
        ver.prerelease = pre;
    return ver;
}
function cmpSemver(a, b) {
    if (a.major !== b.major)
        return a.major - b.major;
    if (a.minor !== b.minor)
        return a.minor - b.minor;
    if (a.patch !== b.patch)
        return a.patch - b.patch;
    // prereleases sort before stable
    if (a.prerelease && !b.prerelease)
        return -1;
    if (!a.prerelease && b.prerelease)
        return 1;
    return 0;
}
// -----------------------------
// discovery
// -----------------------------
function discoverWorkspaceBinRoots() {
    const roots = [];
    const folders = vscode.workspace.workspaceFolders ?? [];
    for (const f of folders) {
        const base = f.uri.fsPath;
        const cands = [
            path.join(base, '.vitte', 'bin'),
            path.join(base, 'vitte', 'bin'),
            path.join(base, '.bin'),
            path.join(base, 'tools', 'vitte', 'bin'),
        ];
        for (const c of cands)
            if (exists(c))
                roots.push(c);
    }
    return roots;
}
/** Detect toolchain root by checking settings, env vars, workspace bins, or bin location. */
function detectVitteToolchainRoot() {
    const cfg = vscode.workspace.getConfiguration('vitte');
    const explicit = cfg.get('toolchain.root') ?? cfg.get('toolchainPath');
    if (explicit && isExecutableSync(path.join(explicit, isWindows() ? 'bin/vitte-runtime.exe' : 'bin/vitte-runtime')))
        return explicit;
    const envRoot = firstNonEmpty(process.env.VITTE_TOOLCHAIN_ROOT, process.env.VITTE_ROOT, process.env.VITTE_HOME, process.env.VITTE_BIN_DIR);
    if (envRoot) {
        const candidate = path.join(envRoot, 'bin', isWindows() ? 'vitte-runtime.exe' : 'vitte-runtime');
        if (isExecutableSync(candidate))
            return real(envRoot) ?? envRoot;
    }
    // workspace embedded bins (e.g., monorepo/tools)
    for (const r of discoverWorkspaceBinRoots()) {
        const candidate = path.join(r, isWindows() ? 'vitte-runtime.exe' : 'vitte-runtime');
        if (isExecutableSync(candidate))
            return real(path.resolve(r, '..')) ?? path.resolve(r, '..');
    }
    const lsp = which('vitte-lsp');
    if (lsp)
        return path.resolve(path.dirname(lsp), '..');
    const runtime = which('vitte-runtime');
    if (runtime)
        return path.resolve(path.dirname(runtime), '..');
    return undefined;
}
function getVersion(bin) {
    if (!bin)
        return undefined;
    try {
        const res = cp.spawnSync(bin, ['--version'], { encoding: 'utf8', timeout: 700 });
        const stdout = (res.stdout ?? '').trim();
        const stderr = (res.stderr ?? '').trim();
        const out = stdout.length > 0 ? stdout : stderr;
        const match = /\b(v?\d+\.\d+\.\d+(?:[-+].*)?)\b/.exec(out);
        if (match)
            return match[1];
        return out.length > 0 ? out : undefined;
    }
    catch {
        return undefined;
    }
}
// -----------------------------
// core location with memoization
// -----------------------------
let MEMO_RESULT;
function clearRuntimeLocatorCache() { MEMO_RESULT = undefined; }
function resolveFromSettingOrRoot(cfg, root, name, settingKey) {
    const fromSetting = cfg.get(settingKey);
    if (fromSetting && isExecutableSync(fromSetting))
        return real(fromSetting) ?? fromSetting;
    if (root) {
        const candidate = path.join(root, 'bin', name + (isWindows() ? '.exe' : ''));
        if (isExecutableSync(candidate))
            return real(candidate) ?? candidate;
    }
    // workspace overrides
    for (const r of discoverWorkspaceBinRoots()) {
        const candidate = path.join(r, name + (isWindows() ? '.exe' : ''));
        if (isExecutableSync(candidate))
            return real(candidate) ?? candidate;
    }
    return which(name);
}
/**
 * Locate runtime and companion binaries. Memoized per session.
 * Prefers workspace settings, then toolchain root, then workspace bins, then PATH.
 */
function locateVitteRuntime(minRequired) {
    if (MEMO_RESULT)
        return Promise.resolve(MEMO_RESULT);
    const cfg = vscode.workspace.getConfiguration('vitte');
    const toolchainRoot = detectVitteToolchainRoot();
    const runtimePath = resolveFromSettingOrRoot(cfg, toolchainRoot, 'vitte-runtime', 'runtime.path')
        ?? resolveFromSettingOrRoot(cfg, toolchainRoot, 'vitte-runtime', 'debug.program')
        ?? 'vitte-runtime';
    const lspPath = resolveFromSettingOrRoot(cfg, toolchainRoot, 'vitte-lsp', 'lsp.path');
    const buildPath = resolveFromSettingOrRoot(cfg, toolchainRoot, 'vitte-build', 'build.path');
    const fmtPath = resolveFromSettingOrRoot(cfg, toolchainRoot, 'vitte-fmt', 'fmt.path');
    const benchPath = resolveFromSettingOrRoot(cfg, toolchainRoot, 'vitte-bench', 'bench.path');
    const found = isExecutableSync(runtimePath);
    const result = { runtimePath, found };
    if (toolchainRoot)
        result.toolchainRoot = toolchainRoot;
    if (lspPath)
        result.lspPath = lspPath;
    if (buildPath)
        result.buildPath = buildPath;
    if (fmtPath)
        result.fmtPath = fmtPath;
    if (benchPath)
        result.benchPath = benchPath;
    // Versions
    const versions = {};
    const vr = getVersion(found ? runtimePath : undefined);
    if (vr) {
        versions.runtime = vr;
    }
    const vl = getVersion(lspPath);
    if (vl) {
        versions.lsp = vl;
    }
    const vb = getVersion(buildPath);
    if (vb) {
        versions.build = vb;
    }
    const vf = getVersion(fmtPath);
    if (vf) {
        versions.fmt = vf;
    }
    const vn = getVersion(benchPath);
    if (vn) {
        versions.bench = vn;
    }
    if (Object.keys(versions).length > 0) {
        result.versions = versions;
    }
    // Optional min version check
    if (minRequired && versions.runtime) {
        const have = parseSemver(versions.runtime);
        const need = parseSemver(minRequired);
        if (have && need && cmpSemver(have, need) < 0) {
            result.message = `vitte-runtime ${versions.runtime} is older than required ${minRequired}`;
        }
    }
    result.message ?? (result.message = found
        ? `vitte-runtime detected at ${runtimePath}`
        : 'vitte-runtime not found in PATH or toolchain root');
    MEMO_RESULT = result;
    return Promise.resolve(result);
}
/** Quick command for users: Detect and show runtime info in VSCode output */
async function detectAndShowRuntimeInfo(output) {
    const result = await locateVitteRuntime();
    const out = output ?? vscode.window.createOutputChannel('Vitte');
    out.appendLine('[Vitte] Toolchain detection');
    if (result.toolchainRoot)
        out.appendLine(`Toolchain root: ${result.toolchainRoot}`);
    out.appendLine(`Runtime: ${result.runtimePath}`);
    if (result.lspPath)
        out.appendLine(`LSP: ${result.lspPath}`);
    if (result.buildPath)
        out.appendLine(`Build: ${result.buildPath}`);
    if (result.fmtPath)
        out.appendLine(`Fmt: ${result.fmtPath}`);
    if (result.benchPath)
        out.appendLine(`Bench: ${result.benchPath}`);
    if (result.versions) {
        const v = result.versions;
        out.appendLine('Versions:');
        if (v?.runtime)
            out.appendLine(`  runtime: ${v.runtime}`);
        if (v?.lsp)
            out.appendLine(`  lsp:     ${v.lsp}`);
        if (v?.build)
            out.appendLine(`  build:   ${v.build}`);
        if (v?.fmt)
            out.appendLine(`  fmt:     ${v.fmt}`);
        if (v?.bench)
            out.appendLine(`  bench:   ${v.bench}`);
    }
    out.appendLine(`Found: ${String(result.found)}`);
    if (result.message)
        out.appendLine(result.message);
    out.show(true);
}
/**
 * Register commands:
 *  - vitte.detectToolchain: show detected paths/versions
 *  - vitte.selectRuntime: let user pick a binary and persist to settings
 *  - vitte.clearRuntimeCache: invalidate memoized result
 */
function registerRuntimeLocatorCommand(ctx) {
    const detectCmd = vscode.commands.registerCommand('vitte.detectToolchain', async () => {
        await detectAndShowRuntimeInfo();
    });
    const clearCmd = vscode.commands.registerCommand('vitte.clearRuntimeCache', () => {
        clearRuntimeLocatorCache();
        vscode.window.showInformationMessage('Vitte: runtime cache cleared.');
    });
    const pickCmd = vscode.commands.registerCommand('vitte.selectRuntime', async () => {
        const opts = {
            canSelectMany: false,
            canSelectFiles: true,
            canSelectFolders: false,
            openLabel: 'Sélectionner vitte-runtime',
            title: 'Sélectionner le binaire vitte-runtime',
        };
        if (isWindows()) {
            opts.filters = { Exécutables: ['exe', 'cmd', 'bat'] };
        }
        const picked = await vscode.window.showOpenDialog(opts);
        const uri = picked?.[0];
        if (!uri)
            return;
        const fsPath = uri.fsPath;
        if (!isExecutableSync(fsPath)) {
            vscode.window.showErrorMessage('Le fichier sélectionné ne semble pas exécutable.');
            return;
        }
        await vscode.workspace.getConfiguration('vitte').update('runtime.path', fsPath, vscode.ConfigurationTarget.Workspace);
        clearRuntimeLocatorCache();
        vscode.window.showInformationMessage(`Vitte: runtime défini sur ${fsPath}`);
    });
    ctx.subscriptions.push(detectCmd, clearCmd, pickCmd);
}
//# sourceMappingURL=runtimeLocator.js.map