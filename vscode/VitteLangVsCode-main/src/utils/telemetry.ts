import * as vscode from 'vscode';
import * as path from 'path';
import { ensureDir, writeFileText, workspaceRoot, findUp, readJson } from './fs';

/**
 * Lightweight, opt‑in telemetry for VitteLangVsCode.
 * - No external network calls. Writes JSONL batches locally under `.vitte/telemetry/`.
 * - Respects project config (vitte.config.json → workspace.telemetry) and optional `vitte.telemetry.*` settings.
 * - Sampling, error tracking, durations, flush on deactivate.
 * - PII guard: we never capture source contents or file paths unless explicitly provided.
 */

export type TelemetryProperties = Record<string, string | number | boolean | undefined>;
export interface TelemetryOptions {
  enabled?: boolean;
  sampleRate?: number; // 0..1
  channelName?: string; // VS Code OutputChannel name
}

interface TelemetryEventBase {
  t: number;           // epoch ms
  s: string;           // session id
  ext?: string;        // extension version
  k: string;           // event key
  p?: TelemetryProperties; // properties (stringifiable)
  d?: number;          // duration ms (optional)
  lvl?: 'info' | 'warn' | 'error';
}

const DEFAULT_SAMPLE = 0.1;
const BATCH_MAX = 256;      // max events per file
const FLUSH_INTERVAL_MS = 5000;

export class Telemetry implements vscode.Disposable {
  private ctx: vscode.ExtensionContext;
  private enabled = false;
  private sample = DEFAULT_SAMPLE;
  private session = randId();
  private extVersion = '0.0.0';
  private queue: TelemetryEventBase[] = [];
  private timer: NodeJS.Timeout | undefined;
  private output?: vscode.OutputChannel;

  private constructor(ctx: vscode.ExtensionContext, opts?: TelemetryOptions) {
    this.ctx = ctx;
    this.enabled = !!opts?.enabled;
    this.sample = clamp01(opts?.sampleRate ?? DEFAULT_SAMPLE);
    const extensionPkg = ctx.extension?.packageJSON as { version?: unknown } | undefined;
    const version = extensionPkg?.version;
    this.extVersion = typeof version === 'string' ? version : '0.0.0';
    if (opts?.channelName) this.output = vscode.window.createOutputChannel(opts.channelName);
    this.armTimer();
  }

  static async create(ctx: vscode.ExtensionContext, opts?: TelemetryOptions): Promise<Telemetry> {
    const { enabled, sampleRate } = await readProjectTelemetryConfig();
    const cfg = vscode.workspace.getConfiguration('vitte');
    const enabledSetting = cfg.get<boolean>('telemetry.enabled');
    const sampleSetting = cfg.get<number>('telemetry.sampleRate');

    const t = new Telemetry(ctx, {
      ...opts,
      enabled: typeof enabledSetting === 'boolean' ? enabledSetting : enabled,
      sampleRate: typeof sampleSetting === 'number' ? sampleSetting : sampleRate,
      channelName: 'Vitte Telemetry',
    });
    return t;
  }

  dispose(): void {
    if (this.timer) { clearInterval(this.timer); this.timer = undefined; }
    void this.flush(true);
    this.output?.dispose();
  }

  isEnabled(): boolean { return this.enabled; }
  setEnabled(v: boolean): void { this.enabled = !!v; }
  setSampleRate(r: number): void { this.sample = clamp01(r); }

  /** Track an event with optional properties. */
  track(key: string, props?: TelemetryProperties): void {
    if (!this.shouldSample()) return;
    const sp = sanitize(props);
    const ev: TelemetryEventBase = { t: Date.now(), s: this.session, ext: this.extVersion, k: key, lvl: 'info' };
    if (sp !== undefined) ev.p = sp;
    this.enqueue(ev);
  }

  /** Track an error (message + optional props). */
  error(key: string, err: unknown, props?: TelemetryProperties): void {
    if (!this.shouldSample()) return;
    const msg = err instanceof Error ? err.message : String(err);
    const sp = sanitize({ ...props, error: msg });
    const ev: TelemetryEventBase = { t: Date.now(), s: this.session, ext: this.extVersion, k: key, lvl: 'error' };
    if (sp !== undefined) ev.p = sp;
    this.enqueue(ev);
  }

  /** Measure a duration around an async function. */
  async time<T>(key: string, fn: () => Promise<T>, props?: TelemetryProperties): Promise<T> {
    const start = Date.now();
    try {
      const res = await fn();
      this.duration(key, Date.now() - start, props);
      return res;
    } catch (e) {
      this.duration(key, Date.now() - start, { ...props, ok: false });
      throw e;
    }
  }

