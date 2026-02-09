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
exports.registerVitlDebugAdapter = registerVitlDebugAdapter;
const vscode = __importStar(require("vscode"));
const cp = __importStar(require("child_process"));
const net = __importStar(require("net"));
const path = __importStar(require("path"));
/**
 * Vitl Debug Adapter
 * Tries to start a DAP server from `vitl-runtime` first, then falls back to stdio.
 * Understands workspace settings under `vitte.*` to keep parity with Vitte.
 */
function registerVitlDebugAdapter(context) {
    const factory = new VitlDebugAdapterFactory();
    context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('vitl', factory), factory);
}
class VitlDebugAdapterFactory {
    constructor() {
        this.sessions = new Map();
    }
    dispose() {
        for (const proc of this.sessions.values()) {
            try {
                proc.kill();
            }
            catch { /* noop */ }
        }
        this.sessions.clear();
    }
    async createDebugAdapterDescriptor(session) {
        const cfg = vscode.workspace.getConfiguration('vitte');
        const { program, cwd, extraArgs } = this.resolveRuntimeAndArgs(cfg, session);
        // Prefer server mode
        const server = await this.tryStartServer(program, cwd, extraArgs);
        if (server.ok) {
            this.sessions.set(session.id, server.proc);
            return new vscode.DebugAdapterServer(server.port, '127.0.0.1');
        }
        // Fallback to stdio
        const exec = this.buildExecutable(program, cwd, extraArgs);
        const opts = { cwd: exec.cwd };
        const envMap = this.toVscodeEnv(exec.env);
        if (envMap) {
            opts.env = envMap;
        }
        return new vscode.DebugAdapterExecutable(exec.command, exec.args, opts);
    }
    // ---- helpers (typing/guards) ------------------------------------------------
    toVscodeEnv(env) {
        if (!env)
            return undefined;
        const out = {};
        for (const [key, value] of Object.entries(env)) {
            if (typeof value === 'string')
                out[key] = value;
        }
        return Object.keys(out).length > 0 ? out : undefined;
    }
    ensureString(x, fallback) {
        return (typeof x === 'string' && x.length > 0) ? x : fallback;
    }
    sanitizeArgs(args) {
        return args.filter((a) => typeof a === 'string' && a.length > 0);
    }
    // ---- resolution -------------------------------------------------------------
    resolveRuntimeAndArgs(cfg, session) {
        // Settings or default (accept vitl‑specific first, then vitte.debug.program, else 'vitl-runtime')
        const explicit = cfg.get('vitl.debug.program')
            ?? cfg.get('debug.program')
            ?? 'vitl-runtime';
        const toolchainRoot = cfg.get('toolchain.root') ?? cfg.get('toolchainPath');
        const program = toolchainRoot && explicit && !path.isAbsolute(explicit)
            ? path.join(toolchainRoot, explicit)
            : explicit;
        const rawConfig = session.configuration;
        const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ?? process.cwd();
        const cwdValue = typeof rawConfig.cwd === 'string' && rawConfig.cwd.length > 0 ? rawConfig.cwd : workspaceRoot;
        // Allow passing args from launch.json
        const configArgs = rawConfig.args;
        let rawArgs = [];
        if (Array.isArray(configArgs)) {
            rawArgs = configArgs.map((value) => (typeof value === 'string' ? value : undefined));
        }
        else if (typeof configArgs === 'string' && configArgs.length > 0) {
            rawArgs = [configArgs];
        }
        const extraArgs = this.sanitizeArgs(rawArgs);
        return { program: this.ensureString(program, 'vitl-runtime'), cwd: this.ensureString(cwdValue, process.cwd()), extraArgs };
    }
    // ---- server probing ---------------------------------------------------------
    async tryStartServer(program, cwd, extraArgs) {
        // Strategy A: `dap --port 0`
        const a = await this.spawnAndDetectPort(program, ['dap', '--port', '0', ...extraArgs], cwd);
        if (a.ok)
            return a;
        // Strategy B: `--dap --port 0`
        const b = await this.spawnAndDetectPort(program, ['--dap', '--port', '0', ...extraArgs], cwd);
        if (b.ok)
            return b;
        // Strategy C: `dap` and parse announced port
        const c = await this.spawnAndDetectPort(program, ['dap', ...extraArgs], cwd);
        if (c.ok)
            return c;
        return { ok: false, error: new Error('No DAP server mode detected for vitl runtime') };
    }
    spawnAndDetectPort(program, args, cwd) {
        return new Promise((resolve) => {
            let resolved = false;
            const cmd = this.ensureString(program, 'vitl-runtime');
            const argv = this.sanitizeArgs(args);
            const proc = cp.spawn(cmd, argv, { cwd: this.ensureString(cwd, process.cwd()), env: process.env, stdio: ['ignore', 'pipe', 'pipe'] });
            const finish = (err) => {
                if (!resolved) {
                    resolved = true;
                    resolve({ ok: false, error: err ?? new Error('port detection failed') });
                }
            };
            const sniff = (buf) => {
                if (resolved)
                    return;
                const text = buf.toString();
                const match = /(?:listening\s+on\s+[^:]+:|port\s*=|DAP_PORT=)(\d{3,5})/i.exec(text);
                if (!match)
                    return;
                const port = Number.parseInt(match[1] ?? '', 10);
                if (!Number.isInteger(port) || port <= 0 || port >= 65536)
                    return;
                resolved = true;
                // quick probe
                const sock = new net.Socket();
                sock.once('error', () => resolve({ ok: true, proc, port }));
                sock.connect(port, '127.0.0.1', () => {
                    sock.destroy();
                    resolve({ ok: true, proc, port });
                });
            };
            proc.stdout?.on('data', sniff);
            proc.stderr?.on('data', sniff);
            proc.once('error', (e) => finish(e instanceof Error ? e : new Error(String(e))));
            proc.once('exit', (code, sig) => finish(new Error(`exited early (code=${String(code)}, sig=${String(sig ?? 'null')})`)));
            setTimeout(() => finish(new Error('timeout waiting for dap port')), 4000);
        });
    }
    // ---- stdio fallback ---------------------------------------------------------
    buildExecutable(program, cwd, extraArgs) {
        // Try stdio flags, otherwise pass through
        const candidates = [
            ['dap', '--stdio'],
            ['--dap', '--stdio'],
            []
        ];
        const base = this.sanitizeArgs(extraArgs);
        const args = base.length > 0 ? base : candidates[0];
        return { command: program, args, cwd, env: process.env };
    }
}
//# sourceMappingURL=vitlDebugAdapter.js.map