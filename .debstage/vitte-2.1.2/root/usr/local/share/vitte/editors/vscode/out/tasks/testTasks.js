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
exports.testCommandLine = testCommandLine;
exports.testReportDir = testReportDir;
const vscode = __importStar(require("vscode"));
const path = __importStar(require("path"));
const fs = __importStar(require("fs"));
// Tiny helpers (strict/optional-safe)
function ensureString(x, fallback) {
    return (typeof x === 'string' && x.length > 0) ? x : fallback;
}
function sanitizeArgs(args) {
    return args.filter((a) => typeof a === 'string' && a.length > 0);
}
// Compat: Uri.joinPath may not exist on older @types/vscode
function joinPathCompat(base, ...parts) {
    const anyUri = vscode.Uri;
    if (typeof anyUri.joinPath === 'function')
        return anyUri.joinPath(base, ...parts);
    return vscode.Uri.file(path.join(base.fsPath, ...parts));
}
/** Build the shell command to run tests. */
function testCommandLine(withReport) {
    const cfg = vscode.workspace.getConfiguration('vitte');
    // Prefer explicit runtime if configured (string), else fallback to 'vitte'
    const runtime = ensureString(cfg.get('runtime.path') ?? cfg.get('debug.program'), 'vitte');
    // Optional args from settings
    const extra = sanitizeArgs([
        cfg.get('test.args'),
    ]);
    // Compose: `vitte test [--report] ...extra`
    const parts = [runtime, 'test'];
    if (withReport)
        parts.push('--report');
    parts.push(...extra);
    // Return a single shell string (ShellExecution will pass it to the shell)
    return parts.join(' ');
}
/** Directory where test reports are written (best-effort). */
function testReportDir() {
    const cfg = vscode.workspace.getConfiguration('vitte');
    // Allow override via settings
    const override = cfg.get('test.reportDir');
    if (override && override.length > 0)
        return override;
    // Default: <first-workspace>/.vitte/reports/tests
    const wf = vscode.workspace.workspaceFolders?.[0];
    if (!wf)
        return undefined;
    const uri = joinPathCompat(wf.uri, '.vitte', 'reports', 'tests');
    const p = uri.fsPath;
    // Best-effort ensure the directory exists
    try {
        fs.mkdirSync(p, { recursive: true });
    }
    catch { /* ignore */ }
    return p;
}
//# sourceMappingURL=testTasks.js.map