"use strict";
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
Object.defineProperty(exports, "__esModule", { value: true });
exports.logFeatures = exports.logServer = exports.logLsp = exports.Logger = exports.LogLevel = void 0;
exports.setGlobalLogLevel = setGlobalLogLevel;
exports.getGlobalLogLevel = getGlobalLogLevel;
exports.enableJson = enableJson;
exports.enableColor = enableColor;
exports.getRecentLogs = getRecentLogs;
exports.attachConnection = attachConnection;
exports.detachConnection = detachConnection;
exports.getLogger = getLogger;
// ---------------------------------------------------------------------------
// Levels & configuration
// ---------------------------------------------------------------------------
var LogLevel;
(function (LogLevel) {
    LogLevel[LogLevel["TRACE"] = 10] = "TRACE";
    LogLevel[LogLevel["DEBUG"] = 20] = "DEBUG";
    LogLevel[LogLevel["INFO"] = 30] = "INFO";
    LogLevel[LogLevel["WARN"] = 40] = "WARN";
    LogLevel[LogLevel["ERROR"] = 50] = "ERROR";
    LogLevel[LogLevel["OFF"] = 100] = "OFF";
})(LogLevel || (exports.LogLevel = LogLevel = {}));
const LEVEL_NAMES = {
    [LogLevel.TRACE]: 'TRACE',
    [LogLevel.DEBUG]: 'DEBUG',
    [LogLevel.INFO]: 'INFO',
    [LogLevel.WARN]: 'WARN',
    [LogLevel.ERROR]: 'ERROR',
    [LogLevel.OFF]: 'OFF',
};
function env(name, def) {
    try {
        return process.env?.[name] ?? def;
    }
    catch {
        return def;
    }
}
const ENV_LEVEL = env('VITTE_LOG_LEVEL');
const ENV_JSON = env('VITTE_LOG_JSON');
const ENV_COLOR = env('VITTE_LOG_COLOR');
function isWriteStream(value) {
    return typeof value === 'object' && value !== null && 'isTTY' in value;
}
function detectStderrTTY() {
    try {
        const stderr = process.stderr;
        return isWriteStream(stderr) && Boolean(stderr.isTTY);
    }
    catch {
        return false;
    }
}
const IS_TTY = detectStderrTTY();
const NAMED_LEVELS = {
    TRACE: LogLevel.TRACE,
    DEBUG: LogLevel.DEBUG,
    INFO: LogLevel.INFO,
    WARN: LogLevel.WARN,
    ERROR: LogLevel.ERROR,
    OFF: LogLevel.OFF,
};
function parseLevel(input) {
    if (!input)
        return LogLevel.INFO;
    const t = String(input).trim().toUpperCase();
    const named = NAMED_LEVELS[t];
    if (named !== undefined)
        return named;
    const numeric = Number(t);
    return Number.isFinite(numeric) ? numeric : LogLevel.INFO;
}
let GLOBAL_LEVEL = parseLevel(ENV_LEVEL);
let GLOBAL_JSON = ENV_JSON === '1' || ENV_JSON === 'true';
let GLOBAL_COLOR = ENV_COLOR === '1' || (ENV_COLOR !== '0' && IS_TTY);
function setGlobalLogLevel(level) { GLOBAL_LEVEL = level; }
function getGlobalLogLevel() { return GLOBAL_LEVEL; }
function enableJson(enabled) { GLOBAL_JSON = enabled; }
function enableColor(enabled) { GLOBAL_COLOR = enabled; }
class RingBuffer {
    constructor(capacity) {
        this.capacity = capacity;
        this.idx = 0;
        this.filled = false;
        this.buf = new Array(capacity);
    }
    push(v) { this.buf[this.idx] = v; this.idx = (this.idx + 1) % this.capacity; if (this.idx === 0)
        this.filled = true; }
    toArray() {
        if (!this.filled)
            return this.buf.slice(0, this.idx);
        return this.buf.slice(this.idx).concat(this.buf.slice(0, this.idx));
    }
}
const HISTORY = new RingBuffer(500);
function getRecentLogs() { return HISTORY.toArray(); }
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
function colorize(s, color) {
    if (!GLOBAL_COLOR)
        return s;
    return ANSI[color] + s + ANSI.reset;
}
function safeStringify(v) {
    const seen = new WeakSet();
    const replacer = (_key, raw) => {
        if (raw instanceof Error) {
            return { name: raw.name, message: raw.message, stack: raw.stack };
        }
        if (isObject(raw)) {
            if (seen.has(raw))
                return '[Circular]';
            seen.add(raw);
        }
        return raw;
    };
    return JSON.stringify(v, replacer);
}
function isObject(value) {
    return typeof value === 'object' && value !== null;
}
class StderrSink {
    write(rec) {
        HISTORY.push(rec);
        if (GLOBAL_JSON) {
            try {
                console.error(safeStringify({ ...rec, levelName: LEVEL_NAMES[rec.level] }));
            }
            catch { }
            return;
        }
        const ts = new Date(rec.time).toISOString();
        const scope = rec.scope ? `[${rec.scope}]` : '';
        const base = `${ts} ${LEVEL_NAMES[rec.level]} ${scope} ${rec.msg}`;
        let line = base;
        if (rec.data !== undefined)
            line += ' ' + safeStringify(rec.data);
        switch (rec.level) {
            case LogLevel.ERROR:
                line = colorize(line, 'red');
                break;
            case LogLevel.WARN:
                line = colorize(line, 'yellow');
                break;
            case LogLevel.INFO:
                line = colorize(line, 'blue');
                break;
            case LogLevel.DEBUG:
                line = colorize(line, 'magenta');
                break;
            case LogLevel.TRACE:
                line = colorize(line, 'gray');
                break;
        }
        try {
            console.error(line);
        }
        catch { }
    }
}
class ConnectionSink {
    constructor(conn) {
        this.conn = conn;
    }
    write(rec) {
        HISTORY.push(rec);
        const text = GLOBAL_JSON
            ? safeStringify({ ...rec, levelName: LEVEL_NAMES[rec.level] })
            : `${new Date(rec.time).toISOString()} ${LEVEL_NAMES[rec.level]} [${rec.scope}] ${rec.msg}` + (rec.data !== undefined ? ' ' + safeStringify(rec.data) : '');
        try {
            if (rec.level >= LogLevel.ERROR)
                this.conn.console.error(text);
            else if (rec.level >= LogLevel.WARN)
                this.conn.console.warn(text);
            else
                this.conn.console.info(text);
        }
        catch { }
    }
}
let SINK = new StderrSink();
function attachConnection(connection) { SINK = new ConnectionSink(connection); }
function detachConnection() { SINK = new StderrSink(); }
// ---------------------------------------------------------------------------
// Logger with scopes, timers, rate limiting, once, children
// ---------------------------------------------------------------------------
class Logger {
    constructor(scope) {
        this.scope = scope;
        this.onceKeys = new Set();
        this.rateMap = new Map();
        this.timers = new Map();
    }
    child(sub) { return new Logger(this.scope ? `${this.scope}.${sub}` : sub); }
    emit(level, msg, data) {
        if (level < GLOBAL_LEVEL)
            return;
        const rec = { level, time: Date.now(), scope: this.scope, msg, data };
        SINK.write(rec);
    }
    trace(msg, data) { this.emit(LogLevel.TRACE, msg, data); }
    debug(msg, data) { this.emit(LogLevel.DEBUG, msg, data); }
    info(msg, data) { this.emit(LogLevel.INFO, msg, data); }
    warn(msg, data) { this.emit(LogLevel.WARN, msg, data); }
    error(msg, data) { this.emit(LogLevel.ERROR, msg, data); }
    once(key, level, msg, data) {
        if (this.onceKeys.has(key))
            return;
        this.onceKeys.add(key);
        this.emit(level, msg, data);
    }
    rateLimited(token, intervalMs, level, msg, data) {
        const now = Date.now();
        const last = this.rateMap.get(token) ?? 0;
        if (now - last < intervalMs)
            return;
        this.rateMap.set(token, now);
        this.emit(level, msg, data);
    }
    time(label, level = LogLevel.DEBUG) {
        const start = Date.now();
        this.timers.set(label, start);
        return (extra) => {
            const t0 = this.timers.get(label) ?? start;
            const dt = Date.now() - t0;
            this.emit(level, `${label} ${dt}ms`, extra);
            this.timers.delete(label);
            return dt;
        };
    }
    async timeAsync(label, fn, level = LogLevel.DEBUG) {
        const stop = this.time(label, level);
        try {
            const r = await fn();
            stop();
            return r;
        }
        catch (e) {
            stop({ error: String(e) });
            throw e;
        }
    }
}
exports.Logger = Logger;
const ROOT = new Logger('');
function getLogger(scope) { return ROOT.child(scope); }
exports.logLsp = getLogger('lsp');
exports.logServer = getLogger('server');
exports.logFeatures = getLogger('features');
exports.logServer.once('boot', LogLevel.INFO, 'Logger initialized', { level: LEVEL_NAMES[getGlobalLogLevel()], json: GLOBAL_JSON, color: GLOBAL_COLOR });
//# sourceMappingURL=logger.js.map