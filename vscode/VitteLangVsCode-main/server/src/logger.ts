
/**
 * Vitte LSP — Logger (ultra complete)
 * -----------------------------------
 * Dependency‑free structured logger with:
 *  - Levels: trace/debug/info/warn/error/off
 *  - Scopes & child loggers ("lsp", "features.hover"…)
 *  - Pretty or JSON line output (env controlled)
 *  - Optional ANSI colors (auto‑disabled when not TTY or env)
 *  - In‑memory ring buffer (recent records)
 *  - Rate‑limiting / once‑only helpers
 *  - Timers & perf marks (time/timeEnd)
 *  - LSP connection sink (connection.console.X)
 */

import type { Connection } from 'vscode-languageserver';

// ---------------------------------------------------------------------------
// Levels & configuration
// ---------------------------------------------------------------------------

export enum LogLevel {
  TRACE = 10,
  DEBUG = 20,
  INFO  = 30,
  WARN  = 40,
  ERROR = 50,
  OFF   = 100,
}

const LEVEL_NAMES: Record<LogLevel, string> = {
  [LogLevel.TRACE]: 'TRACE',
  [LogLevel.DEBUG]: 'DEBUG',
  [LogLevel.INFO]:  'INFO',
  [LogLevel.WARN]:  'WARN',
  [LogLevel.ERROR]: 'ERROR',
  [LogLevel.OFF]:   'OFF',
};

function env(name: string, def?: string): string | undefined {
  try { return process.env?.[name] ?? def; } catch { return def; }
}

const ENV_LEVEL = env('VITTE_LOG_LEVEL');
const ENV_JSON  = env('VITTE_LOG_JSON');
const ENV_COLOR = env('VITTE_LOG_COLOR');
function isWriteStream(value: unknown): value is NodeJS.WriteStream {
  return typeof value === 'object' && value !== null && 'isTTY' in value;
}

function detectStderrTTY(): boolean {
  try {
    const stderr = process.stderr;
    return isWriteStream(stderr) && Boolean(stderr.isTTY);
  } catch {
    return false;
  }
}

const IS_TTY = detectStderrTTY();

const NAMED_LEVELS: Record<string, LogLevel> = {
  TRACE: LogLevel.TRACE,
  DEBUG: LogLevel.DEBUG,
  INFO: LogLevel.INFO,
  WARN: LogLevel.WARN,
  ERROR: LogLevel.ERROR,
  OFF: LogLevel.OFF,
};

function parseLevel(input?: string | null): LogLevel {
  if (!input) return LogLevel.INFO;
  const t = String(input).trim().toUpperCase();
  const named = NAMED_LEVELS[t];
  if (named !== undefined) return named;
  const numeric = Number(t);
  return Number.isFinite(numeric) ? (numeric as LogLevel) : LogLevel.INFO;
}

let GLOBAL_LEVEL: LogLevel = parseLevel(ENV_LEVEL);
let GLOBAL_JSON: boolean = ENV_JSON === '1' || ENV_JSON === 'true';
let GLOBAL_COLOR: boolean = ENV_COLOR === '1' || (ENV_COLOR !== '0' && IS_TTY);

export function setGlobalLogLevel(level: LogLevel) { GLOBAL_LEVEL = level; }
export function getGlobalLogLevel(): LogLevel { return GLOBAL_LEVEL; }
export function enableJson(enabled: boolean) { GLOBAL_JSON = enabled; }
export function enableColor(enabled: boolean) { GLOBAL_COLOR = enabled; }

// ---------------------------------------------------------------------------
// Types & ring buffer
// ---------------------------------------------------------------------------

export interface LogRecord {
  level: LogLevel;
  time: number;            // epoch ms
  scope: string;           // e.g. "lsp", "features.hover"
  msg: string;
  data?: unknown;          // optional structured payload
}

class RingBuffer<T> {
  private buf: T[];
  private idx = 0;
  private filled = false;
  constructor(private capacity: number) { this.buf = new Array<T>(capacity); }
  push(v: T) { this.buf[this.idx] = v; this.idx = (this.idx + 1) % this.capacity; if (this.idx === 0) this.filled = true; }
  toArray(): T[] {
    if (!this.filled) return this.buf.slice(0, this.idx);
    return this.buf.slice(this.idx).concat(this.buf.slice(0, this.idx));
  }
}

const HISTORY = new RingBuffer<LogRecord>(500);
export function getRecentLogs(): LogRecord[] { return HISTORY.toArray(); }

// ---------------------------------------------------------------------------
// ANSI formatting (safe)
// ---------------------------------------------------------------------------

const ANSI = {
  reset: '\u001b[0m',
  dim: '\u001b[2m',
  gray: '\u001b[90m',
  red: '\u001b[31m',
  yellow: '\u001b[33m',
  blue: '\u001b[34m',
  magenta: '\u001b[35m',
};
function colorize(s: string, color: keyof typeof ANSI): string {
  if (!GLOBAL_COLOR) return s;
  return ANSI[color] + s + ANSI.reset;
}

function safeStringify(v: unknown): string {
  const seen = new WeakSet<object>();
  const replacer = (_key: string, raw: unknown): unknown => {
    if (raw instanceof Error) {
      return { name: raw.name, message: raw.message, stack: raw.stack };
    }
    if (isObject(raw)) {
      if (seen.has(raw)) return '[Circular]';
      seen.add(raw);
    }
    return raw;
  };
  return JSON.stringify(v, replacer);
}

