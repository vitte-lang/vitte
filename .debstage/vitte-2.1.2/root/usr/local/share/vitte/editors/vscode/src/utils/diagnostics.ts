import * as path from "node:path";
import * as vscode from "vscode";

export const VITTE_FILE_EXTS = new Set([".vitte", ".vit"]);

type SeverityKey = 'error' | 'warning' | 'information' | 'hint';
const VALID_SEVERITIES: readonly SeverityKey[] = ['error', 'warning', 'information', 'hint'] as const;

function isSeverityKey(value: unknown): value is SeverityKey {
  return typeof value === 'string' && VALID_SEVERITIES.includes(value as SeverityKey);
}

export interface DiagnosticsSummary {
  errors: number;
  warnings: number;
  info: number;
  hints: number;
}

export interface FileDiagnosticsSummary {
  uri: vscode.Uri;
  filePath: string;
  errors: number;
  warnings: number;
  info: number;
  hints: number;
  total: number;
}

export interface WorkspaceDiagnosticsReport {
  total: DiagnosticsSummary;
  perFile: FileDiagnosticsSummary[];
}

export interface VitteDiagnosticsConfig {
  severities?: SeverityKey[];
  /** Sorting for per-file lists: by most errors, warnings, total, or alphabetical path. */
  order?: 'errors' | 'warnings' | 'total' | 'path';
  /** Limit number of rows in summaries (0 = unlimited). */
  maxPerFile?: number;
}

function getConfig(): Required<VitteDiagnosticsConfig> {
  const cfg = vscode.workspace.getConfiguration('vitte').get<VitteDiagnosticsConfig>('diagnostics') ?? {};
  const severities = Array.isArray(cfg.severities) ? cfg.severities.filter(isSeverityKey) : [];
  const order = cfg.order === 'warnings' || cfg.order === 'total' || cfg.order === 'path' ? cfg.order : 'errors';
  const maxPerFile = typeof cfg.maxPerFile === 'number' && cfg.maxPerFile >= 0 ? cfg.maxPerFile : 0;
  return {
    severities,
    order,
    maxPerFile,
  };
}

export type DiagnosticsLevel = "clean" | "warning" | "error";

export function summarizeWorkspaceDiagnostics(): DiagnosticsSummary {
  return summarizeDiagnosticsInternal((uri) => isVitteFile(uri));
}

export function summarizeDiagnosticsForUris(uris: Iterable<string | vscode.Uri>): DiagnosticsSummary {
  const wanted = new Set<string>();
  for (const entry of uris) {
    if (!entry) continue;
    if (typeof entry === "string") {
      wanted.add(entry);
    } else {
      wanted.add(entry.toString());
    }
  }
  if (wanted.size === 0) {
    return summarizeDiagnosticsInternal(() => false);
  }
  return summarizeDiagnosticsInternal((uri) => wanted.has(uri.toString()));
}

export function diagnosticsLevel(summary: DiagnosticsSummary): DiagnosticsLevel {
  if (summary.errors > 0) return "error";
  if (summary.warnings > 0) return "warning";
  return "clean";
}

export function formatDiagnosticsSummary(summary: DiagnosticsSummary): string {
  const parts: string[] = [];
  const fileCount = vscode.languages.getDiagnostics().filter(([uri]) => isVitteFile(uri)).length;
  if (summary.errors > 0) {
    parts.push(formatCount(summary.errors, "erreur"));
  }
  if (summary.warnings > 0) {
    parts.push(formatCount(summary.warnings, "avertissement"));
  }
  if (summary.info > 0) {
    parts.push(formatCount(summary.info, "info"));
  }
  if (summary.hints > 0) {
    parts.push(formatCount(summary.hints, "suggestion"));
  }
  if (parts.length === 0) {
    return "Diagnostics : aucun problème détecté";
  }
  const header = fileCount > 0 ? `${fileCount} fichier${fileCount > 1 ? 's' : ''} — ` : '';
  return `${header}Diagnostics : ${parts.join(', ')}`;
}

/**
 * Returns a per-file breakdown of diagnostics (filtered to Vitte files and by configured severities).
 */
export function summarizeDiagnosticsPerFile(): FileDiagnosticsSummary[] {
  const cfg = getConfig();
  const allowed = new Set(cfg.severities);
  const map = new Map<string, FileDiagnosticsSummary>();

  for (const [uri, diagnostics] of vscode.languages.getDiagnostics()) {
    if (!isVitteFile(uri)) continue;
    for (const d of diagnostics) {
      // Severity filter
      if (allowed.size) {
        const ok = (
          (d.severity === vscode.DiagnosticSeverity.Error && allowed.has('error')) ||
          (d.severity === vscode.DiagnosticSeverity.Warning && allowed.has('warning')) ||
          (d.severity === vscode.DiagnosticSeverity.Information && allowed.has('information')) ||
          (d.severity === vscode.DiagnosticSeverity.Hint && allowed.has('hint'))
        );
        if (!ok) continue;
      }

      const key = uri.toString();
      let rec = map.get(key);
      if (!rec) {
        rec = { uri, filePath: uri.fsPath, errors: 0, warnings: 0, info: 0, hints: 0, total: 0 };
        map.set(key, rec);
      }

      switch (d.severity) {
        case vscode.DiagnosticSeverity.Error: rec.errors++; break;
        case vscode.DiagnosticSeverity.Warning: rec.warnings++; break;
        case vscode.DiagnosticSeverity.Information: rec.info++; break;
        case vscode.DiagnosticSeverity.Hint: rec.hints++; break;
        default: rec.info++; break;
      }
      rec.total++;
    }
  }

  const list = Array.from(map.values());
  list.sort(fileSummaryComparator(cfg.order));
  if (cfg.maxPerFile > 0 && list.length > cfg.maxPerFile) {
    return list.slice(0, cfg.maxPerFile);
  }
  return list;
}

