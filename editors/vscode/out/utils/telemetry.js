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
exports.Telemetry = void 0;
exports.getTelemetry = getTelemetry;
exports.registerTelemetry = registerTelemetry;
const vscode = __importStar(require("vscode"));
const path = __importStar(require("path"));
const fs_1 = require("./fs");
const DEFAULT_SAMPLE = 0.1;
const BATCH_MAX = 256; // max events per file
const FLUSH_INTERVAL_MS = 5000;
class Telemetry {
    constructor(ctx, opts) {
        this.enabled = false;
        this.sample = DEFAULT_SAMPLE;
        this.session = randId();
        this.extVersion = '0.0.0';
        this.queue = [];
        this.ctx = ctx;
        this.enabled = !!opts?.enabled;
        this.sample = clamp01(opts?.sampleRate ?? DEFAULT_SAMPLE);
        const extensionPkg = ctx.extension?.packageJSON;
        const version = extensionPkg?.version;
        this.extVersion = typeof version === 'string' ? version : '0.0.0';
        if (opts?.channelName)
            this.output = vscode.window.createOutputChannel(opts.channelName);
        this.armTimer();
    }
    static async create(ctx, opts) {
        const { enabled, sampleRate } = await readProjectTelemetryConfig();
        const cfg = vscode.workspace.getConfiguration('vitte');
        const enabledSetting = cfg.get('telemetry.enabled');
        const sampleSetting = cfg.get('telemetry.sampleRate');
        const t = new Telemetry(ctx, {
            ...opts,
            enabled: typeof enabledSetting === 'boolean' ? enabledSetting : enabled,
            sampleRate: typeof sampleSetting === 'number' ? sampleSetting : sampleRate,
            channelName: 'Vitte Telemetry',
        });
        return t;
    }
    dispose() {
        if (this.timer) {
            clearInterval(this.timer);
            this.timer = undefined;
        }
        void this.flush(true);
        this.output?.dispose();
    }
    isEnabled() { return this.enabled; }
    setEnabled(v) { this.enabled = !!v; }
    setSampleRate(r) { this.sample = clamp01(r); }
    /** Track an event with optional properties. */
    track(key, props) {
        if (!this.shouldSample())
            return;
        const sp = sanitize(props);
        const ev = { t: Date.now(), s: this.session, ext: this.extVersion, k: key, lvl: 'info' };
        if (sp !== undefined)
            ev.p = sp;
        this.enqueue(ev);
    }
    /** Track an error (message + optional props). */
    error(key, err, props) {
        if (!this.shouldSample())
            return;
        const msg = err instanceof Error ? err.message : String(err);
        const sp = sanitize({ ...props, error: msg });
        const ev = { t: Date.now(), s: this.session, ext: this.extVersion, k: key, lvl: 'error' };
        if (sp !== undefined)
            ev.p = sp;
        this.enqueue(ev);
    }
    /** Measure a duration around an async function. */
    async time(key, fn, props) {
        const start = Date.now();
        try {
            const res = await fn();
            this.duration(key, Date.now() - start, props);
            return res;
        }
        catch (e) {
            this.duration(key, Date.now() - start, { ...props, ok: false });
            throw e;
        }
    }
    /** Record a duration with optional properties. */
    duration(key, ms, props) {
        if (!this.shouldSample())
            return;
        const sp = sanitize(props);
        const ev = { t: Date.now(), s: this.session, ext: this.extVersion, k: key, d: Math.max(0, Math.floor(ms)), lvl: 'info' };
        if (sp !== undefined)
            ev.p = sp;
        this.enqueue(ev);
    }
    /** Force flush queued events to disk. */
    async flush(now = false) {
        if (!this.enabled || this.queue.length === 0)
            return;
        const slice = this.queue.splice(0, Math.min(this.queue.length, BATCH_MAX));
        try {
            const root = (0, fs_1.workspaceRoot)()?.fsPath;
            if (!root)
                return; // nothing we can do without a workspace
            const dir = vscode.Uri.file(`${root}/.vitte/telemetry`);
            await (0, fs_1.ensureDir)(dir);
            const ts = new Date().toISOString().replace(/[:.]/g, '-');
            const file = vscode.Uri.file(path.join(dir.fsPath, `events-${ts}-${randId()}.jsonl`));
            const lines = slice.map(e => JSON.stringify(e)).join('\n') + '\n';
            await (0, fs_1.writeFileText)(file, lines);
            this.output?.appendLine(`[telemetry] wrote ${slice.length} event(s) to ${file.fsPath}`);
        }
        catch (e) {
            const message = e instanceof Error ? e.message : String(e);
            this.output?.appendLine(`[telemetry] flush failed: ${message}`);
        }
        if (!now)
            this.armTimer();
    }
    // ---- internals ----
    armTimer() {
        if (this.timer)
            clearInterval(this.timer);
        this.timer = setInterval(() => { void this.flush(); }, FLUSH_INTERVAL_MS);
    }
    shouldSample() {
        return this.enabled && Math.random() < this.sample;
    }
    enqueue(ev) {
        this.queue.push(ev);
        if (this.queue.length >= BATCH_MAX) {
            void this.flush(true);
        }
    }
}
exports.Telemetry = Telemetry;
async function readProjectTelemetryConfig() {
    try {
        const found = await (0, fs_1.findUp)(['vitte.config.json', '.vitte/config.json']);
        if (found) {
            const json = await (0, fs_1.readJson)(found);
            const enabled = !!json?.workspace?.telemetry?.enabled;
            const rate = Number(json?.workspace?.telemetry?.sampleRate);
            return { enabled, sampleRate: Number.isFinite(rate) ? clamp01(rate) : DEFAULT_SAMPLE };
        }
    }
    catch { /* ignore */ }
    return { enabled: false, sampleRate: DEFAULT_SAMPLE };
}
// ---- helpers ----
function clamp01(n) { return Math.max(0, Math.min(1, n)); }
function randId() { return Math.random().toString(36).slice(2) + Math.random().toString(36).slice(2); }
function sanitize(input) {
    if (!input)
        return undefined;
    const out = {};
    for (const [k, v] of Object.entries(input)) {
        if (v === undefined)
            continue;
        const key = String(k).slice(0, 64);
        if (typeof v === 'string') {
            // Drop suspicious long strings (no source code, no paths). Keep small identifiers.
            if (v.length > 256)
                continue;
            if (/[/\\]/.test(v) && v.length > 64)
                continue; // path-like, discard if long
            out[key] = v;
        }
        else if (typeof v === 'number') {
            if (Number.isFinite(v))
                out[key] = v;
        }
        else if (typeof v === 'boolean') {
            out[key] = v;
        }
        else {
            out[key] = String(v).slice(0, 128);
        }
    }
    return Object.keys(out).length ? out : undefined;
}
// ---- singleton convenience ----
let _telemetry;
async function getTelemetry(ctx) {
    _telemetry ?? (_telemetry = await Telemetry.create(ctx));
    return _telemetry;
}
async function registerTelemetry(ctx) {
    const tel = await getTelemetry(ctx);
    // Optional toggle command if user adds to package.json
    const toggle = vscode.commands.registerCommand('vitte.telemetry.toggle', () => {
        tel.setEnabled(!tel.isEnabled());
        void vscode.window.showInformationMessage(`Vitte Telemetry: ${tel.isEnabled() ? 'enabled' : 'disabled'}`);
    });
    // Flush on shutdown
    ctx.subscriptions.push(tel, toggle);
}
//# sourceMappingURL=telemetry.js.map