function isObject(value: unknown): value is object {
  return typeof value === 'object' && value !== null;
}

// ---------------------------------------------------------------------------
// Sinks: stderr & LSP connection
// ---------------------------------------------------------------------------

export interface LogSink { write(rec: LogRecord): void; }

class StderrSink implements LogSink {
  write(rec: LogRecord) {
    HISTORY.push(rec);
    if (GLOBAL_JSON) {
      try { console.error(safeStringify({ ...rec, levelName: LEVEL_NAMES[rec.level] })); } catch {}
      return;
    }
    const ts = new Date(rec.time).toISOString();
    const scope = rec.scope ? `[${rec.scope}]` : '';
    const base = `${ts} ${LEVEL_NAMES[rec.level]} ${scope} ${rec.msg}`;
    let line = base;
    if (rec.data !== undefined) line += ' ' + safeStringify(rec.data);

    switch (rec.level) {
      case LogLevel.ERROR: line = colorize(line, 'red'); break;
      case LogLevel.WARN:  line = colorize(line, 'yellow'); break;
      case LogLevel.INFO:  line = colorize(line, 'blue'); break;
      case LogLevel.DEBUG: line = colorize(line, 'magenta'); break;
      case LogLevel.TRACE: line = colorize(line, 'gray'); break;
    }
    try { console.error(line); } catch {}
  }
}

class ConnectionSink implements LogSink {
  constructor(private conn: Connection) {}
  write(rec: LogRecord) {
    HISTORY.push(rec);
    const text = GLOBAL_JSON
      ? safeStringify({ ...rec, levelName: LEVEL_NAMES[rec.level] })
      : `${new Date(rec.time).toISOString()} ${LEVEL_NAMES[rec.level]} [${rec.scope}] ${rec.msg}` + (rec.data !== undefined ? ' ' + safeStringify(rec.data) : '');
    try {
      if (rec.level >= LogLevel.ERROR) this.conn.console.error(text);
      else if (rec.level >= LogLevel.WARN) this.conn.console.warn(text);
      else this.conn.console.info(text);
    } catch {}
  }
}

let SINK: LogSink = new StderrSink();
export function attachConnection(connection: Connection) { SINK = new ConnectionSink(connection); }
export function detachConnection() { SINK = new StderrSink(); }

// ---------------------------------------------------------------------------
// Logger with scopes, timers, rate limiting, once, children
// ---------------------------------------------------------------------------

export class Logger {
  private onceKeys = new Set<string>();
  private rateMap = new Map<string, number>();
  private timers = new Map<string, number>();
  constructor(private scope: string) {}

  child(sub: string): Logger { return new Logger(this.scope ? `${this.scope}.${sub}` : sub); }

  private emit(level: LogLevel, msg: string, data?: unknown) {
    if (level < GLOBAL_LEVEL) return;
    const rec: LogRecord = { level, time: Date.now(), scope: this.scope, msg, data };
    SINK.write(rec);
  }

  trace(msg: string, data?: unknown) { this.emit(LogLevel.TRACE, msg, data); }
  debug(msg: string, data?: unknown) { this.emit(LogLevel.DEBUG, msg, data); }
  info(msg: string, data?: unknown)  { this.emit(LogLevel.INFO,  msg, data); }
  warn(msg: string, data?: unknown)  { this.emit(LogLevel.WARN,  msg, data); }
  error(msg: string, data?: unknown) { this.emit(LogLevel.ERROR, msg, data); }

  once(key: string, level: LogLevel, msg: string, data?: unknown) {
    if (this.onceKeys.has(key)) return; this.onceKeys.add(key); this.emit(level, msg, data);
  }

  rateLimited(token: string, intervalMs: number, level: LogLevel, msg: string, data?: unknown) {
    const now = Date.now();
    const last = this.rateMap.get(token) ?? 0;
    if (now - last < intervalMs) return;
    this.rateMap.set(token, now);
    this.emit(level, msg, data);
  }

  time(label: string, level: LogLevel = LogLevel.DEBUG) {
    const start = Date.now();
    this.timers.set(label, start);
    return (extra?: unknown) => {
      const t0 = this.timers.get(label) ?? start;
      const dt = Date.now() - t0;
      this.emit(level, `${label} ${dt}ms`, extra);
      this.timers.delete(label);
      return dt;
    };
  }

  async timeAsync<T>(label: string, fn: () => Promise<T>, level: LogLevel = LogLevel.DEBUG): Promise<T> {
    const stop = this.time(label, level);
    try { const r = await fn(); stop(); return r; } catch (e) { stop({ error: String(e) }); throw e; }
  }
}

const ROOT = new Logger('');
export function getLogger(scope: string): Logger { return ROOT.child(scope); }

export const logLsp = getLogger('lsp');
export const logServer = getLogger('server');
export const logFeatures = getLogger('features');

logServer.once('boot', LogLevel.INFO, 'Logger initialized', { level: LEVEL_NAMES[getGlobalLogLevel()], json: GLOBAL_JSON, color: GLOBAL_COLOR });
