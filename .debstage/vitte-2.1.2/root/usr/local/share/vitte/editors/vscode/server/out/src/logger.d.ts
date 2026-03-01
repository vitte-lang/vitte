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
export declare enum LogLevel {
    TRACE = 10,
    DEBUG = 20,
    INFO = 30,
    WARN = 40,
    ERROR = 50,
    OFF = 100
}
export declare function setGlobalLogLevel(level: LogLevel): void;
export declare function getGlobalLogLevel(): LogLevel;
export declare function enableJson(enabled: boolean): void;
export declare function enableColor(enabled: boolean): void;
export interface LogRecord {
    level: LogLevel;
    time: number;
    scope: string;
    msg: string;
    data?: unknown;
}
export declare function getRecentLogs(): LogRecord[];
export interface LogSink {
    write(rec: LogRecord): void;
}
export declare function attachConnection(connection: Connection): void;
export declare function detachConnection(): void;
export declare class Logger {
    private scope;
    private onceKeys;
    private rateMap;
    private timers;
    constructor(scope: string);
    child(sub: string): Logger;
    private emit;
    trace(msg: string, data?: unknown): void;
    debug(msg: string, data?: unknown): void;
    info(msg: string, data?: unknown): void;
    warn(msg: string, data?: unknown): void;
    error(msg: string, data?: unknown): void;
    once(key: string, level: LogLevel, msg: string, data?: unknown): void;
    rateLimited(token: string, intervalMs: number, level: LogLevel, msg: string, data?: unknown): void;
    time(label: string, level?: LogLevel): (extra?: unknown) => number;
    timeAsync<T>(label: string, fn: () => Promise<T>, level?: LogLevel): Promise<T>;
}
export declare function getLogger(scope: string): Logger;
export declare const logLsp: Logger;
export declare const logServer: Logger;
export declare const logFeatures: Logger;
