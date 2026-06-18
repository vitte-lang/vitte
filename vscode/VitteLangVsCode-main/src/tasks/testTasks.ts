import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';

// Tiny helpers (strict/optional-safe)
function ensureString(x: string | undefined, fallback: string): string {
  return (typeof x === 'string' && x.length > 0) ? x : fallback;
}
function sanitizeArgs(args: (string | undefined)[]): string[] {
  return args.filter((a): a is string => typeof a === 'string' && a.length > 0);
}

// Compat: Uri.joinPath may not exist on older @types/vscode
function joinPathCompat(base: vscode.Uri, ...parts: string[]): vscode.Uri {
  const anyUri = vscode.Uri as unknown as { joinPath?: (base: vscode.Uri, ...pathSegments: string[]) => vscode.Uri };
  if (typeof anyUri.joinPath === 'function') return anyUri.joinPath(base, ...parts);
  return vscode.Uri.file(path.join(base.fsPath, ...parts));
}

/** Build the shell command to run tests. */
export function testCommandLine(withReport: boolean): string {
  const cfg = vscode.workspace.getConfiguration('vitte');

  // Prefer explicit runtime if configured (string), else fallback to 'vitte'
  const runtime = ensureString(cfg.get<string>('runtime.path') ?? cfg.get<string>('debug.program'), 'vitte');

  // Optional args from settings
  const extra = sanitizeArgs([
    cfg.get<string>('test.args'),
  ]);

  // Compose: `vitte test [--report] ...extra`
  const parts = [runtime, 'test'];
  if (withReport) parts.push('--report');
  parts.push(...extra);

  // Return a single shell string (ShellExecution will pass it to the shell)
  return parts.join(' ');
}

/** Directory where test reports are written (best-effort). */
export function testReportDir(): string | undefined {
  const cfg = vscode.workspace.getConfiguration('vitte');

  // Allow override via settings
  const override = cfg.get<string>('test.reportDir');
  if (override && override.length > 0) return override;

  // Default: <first-workspace>/.vitte/reports/tests
  const wf = vscode.workspace.workspaceFolders?.[0];
  if (!wf) return undefined;
  const uri = joinPathCompat(wf.uri, '.vitte', 'reports', 'tests');
  const p = uri.fsPath;

  // Best-effort ensure the directory exists
  try { fs.mkdirSync(p, { recursive: true }); } catch { /* ignore */ }
  return p;
}
