import * as fs from "node:fs/promises";
import type { Dir, Dirent } from "node:fs";
import type { Location } from "vscode-languageserver/node";
import * as path from "node:path";
import * as os from "node:os";
import { fileURLToPath, pathToFileURL } from "node:url";
import { TextDocument } from "vscode-languageserver-textdocument";
import { parseCompletionModel, type ProcedureSymbol, type TypeSymbol } from "./completionModel.js";
import { maskNonCode } from "./lexical.js";
import { getDocstringAtLine } from "./docstrings.js";

export interface CompletionEnvironment {
  workspaceRoots?: string[];
  projectRoot?: string;
  modulePaths?: string[];
  compilerPath?: string;
}

export interface ModulePathCandidate {
  name: string;
  insertText: string;
  isDirectory: boolean;
  modulePath: string;
  detail: string;
}

export interface ImportedCompletionSymbol {
  name: string;
  originalName: string;
  modulePath: string;
  uri: string;
  procedure?: ProcedureSymbol;
  type?: TypeSymbol;
  kind: "procedure" | "form" | "pick" | "trait" | "type" | "opaque" | "constant";
  documentation?: string;
  constantType?: string;
  location?: Location;
  memberLocations?: Record<string, Location>;
}

export interface ImportedModuleAlias {
  alias: string;
  modulePath: string;
  uri?: string;
}

export interface ModulePathFileMove {
  root: string;
  oldModule: string;
  newModule: string;
}

export interface ImportedCompletionModel {
  symbols: ImportedCompletionSymbol[];
  modules: ImportedModuleAlias[];
  moduleUri?: string;
}

interface ModuleFileSnapshot {
  mtimeMs: number;
  ctimeMs: number;
  size: number;
  uri: string;
  document: TextDocument;
  procedures: ProcedureSymbol[];
  types: TypeSymbol[];
  constants: ModuleConstant[];
}

interface ModuleConstant {
  name: string;
  nameStart: number;
  type?: string;
  documentation?: string;
}

interface UseImport {
  modulePath: string;
  suffix: string;
}

const MODULE_FILE_CACHE = new Map<string, ModuleFileSnapshot>();
const MAX_MODULE_CACHE_ENTRIES = 64;
const MAX_MODULE_ENTRIES = 120;
const MAX_MODULE_BYTES = 1024 * 1024;
const MAX_ROOTS = 24;

export async function listModulePathCandidates(
  document: TextDocument,
  parentPath: string,
  partialName: string,
  environment: CompletionEnvironment = {},
): Promise<ModulePathCandidate[]> {
  const moduleDirectory = modulePathToFilePath(parentPath);
  const roots = getModuleSearchRoots(document.uri, environment);
  const results = new Map<string, ModulePathCandidate>();
  const pending = roots.map(async (root) => {
    const directory = path.join(root, moduleDirectory);
    const entries = await readDirectoryEntries(directory, MAX_MODULE_ENTRIES);
    for (const entry of entries) {
      if (entry.name.startsWith(".") || entry.name === "node_modules") continue;
      let name = entry.name;
      const isDirectory = entry.isDirectory();
      if (entry.isFile()) {
        const extension = path.extname(name).toLowerCase();
        if (![".vit", ".vitl", ".vitte"].includes(extension)) continue;
        name = path.basename(name, extension);
      } else if (isDirectory) {
        if (!(await directoryContainsVitteModule(path.join(directory, entry.name)))) continue;
      } else {
        continue;
      }
      if (!/^[A-Za-z_]\w*$/.test(name) || !fuzzyPrefix(name, partialName)) continue;
      const modulePath = parentPath ? `${parentPath}::${name}` : name;
      const insertText = isDirectory ? `${name}::` : name;
      results.set(name, {
        name,
        insertText,
        isDirectory,
        modulePath,
        detail: isDirectory ? "Répertoire de modules Vitte" : "Module Vitte sur disque",
      });
    }
  });
  await Promise.all(pending);
  return Array.from(results.values()).sort((left, right) => Number(right.isDirectory) - Number(left.isDirectory) || left.name.localeCompare(right.name)).slice(0, 100);
}

