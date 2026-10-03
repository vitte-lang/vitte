import * as vscode from 'vscode';
import * as path from 'path';
import * as os from 'os';

/** Lightweight FS helpers over vscode.workspace.fs (portable, no Node fs dependency). */
export type MaybeUri = vscode.Uri | string;

/** Convert string → Uri (file scheme) or passthrough if already Uri. */
export function asUri(p: MaybeUri): vscode.Uri {
  return typeof p === 'string' ? vscode.Uri.file(p) : p;
}

/** Join segments into a file Uri (using Node path for correctness). */
export function join(...segments: string[]): vscode.Uri {
  const p = path.join(...segments);
  return vscode.Uri.file(p);
}

/** Workspace root (first folder) if any. */
export function workspaceRoot(): vscode.Uri | undefined {
  return vscode.workspace.workspaceFolders?.[0]?.uri;
}

/** Stats & existence */
export async function stat(uri: MaybeUri): Promise<vscode.FileStat | undefined> {
  try { return await vscode.workspace.fs.stat(asUri(uri)); } catch { return undefined; }
}

export async function exists(uri: MaybeUri): Promise<boolean> {
  return (await stat(uri)) !== undefined;
}

export async function isDirectory(uri: MaybeUri): Promise<boolean> {
  const s = await stat(uri); return !!s && (s.type & vscode.FileType.Directory) !== 0;
}

export async function isFile(uri: MaybeUri): Promise<boolean> {
  const s = await stat(uri); return !!s && (s.type & vscode.FileType.File) !== 0;
}

/** Ensure directory exists (mkdir -p). */
export async function ensureDir(dir: MaybeUri): Promise<void> {
  const uri = asUri(dir);
  const parts = uri.fsPath.split(path.sep);
  for (let i = 1; i <= parts.length; i++) {
    const prefix = parts.slice(0, i).join(path.sep);
    if (!prefix) continue;
    const u = vscode.Uri.file(prefix);
    if (!(await exists(u))) {
      try { await vscode.workspace.fs.createDirectory(u); } catch { /* ignore */ }
    }
  }
}

/** Recursively remove a file or directory (rm -rf). */
export async function rmrf(target: MaybeUri): Promise<void> {
  const uri = asUri(target);
  const s = await stat(uri);
  if (!s) return;
  if ((s.type & vscode.FileType.File) !== 0) {
    try { await vscode.workspace.fs.delete(uri, { useTrash: false }); } catch { /* ignore */ }
    return;
  }
  if ((s.type & vscode.FileType.Directory) !== 0) {
    try {
      const entries = await vscode.workspace.fs.readDirectory(uri);
      for (const [name, t] of entries) {
        const child = vscode.Uri.file(path.join(uri.fsPath, name));
        if (t & vscode.FileType.Directory) await rmrf(child);
        else { try { await vscode.workspace.fs.delete(child, { useTrash: false }); } catch { /* ignore */ } }
      }
      await vscode.workspace.fs.delete(uri, { recursive: false, useTrash: false });
    } catch { /* ignore */ }
  }
}

/** Read directory entries. */
export async function listDir(dir: MaybeUri): Promise<{ name: string; uri: vscode.Uri; type: vscode.FileType }[]> {
  const uri = asUri(dir);
  try {
    const entries = await vscode.workspace.fs.readDirectory(uri);
    return entries.map(([name, type]) => ({ name, type, uri: vscode.Uri.file(path.join(uri.fsPath, name)) }));
  } catch { return []; }
}

/** Read UTF-8 text file. */
export async function readFileText(file: MaybeUri): Promise<string> {
  const data = await vscode.workspace.fs.readFile(asUri(file));
  return new TextDecoder('utf-8').decode(data);
}

/** Write UTF-8 text file, creating parent dirs. */
export async function writeFileText(file: MaybeUri, text: string): Promise<void> {
  const uri = asUri(file);
  await ensureDir(path.dirname(uri.fsPath));
  const data = new TextEncoder().encode(text);
  await vscode.workspace.fs.writeFile(uri, data);
}