  /** Record a duration with optional properties. */
  duration(key: string, ms: number, props?: TelemetryProperties): void {
    if (!this.shouldSample()) return;
    const sp = sanitize(props);
    const ev: TelemetryEventBase = { t: Date.now(), s: this.session, ext: this.extVersion, k: key, d: Math.max(0, Math.floor(ms)), lvl: 'info' };
    if (sp !== undefined) ev.p = sp;
    this.enqueue(ev);
  }

  /** Force flush queued events to disk. */
  async flush(now = false): Promise<void> {
    if (!this.enabled || this.queue.length === 0) return;
    const slice = this.queue.splice(0, Math.min(this.queue.length, BATCH_MAX));
    try {
      const root = workspaceRoot()?.fsPath;
      if (!root) return; // nothing we can do without a workspace
      const dir = vscode.Uri.file(`${root}/.vitte/telemetry`);
      await ensureDir(dir);
      const ts = new Date().toISOString().replace(/[:.]/g, '-');
      const file = vscode.Uri.file(path.join(dir.fsPath, `events-${ts}-${randId()}.jsonl`));
      const lines = slice.map(e => JSON.stringify(e)).join('\n') + '\n';
      await writeFileText(file, lines);
      this.output?.appendLine(`[telemetry] wrote ${slice.length} event(s) to ${file.fsPath}`);
    } catch (e) {
      const message = e instanceof Error ? e.message : String(e);
      this.output?.appendLine(`[telemetry] flush failed: ${message}`);
    }
    if (!now) this.armTimer();
  }

  // ---- internals ----
  private armTimer() {
    if (this.timer) clearInterval(this.timer);
    this.timer = setInterval(() => { void this.flush(); }, FLUSH_INTERVAL_MS);
  }

  private shouldSample(): boolean {
    return this.enabled && Math.random() < this.sample;
  }

  private enqueue(ev: TelemetryEventBase) {
    this.queue.push(ev);
    if (this.queue.length >= BATCH_MAX) { void this.flush(true); }
  }
}

// ---- config loader ----
interface ProjectTelemetryConfig {
  workspace?: {
    telemetry?: {
      enabled?: boolean;
      sampleRate?: number;
    };
  };
}

async function readProjectTelemetryConfig(): Promise<{ enabled: boolean; sampleRate: number }> {
  try {
    const found = await findUp(['vitte.config.json', '.vitte/config.json']);
    if (found) {
      const json = await readJson<ProjectTelemetryConfig>(found);
      const enabled = !!json?.workspace?.telemetry?.enabled;
      const rate = Number(json?.workspace?.telemetry?.sampleRate);
      return { enabled, sampleRate: Number.isFinite(rate) ? clamp01(rate) : DEFAULT_SAMPLE };
    }
  } catch { /* ignore */ }
  return { enabled: false, sampleRate: DEFAULT_SAMPLE };
}

// ---- helpers ----
function clamp01(n: number): number { return Math.max(0, Math.min(1, n)); }
function randId(): string { return Math.random().toString(36).slice(2) + Math.random().toString(36).slice(2); }

function sanitize(input?: TelemetryProperties): TelemetryProperties | undefined {
  if (!input) return undefined;
  const out: TelemetryProperties = {};
  for (const [k, v] of Object.entries(input)) {
    if (v === undefined) continue;
    const key = String(k).slice(0, 64);
    if (typeof v === 'string') {
      // Drop suspicious long strings (no source code, no paths). Keep small identifiers.
      if (v.length > 256) continue;
      if (/[/\\]/.test(v) && v.length > 64) continue; // path-like, discard if long
      out[key] = v;
    } else if (typeof v === 'number') {
      if (Number.isFinite(v)) out[key] = v;
    } else if (typeof v === 'boolean') {
      out[key] = v;
    } else {
      out[key] = String(v).slice(0, 128);
    }
  }
  return Object.keys(out).length ? out : undefined;
}

// ---- singleton convenience ----
let _telemetry: Telemetry | undefined;
export async function getTelemetry(ctx: vscode.ExtensionContext): Promise<Telemetry> {
  _telemetry ??= await Telemetry.create(ctx);
  return _telemetry;
}

export async function registerTelemetry(ctx: vscode.ExtensionContext) {
  const tel = await getTelemetry(ctx);
  // Optional toggle command if user adds to package.json
  const toggle = vscode.commands.registerCommand('vitte.telemetry.toggle', () => {
    tel.setEnabled(!tel.isEnabled());
    void vscode.window.showInformationMessage(`Vitte Telemetry: ${tel.isEnabled() ? 'enabled' : 'disabled'}`);
  });
  // Flush on shutdown
  ctx.subscriptions.push(tel, toggle);
}