export async function importedCompletionModel(
  document: TextDocument,
  environment: CompletionEnvironment = {},
): Promise<ImportedCompletionModel> {
  const source = document.getText();
  const code = maskNonCode(source);
  const imports = parseUseImports(source, code);
  const symbols = new Map<string, ImportedCompletionSymbol>();
  const modules = new Map<string, ImportedModuleAlias>();

  for (const imported of imports) {
    const snapshot = await readModule(imported.modulePath, document.uri, environment);
    if (!snapshot) continue;
    const aliasMatch = /^\s*as\s+([A-Za-z_]\w*)/.exec(imported.suffix);
    if (aliasMatch?.[1]) {
      modules.set(aliasMatch[1], { alias: aliasMatch[1], modulePath: imported.modulePath, uri: snapshot.uri });
      continue;
    }
    const glob = /^\s*::\s*\*/.test(imported.suffix);
    const group = /^\s*::\s*\{([\s\S]*)\}/.exec(imported.suffix)?.[1];
    if (group !== undefined) {
      const itemSpecs = parseImportItems(group);
      for (const spec of itemSpecs) {
        if (spec.nestedPath) {
          const nestedPath = `${imported.modulePath}::${spec.nestedPath}`;
          const nested = await readModule(nestedPath, document.uri, environment);
          if (nested) addModuleExports(symbols, nested, nestedPath, spec.aliases);
        } else {
          addModuleExports(symbols, snapshot, imported.modulePath, spec.aliases);
        }
      }
      continue;
    }
    if (glob) {
      addModuleExports(symbols, snapshot, imported.modulePath);
      continue;
    }
    const namespaceName = imported.modulePath.split(/::|\//).pop();
    if (namespaceName) modules.set(namespaceName, { alias: namespaceName, modulePath: imported.modulePath, uri: snapshot.uri });
  }

  return { symbols: Array.from(symbols.values()), modules: Array.from(modules.values()) };
}

export async function readModuleExports(
  modulePath: string,
  importerUri: string,
  environment: CompletionEnvironment = {},
): Promise<ImportedCompletionModel> {
  const snapshot = await readModule(modulePath, importerUri, environment);
  if (!snapshot) return { symbols: [], modules: [] };
  const symbols = new Map<string, ImportedCompletionSymbol>();
  addModuleExports(symbols, snapshot, modulePath);
  return { symbols: Array.from(symbols.values()), modules: [], moduleUri: snapshot.uri };
}

export function importPosition(text: string, offset: number): { parentPath: string; partialName: string; segmentStart: number; segmentEnd: number } | undefined {
  const code = maskNonCode(text);
  const useStart = code.lastIndexOf("use", offset);
  if (useStart < 0 || code.slice(useStart, offset).includes(";")) return undefined;
  const prefix = code.slice(useStart, offset);
  const match = /^use\s+([A-Za-z0-9_:/.-]*)$/.exec(prefix);
  if (!match) return undefined;
  const raw = match[1] ?? "";
  const pathStart = useStart + prefix.indexOf(raw);
  const hasTrailingSeparator = /(?:::|\/)$/.test(raw);
  const segments = raw.split(/::|\//).filter((segment, index, all) => segment || index < all.length - 1);
  const partialName = hasTrailingSeparator ? "" : (segments.pop() ?? "");
  const parentPath = segments.join("::");
  const segmentStart = pathStart + raw.length - partialName.length;
  const rightLineEnd = (() => {
    const end = code.indexOf("\n", offset);
    return end < 0 ? code.length : end;
  })();
  const rest = code.slice(offset, rightLineEnd);
  const rightSegmentLength = /^[A-Za-z0-9_]*/.exec(rest)?.[0].length ?? 0;
  return { parentPath, partialName, segmentStart, segmentEnd: offset + rightSegmentLength };
}

export function importItemPosition(text: string, offset: number): { modulePath: string; partialName: string; segmentStart: number; segmentEnd: number } | undefined {
  const code = maskNonCode(text);
  const useStart = code.lastIndexOf("use", offset);
  if (useStart < 0 || code.slice(useStart, offset).includes(";")) return undefined;
  const prefix = code.slice(useStart, offset);
  const match = /^use\s+([A-Za-z0-9_:/.-]+)::\s*\{([\s\S]*)$/.exec(prefix);
  if (!match) return undefined;
  const modulePath = match[1] ?? "";
  const group = match[2] ?? "";
  const partialName = /^[A-Za-z_]\w*$/.exec(group.slice(Math.max(0, group.lastIndexOf(",") + 1)).trim())?.[0] ?? "";
  const end = code.indexOf("\n", offset);
  const right = code.slice(offset, end < 0 ? code.length : end);
  const rightSegmentLength = /^[A-Za-z_]\w*/.exec(right)?.[0].length ?? 0;
  return { modulePath, partialName, segmentStart: offset - partialName.length, segmentEnd: offset + rightSegmentLength };
}

export function getModuleSearchRoots(uri: string, environment: CompletionEnvironment = {}): string[] {
  const roots: string[] = [];
  const add = (value: string | undefined): void => {
    if (!value) return;
    const expanded = expandHome(value.trim());
    if (expanded && !roots.includes(path.resolve(expanded))) roots.push(path.resolve(expanded));
  };
  const filePath = filePathFromUri(uri);
  if (filePath) {
    let current = path.dirname(filePath);
    for (let depth = 0; depth < 12; depth++) {
      add(current);
      const parent = path.dirname(current);
      if (parent === current) break;
      current = parent;
    }
  }
  add(environment.projectRoot);
  for (const root of environment.workspaceRoots ?? []) add(root);
  for (const root of environment.modulePaths ?? []) add(root);
  for (const root of (process.env.VITTE_MODULE_PATH ?? "").split(path.delimiter)) add(root);
  add(path.join(os.homedir(), ".vitte", "modules"));
  if (environment.compilerPath) {
    const binDirectory = path.dirname(expandHome(environment.compilerPath));
    add(path.resolve(binDirectory, "..", "share", "vitte", "modules"));
  }
  add("/usr/local/share/vitte/modules");
  add("/usr/share/vitte/modules");
  return roots.slice(0, MAX_ROOTS);
}

export function findUniqueModulePathFileMove(oldPath: string, newPath: string, roots: readonly string[]): {
  mapping?: ModulePathFileMove;
  candidateCount: number;
} {
  const candidates = roots.flatMap((root) => {
    const oldModule = modulePathForFile(oldPath, root);
    const newModule = modulePathForFile(newPath, root);
    return oldModule && newModule ? [{ root, oldModule, newModule }] : [];
  });
  return candidates.length === 1 ? { mapping: candidates[0], candidateCount: 1 } : { candidateCount: candidates.length };
}

function modulePathForFile(file: string, root: string): string | undefined {
  const relative = path.relative(root, file);
  if (relative === ".." || relative.startsWith(`..${path.sep}`) || path.isAbsolute(relative)) return undefined;
  const parsed = path.parse(relative);
  const segments = parsed.dir ? parsed.dir.split(path.sep).concat(parsed.name) : [parsed.name];
  if (segments[segments.length - 1] === "mod") segments.pop();
  if (segments.length === 0 || segments.some((segment) => !/^[A-Za-z_]\w*$/.test(segment))) return undefined;
  return segments.join("::");
}

export async function resolveModuleFile(
  modulePath: string,
  importerUri: string,
  environment: CompletionEnvironment = {},
): Promise<string | undefined> {
  return (await resolveModuleFiles(modulePath, importerUri, environment))[0];
}

export async function resolveModuleFiles(
  modulePath: string,
  importerUri: string,
  environment: CompletionEnvironment = {},
): Promise<string[]> {
  const relative = modulePathToFilePath(modulePath);
  const resolved = new Map<string, string>();
  for (const root of getModuleSearchRoots(importerUri, environment)) {
    const base = path.join(root, relative);
    for (const candidate of [`${base}.vit`, `${base}.vitl`, `${base}.vitte`, path.join(base, "mod.vit"), path.join(base, "mod.vitte")]) {
      try {
        if (!(await fs.stat(candidate)).isFile()) continue;
        // macOS commonly exposes the same temporary directory through both
        // /var and /private/var. Canonicalize existing files so one module is
        // never mistaken for an ambiguous import merely because its roots use
        // different symlink paths.
        const realPath = await fs.realpath(candidate);
        const identity = process.platform === "win32" ? realPath.toLocaleLowerCase("en-US") : realPath;
        // Preserve the workspace's spelling in returned URIs; the canonical
        // path is only the identity used to collapse aliases.
        resolved.set(identity, path.resolve(candidate));
      }
      catch { /* continue through the compiler-compatible roots */ }
    }
  }
  return Array.from(resolved.values());
}

async function readModule(modulePath: string, importerUri: string, environment: CompletionEnvironment): Promise<ModuleFileSnapshot | undefined> {
  const resolved = await resolveModuleFile(modulePath, importerUri, environment);
  if (!resolved) return undefined;
  try {
    const stats = await fs.stat(resolved);
    if (stats.size > MAX_MODULE_BYTES) return undefined;
    const cached = MODULE_FILE_CACHE.get(resolved);
    if (cached && cached.mtimeMs === stats.mtimeMs && cached.ctimeMs === stats.ctimeMs && cached.size === stats.size) return cached;
    const text = await fs.readFile(resolved, "utf8");
    const uri = pathToFileUri(resolved);
    const document = requireTextDocument(uri, text);
    const model = parseCompletionModel(document);
    const code = maskNonCode(text);
    const constants = Array.from(code.matchAll(/\b(?:pub|export)\s+(?:const|static)\s+([A-Za-z_]\w*)\s*(?::\s*([^=;,\n]+))?/g), (match) => {
      const name = match[1];
      if (!name) return undefined;
      const offset = match.index + match[0].indexOf(name);
      return {
        name,
        nameStart: offset,
        type: match[2]?.trim(),
        documentation: getDocstringAtLine(document, document.positionAt(offset).line) ?? undefined,
      };
    }).filter(isPresent);
    const snapshot: ModuleFileSnapshot = { mtimeMs: stats.mtimeMs, ctimeMs: stats.ctimeMs, size: stats.size, uri, document, procedures: model.procedures, types: model.types, constants };
    if (!cached && MODULE_FILE_CACHE.size >= MAX_MODULE_CACHE_ENTRIES) {
      const oldest = MODULE_FILE_CACHE.keys().next().value;
      if (oldest) MODULE_FILE_CACHE.delete(oldest);
    }
    MODULE_FILE_CACHE.set(resolved, snapshot);
    return snapshot;
  } catch {
    return undefined;
  }
}

function requireTextDocument(uri: string, text: string): TextDocument {
  // Avoid importing another TextDocument implementation or storing mutable editor state.
  return TextDocument.create(uri, "vit", 0, text);
}

function parseUseImports(source: string, code: string): UseImport[] {
  const imports: UseImport[] = [];
  const declaration = /\buse\s+([A-Za-z_]\w*(?:(?:::|\/)[A-Za-z_]\w*)*)/g;
  let match: RegExpExecArray | null;
  while ((match = declaration.exec(code))) {
    if (!match[1] || !code[match.index]) continue;
    const pathEnd = declaration.lastIndex;
    const semicolon = code.indexOf(";", pathEnd);
    const statementEnd = semicolon < 0 ? code.length : semicolon + 1;
    const suffix = source.slice(pathEnd, statementEnd);
    if (semicolon >= 0) imports.push({ modulePath: match[1], suffix });
    declaration.lastIndex = Math.max(declaration.lastIndex, statementEnd);
  }
  return imports;
}

function parseImportItems(group: string): { aliases?: Map<string, string>; nestedPath?: string }[] {
  const result: { aliases?: Map<string, string>; nestedPath?: string }[] = [];
  for (const item of splitTopLevel(group)) {
    const value = item.trim();
    if (!value) continue;
    const nested = /^([A-Za-z_]\w*(?:(?:::|\/)[A-Za-z_]\w*)*)::\s*\{/.exec(value);
    if (nested?.[1]) {
      const inner = value.slice(nested[0].length, value.lastIndexOf("}"));
      const aliases = new Map<string, string>();
      for (const child of parseImportItems(inner)) {
        if (!child.aliases) continue;
        for (const [originalName, alias] of child.aliases) aliases.set(originalName, alias);
      }
      result.push({ nestedPath: nested[1], aliases });
      continue;
    }
    const match = /^([A-Za-z_]\w*)(?:\s+as\s+([A-Za-z_]\w*))?$/.exec(value);
    if (match?.[1]) result.push({ aliases: new Map([[match[1], match[2] ?? match[1]]]) });
  }
  return result;
}

function addModuleExports(
  target: Map<string, ImportedCompletionSymbol>,
  module: ModuleFileSnapshot,
  modulePath: string,
  wanted?: Map<string, string>,
): void {
  for (const procedure of module.procedures) {
    const importedWithOwnerType = Boolean(procedure.ownerType && wanted?.has(procedure.ownerType));
    const publicTraitMethod = Boolean(procedure.implementedTrait && module.types.some((type) => type.kind === "trait" && type.name === procedure.implementedTrait && type.isPublic));
    if ((!procedure.isPublic && !publicTraitMethod) || (wanted && !wanted.has(procedure.name) && !importedWithOwnerType)) continue;
    const name = wanted?.get(procedure.name) ?? procedure.name;
    const nameStart = procedure.nameStart ?? procedure.start;
    target.set(`${name}:${module.uri}`, {
      name,
      originalName: procedure.name,
      modulePath,
      uri: module.uri,
      procedure,
      kind: "procedure",
      documentation: procedure.documentation,
      location: moduleLocation(module, nameStart, nameStart + procedure.name.length),
    });
  }
  for (const type of module.types) {
    if (!type.isPublic || (wanted && !wanted.has(type.name))) continue;
    const name = wanted?.get(type.name) ?? type.name;
    const nameStart = type.nameStart ?? type.start;
    target.set(`${name}:${module.uri}`, {
      name,
      originalName: type.name,
      modulePath,
      uri: module.uri,
      type,
      kind: type.kind,
      documentation: type.documentation,
      location: moduleLocation(module, nameStart, nameStart + type.name.length),
      memberLocations: Object.fromEntries([
        ...type.fields.filter((field) => field.nameStart !== undefined).map((field) => [field.name, moduleLocation(module, field.nameStart ?? 0, (field.nameStart ?? 0) + field.name.length)] as const),
        ...type.variants.filter((variant) => variant.nameStart !== undefined).map((variant) => [variant.name, moduleLocation(module, variant.nameStart ?? 0, (variant.nameStart ?? 0) + variant.name.length)] as const),
        ...type.methods.filter((method) => method.nameStart !== undefined).map((method) => [method.name, moduleLocation(module, method.nameStart ?? 0, (method.nameStart ?? 0) + method.name.length)] as const),
      ]),
    });
  }
  for (const constant of module.constants) {
    if (wanted && !wanted.has(constant.name)) continue;
    const name = wanted?.get(constant.name) ?? constant.name;
    target.set(`${name}:${module.uri}`, {
      name,
      originalName: constant.name,
      modulePath,
      uri: module.uri,
      kind: "constant",
      constantType: constant.type,
      documentation: constant.documentation,
      location: moduleLocation(module, constant.nameStart, constant.nameStart + constant.name.length),
    });
  }
}

function splitTopLevel(value: string): string[] {
  const items: string[] = [];
  const stack: string[] = [];
  let start = 0;
  for (let index = 0; index < value.length; index++) {
    const character = value[index];
    if (character === "{" || character === "(" || character === "[") stack.push(character);
    else if (character === "}" || character === ")" || character === "]") stack.pop();
    else if (character === "," && stack.length === 0) {
      items.push(value.slice(start, index));
      start = index + 1;
    }
  }
  if (start < value.length) items.push(value.slice(start));
  return items;
}

function directoryContainsVitteModule(directory: string, depth = 0): Promise<boolean> {
  return (async () => {
    if (depth > 3) return false;
    const entries = await readDirectoryEntries(directory, MAX_MODULE_ENTRIES);
    const subdirectories: string[] = [];
    for (const entry of entries) {
      if (entry.isFile() && [".vit", ".vitl", ".vitte"].includes(path.extname(entry.name).toLowerCase())) return true;
      if (entry.isDirectory() && !entry.name.startsWith(".") && entry.name !== "node_modules") subdirectories.push(path.join(directory, entry.name));
    }
    for (const subdirectory of subdirectories.slice(0, 12)) {
      if (await directoryContainsVitteModule(subdirectory, depth + 1)) return true;
    }
    return false;
  })();
}

async function readDirectoryEntries(directory: string, limit: number): Promise<Dirent[]> {
  let dir: Dir;
  try { dir = await fs.opendir(directory); }
  catch { return []; }
  const entries: Dirent[] = [];
  try {
    while (entries.length < limit) {
      const entry = await dir.read();
      if (!entry) break;
      entries.push(entry);
    }
  } catch {
    // A directory may disappear while module roots are being refreshed.
  } finally {
    try { await dir.close(); }
    catch { /* already closed after reaching EOF */ }
  }
  return entries;
}

function modulePathToFilePath(modulePath: string): string {
  return modulePath.trim().replace(/::/g, path.sep).replace(/\//g, path.sep).replace(/^(?:[.][\/])/, "");
}

function pathToFileUri(file: string): string {
  return pathToFileURL(path.resolve(file)).toString();
}

function moduleLocation(module: ModuleFileSnapshot, start: number, end: number): Location {
  const textLength = module.document.getText().length;
  const safeStart = Math.max(0, Math.min(start, textLength));
  const safeEnd = Math.max(safeStart, Math.min(end, textLength));
  return {
    uri: module.uri,
    range: {
      start: module.document.positionAt(safeStart),
      end: module.document.positionAt(safeEnd),
    },
  };
}

function filePathFromUri(uri: string): string | undefined {
  try { return uri.startsWith("file:") ? fileURLToPath(uri) : undefined; }
  catch { return undefined; }
}

function expandHome(value: string): string {
  return value === "~" ? os.homedir() : value.startsWith(`~${path.sep}`) ? path.join(os.homedir(), value.slice(2)) : value;
}

function fuzzyPrefix(candidate: string, query: string): boolean {
  if (!query) return true;
  if (candidate.toLowerCase().startsWith(query.toLowerCase())) return true;
  let index = 0;
  for (const character of candidate.toLowerCase()) if (character === query.toLowerCase()[index]) index++;
  return index === query.length;
}

function isPresent<T>(value: T | undefined): value is T { return value !== undefined; }