/**
 * Build a workspace report with both totals and per-file stats.
 */
export function buildWorkspaceDiagnosticsReport(): WorkspaceDiagnosticsReport {
  const perFile = summarizeDiagnosticsPerFile();
  const total = perFile.reduce<DiagnosticsSummary>((acc, f) => {
    acc.errors += f.errors;
    acc.warnings += f.warnings;
    acc.info += f.info;
    acc.hints += f.hints;
    return acc;
  }, { errors: 0, warnings: 0, info: 0, hints: 0 });
  return { total, perFile };
}

/**
 * Render a concise status text (e.g., for status bar): "⛔ 2 • ⚠ 5 • ℹ 1 • ✳ 0".
 */
export function formatDiagnosticsBadge(summary: DiagnosticsSummary): string {
  const parts: string[] = [];
  parts.push(`⛔ ${summary.errors}`);
  parts.push(`⚠ ${summary.warnings}`);
  parts.push(`ℹ ${summary.info}`);
  parts.push(`✳ ${summary.hints}`);
  return parts.join(' • ');
}

/** Choose a comparator for FileDiagnosticsSummary based on config.order */
export function fileSummaryComparator(order: 'errors'|'warnings'|'total'|'path'): (a: FileDiagnosticsSummary, b: FileDiagnosticsSummary) => number {
  switch (order) {
    case 'warnings':
      return (a, b) =>
        b.warnings - a.warnings || b.errors - a.errors || b.total - a.total || a.filePath.localeCompare(b.filePath);
    case 'total':
      return (a, b) =>
        b.total - a.total || b.errors - a.errors || b.warnings - a.warnings || a.filePath.localeCompare(b.filePath);
    case 'path':
      return (a, b) => a.filePath.localeCompare(b.filePath);
    case 'errors':
    default:
      return (a, b) =>
        b.errors - a.errors || b.warnings - a.warnings || b.total - a.total || a.filePath.localeCompare(b.filePath);
  }
}

/** Aggregate diagnostics by top-level directory (relative to workspace). */
export interface DirectoryDiagnosticsSummary extends DiagnosticsSummary {
  dir: string;            // workspace-relative directory
  files: number;          // number of files contributing
}

export function summarizeDiagnosticsByDirectory(): DirectoryDiagnosticsSummary[] {
  const cfg = getConfig();
  const allowed = new Set(cfg.severities);
  const folders = vscode.workspace.workspaceFolders ?? [];
  const map = new Map<string, DirectoryDiagnosticsSummary & { _set: Set<string> }>();

  const rel = (uri: vscode.Uri): string => {
    for (const f of folders) {
      const root = f.uri.fsPath;
      if (uri.fsPath.startsWith(root)) {
        const p = path.relative(root, uri.fsPath);
        const d = path.dirname(p);
        return d === '.' ? '' : (d.split(path.sep)[0] ?? '');
      }
    }
    return '';
  };

  for (const [uri, diagnostics] of vscode.languages.getDiagnostics()) {
    if (!isVitteFile(uri)) continue;
    const top = rel(uri);
    let rec = map.get(top);
	  if (!rec) {
	    rec = { dir: top, errors: 0, warnings: 0, info: 0, hints: 0, files: 0, _set: new Set<string>() };
	    map.set(top, rec);
	  }
	  for (const d of diagnostics) {
	    if (allowed.size > 0) {
	      const ok = (
	        (d.severity === vscode.DiagnosticSeverity.Error && allowed.has('error')) ||
	        (d.severity === vscode.DiagnosticSeverity.Warning && allowed.has('warning')) ||
	        (d.severity === vscode.DiagnosticSeverity.Information && allowed.has('information')) ||
	        (d.severity === vscode.DiagnosticSeverity.Hint && allowed.has('hint'))
        );
        if (!ok) continue;
      }
      switch (d.severity) {
        case vscode.DiagnosticSeverity.Error: rec.errors++; break;
        case vscode.DiagnosticSeverity.Warning: rec.warnings++; break;
        case vscode.DiagnosticSeverity.Information: rec.info++; break;
        case vscode.DiagnosticSeverity.Hint: rec.hints++; break;
      }
      rec._set.add(uri.toString());
    }
  }

  const out: DirectoryDiagnosticsSummary[] = [];
  for (const r of map.values()) {
    out.push({ dir: r.dir ?? '/', errors: r.errors, warnings: r.warnings, info: r.info, hints: r.hints, files: r._set.size });
  }
  out.sort((a, b) => b.errors - a.errors || b.warnings - a.warnings || b.files - a.files || a.dir.localeCompare(b.dir));
  return out;
}

