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
exports.VitteDebugAdapterDescriptorFactory = void 0;
exports.registerDebugFactory = registerDebugFactory;
const vscode = __importStar(require("vscode"));
const cp = __importStar(require("child_process"));
const net = __importStar(require("net"));
const path = __importStar(require("path"));
/**
 * Vitte Debug Adapter factory.
 * Spawns `vitte-runtime` in DAP server mode when possible.
 * Falls back to stdio if server mode is not available.
 */
class VitteDebugAdapterDescriptorFactory {
    constructor() {
        this._subscriptions = [];
        this._sessions = new Map();
    }
    toVscodeEnv(env) {
        if (!env)
            return undefined;
        const out = {};
        for (const [key, value] of Object.entries(env)) {
            if (typeof value === 'string')
                out[key] = value;
        }
        return out;
    }
    ensureString(x, fallback) {
        return (typeof x === 'string' && x.length > 0) ? x : fallback;
    }
    sanitizeArgs(args) {
        return args.filter((a) => typeof a === 'string' && a.length > 0);
    }
    dispose() {
        for (const d of this._subscriptions)
            d.dispose();
        for (const { proc } of this._sessions.values()) {
            try {
                proc.kill();
            }
            catch { /* noop */ }
        }
        this._sessions.clear();
    }
    async createDebugAdapterDescriptor(session) {
        const cfg = vscode.workspace.getConfiguration('vitte');
        const program = this.resolveRuntimePath(cfg);
        const rawArgs = Array.isArray(session.configuration.args) ? session.configuration.args : [];
        const extraArgs = this.sanitizeArgs(rawArgs);
        const cwd = typeof session.configuration.cwd === 'string' && session.configuration.cwd.length > 0
            ? session.configuration.cwd
            : (vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ?? process.cwd());
        // Try DAP server mode first.
        const serverTry = await this.tryStartServer(program, cwd, extraArgs);
        if (serverTry.ok) {
            const { proc, port } = serverTry;
            this._sessions.set(session.id, { proc, port });
            return new vscode.DebugAdapterServer(port, '127.0.0.1');
        }
        // Fallback to stdio executable.
        const exec = this.buildExecutable(program, cwd, extraArgs);
        const opts = { cwd: exec.cwd };
        const envMap = this.toVscodeEnv(exec.env);
        if (envMap) {
            opts.env = envMap;
        }
        const desc = new vscode.DebugAdapterExecutable(exec.command, exec.args, opts);
        return desc;
    }
    resolveRuntimePath(cfg) {
        // Prefer project setting `vitte.debug.program`, then `vitte.runtime` or `vitte.build.path`, then default.
        const direct = cfg.get('debug.program');
        if (direct?.trim().length)
            return direct;
        const toolchainRoot = cfg.get('toolchainPath') ?? cfg.get('toolchain.root');
        const candidate = cfg.get('runtime.path') ?? cfg.get('runtime') ?? 'vitte-runtime';
        if (toolchainRoot && !path.isAbsolute(candidate)) {
            return path.join(toolchainRoot, candidate);
        }
        return candidate;
    }
    async tryStartServer(program, cwd, extraArgs) {
        // Strategy A: runtime supports `dap --port 0` and prints the selected port.
        const argsA = ['dap', '--port', '0', ...extraArgs];
        const attemptA = await this.spawnAndDetectPort(program, argsA, cwd);
        if (attemptA.ok)
            return attemptA;
        // Strategy B: runtime supports `--dap --port 0`.
        const argsB = ['--dap', '--port', '0', ...extraArgs];
        const attemptB = await this.spawnAndDetectPort(program, argsB, cwd);
        if (attemptB.ok)
            return attemptB;
        // Strategy C: runtime supports `dap` with a fixed ephemeral port printed on stdout.
        const argsC = ['dap', ...extraArgs];
        const attemptC = await this.spawnAndDetectPort(program, argsC, cwd);
        if (attemptC.ok)
            return attemptC;
        return { ok: false, error: new Error('DAP server mode not detected on vitte-runtime.') };
    }
    spawnAndDetectPort(program, args, cwd) {
        return new Promise((resolve) => {
            let resolved = false;
            const cmd = this.ensureString(program, 'vitte-runtime');
            const argv = this.sanitizeArgs(args);
            const proc = cp.spawn(cmd, argv, { cwd: this.ensureString(cwd, process.cwd()), env: process.env, stdio: ['ignore', 'pipe', 'pipe'] });
            const cleanup = (err) => {
                if (!resolved) {
                    resolved = true;
                    resolve({ ok: false, error: err ?? new Error('Failed to detect DAP port') });
                }
            };
            const onData = (data) => {
                if (resolved)
                    return;
                const text = data.toString();
                // Common patterns to detect an announced port
                // Examples:
                //  - DAP listening on 127.0.0.1:51234
                //  - debug-adapter: port=51234
                //  - DAP_PORT=51234
                const portMatch = /(?:listening\s+on\s+[^:]+:|port\s*=|DAP_PORT=)(\d{3,5})/i;
                const match = portMatch.exec(text);
                if (match) {
                    const g = match[1];
                    if (!g)
                        return; // group may be undefined in TS types
                    const port = Number.parseInt(g, 10);
                    if (Number.isInteger(port) && port > 0 && port < 65536) {
                        resolved = true;
                        // Verify the port is accepting connections briefly
                        const sock = new net.Socket();
                        sock.once('error', () => {
                            // Might not be ready yet, still resolve and let VSCode retry
                            resolve({ ok: true, proc, port });
                        });
                        sock.connect(port, '127.0.0.1', () => {
                            sock.destroy();
                            resolve({ ok: true, proc, port });
                        });
                    }
                }
            };
            proc.stdout?.on('data', onData);
            proc.stderr?.on('data', onData);
            proc.once('error', (err) => cleanup(err));
            proc.once('exit', (code, signal) => {
                if (!resolved) {
                    cleanup(new Error(`runtime exited before announcing port (code=${code}, signal=${signal ?? 'null'})`));
                }
            });
            // Safety timeout
            setTimeout(() => cleanup(new Error('timeout waiting for DAP port')), 4000);
        });
    }
    buildExecutable(program, cwd, extraArgs) {
        // Fallback: stdio mode executed as a DebugAdapterExecutable
        // Try common flags to force stdio DAP if supported. Otherwise, leave args as-is.
        const stdioArgsCandidates = [
            ['dap', '--stdio'],
            ['--dap', '--stdio'],
            []
        ];
        let args = extraArgs;
        if (args.length === 0) {
            args = stdioArgsCandidates.find(a => a.length > 0) ?? [];
        }
        return { command: program, args: this.sanitizeArgs(args), cwd, env: process.env };
    }
}
exports.VitteDebugAdapterDescriptorFactory = VitteDebugAdapterDescriptorFactory;
function registerDebugFactory(ctx) {
    const factory = new VitteDebugAdapterDescriptorFactory();
    ctx.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('vitte', factory), factory);
}
//# sourceMappingURL=adapterFactory.js.map