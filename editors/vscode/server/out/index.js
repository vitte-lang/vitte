"use strict";
/**
 * Vitte LSP — Server entrypoint (ultra complete +++)
 * -------------------------------------------------
 * Hardened bootstrap for the Vitte Language Server process.
 * Handles lifecycle, diagnostics, environment logging, signal safety,
 * and optional CLI inspection mode (for debugging and CI).
 *
 * DO NOT initialize the LSP connection here — `./lsp` owns it.
 * Keep stdio untouched to preserve protocol integrity.
 */
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
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const node_fs_1 = __importDefault(require("node:fs"));
const node_os_1 = __importDefault(require("node:os"));
const node_path_1 = __importDefault(require("node:path"));
// Process identity
try {
    process.title = 'vitte-lsp-server';
}
catch { /* noop */ }
// ---------------------------------------------------------------------------
// Time origin and environment diagnostics
// ---------------------------------------------------------------------------
const BOOT_T0 = Date.now();
// Basic environment info (logged to stderr, never to stdout)
function logEnvInfo() {
    try {
        const info = {
            node: process.version,
            platform: `${node_os_1.default.platform()} ${node_os_1.default.release()}`,
            arch: node_os_1.default.arch(),
            pid: process.pid,
            cwd: process.cwd(),
            argv: process.argv.slice(2),
            memory: process.memoryUsage(),
        };
        console.error('[vitte-lsp] environment', JSON.stringify(info, null, 2));
    }
    catch { /* ignore */ }
}
// ---------------------------------------------------------------------------
// Hardening
// ---------------------------------------------------------------------------
process.on('uncaughtException', (err) => {
    try {
        const msg = err instanceof Error ? err.stack ?? err.message ?? String(err) : String(err);
        console.error(`[vitte-lsp] uncaughtException: ${msg}`);
    }
    catch { /* ignore */ }
});
process.on('unhandledRejection', (reason) => {
    try {
        console.error('[vitte-lsp] unhandledRejection:', reason);
    }
    catch { /* ignore */ }
});
// ---------------------------------------------------------------------------
// Graceful shutdown
// ---------------------------------------------------------------------------
const graceful = (sig) => () => {
    try {
        console.error(`[vitte-lsp] received ${sig}, exiting…`);
    }
    catch { }
    setTimeout(() => process.exit(0), 10);
};
for (const s of ['SIGINT', 'SIGTERM', 'SIGHUP']) {
    try {
        process.on(s, graceful(s));
    }
    catch { /* ignore */ }
}
// ---------------------------------------------------------------------------
// CLI utility mode (for diagnostics or testing, not LSP)
// ---------------------------------------------------------------------------
if (process.argv.includes('--info')) {
    logEnvInfo();
    console.error(`[vitte-lsp] boot time ${Date.now() - BOOT_T0}ms`);
    process.exit(0);
}
if (process.argv.includes('--check-write')) {
    const tmp = node_path_1.default.join(node_os_1.default.tmpdir(), 'vitte-lsp-check.tmp');
    try {
        node_fs_1.default.writeFileSync(tmp, 'ok');
        console.error(`[vitte-lsp] write test ok (${tmp})`);
        node_fs_1.default.unlinkSync(tmp);
    }
    catch (err) {
        console.error('[vitte-lsp] write test failed:', err);
    }
    process.exit(0);
}
// ---------------------------------------------------------------------------
// Boot message and memory baseline
// ---------------------------------------------------------------------------
try {
    const rss = (process.memoryUsage().rss / 1024 / 1024).toFixed(1);
    console.error(`[vitte-lsp] boot in ${Date.now() - BOOT_T0}ms, RSS ${rss} MB`);
}
catch { /* ignore */ }
// ---------------------------------------------------------------------------
// Load LSP server implementation (no top-level await)
Promise.resolve().then(() => __importStar(require('./lsp'))).catch((err) => {
    try {
        console.error('[vitte-lsp] failed to load ./lsp:', err);
    }
    catch { }
    process.exit(1);
});
//# sourceMappingURL=index.js.map