/** Open first diagnostic of given severity preference (errors→warnings→info→hints). */
export async function openFirstDiagnostic(prefer: SeverityKey[] = ['error','warning','information','hint']): Promise<boolean> {
  let best: { uri: vscode.Uri; index: number } | undefined;
  for (const [uri, list] of vscode.languages.getDiagnostics()) {
    if (!isVitteFile(uri)) continue;
    const idx = list.findIndex((d) => {
      const sev = severityToName(d.severity);
      return sev !== 'unknown' && prefer.includes(sev);
    });
    if (idx >= 0) { best = { uri, index: idx }; break; }
  }
  if (!best) return false;
  const list = vscode.languages.getDiagnostics(best.uri);
  const diag = list && list.length > best.index ? list[best.index] : undefined;
  if (!diag) return false;

  const doc = await vscode.workspace.openTextDocument(best.uri);
  const editor = await vscode.window.showTextDocument(doc, { preview: true });
  editor.revealRange(diag.range, vscode.TextEditorRevealType.InCenter);
  editor.selection = new vscode.Selection(diag.range.start, diag.range.start);
  return true;
}

function severityToName(sev?: vscode.DiagnosticSeverity): 'error'|'warning'|'information'|'hint'|'unknown' {
  switch (sev) {
    case vscode.DiagnosticSeverity.Error: return 'error';
    case vscode.DiagnosticSeverity.Warning: return 'warning';
    case vscode.DiagnosticSeverity.Information: return 'information';
    case vscode.DiagnosticSeverity.Hint: return 'hint';
    default: return 'unknown';
  }
}

/** Export helpers: JSON & CSV */
export function toJSONReport(report: WorkspaceDiagnosticsReport): string {
  return JSON.stringify(report, null, 2);
}

export function toCSVReport(report: WorkspaceDiagnosticsReport): string {
  const rows: string[] = [];
  rows.push('file,errors,warnings,info,hints,total');
  for (const f of report.perFile) {
    const esc = (s: string) => '"' + s.replace(/"/g, '""') + '"';
    rows.push([esc(f.filePath), f.errors, f.warnings, f.info, f.hints, f.total].join(','));
  }
  return rows.join('\n');
}

/**
 * Render a Markdown table for panel/webview from a per-file list.
 */
export function formatDiagnosticsMarkdownTable(perFile: FileDiagnosticsSummary[]): string {
  if (!perFile.length) return '*Aucun diagnostic Vitte.*';
  const header = '| Fichier | Erreurs | Avert. | Infos | Astuces | Total |\n|---|---:|---:|---:|---:|---:|';
  const rows = perFile.map((f) => `| ${escapeMd(path.basename(f.filePath))} | ${f.errors} | ${f.warnings} | ${f.info} | ${f.hints} | ${f.total} |`);
  return [header, ...rows].join('\n');
}

function escapeMd(s: string): string {
  return s.replace(/[|*_`\\]/g, (m) => `\\${m}`);
}

function summarizeDiagnosticsInternal(predicate: (uri: vscode.Uri) => boolean): DiagnosticsSummary {
  const summary: DiagnosticsSummary = { errors: 0, warnings: 0, info: 0, hints: 0 };
  const cfg = getConfig();
  const allowed = new Set(cfg.severities);
  const filterBySeverity = (sev: vscode.DiagnosticSeverity): boolean => {
    if (!allowed.size) return true;
    switch (sev) {
      case vscode.DiagnosticSeverity.Error: return allowed.has('error');
      case vscode.DiagnosticSeverity.Warning: return allowed.has('warning');
      case vscode.DiagnosticSeverity.Information: return allowed.has('information');
      case vscode.DiagnosticSeverity.Hint: return allowed.has('hint');
      default: return true;
    }
  };
  for (const [uri, diagnostics] of vscode.languages.getDiagnostics()) {
    if (!predicate(uri)) continue;
    for (const diagnostic of diagnostics) {
      if (!filterBySeverity(diagnostic.severity)) continue;
      switch (diagnostic.severity) {
        case vscode.DiagnosticSeverity.Error:
          summary.errors++;
          break;
        case vscode.DiagnosticSeverity.Warning:
          summary.warnings++;
          break;
        case vscode.DiagnosticSeverity.Information:
          summary.info++;
          break;
        case vscode.DiagnosticSeverity.Hint:
          summary.hints++;
          break;
        default:
          summary.info++;
      }
    }
  }
  return summary;
}

function isVitteFile(uri: vscode.Uri): boolean {
  if (uri.scheme !== "file") return false;
  const ext = path.extname(uri.fsPath).toLowerCase();
  return VITTE_FILE_EXTS.has(ext);
}

function formatCount(count: number, label: string): string {
  const plural = count > 1 ? "s" : "";
  return `${count} ${label}${plural}`;
}
