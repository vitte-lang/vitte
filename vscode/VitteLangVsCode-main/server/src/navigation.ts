import { maskNonCode, scanLexical } from "./lexical.js";
import { RESERVED_WORDS } from "./languageFacts.js";
// navigation.ts — symboles, définitions, références, rename, workspace symbols
// Version enrichie :
// - Parsing tolérant qui ignore commentaires/chaînes
// - Hiérarchie de symboles via pile d’accolades
// - Règles étendues alignées sur la syntaxe Vitte actuelle
// - Références/rename hors commentaires/chaînes
// - Cache par document/version
// - Workspace symbols avec fuzzy match et scoring stable
// - API additionnelle: prepareRename, indexDocument

import type { TextDocument } from "vscode-languageserver-textdocument";
import {
  Location,
  Position,
  Range,
  SymbolKind,
} from "vscode-languageserver/node";
import type {
  CallHierarchyIncomingCall,
  CallHierarchyItem,
  CallHierarchyOutgoingCall,
  DocumentSymbol,
  TypeHierarchyItem,
  TypeHierarchySubtypesParams,
  TypeHierarchySupertypesParams,
  WorkspaceSymbol,
} from "vscode-languageserver/node";
import {
  resolveSourceSymbol,
  resolveAliasType,
  sourceIndex,
  sourceTokenAt,
  visibleBindingsAt,
  type SourceIndex,
  type SourceSymbol,
} from "./sourceSymbols.js";
import { importedCompletionModel, readModuleExports, type CompletionEnvironment, type ImportedCompletionSymbol, type ImportedModuleAlias } from "./moduleResolver.js";

/* --------------------------------- Types ---------------------------------- */

interface FlatSymbol {
  name: string;
  kind: SymbolKind;
  range: Range;           // entier bloc (ex: de 'proc' à la fin de la signature/ligne)
  selectionRange: Range;  // le nom seul
  containerName?: string; // parent logique (space/form/trait)
}

interface DocIndex {
  flat: FlatSymbol[];
  outline: DocumentSymbol[];
  byName: Map<string, FlatSymbol[]>;
}

/* -------------------------- Masque code (lexer light) ---------------------- */
// Construit un masque booléen de positions « valides » (hors commentaires et chaînes)
// Supporte //, /* */, "...", '...', r"..." et r#"..."# (style Rust/Vitte). Sans interpolation.

export function buildCodeMask(text: string): Uint8Array {
  return scanLexical(text).mask;
}

function isIdentChar(ch: number): boolean {
  return (ch >= 48 && ch <= 57) || // 0-9
         (ch >= 65 && ch <= 90) || // A-Z
         (ch >= 97 && ch <= 122) || // a-z
         ch === 95; // _
}

function isPathChar(ch: number): boolean {
  return isIdentChar(ch) || ch === 0x2f /* / */ || ch === 0x2e /* . */ || ch === 0x3a /* : */ || ch === 0x2d /* - */;
}

function wordAt(doc: TextDocument, pos: Position): string | null {
  const text = doc.getText();
  const off = doc.offsetAt(pos);
  let s = off, e = off;
  while (s > 0 && isIdentChar(text.charCodeAt(s - 1))) s--;
  while (e < text.length && isIdentChar(text.charCodeAt(e))) e++;
  return e > s ? text.slice(s, e) : null;
}

function tokenAt(doc: TextDocument, pos: Position): string | null {
  const text = doc.getText();
  const off = doc.offsetAt(pos);
  let s = off, e = off;
  while (s > 0 && isPathChar(text.charCodeAt(s - 1))) s--;
  while (e < text.length && isPathChar(text.charCodeAt(e))) e++;
  return e > s ? text.slice(s, e) : null;
}

