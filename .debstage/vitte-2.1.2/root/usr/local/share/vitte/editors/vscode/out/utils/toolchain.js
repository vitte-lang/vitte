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
exports.configuredToolchainRoot = configuredToolchainRoot;
exports.resolveBinary = resolveBinary;
exports.getToolchain = getToolchain;
exports.refreshToolchain = refreshToolchain;
exports.registerToolchainCommands = registerToolchainCommands;
exports.which = which;
exports.formatToolchainStatus = formatToolchainStatus;
exports.childEnvWithToolchain = childEnvWithToolchain;
const vscode = __importStar(require("vscode"));
const path = __importStar(require("path"));
const os = __importStar(require("os"));
const cp = __importStar(require("child_process"));
const fs_1 = require("./fs");
const CACHE_KEY = 'vitte.toolchain.cache';
const CACHE_TTL_MS = 30000; // 30s — safe for repeated commands
/** Resolve the configured toolchain root from settings/env, or undefined. */
function configuredToolchainRoot() {
    const cfg = vscode.workspace.getConfiguration('vitte');
    const s1 = cfg.get('toolchain.root');
    const s2 = cfg.get('toolchainPath'); // legacy
    const env = process.env.VITTE_TOOLCHAIN_ROOT;
    return firstNonEmpty(s1, s2, env);
}
/** Try to resolve a binary path by checking setting, toolchain root, then PATH. */
async function resolveBinary(name, opts) {
    const cfg = vscode.workspace.getConfiguration('vitte');
    // 1) explicit settings
    const keys = opts?.settingKeys ?? [];
    for (const k of keys) {
        const raw = cfg.get(k)?.trim();
        if (raw) {
            const p = absolutize(raw, opts?.root);
            if (await (0, fs_1.exists)(p))
                return p;
        }
    }
    // 2) toolchain root/bin
    const root = opts?.root ?? configuredToolchainRoot();
    if (root) {
        const candidate = path.join(root, 'bin', nameWithExt(name));
        if (await (0, fs_1.exists)(candidate))
            return candidate;
    }
    // 3) PATH lookup
    return await which(name);
}
/** Main resolution entry point with short-lived cache. */
async function getToolchain(ctx, forceRefresh = false) {
    const state = ctx?.globalState;
    if (!forceRefresh && state) {
        const cache = state.get(CACHE_KEY);
        if (cache && Date.now() - cache.ts < CACHE_TTL_MS)
            return cache.info;
    }
    const messages = [];
    const root = configuredToolchainRoot();
    if (!root)
        messages.push('Aucun "vitte.toolchain.root" configuré — recherche via PATH…');
    const rootOpt = root !== undefined ? { root } : undefined;
    const runtime = await resolveBinary('vitte-runtime', { settingKeys: ['debug.program', 'runtime.path', 'runtime'], ...rootOpt });
    const lsp = await resolveBinary('vitte-lsp', { settingKeys: ['lsp.path'], ...rootOpt });
    const build = await resolveBinary('vitte-build', { settingKeys: ['build.path'], ...rootOpt });
    const fmt = await resolveBinary('vitte-fmt', { settingKeys: ['fmt.path'], ...rootOpt });
    const bench = await resolveBinary('vitte-bench', { settingKeys: ['bench.path'], ...rootOpt });
    const bins = {};
    if (runtime !== undefined)
        bins.runtime = runtime;
    if (lsp !== undefined)
        bins.lsp = lsp;
    if (build !== undefined)
        bins.build = build;
    if (fmt !== undefined)
        bins.fmt = fmt;
    if (bench !== undefined)
        bins.bench = bench;
    const ok = !!runtime;
    if (!ok)
        messages.push('vitte-runtime introuvable (paramètre vitte.debug.program ou PATH).');
    const info = { bins, versions: {}, ok, messages };
    if (root !== undefined)
        info.root = root;
    // Optionally read versions (best-effort, don't throw)
    await Promise.all(Object.entries(bins).map(async ([k, p]) => {
        const ver = await readVersion(p);
        if (ver)
            info.versions[k] = ver;
    }));
    if (state)
        await state.update(CACHE_KEY, { info, ts: Date.now() });
    return info;
}
/** Force refresh of the toolchain cache. */
async function refreshToolchain(ctx) {
    if (ctx)
        await ctx.globalState.update(CACHE_KEY, undefined);
    return getToolchain(ctx, true);
}
/** Register a user-facing command that shows a quick status of the toolchain. */
function registerToolchainCommands(ctx) {
    const cmd = vscode.commands.registerCommand('vitte.detectToolchain', async () => {
        const info = await getToolchain(ctx, true);
        const out = vscode.window.createOutputChannel('Vitte Toolchain');
        out.clear();
        out.appendLine('[Vitte] Toolchain detection');
        if (info.root)
            out.appendLine(`root: ${info.root}`);
        for (const [k, p] of Object.entries(info.bins))
            out.appendLine(`${k}: ${p ?? '—'}`);
        for (const [k, v] of Object.entries(info.versions))
            out.appendLine(`${k} version: ${v}`);
        if (info.messages.length)
            out.appendLine(info.messages.map(m => `! ${m}`).join('\n'));
        out.show(true);
    });
    ctx.subscriptions.push(cmd);
}
// ------------------------- internals -------------------------
function nameWithExt(name) {
    // Defensive: ensure a string even under odd narrowings
    if (!name)
        return '';
    if (process.platform === 'win32') {
        const pathext = (process.env.PATHEXT ?? '.EXE;.CMD;.BAT').toLowerCase().split(';');
        for (const ext of pathext)
            if (name.toLowerCase().endsWith(ext.toLowerCase()))
                return name; // already has an ext
        return name + '.exe';
    }
    return name;
}
/** PATH lookup that respects PATHEXT on Windows. */
async function which(bin) {
    if (!bin)
        return undefined;
    const b = bin; // narrow to plain string for downstream calls
    const parts = (process.env.PATH ?? '').split(path.delimiter).filter(Boolean);
    const exts = process.platform === 'win32' ? (process.env.PATHEXT ?? '.EXE;.CMD;.BAT').split(';') : [''];
    for (const dir of parts) {
        for (const ext of exts) {
            const candidate = path.join(dir, b + ext);
            if (await (0, fs_1.exists)(candidate))
                return candidate;
        }
    }
    return undefined;
}
async function readVersion(bin) {
    try {
        const out = await execCapture([bin, '--version']);
        const match = /\b(\d+\.\d+\.\d+(?:[-+][\w.]+)?)\b/.exec(String(out));
        return match ? match[1] : String(out).trim().split(/\r?\n/)[0];
    }
    catch {
        return undefined;
    }
}
function firstNonEmpty(...values) {
    for (const value of values) {
        if (typeof value === 'string') {
            const trimmed = value.trim();
            if (trimmed.length > 0)
                return trimmed;
        }
    }
    return undefined;
}
/** Execute a command and capture stdout (utf-8). */
async function execCapture(cmd, opts) {
    return new Promise((resolve, reject) => {
        const timeout = Math.max(1000, Math.min(10000, opts?.timeoutMs ?? 3000));
        const fileOrArray = Array.isArray(cmd) ? cmd[0] : cmd;
        const args = Array.isArray(cmd) ? cmd.slice(1) : [];
        // ⛑️ Narrow strict: forbid undefined/empty command reaching spawn
        if (typeof fileOrArray !== 'string' || fileOrArray.length === 0) {
            reject(new Error('invalid command'));
            return;
        }
        const file = fileOrArray;
        const options = {
            cwd: opts?.cwd,
            env: opts?.env ?? process.env,
            shell: false,
        };
        const child = cp.spawn(file, args, options); // type: cp.ChildProcess
        let out = '';
        let err = '';
        const to = setTimeout(() => {
            try {
                child.kill();
            }
            catch { /* ignore */ }
            reject(new Error('timeout'));
        }, timeout);
        child.stdout?.on('data', (b) => { out += b.toString(); });
        child.stderr?.on('data', (b) => { err += b.toString(); });
        child.on('error', (e) => { clearTimeout(to); reject(e); });
        child.on('close', (code) => {
            clearTimeout(to);
            if (code === 0)
                resolve(out);
            else if (out)
                resolve(out); // some tools print version even on non-zero exit
            else
                reject(new Error(err || `exit ${code}`));
        });
    });
}
/** Convenience: return a friendly snapshot string (for status bars, etc.). */
function formatToolchainStatus(info) {
    const v = info.versions.runtime ?? info.versions.lsp ?? info.versions.build;
    const ver = v ? `v${v}` : 'unknown';
    const ok = info.ok ? '✓' : '✗';
    return `Vitte ${ok} · ${ver}`;
}
/** Returns recommended default env for child processes (PATH patched with toolchain bin). */
function childEnvWithToolchain(info, base = process.env) {
    const env = { ...base };
    let binDir;
    const r = info.root;
    if (typeof r === 'string' && r.length > 0) {
        binDir = path.join(r, 'bin');
    }
    if (binDir)
        env.PATH = binDir + path.delimiter + (base.PATH ?? '');
    env.HOME ?? (env.HOME = os.homedir());
    return env;
}
function absolutize(p, root) {
    if (path.isAbsolute(p))
        return p;
    if (root)
        return path.join(root, p);
    return path.resolve(p);
}
//# sourceMappingURL=toolchain.js.map