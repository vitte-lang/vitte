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
exports.EOL = void 0;
exports.asUri = asUri;
exports.join = join;
exports.workspaceRoot = workspaceRoot;
exports.stat = stat;
exports.exists = exists;
exports.isDirectory = isDirectory;
exports.isFile = isFile;
exports.ensureDir = ensureDir;
exports.rmrf = rmrf;
exports.listDir = listDir;
exports.readFileText = readFileText;
exports.writeFileText = writeFileText;
exports.writeFileIfChanged = writeFileIfChanged;
exports.readJson = readJson;
exports.writeJson = writeJson;
exports.copy = copy;
exports.move = move;
exports.findUp = findUp;
exports.ensureTmpDir = ensureTmpDir;
exports.makeTempFile = makeTempFile;
exports.writeTempText = writeTempText;
const vscode = __importStar(require("vscode"));
const path = __importStar(require("path"));
const os = __importStar(require("os"));
/** Convert string → Uri (file scheme) or passthrough if already Uri. */
function asUri(p) {
    return typeof p === 'string' ? vscode.Uri.file(p) : p;
}
/** Join segments into a file Uri (using Node path for correctness). */
function join(...segments) {
    const p = path.join(...segments);
    return vscode.Uri.file(p);
}
/** Workspace root (first folder) if any. */
function workspaceRoot() {
    return vscode.workspace.workspaceFolders?.[0]?.uri;
}
/** Stats & existence */
async function stat(uri) {
    try {
        return await vscode.workspace.fs.stat(asUri(uri));
    }
    catch {
        return undefined;
    }
}
async function exists(uri) {
    return (await stat(uri)) !== undefined;
}
async function isDirectory(uri) {
    const s = await stat(uri);
    return !!s && (s.type & vscode.FileType.Directory) !== 0;
}
async function isFile(uri) {
    const s = await stat(uri);
    return !!s && (s.type & vscode.FileType.File) !== 0;
}
/** Ensure directory exists (mkdir -p). */
async function ensureDir(dir) {
    const uri = asUri(dir);
    const parts = uri.fsPath.split(path.sep);
    for (let i = 1; i <= parts.length; i++) {
        const prefix = parts.slice(0, i).join(path.sep);
        if (!prefix)
            continue;
        const u = vscode.Uri.file(prefix);
        if (!(await exists(u))) {
            try {
                await vscode.workspace.fs.createDirectory(u);
            }
            catch { /* ignore */ }
        }
    }
}
/** Recursively remove a file or directory (rm -rf). */
async function rmrf(target) {
    const uri = asUri(target);
    const s = await stat(uri);
    if (!s)
        return;
    if ((s.type & vscode.FileType.File) !== 0) {
        try {
            await vscode.workspace.fs.delete(uri, { useTrash: false });
        }
        catch { /* ignore */ }
        return;
    }
    if ((s.type & vscode.FileType.Directory) !== 0) {
        try {
            const entries = await vscode.workspace.fs.readDirectory(uri);
            for (const [name, t] of entries) {
                const child = vscode.Uri.file(path.join(uri.fsPath, name));
                if (t & vscode.FileType.Directory)
                    await rmrf(child);
                else {
                    try {
                        await vscode.workspace.fs.delete(child, { useTrash: false });
                    }
                    catch { /* ignore */ }
                }
            }
            await vscode.workspace.fs.delete(uri, { recursive: false, useTrash: false });
        }
        catch { /* ignore */ }
    }
}
/** Read directory entries. */
async function listDir(dir) {
    const uri = asUri(dir);
    try {
        const entries = await vscode.workspace.fs.readDirectory(uri);
        return entries.map(([name, type]) => ({ name, type, uri: vscode.Uri.file(path.join(uri.fsPath, name)) }));
    }
    catch {
        return [];
    }
}
/** Read UTF-8 text file. */
async function readFileText(file) {
    const data = await vscode.workspace.fs.readFile(asUri(file));
    return new TextDecoder('utf-8').decode(data);
}
/** Write UTF-8 text file, creating parent dirs. */
async function writeFileText(file, text) {
    const uri = asUri(file);
    await ensureDir(path.dirname(uri.fsPath));
    const data = new TextEncoder().encode(text);
    await vscode.workspace.fs.writeFile(uri, data);
}
/** Write only if content changed (avoids churn). Returns true if written. */
async function writeFileIfChanged(file, text) {
    const uri = asUri(file);
    if (await exists(uri)) {
        try {
            const cur = await readFileText(uri);
            if (cur === text)
                return false;
        }
        catch { /* rewrite on error */ }
    }
    await writeFileText(uri, text);
    return true;
}
/** JSON helpers */
async function readJson(file) {
    try {
        return JSON.parse(await readFileText(file));
    }
    catch {
        return undefined;
    }
}
async function writeJson(file, value, indent = 2, eol = '\n') {
    const text = JSON.stringify(value, null, indent) + eol;
    await writeFileText(file, text);
}
/** Copy/move wrappers. */
async function copy(src, dst, overwrite = true) {
    await ensureDir(path.dirname(asUri(dst).fsPath));
    await vscode.workspace.fs.copy(asUri(src), asUri(dst), { overwrite });
}
async function move(src, dst, overwrite = true) {
    await ensureDir(path.dirname(asUri(dst).fsPath));
    await vscode.workspace.fs.rename(asUri(src), asUri(dst), { overwrite });
}
/** Find up: search for any of `names` from start → up to workspace root. */
async function findUp(names, start) {
    let list;
    if (Array.isArray(names)) {
        const arr = names;
        list = [...arr];
    }
    else {
        list = [names];
    }
    const root = workspaceRoot()?.fsPath;
    let cur = asUri(start ?? workspaceRoot() ?? vscode.Uri.file(process.cwd())).fsPath;
    while (true) {
        for (const n of list) {
            const candidate = path.join(cur, n);
            if (await exists(candidate))
                return vscode.Uri.file(candidate);
        }
        const parent = path.dirname(cur);
        if (parent === cur)
            break;
        if (root && cur === root)
            break;
        cur = parent;
    }
    return undefined;
}
/** Temp files under ${workspace}/.vitte/tmp or OS temp if no workspace. */
async function ensureTmpDir() {
    const base = workspaceRoot()?.fsPath;
    const dir = base ? path.join(base, '.vitte', 'tmp') : path.join(os.tmpdir(), 'vitte');
    await ensureDir(dir);
    return vscode.Uri.file(dir);
}
async function makeTempFile(prefix = 'tmp', ext = '.txt') {
    const dir = await ensureTmpDir();
    const name = `${prefix}-${Date.now().toString(36)}-${Math.random().toString(36).slice(2)}${ext}`;
    const uri = vscode.Uri.file(path.join(dir.fsPath, name));
    await writeFileText(uri, '');
    return uri;
}
async function writeTempText(text, prefix = 'tmp', ext = '.txt') {
    const f = await makeTempFile(prefix, ext);
    await writeFileText(f, text);
    return f;
}
/** Convenience EOL */
exports.EOL = '\n';
/** Export a default namespace-like object for convenience imports. */
exports.default = {
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
    EOL: exports.EOL,
};
//# sourceMappingURL=fs.js.map