/** Write only if content changed (avoids churn). Returns true if written. */
export async function writeFileIfChanged(file: MaybeUri, text: string): Promise<boolean> {
  const uri = asUri(file);
  if (await exists(uri)) {
    try {
      const cur = await readFileText(uri);
      if (cur === text) return false;
    } catch { /* rewrite on error */ }
  }
  await writeFileText(uri, text);
  return true;
}

/** JSON helpers */
export async function readJson<T = unknown>(file: MaybeUri): Promise<T | undefined> {
  try { return JSON.parse(await readFileText(file)) as T; } catch { return undefined; }
}

export async function writeJson(file: MaybeUri, value: unknown, indent = 2, eol = '\n'): Promise<void> {
  const text = JSON.stringify(value, null, indent) + eol;
  await writeFileText(file, text);
}

/** Copy/move wrappers. */
export async function copy(src: MaybeUri, dst: MaybeUri, overwrite = true): Promise<void> {
  await ensureDir(path.dirname(asUri(dst).fsPath));
  await vscode.workspace.fs.copy(asUri(src), asUri(dst), { overwrite });
}

export async function move(src: MaybeUri, dst: MaybeUri, overwrite = true): Promise<void> {
  await ensureDir(path.dirname(asUri(dst).fsPath));
  await vscode.workspace.fs.rename(asUri(src), asUri(dst), { overwrite });
}

/** Find up: search for any of `names` from start → up to workspace root. */
export async function findUp(names: string | readonly string[], start?: MaybeUri): Promise<vscode.Uri | undefined> {
  let list: string[];
  if (Array.isArray(names)) {
    const arr = names as readonly string[];
    list = [...arr];
  } else {
    list = [names as string];
  }
  const root = workspaceRoot()?.fsPath;
  let cur = asUri(start ?? workspaceRoot() ?? vscode.Uri.file(process.cwd())).fsPath;
  while (true) {
    for (const n of list) {
      const candidate = path.join(cur, n);
      if (await exists(candidate)) return vscode.Uri.file(candidate);
    }
    const parent = path.dirname(cur);
    if (parent === cur) break;
    if (root && cur === root) break;
    cur = parent;
  }
  return undefined;
}

/** Temp files under ${workspace}/.vitte/tmp or OS temp if no workspace. */
export async function ensureTmpDir(): Promise<vscode.Uri> {
  const base = workspaceRoot()?.fsPath;
  const dir = base ? path.join(base, '.vitte', 'tmp') : path.join(os.tmpdir(), 'vitte');
  await ensureDir(dir);
  return vscode.Uri.file(dir);
}

export async function makeTempFile(prefix = 'tmp', ext = '.txt'): Promise<vscode.Uri> {
  const dir = await ensureTmpDir();
  const name = `${prefix}-${Date.now().toString(36)}-${Math.random().toString(36).slice(2)}${ext}`;
  const uri = vscode.Uri.file(path.join(dir.fsPath, name));
  await writeFileText(uri, '');
  return uri;
}

export async function writeTempText(text: string, prefix = 'tmp', ext = '.txt'): Promise<vscode.Uri> {
  const f = await makeTempFile(prefix, ext);
  await writeFileText(f, text);
  return f;
}

/** Convenience EOL */
export const EOL = '\n';

/** Export a default namespace-like object for convenience imports. */
export default {
  asUri,
  join,
  workspaceRoot,
  stat,
  exists,
  isDirectory,
  isFile,
  ensureDir,
  rmrf,
  listDir,
  readFileText,
  writeFileText,
  writeFileIfChanged,
  readJson,
  writeJson,
  copy,
  move,
  findUp,
  ensureTmpDir,
  makeTempFile,
  writeTempText,
  EOL,
};
