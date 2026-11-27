import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';

// Compat: Uri.joinPath may not exist on older @types/vscode
function joinPathCompat(base: vscode.Uri, ...parts: string[]): vscode.Uri {
  const anyUri = vscode.Uri as unknown as { joinPath?: (base: vscode.Uri, ...pathSegments: string[]) => vscode.Uri };
  if (typeof anyUri.joinPath === 'function') return anyUri.joinPath(base, ...parts);
  return vscode.Uri.file(path.join(base.fsPath, ...parts));
}

/** The native 0.1.1 driver does not implement this legacy command builder. */
export function testCommandLine(withReport: boolean): string {
  void withReport;
  throw new Error('vitte test is unavailable in compiler 0.1.1; use the compiler repository Make target when working on Vitte itself.');
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