function escapeRx(s: string): string {
  return s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function posKey(p: Position): string { return `${p.line}:${p.character}`; }

function dedupeBy<T>(arr: T[], keyFn: (t: T) => string): T[] {
  const seen = new Set<string>();
  const out: T[] = [];
  for (const v of arr) {
    const k = keyFn(v);
    if (seen.has(k)) continue;
    seen.add(k);
    out.push(v);
  }
  return out;
}

function isValidIdent(s: string): boolean { return /^[A-Za-z_]\w*$/.test(s); }

/* ------------------------------ Règles symboles ---------------------------- */
// Les regex sont évaluées uniquement sur les positions mask==1

const RULES: { rx: RegExp; kind: SymbolKind; nameGroup: number; containerHint?: (m: RegExpExecArray) => string | undefined }[] = [
  { rx: /\bspace\s+([A-Za-z_][\w./:]*)/g,                               kind: SymbolKind.Namespace, nameGroup: 1 },
  { rx: /\buse\s+([A-Za-z_][\w./:]*)/g,                                 kind: SymbolKind.Namespace, nameGroup: 1 },
  { rx: /\bpull\s+([A-Za-z_][\w./:]*)/g,                                kind: SymbolKind.Namespace, nameGroup: 1 },
  { rx: /\bentry\s+([A-Za-z_]\w*)\s+at\s+[A-Za-z_][\w./:]*/g,           kind: SymbolKind.Function, nameGroup: 1 },
  { rx: /\bentry\s+[A-Za-z_]\w*\s+at\s+([A-Za-z_][\w./:]*)/g,           kind: SymbolKind.Namespace, nameGroup: 1 },
  { rx: /\bshare\s+([A-Za-z_]\w*)/g,                                     kind: SymbolKind.Variable, nameGroup: 1 },
  { rx: /\bproc\s+([A-Za-z_]\w*)\s*\(/g,                                kind: SymbolKind.Function, nameGroup: 1 },
  { rx: /\bform\s+([A-Za-z_]\w*)/g,                                     kind: SymbolKind.Struct, nameGroup: 1 },
  { rx: /\bpick\s+([A-Za-z_]\w*)/g,                                     kind: SymbolKind.Enum, nameGroup: 1 },
  { rx: /\btrait\s+([A-Za-z_]\w*)/g,                                    kind: SymbolKind.Interface, nameGroup: 1 },
  { rx: /\btype\s+([A-Za-z_]\w*)/g,                                     kind: SymbolKind.TypeParameter, nameGroup: 1 },
  { rx: /\bconst\s+([A-Za-z_]\w*)/g,                                    kind: SymbolKind.Constant, nameGroup: 1 },
  { rx: /\b(?:let|make)\s+([A-Za-z_]\w*)/g,                             kind: SymbolKind.Variable, nameGroup: 1 },
  { rx: /\b(?:field|case)\s+([A-Za-z_]\w*)/g,                           kind: SymbolKind.Field, nameGroup: 1 },
];

function* iterMaskedMatches(rx: RegExp, text: string, mask: Uint8Array): Generator<RegExpExecArray> {
  rx.lastIndex = 0;
  let m: RegExpExecArray | null;
  while ((m = rx.exec(text))) {
    const idx = m.index ?? 0;
    // Vérifie que le début du match tombe sur zone code
    if (mask[idx]) {
      yield m;
    }
    if (m[0].length === 0) rx.lastIndex++;
  }
}

/* ------------------------ Collecte + hiérarchie outline -------------------- */

function collectFlatSymbols(doc: TextDocument): FlatSymbol[] {
  const text = doc.getText();
  const mask = buildCodeMask(text);
  const out: FlatSymbol[] = [];

  for (const { rx, kind, nameGroup } of RULES) {
    for (const m of iterMaskedMatches(rx, text, mask)) {
      const name = m[nameGroup];
      if (!name) continue;
      const start = doc.positionAt(m.index ?? 0);
      const end = doc.positionAt((m.index ?? 0) + m[0].length);
      const nameOffset = (m.index ?? 0) + m[0].indexOf(name);
      const nameStart = doc.positionAt(nameOffset);
      const nameEnd = doc.positionAt(nameOffset + name.length);
      out.push({
        name,
        kind,
        range: Range.create(start, end),
        selectionRange: Range.create(nameStart, nameEnd),
      });
    }
  }

  // Déduplication conservatrice
  return dedupeBy(out, s => `${s.kind}:${s.name}:${posKey(s.selectionRange.start)}`);
}

function buildOutline(doc: TextDocument, flat: FlatSymbol[]): DocumentSymbol[] {
  // Trie par position pour associer via une pile d’accolades
  const text = doc.getText();
  type Node = DocumentSymbol & { children: DocumentSymbol[] };
  const nodes: Node[] = flat
    .map(s => ({
      name: s.name,
      kind: s.kind,
      range: s.range,
      selectionRange: s.selectionRange,
      children: [],
    }))
    .sort((a, b) => {
      const da = doc.offsetAt(a.range.start);
      const db = doc.offsetAt(b.range.start);
      return da - db;
    });

  // Pile de conteneurs basée sur accolades
  const root: Node = { name: "<root>", kind: SymbolKind.Namespace, range: Range.create(Position.create(0,0), Position.create(0,0)), selectionRange: Range.create(Position.create(0,0), Position.create(0,0)), children: [] };
  const stack: Node[] = [root];

  // Prépare positions des '{' et '}' sur zones code
  const mask = buildCodeMask(text);
  const opens: number[] = [];
  for (let i = 0; i < text.length; i++) {
    if (!mask[i]) continue;
    const ch = text.charCodeAt(i);
    if (ch === 0x7b /* { */) opens.push(i);
    else if (ch === 0x7d /* } */) {
      if (opens.length) opens.pop();
    }
  }
  // Index rapide: map startOffset->depth at that point
  // Simplification: nous utilisons l’ordre croissant + heuristique par proximité

  function pushToBestContainer(node: Node) {
    // Heuristique: rattacher au dernier symbole dont la position est avant node et qui n’est pas clos avant.
    for (let j = stack.length - 1; j >= 0; j--) {
      const top = stack[j];
      const topStart = doc.offsetAt(top.range.start);
      const nodeStart = doc.offsetAt(node.range.start);
      if (nodeStart >= topStart) {
        top.children.push(node);
        stack.push(node);
        return;
      }
    }
    root.children.push(node);
    stack.push(node);
  }

  // On parcourt les nodes et simule la fermeture quand on rencontre '}' avant prochain node
  const bracesPositions: number[] = [];
  const n = text.length;
  for (let i = 0; i < n; i++) if (mask[i] && (text[i] === '{' || text[i] === '}')) bracesPositions.push(i);

  let bp = 0;
  for (const node of nodes) {
    const nodeStart = doc.offsetAt(node.range.start);
    // ferme conteneurs jusqu’à la position courante
    while (bp < bracesPositions.length && bracesPositions[bp] < nodeStart) {
      const ch = text[bracesPositions[bp]];
      if (ch === '}') {
        if (stack.length > 1) stack.pop();
      }
      bp++;
    }
    pushToBestContainer(node);
  }
  // Ferme tout ce qui reste
  while (bp < bracesPositions.length) {
    if (text[bracesPositions[bp]] === '}' && stack.length > 1) stack.pop();
    bp++;
  }

  return root.children;
}

/* ------------------------------- Index & cache ----------------------------- */

const docCache = new WeakMap<TextDocument, { version: number; index: DocIndex }>();

function indexDocument(doc: TextDocument): DocIndex {
  const cached = docCache.get(doc);
  if (cached && cached.version === doc.version) return cached.index;

  const flat = collectFlatSymbols(doc);
  const outline = buildOutline(doc, flat);
  const byName = new Map<string, FlatSymbol[]>();
  for (const s of flat) {
    const arr = byName.get(s.name) ?? [];
    arr.push(s);
    byName.set(s.name, arr);
  }
  const index = { flat, outline, byName };
  docCache.set(doc, { version: doc.version, index });
  return index;
}

/* --------------------------------- API doc --------------------------------- */

export function documentSymbols(doc: TextDocument): DocumentSymbol[] {
  return sourceIndex(doc).outline;
}

export function symbolOutline(doc: TextDocument): DocumentSymbol[] {
  return documentSymbols(doc);
}

/* --------------------------- Définitions / refs ---------------------------- */

export function definitionAtPosition(doc: TextDocument, pos: Position, uri: string): Location[] {
  void uri;
  const resolved = resolveSourceSymbol(doc, pos)?.symbol;
  return resolved ? [resolved.location] : [];
}

export function referencesAtPosition(doc: TextDocument, pos: Position, uri: string): Location[] {
  const tok = tokenAt(doc, pos) ?? wordAt(doc, pos);
  if (!tok) return [];
  const pathLike = /[./:-]/.test(tok);
  const text = doc.getText();
  const mask = buildCodeMask(text);
  const out: Location[] = [];

  const re = pathLike
    ? new RegExp(`(?<![A-Za-z0-9_./:-])${escapeRx(tok)}(?![A-Za-z0-9_./:-])`, "g")
    : new RegExp(`(?<![A-Za-z0-9_])${escapeRx(tok)}(?![A-Za-z0-9_])`, "g");
  re.lastIndex = 0;
  let m: RegExpExecArray | null;
  while ((m = re.exec(text))) {
    const idx = m.index ?? 0;
    if (!mask[idx]) { if (m[0].length === 0) re.lastIndex++; continue; }
    const start = doc.positionAt(idx);
    const end = doc.positionAt(idx + m[0].length);
    out.push(Location.create(uri, Range.create(start, end)));
    if (m[0].length === 0) re.lastIndex++;
  }
  return out;
}

export interface NavigationOutcome {
  locations: Location[];
  unavailableReason?: string;
}

export async function definitionAtPositionWithImports(
  doc: TextDocument,
  pos: Position,
  environment: CompletionEnvironment,
): Promise<NavigationOutcome> {
  const resolved = resolveSourceSymbol(doc, pos)?.symbol;
  if (resolved?.role === "procedure" && resolved.traitOwner) {
    const index = sourceIndex(doc);
    const span = sourceTokenAt(doc, pos);
    const prefix = maskNonCode(doc.getText().slice(0, span?.start ?? doc.offsetAt(pos)));
    const receiver = /([A-Za-z_]\w*)\s*\.\s*$/.exec(prefix)?.[1];
    const receiverType = receiver ? visibleBindingsAt(index.symbols, receiver, doc.offsetAt(pos))[0]?.type?.match(/[A-Za-z_]\w*/)?.[0] : undefined;
    const owner = receiverType ? resolveAliasType(index.model.types, receiverType) : undefined;
    const activeProcedure = index.model.procedures.find((procedure) => procedure.bodyStart !== undefined && procedure.bodyStart <= doc.offsetAt(pos) && (procedure.bodyEnd ?? doc.getText().length) >= doc.offsetAt(pos));
    const isGenericReceiver = Boolean(receiverType && activeProcedure?.generics.includes(receiverType));
    if (owner && !isGenericReceiver) {
      const implementation = index.model.procedures.find((procedure) => procedure.name === resolved.name && procedure.implementedTrait === resolved.traitOwner && procedure.ownerType === owner);
      const implementationSymbol = implementation ? index.symbols.find((symbol) => symbol.role === "procedure" && symbol.nameStart === implementation.nameStart) : undefined;
      if (implementationSymbol) return { locations: [implementationSymbol.location] };
    }
  }
  const local = definitionAtPosition(doc, pos, doc.uri);
  if (local.length > 0) return { locations: local };
  const span = sourceTokenAt(doc, pos);
  if (!span) return { locations: [], unavailableReason: "Aucun nom Vitte sous le curseur." };
  const imports = await importedCompletionModel(doc, environment);
  const source = doc.getText();
  const prefix = maskNonCode(source.slice(0, span.start));
  const memberPrefix = /([A-Za-z_]\w*)\s*\.\s*$/.exec(prefix);
  if (memberPrefix?.[1]) {
    const sourceModel = sourceIndex(doc);
    const receiver = visibleBindingsAt(sourceModel.symbols, memberPrefix[1], span.start)[0];
    const receiverType = receiver?.type?.match(/[A-Za-z_]\w*/)?.[0];
    const importedType = only(imports.symbols.filter((symbol) => symbol.name === receiverType && symbol.type));
    const memberLocation = importedType?.memberLocations?.[span.name];
    if (memberLocation) return { locations: [memberLocation] };
  }
  const qualifiedPrefix = /([A-Za-z_]\w*)\s*::\s*$/.exec(prefix);
  if (qualifiedPrefix?.[1]) {
    const importedType = only(imports.symbols.filter((symbol) => symbol.name === qualifiedPrefix[1] && symbol.type));
    const memberLocation = importedType?.memberLocations?.[span.name];
    if (memberLocation) return { locations: [memberLocation] };
  }
  const matchingImports = imports.symbols.filter((symbol) => symbol.name === span.name);
  const imported = only(matchingImports);
  if (imported?.location) return { locations: [imported.location] };
  if (matchingImports.length > 1) return { locations: [], unavailableReason: `Plusieurs imports portent le nom « ${span.name} »; la cible reste ambiguë.` };
  const matchingModules = imports.modules.filter((module) => module.alias === span.name);
  const moduleAlias = only(matchingModules);
  if (moduleAlias?.uri) return { locations: [emptyModuleLocation(moduleAlias.uri)] };
  if (matchingModules.length > 1) return { locations: [], unavailableReason: `Plusieurs modules portent l’alias « ${span.name} »; la cible reste ambiguë.` };
  const modulePath = importedModulePathAt(doc, span.start, imports.modules);
  if (modulePath) {
    const exported = await readModuleExports(modulePath, doc.uri, environment);
    if (span.name === modulePath.split(/::|\//).pop() && exported.moduleUri) return { locations: [emptyModuleLocation(exported.moduleUri)] };
    const target = only(exported.symbols.filter((symbol) => symbol.name === span.name || symbol.originalName === span.name));
    if (target?.location) return { locations: [target.location] };
  }
  return { locations: [], unavailableReason: `La déclaration de « ${span.name} » n’est pas résolue dans ce fichier ni dans ses imports explicites.` };
}

export async function declarationAtPositionWithImports(
  doc: TextDocument,
  pos: Position,
  environment: CompletionEnvironment,
): Promise<NavigationOutcome> {
  // Vitte 0.1.1 does not distinguish declaration and definition locations in
  // an editor API, so source declarations are the best verified target.
  return definitionAtPositionWithImports(doc, pos, environment);
}

export async function typeDefinitionAtPositionWithImports(
  doc: TextDocument,
  pos: Position,
  environment: CompletionEnvironment,
): Promise<NavigationOutcome> {
  const resolved = resolveSourceSymbol(doc, pos);
  const typeIndex = sourceIndex(doc);
  if (!resolved?.symbol) {
    const imports = await importedCompletionModel(doc, environment);
    const importedType = only(imports.symbols.filter((candidate) => candidate.name === resolved?.token && candidate.type));
    if (!importedType?.type) return { locations: [], unavailableReason: "Le type du symbole n’est pas résolu par le frontend actif." };
    const aliasName = importedType.type.aliasTarget ? /[A-Za-z_]\w*/.exec(importedType.type.aliasTarget)?.[0] : undefined;
    const moduleExports = aliasName
      ? await readModuleExports(importedType.modulePath, doc.uri, environment)
      : undefined;
    const target = aliasName
      ? only([...(moduleExports?.symbols ?? []), ...imports.symbols].filter((candidate) => candidate.name === aliasName && candidate.type))
      : undefined;
    if (target?.location) return { locations: [target.location] };
    if (importedType.location) return { locations: [importedType.location] };
    return { locations: [], unavailableReason: `La définition du type « ${resolved?.token ?? "?"} » n’est pas résolue dans les imports explicites.` };
  }
  const symbol = resolved.symbol;
  let targetName = symbol.role === "type" ? symbol.aliasTarget ?? symbol.name : symbol.type;
  if (!targetName) {
    const procedure = typeIndex.model.procedures.find((candidate) => candidate.nameStart === symbol.nameStart);
    targetName = procedure?.returnType;
  }
  const name = targetName ? /[A-Za-z_]\w*/.exec(targetName)?.[0] : undefined;
  if (!name) return { locations: [], unavailableReason: `Aucune définition de type n’est connue pour « ${symbol.name} ».` };
  const local = typeIndex.symbols.find((candidate) => candidate.role === "type" && candidate.name === name);
  if (local) {
    let actual = local;
    const seen = new Set<string>();
    while (actual.aliasTarget && !seen.has(actual.name)) {
      seen.add(actual.name);
      const aliasName = /[A-Za-z_]\w*/.exec(actual.aliasTarget)?.[0];
      const next = typeIndex.symbols.find((candidate) => candidate.role === "type" && candidate.name === aliasName);
      if (!next) break;
      actual = next;
    }
    return { locations: [actual.location] };
  }
  const imports = await importedCompletionModel(doc, environment);
  const imported = only(imports.symbols.filter((candidate) => candidate.name === name && candidate.type));
  if (imported?.type?.aliasTarget) {
    const targetName = /[A-Za-z_]\w*/.exec(imported.type.aliasTarget)?.[0];
    const target = imports.symbols.find((candidate) => candidate.name === targetName && candidate.type);
    if (target?.location) return { locations: [target.location] };
  }
  if (imported?.location) return { locations: [imported.location] };
  return { locations: [], unavailableReason: `La définition du type « ${name} » n’est pas résolue dans le fichier ou ses imports explicites.` };
}

export async function implementationAtPositionWithImports(
  doc: TextDocument,
  pos: Position,
  environment: CompletionEnvironment,
  workspaceDocs: readonly { uri: string; doc: TextDocument }[] = [],
): Promise<NavigationOutcome> {
  const resolved = resolveSourceSymbol(doc, pos);
  const imports = await importedCompletionModel(doc, environment);
  const selected = resolved?.symbol;
  const importedType = !selected ? only(imports.symbols.filter((symbol) =>
    symbol.type?.kind === "trait" && (symbol.name === resolved?.token || symbol.type.methods.some((method) => method.name === resolved?.token)))) : undefined;
  const traitName = selected?.role === "type" && typeIsTrait(sourceIndex(doc), selected.name)
    ? selected.name
    : selected?.traitOwner ?? importedType?.type?.name;
  if (!traitName) return { locations: [], unavailableReason: `Aucune relation d’implémentation validée pour « ${selected?.name ?? resolved?.token ?? "ce symbole"} ».` };
  const methodName = selected?.role === "procedure" ? selected.name
    : importedType?.type?.methods.some((method) => method.name === resolved?.token) ? resolved?.token
      : undefined;
  const locations = new Map<string, Location>();
  for (const candidateDoc of [doc, ...workspaceDocs.filter((candidate) => candidate.uri !== doc.uri).map((candidate) => candidate.doc)]) {
    const index = sourceIndex(candidateDoc);
    for (const symbol of index.symbols) {
      if (symbol.role !== "procedure" || symbol.implementedTrait !== traitName || (methodName && symbol.name !== methodName)) continue;
      locations.set(locationKey(symbol.location), symbol.location);
    }
  }
  for (const symbol of imports.symbols) {
    const procedure = symbol.procedure;
    if (!procedure?.implementedTrait || procedure.implementedTrait !== traitName || (methodName && procedure.name !== methodName) || !symbol.location) continue;
    locations.set(locationKey(symbol.location), symbol.location);
  }
  if (importedType) {
    const exports = await readModuleExports(importedType.modulePath, doc.uri, environment);
    for (const symbol of exports.symbols) {
      const procedure = symbol.procedure;
      if (!procedure?.implementedTrait || procedure.implementedTrait !== traitName || !symbol.location) continue;
      locations.set(locationKey(symbol.location), symbol.location);
    }
  }
  if (locations.size === 0) return { locations: [], unavailableReason: `Aucune implémentation source visible de « ${traitName} » n’a été trouvée.` };
  return { locations: Array.from(locations.values()) };
}

export async function referencesAtPositionWithWorkspace(
  doc: TextDocument,
  pos: Position,
  environment: CompletionEnvironment,
  workspaceDocs: readonly { uri: string; doc: TextDocument }[],
  includeDeclaration = true,
): Promise<NavigationOutcome> {
  const resolved = resolveSourceSymbol(doc, pos);
  if (!resolved?.token) return { locations: [], unavailableReason: "Aucun nom Vitte sous le curseur." };
  const originImports = await importedCompletionModel(doc, environment);
  let localTarget = resolved.symbol;
  const matchingImportedTargets = localTarget ? [] : originImports.symbols.filter((symbol) => symbol.name === resolved.token && symbol.location);
  let importedTarget = only(matchingImportedTargets);
  if (!localTarget && matchingImportedTargets.length > 1) return { locations: [], unavailableReason: `Plusieurs imports portent le nom « ${resolved.token} »; les références ne sont pas résolues.` };
  let memberOwner: string | undefined;
  if (!localTarget && !importedTarget) {
    const prefix = maskNonCode(doc.getText().slice(0, doc.offsetAt(pos)));
    const receiverName = /([A-Za-z_]\w*)\s*\.\s*$/.exec(prefix)?.[1];
    const receiver = receiverName ? visibleBindingsAt(sourceIndex(doc).symbols, receiverName, doc.offsetAt(pos))[0] : undefined;
    const receiverType = receiver?.type?.match(/[A-Za-z_]\w*/)?.[0];
    importedTarget = only(originImports.symbols.filter((symbol) => symbol.name === receiverType
      && (Boolean(symbol.type?.fields.some((field) => field.name === resolved.token))
        || Boolean(symbol.type?.variants.some((variant) => variant.name === resolved.token)))));
    memberOwner = importedTarget?.type?.name;
    if (importedTarget?.memberLocations?.[resolved.token]) {
      const memberLocation = importedTarget.memberLocations[resolved.token];
      localTarget = {
        name: resolved.token,
        role: importedTarget.type?.variants.some((variant) => variant.name === resolved.token) ? "variant" : "field",
        ownerType: memberOwner,
        isPublic: importedTarget.type?.isPublic,
        location: memberLocation,
      } as SourceSymbol;
    }
  }
  if (!localTarget && !importedTarget?.location) return { locations: [], unavailableReason: "La référence n’est pas reliée à une déclaration connue." };
  const targetLocation = localTarget?.location ?? importedTarget?.location;
  if (!targetLocation) return { locations: [], unavailableReason: "La déclaration source de cette référence est indisponible." };
  const targetName = localTarget?.name ?? importedTarget?.originalName ?? resolved.token;
  const targetRole = localTarget?.role ?? (importedTarget?.type ? "type" : importedTarget?.procedure ? "procedure" : "binding");
  const targetPublic = localTarget?.isPublic ?? Boolean(importedTarget);
  const targetOwner = localTarget?.ownerType ?? memberOwner;
  const localScope = targetRole === "binding" || targetRole === "parameter" || ((targetRole === "field" || targetRole === "variant") && !targetPublic);
  const documents = [doc, ...workspaceDocs.filter((candidate) => candidate.uri !== doc.uri).map((candidate) => candidate.doc)];
  const locations: Location[] = [];
  if (includeDeclaration) locations.push(targetLocation);
  for (const candidateDoc of documents) {
    if (localScope && candidateDoc.uri !== doc.uri) continue;
    if (candidateDoc.uri !== doc.uri && !targetPublic) continue;
    const code = maskNonCode(candidateDoc.getText());
    let searchableNames: string[] = [];
    let importModel: Awaited<ReturnType<typeof importedCompletionModel>> | undefined;
    if (candidateDoc.uri === targetLocation.uri) searchableNames = [targetName];
    else if (candidateDoc.uri === doc.uri) searchableNames = [resolved.token];
    else {
      importModel = await importedCompletionModel(candidateDoc, environment);
      searchableNames = importModel.symbols
        .filter((symbol) => symbol.uri === targetLocation.uri && symbol.originalName === targetName)
        .map((symbol) => symbol.name);
      if (targetOwner && (targetRole === "field" || targetRole === "variant")) {
        const importsOwner = importModel.symbols.some((symbol) => symbol.uri === targetLocation.uri && symbol.type?.name === targetOwner
          && Boolean(symbol.memberLocations?.[targetName]));
        if (importsOwner) searchableNames.push(targetName);
      }
    }
    if (candidateDoc.uri === doc.uri && importedTarget) importModel = originImports;
    for (const name of new Set(searchableNames)) {
      const pattern = new RegExp(`(?<![A-Za-z0-9_])${escapeRx(name)}(?![A-Za-z0-9_])`, "g");
      let match: RegExpExecArray | null;
      while ((match = pattern.exec(code))) {
        const start = match.index;
        const occurrenceSymbol = resolveSourceSymbol(candidateDoc, candidateDoc.positionAt(start))?.symbol;
        if (candidateDoc.uri === targetLocation.uri) {
          if (!occurrenceSymbol || locationKey(occurrenceSymbol.location) !== locationKey(targetLocation)) continue;
        } else {
          if (occurrenceSymbol) continue;
          if (!importModel?.symbols.some((symbol) => symbol.uri === targetLocation.uri && (symbol.name === name || symbol.type?.name === targetOwner))) continue;
        }
        const range = Range.create(candidateDoc.positionAt(start), candidateDoc.positionAt(start + name.length));
        if (!includeDeclaration && candidateDoc.uri === targetLocation.uri && range.start.line === targetLocation.range.start.line && range.start.character === targetLocation.range.start.character) continue;
        locations.push(Location.create(candidateDoc.uri, range));
      }
    }
  }
  return { locations: dedupeBy(locations, (location) => locationKey(location)) };
}

function importedModulePathAt(doc: TextDocument, offset: number, aliases: readonly { alias: string; modulePath: string }[]): string | undefined {
  const code = maskNonCode(doc.getText());
  const lineStart = code.lastIndexOf("\n", offset) + 1;
  const line = code.slice(lineStart, offset);
  const importMatch = /\b(?:use|pull)\s+([A-Za-z_]\w*(?:(?:::|\/)[A-Za-z_]\w*)*)/.exec(line);
  if (importMatch?.[1]) return importMatch[1];
  const qualified = /([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)::$/.exec(line);
  if (!qualified?.[1]) return undefined;
  const path = qualified[1];
  const alias = aliases.find((candidate) => candidate.alias === path || path.startsWith(candidate.alias + "::"));
  return alias ? alias.modulePath + path.slice(alias.alias.length) : path;
}

function emptyModuleLocation(uri: string): Location {
  return Location.create(uri, Range.create(Position.create(0, 0), Position.create(0, 0)));
}

function typeIsTrait(index: SourceIndex, name: string): boolean {
  return index.model.types.some((type) => type.name === name && type.kind === "trait");
}

function locationKey(location: Location): string {
  return `${location.uri}:${location.range.start.line}:${location.range.start.character}:${location.range.end.line}:${location.range.end.character}`;
}

function only<T>(items: readonly T[]): T | undefined {
  return items.length === 1 ? items[0] : undefined;
}

/* --------------------------------- Rename ---------------------------------- */

export type RenameTarget =
  | { kind: "source"; name: string; range: Range; symbol: SourceSymbol }
  | { kind: "imported-symbol"; name: string; range: Range; symbol: ImportedCompletionSymbol }
  | { kind: "import-alias"; name: string; range: Range; symbol: ImportedCompletionSymbol }
  | { kind: "module-alias"; name: string; range: Range; module: ImportedModuleAlias };

export interface RenameTargetOutcome {
  target?: RenameTarget;
  unavailableReason?: string;
}

export async function resolveRenameTarget(
  doc: TextDocument,
  pos: Position,
  environment: CompletionEnvironment,
): Promise<RenameTargetOutcome> {
  const span = sourceTokenAt(doc, pos);
  if (!span) return { unavailableReason: "Placez le curseur sur un identifiant Vitte dans du code." };
  const range = Range.create(doc.positionAt(span.start), doc.positionAt(span.end));
  const resolved = resolveSourceSymbol(doc, pos);
  if (resolved?.symbol) {
    const symbol = resolved.symbol;
    if (["procedure", "type", "binding", "global", "parameter", "field", "variant"].includes(symbol.role)) {
      return { target: { kind: "source", name: span.name, range, symbol } };
    }
    if (symbol.role === "module") {
      return { unavailableReason: "Les chemins de module sont modifiés par le renommage du fichier uniquement lorsque leur mapping de projet est unique." };
    }
    return { unavailableReason: `La déclaration « ${span.name} » n’est pas une cible renommable.` };
  }

  const imports = await importedCompletionModel(doc, environment);
  const matchingSymbols = imports.symbols.filter((symbol) => symbol.name === span.name && symbol.location);
  if (matchingSymbols.length > 1) return { unavailableReason: `Plusieurs imports portent le nom « ${span.name} »; choisissez une déclaration non ambiguë.` };
  const imported = matchingSymbols[0];
  const importedDeclaration = imports.symbols.filter((symbol) => symbol.originalName === span.name && symbol.name !== span.name && symbol.location);
  if (!imported && importedDeclaration.length === 1) {
    const symbol = importedDeclaration[0];
    if (!symbol) return { unavailableReason: `L’import « ${span.name} » ne peut pas être résolu.` };
    return { target: { kind: "imported-symbol", name: span.name, range, symbol } };
  }
  if (!imported && importedDeclaration.length > 1) {
    return { unavailableReason: `Plusieurs imports aliasés déclarent le nom source « ${span.name} »; le renommage est ambigu.` };
  }
  if (imported) {
    return {
      target: {
        kind: imported.name === imported.originalName ? "imported-symbol" : "import-alias",
        name: span.name,
        range,
        symbol: imported,
      },
    };
  }
  const matchingModules = imports.modules.filter((module) => module.alias === span.name);
  if (matchingModules.length > 1) return { unavailableReason: `Plusieurs modules utilisent l’alias « ${span.name} ».` };
  const module = matchingModules[0];
  if (module) return { target: { kind: "module-alias", name: span.name, range, module } };
  return { unavailableReason: `« ${span.name} » n’est pas relié à une déclaration renommable; mots-clés, builtins et cibles non résolues sont exclus.` };
}

export async function renameReferencesForTarget(
  doc: TextDocument,
  pos: Position,
  target: RenameTarget,
  environment: CompletionEnvironment,
  workspaceDocs: readonly { uri: string; doc: TextDocument }[],
): Promise<NavigationOutcome> {
  if (target.kind === "import-alias") return importedAliasReferences(doc, target, environment);
  if (target.kind === "module-alias") return moduleAliasReferences(doc, target);

  let references: NavigationOutcome;
  if (target.kind === "imported-symbol") {
    const sourceDoc = workspaceDocs.find((candidate) => candidate.uri === target.symbol.uri)?.doc;
    const sourceLocation = target.symbol.location;
    if (!sourceDoc || !sourceLocation) return { locations: [], unavailableReason: "La déclaration importée n’est pas incluse dans le scan vérifié du workspace." };
    references = await referencesAtPositionWithWorkspace(
      sourceDoc,
      sourceLocation.range.start,
      environment,
      workspaceDocs,
      true,
    );
  } else {
    references = await referencesAtPositionWithWorkspace(doc, pos, environment, workspaceDocs, true);
  }
  if (references.unavailableReason) return references;
  if (target.kind !== "imported-symbol" && !(target.kind === "source" && target.symbol.isPublic)) return references;

  const sourceUri = target.symbol.location?.uri;
  if (!sourceUri) return { locations: [], unavailableReason: "La déclaration importée ne possède pas de position source." };
  const originalName = target.kind === "source" ? target.symbol.name : target.symbol.originalName;
  const locations = new Map(references.locations.map((location) => [locationKey(location), location]));
  for (const { uri, doc: candidateDoc } of workspaceDocs) {
    const importModel = await importedCompletionModel(candidateDoc, environment);
    const aliases = importModel.symbols.filter((symbol) => symbol.uri === sourceUri
      && symbol.originalName === originalName && symbol.name !== symbol.originalName);
    for (const alias of aliases) {
      const sameAliasOtherTargets = importModel.symbols.filter((symbol) => symbol.name === alias.name && symbol.uri !== sourceUri);
      if (sameAliasOtherTargets.length > 0) {
        return { locations: [], unavailableReason: `L’alias « ${alias.name} » masque plusieurs imports dans ${uri}; le renommage est refusé.` };
      }
      for (const [key, location] of locations) {
        if (location.uri === uri && locationText(candidateDoc, location) === alias.name) locations.delete(key);
      }
      const importNameLocations = findAliasedImportNameLocations(candidateDoc, alias);
      if (importNameLocations.length !== 1) {
        return { locations: [], unavailableReason: `L’import aliasé « ${alias.originalName} as ${alias.name} » ne peut pas être relié à une position unique.` };
      }
      const location = importNameLocations[0];
      locations.set(locationKey(location), location);
    }
  }
  return { locations: Array.from(locations.values()) };
}

export function validateRenameName(newName: string): string | undefined {
  if (!isValidIdent(newName)) return "Le nouveau nom doit être un identifiant Vitte valide.";
  if (RESERVED_WORDS.has(newName)) return `« ${newName} » est un mot réservé et ne peut pas être un nom de déclaration.`;
  return undefined;
}

export function validateRenameLocations(
  oldName: string,
  newName: string,
  locations: readonly Location[],
  docs: readonly { uri: string; doc: TextDocument }[],
): string | undefined {
  const byUri = new Map(docs.map((entry) => [entry.uri, entry.doc]));
  const edited = new Map<string, Set<number>>();
  for (const location of locations) {
    const doc = byUri.get(location.uri);
    if (!doc || locationText(doc, location).length === 0) return "Une position de renommage ne correspond plus à un buffer disponible.";
    const start = doc.offsetAt(location.range.start);
    if (locationText(doc, location) !== oldName) {
      return "Le contenu du buffer a changé depuis la résolution du symbole; relancez le renommage.";
    }
    const starts = edited.get(location.uri) ?? new Set<number>();
    starts.add(start);
    edited.set(location.uri, starts);
  }
  for (const [uri, starts] of edited) {
    const doc = byUri.get(uri);
    if (!doc) continue;
    const code = maskNonCode(doc.getText());
    const pattern = new RegExp(`(?<![A-Za-z0-9_])${escapeRx(newName)}(?![A-Za-z0-9_])`, "g");
    let match: RegExpExecArray | null;
    while ((match = pattern.exec(code))) {
      if (!starts.has(match.index)) return `« ${newName} » existe déjà dans ${uri}; un conflit ou une capture de portée est possible.`;
    }
  }
  return undefined;
}

function importedAliasReferences(
  doc: TextDocument,
  target: Extract<RenameTarget, { kind: "import-alias" }>,
  environment: CompletionEnvironment,
): Promise<NavigationOutcome> {
  return importedAliasReferencesAsync(doc, target, environment);
}

async function importedAliasReferencesAsync(
  doc: TextDocument,
  target: Extract<RenameTarget, { kind: "import-alias" }>,
  environment: CompletionEnvironment,
): Promise<NavigationOutcome> {
  const model = await importedCompletionModel(doc, environment);
  const identities = model.symbols.filter((symbol) => symbol.name === target.name
    && symbol.uri === target.symbol.uri && symbol.originalName === target.symbol.originalName);
  if (identities.length !== 1) return { locations: [], unavailableReason: `L’alias « ${target.name} » ne correspond pas à un import unique.` };
  const code = maskNonCode(doc.getText());
  const pattern = new RegExp(`(?<![A-Za-z0-9_])${escapeRx(target.name)}(?![A-Za-z0-9_])`, "g");
  const locations: Location[] = [];
  let match: RegExpExecArray | null;
  while ((match = pattern.exec(code))) {
    const occurrence = doc.positionAt(match.index);
    if (resolveSourceSymbol(doc, occurrence)?.symbol) continue;
    locations.push(Location.create(doc.uri, Range.create(occurrence, doc.positionAt(match.index + target.name.length))));
  }
  if (!locations.some((location) => location.range.start.line === target.range.start.line && location.range.start.character === target.range.start.character)) {
    return { locations: [], unavailableReason: `Aucune référence fiable à l’alias « ${target.name} » n’a été trouvée.` };
  }
  return { locations: dedupeBy(locations, locationKey) };
}

function moduleAliasReferences(doc: TextDocument, target: Extract<RenameTarget, { kind: "module-alias" }>): NavigationOutcome {
  const code = maskNonCode(doc.getText());
  const locations = new Map<string, Location>();
  const declaration = /\b(?:use|pull)\s+([A-Za-z_]\w*(?:(?:::|\/)[A-Za-z_]\w*)*)\s+as\s+([A-Za-z_]\w*)/g;
  let match: RegExpExecArray | null;
  while ((match = declaration.exec(code))) {
    if (match[1] !== target.module.modulePath || match[2] !== target.name) continue;
    const start = match.index + match[0].lastIndexOf(target.name);
    const location = Location.create(doc.uri, Range.create(doc.positionAt(start), doc.positionAt(start + target.name.length)));
    locations.set(locationKey(location), location);
  }
  const pattern = new RegExp(`(?<![A-Za-z0-9_])${escapeRx(target.name)}(?![A-Za-z0-9_])`, "g");
  while ((match = pattern.exec(code))) {
    const start = match.index;
    if (resolveSourceSymbol(doc, doc.positionAt(start))?.symbol) continue;
    let after = start + target.name.length;
    while (/\s/.test(code[after] ?? "")) after++;
    if (!code.startsWith("::", after) && code[after] !== ".") continue;
    const location = Location.create(doc.uri, Range.create(doc.positionAt(start), doc.positionAt(start + target.name.length)));
    locations.set(locationKey(location), location);
  }
  return locations.size > 0
    ? { locations: Array.from(locations.values()) }
    : { locations: [], unavailableReason: `Aucune utilisation fiable de l’alias de module « ${target.name} » n’a été trouvée.` };
}

function findAliasedImportNameLocations(doc: TextDocument, imported: ImportedCompletionSymbol): Location[] {
  const code = maskNonCode(doc.getText());
  const use = /\buse\s+([A-Za-z_]\w*(?:(?:::|\/)[A-Za-z_]\w*)*)/g;
  const expectedPath = imported.modulePath.replace(/\//g, "::");
  const aliasPattern = new RegExp(`(?<![A-Za-z0-9_])${escapeRx(imported.originalName)}(?![A-Za-z0-9_])\\s+as\\s+${escapeRx(imported.name)}(?![A-Za-z0-9_])`, "g");
  const locations: Location[] = [];
  let match: RegExpExecArray | null;
  while ((match = use.exec(code))) {
    if ((match[1] ?? "").replace(/\//g, "::") !== expectedPath) continue;
    const semicolon = code.indexOf(";", use.lastIndex);
    if (semicolon < 0) continue;
    const suffixStart = use.lastIndex;
    const suffix = code.slice(suffixStart, semicolon);
    let aliasMatch: RegExpExecArray | null;
    while ((aliasMatch = aliasPattern.exec(suffix))) {
      const start = suffixStart + aliasMatch.index;
      locations.push(Location.create(doc.uri, Range.create(doc.positionAt(start), doc.positionAt(start + imported.originalName.length))));
    }
    aliasPattern.lastIndex = 0;
    use.lastIndex = semicolon + 1;
  }
  return dedupeBy(locations, locationKey);
}

function locationText(doc: TextDocument, location: Location): string {
  return doc.getText(location.range);
}

export function tokenAtPosition(doc: TextDocument, pos: Position): string | null {
  return tokenAt(doc, pos) ?? wordAt(doc, pos);
}

/* --------------------------- Call/Type hierarchy --------------------------- */

function fullRangeForLine(doc: TextDocument, line: number): Range {
  const start = Position.create(line, 0);
  const end = Position.create(line, Number.MAX_SAFE_INTEGER);
  const text = doc.getText(Range.create(start, end));
  return Range.create(start, Position.create(line, text.length));
}

export function prepareCallHierarchy(doc: TextDocument, pos: Position, uri: string): CallHierarchyItem[] {
  const name = wordAt(doc, pos);
  if (!name) return [];
  const text = doc.getText();
  const rx = new RegExp(`\\b(?:proc|entry)\\s+${escapeRx(name)}\\b`, "g");
  let m: RegExpExecArray | null;
  const out: CallHierarchyItem[] = [];
  while ((m = rx.exec(text))) {
    const off = (m.index ?? 0) + m[0].lastIndexOf(name);
    const s = doc.positionAt(off);
    out.push({
      name,
      kind: SymbolKind.Function,
      uri,
      range: fullRangeForLine(doc, s.line),
      selectionRange: Range.create(s, Position.create(s.line, s.character + name.length)),
    });
    if (m[0].length === 0) rx.lastIndex++;
  }
  return dedupeBy(out, (i) => `${i.uri}:${i.selectionRange.start.line}:${i.selectionRange.start.character}:${i.name}`);
}

function findCallSites(doc: TextDocument, name: string): Range[] {
  const text = doc.getText();
  const mask = buildCodeMask(text);
  const rx = new RegExp(`(?<![A-Za-z0-9_])${escapeRx(name)}\\s*\\(`, "g");
  const out: Range[] = [];
  let m: RegExpExecArray | null;
  while ((m = rx.exec(text))) {
    const idx = m.index ?? 0;
    if (!mask[idx]) continue;
    const s = doc.positionAt(idx);
    out.push(Range.create(s, Position.create(s.line, s.character + name.length)));
    if (m[0].length === 0) rx.lastIndex++;
  }
  return out;
}

function nearestOwnerFn(doc: TextDocument, line: number): { name: string; line: number } | null {
  const lines = doc.getText().split(/\r?\n/);
  for (let i = line; i >= 0; i--) {
    const m = /^\s*(?:proc|entry)\s+([A-Za-z_]\w*)\b/.exec(lines[i] ?? "");
    if (m?.[1]) return { name: m[1], line: i };
  }
  return null;
}

export function callHierarchyIncoming(doc: TextDocument, item: CallHierarchyItem): CallHierarchyIncomingCall[] {
  const refs = findCallSites(doc, item.name);
  const byCaller = new Map<string, { caller: CallHierarchyItem; fromRanges: Range[] }>();
  for (const r of refs) {
    const owner = nearestOwnerFn(doc, r.start.line);
    if (!owner) continue;
    const key = `${owner.name}:${owner.line}`;
    let entry = byCaller.get(key);
    if (!entry) {
      const ownerStart = Position.create(owner.line, 0);
      entry = {
        caller: {
          name: owner.name,
          kind: SymbolKind.Function,
          uri: item.uri,
          range: fullRangeForLine(doc, owner.line),
          selectionRange: Range.create(ownerStart, Position.create(owner.line, owner.name.length)),
        },
        fromRanges: [],
      };
      byCaller.set(key, entry);
    }
    entry.fromRanges.push(r);
  }
  return Array.from(byCaller.values()).map((x) => ({ from: x.caller, fromRanges: x.fromRanges }));
}

function calledFnsInBody(doc: TextDocument, ownerLine: number): string[] {
  const lines = doc.getText().split(/\r?\n/);
  const start = ownerLine;
  const max = Math.min(lines.length - 1, ownerLine + 80);
  const out = new Set<string>();
  const rx = /\b([A-Za-z_]\w*)\s*\(/g;
  for (let i = start; i <= max; i++) {
    const line = lines[i] ?? "";
    if (i > start && /^\s*(?:proc|entry)\b/.test(line)) break;
    let m: RegExpExecArray | null;
    while ((m = rx.exec(line))) {
      const name = m[1];
      if (!name || name === "if" || name === "for" || name === "match") continue;
      out.add(name);
      if (m[0].length === 0) rx.lastIndex++;
    }
    rx.lastIndex = 0;
  }
  return Array.from(out);
}

export function callHierarchyOutgoing(doc: TextDocument, item: CallHierarchyItem): CallHierarchyOutgoingCall[] {
  const owner = nearestOwnerFn(doc, item.selectionRange.start.line);
  if (!owner) return [];
  const calls = calledFnsInBody(doc, owner.line);
  const out: CallHierarchyOutgoingCall[] = [];
  for (const name of calls) {
    const targets = prepareCallHierarchy(doc, Position.create(owner.line, 0), item.uri)
      .filter((i) => i.name === name);
    if (targets.length === 0) continue;
    const fromRanges = findCallSites(doc, name).filter((r) => r.start.line >= owner.line);
    out.push({ to: targets[0], fromRanges });
  }
  return out;
}

export function prepareTypeHierarchy(doc: TextDocument, pos: Position, uri: string): TypeHierarchyItem[] {
  const name = wordAt(doc, pos);
  if (!name) return [];
  const text = doc.getText();
  const rx = new RegExp(`\\b(?:type|form|trait|pick)\\s+${escapeRx(name)}\\b`, "g");
  let m: RegExpExecArray | null;
  const out: TypeHierarchyItem[] = [];
  while ((m = rx.exec(text))) {
    const off = (m.index ?? 0) + m[0].lastIndexOf(name);
    const s = doc.positionAt(off);
    out.push({
      name,
      kind: SymbolKind.Class,
      uri,
      range: fullRangeForLine(doc, s.line),
      selectionRange: Range.create(s, Position.create(s.line, s.character + name.length)),
    });
    if (m[0].length === 0) rx.lastIndex++;
  }
  return out;
}

export function typeHierarchySupertypes(doc: TextDocument, params: TypeHierarchySupertypesParams): TypeHierarchyItem[] {
  const item = params.item;
  const lines = doc.getText().split(/\r?\n/);
  const line = lines[item.selectionRange.start.line] ?? "";
  const out: TypeHierarchyItem[] = [];
  const m1 = /\b(?:type|trait|form|pick)\s+[A-Za-z_]\w*\s+as\s+([A-Za-z_]\w*)/.exec(line);
  const m2 = /\bis\s+([A-Za-z_]\w*)/.exec(line);
  for (const candidate of [m1?.[1], m2?.[1]]) {
    if (!candidate) continue;
    out.push({
      name: candidate,
      kind: SymbolKind.Interface,
      uri: item.uri,
      range: item.range,
      selectionRange: item.selectionRange,
    });
  }
  return out;
}

export function typeHierarchySubtypes(doc: TextDocument, params: TypeHierarchySubtypesParams): TypeHierarchyItem[] {
  const name = params.item.name;
  const text = doc.getText();
  const rx = new RegExp(`\\b(?:type|trait|form|pick)\\s+([A-Za-z_]\\w*)\\s+as\\s+${escapeRx(name)}\\b`, "g");
  let m: RegExpExecArray | null;
  const out: TypeHierarchyItem[] = [];
  while ((m = rx.exec(text))) {
    const sub = m[1];
    if (!sub) continue;
    const off = (m.index ?? 0) + m[0].indexOf(sub);
    const s = doc.positionAt(off);
    out.push({
      name: sub,
      kind: SymbolKind.Class,
      uri: params.item.uri,
      range: fullRangeForLine(doc, s.line),
      selectionRange: Range.create(s, Position.create(s.line, s.character + sub.length)),
    });
    if (m[0].length === 0) rx.lastIndex++;
  }
  return out;
}

/* ----------------------------- Workspace symbols --------------------------- */

export function workspaceSymbols(
  query: string,
  openDocs: { uri: string; doc: TextDocument }[],
  limit = 200
): WorkspaceSymbol[] {
  const q = query.trim();
  const result: { ws: WorkspaceSymbol; score: number; idx: number }[] = [];
  let seq = 0;
  for (const { uri, doc } of openDocs) {
    const { symbols } = sourceIndex(doc);
    for (const s of symbols) {
      if (s.role === "parameter" || s.role === "binding" || s.role === "module" || s.role === "implementation") continue;
      const score = q ? fuzzyScore(s.name, q) : 1;
      if (q && score <= 0) continue;
      result.push({
        ws: { name: s.name, kind: s.kind, location: Location.create(uri, s.selectionRange), ...(s.containerName ? { containerName: s.containerName } : {}) },
        score,
        idx: seq++,
      });
      if (result.length >= limit * 4) break; // cap intermédiaire avant tri
    }
  }
  result.sort((a, b) => b.score - a.score || a.idx - b.idx);
  return result.slice(0, limit).map(r => r.ws);
}

/* --------------------------------- Helpers -------------------------------- */

function fuzzyScore(name: string, query: string): number {
  // subsequence scoring sensible à la casse avec bonus pour débuts de mots et camelCase
  let i = 0, j = 0, score = 0, lastMatch = -2;
  while (i < name.length && j < query.length) {
    const nc = name[i];
    const qc = query[j];
    if (nc.toLowerCase() === qc.toLowerCase()) {
      let s = 1;
      if (i === 0 || !isIdentChar(name.charCodeAt(i - 1))) s += 2; // début de mot
      if (nc === qc) s += 1; // casse exacte
      if (i === lastMatch + 1) s += 2; // consécutif
      score += s;
      lastMatch = i;
      j++; i++;
    } else {
      i++;
    }
  }
  return j === query.length ? score : -1;
}

/* --------------------------------- Exports -------------------------------- */

export function indexDocForTests(doc: TextDocument): DocIndex { return indexDocument(doc); }
