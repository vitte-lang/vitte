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

import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

// Process identity
try { process.title = 'vitte-lsp-server'; } catch { /* noop */ }

// ---------------------------------------------------------------------------
// Time origin and environment diagnostics
// ---------------------------------------------------------------------------
const BOOT_T0 = Date.now();

// Basic environment info (logged to stderr, never to stdout)
function logEnvInfo() {
  try {
    const info = {
      node: process.version,
      platform: `${os.platform()} ${os.release()}`,
      arch: os.arch(),
      pid: process.pid,
      cwd: process.cwd(),
      argv: process.argv.slice(2),
      memory: process.memoryUsage(),
    };
    console.error('[vitte-lsp] environment', JSON.stringify(info, null, 2));
  } catch { /* ignore */ }
}

// ---------------------------------------------------------------------------
// Hardening
// ---------------------------------------------------------------------------

process.on('uncaughtException', (err) => {
  try {
    const msg = err instanceof Error ? err.stack ?? err.message ?? String(err) : String(err);
    console.error(`[vitte-lsp] uncaughtException: ${msg}`);
  } catch { /* ignore */ }
});

process.on('unhandledRejection', (reason) => {
  try {
    console.error('[vitte-lsp] unhandledRejection:', reason);
  } catch { /* ignore */ }
});

// ---------------------------------------------------------------------------
// Graceful shutdown
// ---------------------------------------------------------------------------

const graceful = (sig: NodeJS.Signals) => () => {
  try { console.error(`[vitte-lsp] received ${sig}, exiting…`); } catch {}
  setTimeout(() => process.exit(0), 10);
};
for (const s of ['SIGINT', 'SIGTERM', 'SIGHUP'] as NodeJS.Signals[]) {
  try { process.on(s, graceful(s)); } catch { /* ignore */ }
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
  const tmp = path.join(os.tmpdir(), 'vitte-lsp-check.tmp');
  try {
    fs.writeFileSync(tmp, 'ok');
    console.error(`[vitte-lsp] write test ok (${tmp})`);
    fs.unlinkSync(tmp);
  } catch (err) {
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
} catch { /* ignore */ }

// ---------------------------------------------------------------------------
// Load LSP server implementation (no top-level await)
import('./lsp').catch((err) => {
  try { console.error('[vitte-lsp] failed to load ./lsp:', err); } catch {}
  process.exit(1);
});
