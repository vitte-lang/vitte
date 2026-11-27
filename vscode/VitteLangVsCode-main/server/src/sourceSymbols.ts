import { Location, Range, SymbolKind } from "vscode-languageserver/node";
import type { DocumentSymbol, Position } from "vscode-languageserver/node";
import type { TextDocument } from "vscode-languageserver-textdocument";
import { parseCompletionModel, type ProcedureSymbol, type TypeSymbol } from "./completionModel.js";
import { maskNonCode } from "./lexical.js";

export type SourceSymbolRole = "module" | "procedure" | "type" | "field" | "variant" | "parameter" | "binding" | "implementation";

export interface SourceSymbol {
  name: string;
  kind: SymbolKind;
  role: SourceSymbolRole;
  location: Location;
  range: Range;
  selectionRange: Range;
  start: number;
  nameStart: number;
  nameEnd: number;
  end: number;
  detail?: string;
  documentation?: string;
  type?: string;
  signature?: string;
  isPublic?: boolean;
  ownerType?: string;
  traitOwner?: string;
  implementedTrait?: string;
  aliasTarget?: string;
  scopeStart?: number;
  scopeEnd?: number;
  bodyStart?: number;
  bodyEnd?: number;
  containerName?: string;
}

export interface SourceIndex {
  symbols: SourceSymbol[];
  outline: DocumentSymbol[];
  model: ReturnType<typeof parseCompletionModel>;
}

const CACHE = new WeakMap<TextDocument, { version: number; index: SourceIndex }>();

export function sourceIndex(doc: TextDocument): SourceIndex {
  const cached = CACHE.get(doc);
  if (cached?.version === doc.version) return cached.index;
  const text = doc.getText();
  const code = maskNonCode(text);
  const model = parseCompletionModel(doc, text.length);
  const symbols: SourceSymbol[] = [];

  for (const type of model.types) symbols.push(typeSymbol(doc, type));
  for (const procedure of model.procedures) {
    const symbol = procedureSymbol(doc, procedure);
    if (procedure.traitOwner && model.types.some((type) => type.name === procedure.traitOwner && type.isPublic)) symbol.isPublic = true;
    if (procedure.implementedTrait && model.types.some((type) => type.name === procedure.implementedTrait && type.isPublic)) symbol.isPublic = true;
    symbols.push(symbol);
  }
  addImplementationContainers(doc, code, text, symbols);
  addBindings(doc, code, model.procedures, symbols);
  addModuleSymbols(doc, code, symbols);

  for (const type of model.types) {
    for (const field of type.fields) {
      if (field.nameStart === undefined || field.nameEnd === undefined) continue;
      symbols.push(makeSymbol(doc, {
        name: field.name,
        kind: SymbolKind.Field,
        role: "field",
        start: field.nameStart,
        nameStart: field.nameStart,
        nameEnd: field.nameEnd,
        end: field.nameEnd,
        detail: field.type,
        type: field.type,
        documentation: field.documentation,
        ownerType: type.name,
        isPublic: type.isPublic,
      }));
    }
    for (const variant of type.variants) {
      if (variant.nameStart === undefined || variant.nameEnd === undefined) continue;
      symbols.push(makeSymbol(doc, {
        name: variant.name,
        kind: SymbolKind.EnumMember,
        role: "variant",
        start: variant.nameStart,
        nameStart: variant.nameStart,
        nameEnd: variant.nameEnd,
        end: variant.nameEnd,
        detail: variant.payload.length ? variant.payload.join(", ") : type.name,
        type: type.name,
        documentation: variant.documentation,
        ownerType: type.name,
        isPublic: type.isPublic,
      }));
    }
  }

  const withContainers = assignContainers(doc, symbols);
  const index = { symbols: withContainers, outline: buildOutline(withContainers), model };
  CACHE.set(doc, { version: doc.version, index });
  return index;
}

export function sourceTokenAt(doc: TextDocument, position: Position): { name: string; start: number; end: number } | undefined {
  const text = doc.getText();
  const code = maskNonCode(text);
  const offset = doc.offsetAt(position);
  let point = offset;
  if (!(point < text.length && isIdentifier(code[point])) && point > 0 && isIdentifier(code[point - 1])) point--;
  if (!(point < text.length && isIdentifier(code[point]))) return undefined;
  let start = point;
  let end = point + 1;
  while (start > 0 && isIdentifier(code[start - 1])) start--;
  while (end < text.length && isIdentifier(code[end])) end++;
  return { name: text.slice(start, end), start, end };
}

export function resolveSourceSymbol(doc: TextDocument, position: Position): { token: string; symbol?: SourceSymbol } | undefined {
  const span = sourceTokenAt(doc, position);
  if (!span) return undefined;
  const index = sourceIndex(doc);
  const offset = doc.offsetAt(position);
  const declaredHere = index.symbols.find((symbol) => symbol.nameStart <= span.start && span.start < symbol.nameEnd);
  if (declaredHere) return { token: span.name, symbol: declaredHere };

  const text = doc.getText();
  const prefix = maskNonCode(text.slice(0, span.start));
  const memberPrefix = /([A-Za-z_]\w*)\s*\.\s*$/.exec(prefix);
  if (memberPrefix?.[1]) {
    const receiver = memberPrefix[1];
    const visibleBindings = visibleBindingsAt(index.symbols, receiver, offset);
    const receiverType = visibleBindings[0]?.type?.match(/[A-Za-z_]\w*/)?.[0];
    const owner = resolveAliasType(index.model.types, receiverType);
    const fields = index.symbols.filter((symbol) => symbol.role === "field" && symbol.name === span.name && symbol.ownerType === owner);
    const member = only(fields);
    if (member) return { token: span.name, symbol: member };
    if (fields.length > 1) return { token: span.name };
    const methods = index.model.types.flatMap((type) => type.methods).filter((procedure) => procedure.name === span.name && typeMatchesOwner(procedure, owner));
    const method = only(methods);
    if (method) {
      const symbol = index.symbols.find((candidate) => candidate.role === "procedure" && candidate.nameStart === method.nameStart);
      if (symbol) return { token: span.name, symbol };
    }
    if (methods.length > 1) return { token: span.name };
  }

  const qualifierPrefix = /([A-Za-z_]\w*)\s*::\s*$/.exec(prefix);
  if (qualifierPrefix?.[1]) {
    const owner = qualifierPrefix[1];
    const variants = index.symbols.filter((symbol) => symbol.role === "variant" && symbol.name === span.name && symbol.ownerType === owner);
    const variant = only(variants);
    if (variant) return { token: span.name, symbol: variant };
    if (variants.length > 1) return { token: span.name };
  }

  const local = visibleBindingsAt(index.symbols, span.name, offset)[0];
  if (local) return { token: span.name, symbol: local };

  const lineStart = prefix.lastIndexOf("\n") + 1;
  const linePrefix = prefix.slice(lineStart);
  if (/:|->|\bas\s*$/.test(linePrefix)) {
    const types = index.symbols.filter((symbol) => symbol.role === "type" && symbol.name === span.name);
    const type = only(types);
    if (type) return { token: span.name, symbol: type };
    if (types.length > 1) return { token: span.name };
  }
  const following = maskNonCode(text.slice(span.end)).trimStart();
  const candidates = index.symbols.filter((symbol) => symbol.name === span.name && (symbol.role === "procedure" || symbol.role === "type" || symbol.role === "binding"));
  if (following.startsWith("(") || /\b(?:proc|intrinsic)\s*$/.test(linePrefix)) {
    const procedure = only(candidates.filter((symbol) => symbol.role === "procedure" && !symbol.traitOwner && !symbol.ownerType));
    if (procedure) return { token: span.name, symbol: procedure };
  }
  const types = candidates.filter((symbol) => symbol.role === "type");
  const type = only(types);
  if (type) return { token: span.name, symbol: type };
  const procedure = only(candidates.filter((symbol) => symbol.role === "procedure" && !symbol.ownerType && !symbol.traitOwner));
  if (procedure) return { token: span.name, symbol: procedure };
  return { token: span.name };
}

export function symbolAtDeclaration(index: SourceIndex, offset: number): SourceSymbol | undefined {
  return index.symbols.find((symbol) => symbol.nameStart <= offset && offset < symbol.nameEnd);
}

export function visibleBindingsAt(symbols: readonly SourceSymbol[], name: string, offset: number): SourceSymbol[] {
  return symbols
    .filter((symbol) => (symbol.role === "binding" || symbol.role === "parameter")
      && symbol.name === name
      && symbol.start <= offset
      && (symbol.scopeStart === undefined || symbol.scopeStart <= offset)
      && (symbol.scopeEnd === undefined || offset <= symbol.scopeEnd))
    .sort((left, right) => {
      const leftWidth = (left.scopeEnd ?? Number.MAX_SAFE_INTEGER) - (left.scopeStart ?? 0);
      const rightWidth = (right.scopeEnd ?? Number.MAX_SAFE_INTEGER) - (right.scopeStart ?? 0);
      return leftWidth - rightWidth || right.start - left.start;
    });
}

export function resolveAliasType(types: readonly TypeSymbol[], name: string | undefined): string | undefined {
  let current = name;
  const seen = new Set<string>();
  while (current && !seen.has(current)) {
    seen.add(current);
    const alias = types.find((type) => type.name === current && type.aliasTarget);
    if (!alias?.aliasTarget) return current;
    current = /^[A-Za-z_]\w*/.exec(alias.aliasTarget)?.[0];
  }
  return current;
}

function typeSymbol(doc: TextDocument, type: TypeSymbol): SourceSymbol {
  const start = type.start;
  const nameStart = type.nameStart ?? start;
  const end = Math.min(doc.getText().length, (type.bodyEnd ?? type.declarationEnd) + 1);
  const generics = type.generics.length ? "<" + type.generics.join(", ") + ">" : "";
  const signature = type.kind + " " + type.name + generics + (type.aliasTarget ? " = " + type.aliasTarget : "");
  return makeSymbol(doc, {
    name: type.name,
    kind: type.kind === "form" ? SymbolKind.Struct : type.kind === "pick" ? SymbolKind.Enum : type.kind === "trait" ? SymbolKind.Interface : SymbolKind.TypeParameter,
    role: "type",
    start,
    nameStart,
    nameEnd: type.nameEnd ?? nameStart + type.name.length,
    end,
    detail: signature,
    signature,
    documentation: type.documentation,
    isPublic: type.isPublic,
    type: type.aliasTarget,
    aliasTarget: type.aliasTarget,
    bodyStart: type.bodyStart,
    bodyEnd: type.bodyEnd,
  });
}

function procedureSymbol(doc: TextDocument, procedure: ProcedureSymbol): SourceSymbol {
  const nameStart = procedure.nameStart ?? procedure.start;
  const end = Math.min(doc.getText().length, (procedure.bodyEnd ?? procedure.declarationEnd) + (procedure.bodyEnd === undefined ? 1 : 1));
  const generic = procedure.generics.length ? "<" + procedure.generics.join(", ") + ">" : "";
  const params = procedure.parameters.map((param) => (param.mode ? param.mode + " " : "") + param.name + (param.type ? ": " + param.type : "")).join(", ");
  const signature = "proc " + procedure.name + generic + "(" + params + ")" + (procedure.returnType ? " -> " + procedure.returnType : "");
  return makeSymbol(doc, {
    name: procedure.name,
    kind: procedure.ownerType || procedure.traitOwner ? SymbolKind.Method : SymbolKind.Function,
    role: "procedure",
    start: procedure.start,
    nameStart,
    nameEnd: procedure.nameEnd ?? nameStart + procedure.name.length,
    end,
    detail: signature,
    signature,
    documentation: procedure.documentation,
    isPublic: procedure.isPublic,
    ownerType: procedure.ownerType,
    traitOwner: procedure.traitOwner,
    implementedTrait: procedure.implementedTrait,
    scopeStart: procedure.bodyStart,
    scopeEnd: procedure.bodyEnd,
    bodyStart: procedure.bodyStart,
    bodyEnd: procedure.bodyEnd,
    type: procedure.returnType,
  });
}

function addBindings(doc: TextDocument, code: string, procedures: readonly ProcedureSymbol[], symbols: SourceSymbol[]): void {
  const blocks = braceScopes(code);
  const pattern = /\b(let|make|const|static|global|share)\s+(?:mut\s+)?([A-Za-z_]\w*)(?:\s*:\s*([^=;,)\n]+))?/g;
  let match: RegExpExecArray | null;
  while ((match = pattern.exec(code))) {
    const name = match[2];
    if (!name) continue;
    const start = match.index;
    const nameStart = start + match[0].indexOf(name);
    const scope = innermostScope(blocks, nameStart);
    const proc = procedures.filter((procedure) => procedure.bodyStart !== undefined && procedure.bodyStart < nameStart && (procedure.bodyEnd ?? code.length) >= nameStart).sort((a, b) => (a.bodyEnd ?? code.length) - (a.bodyStart ?? 0) - ((b.bodyEnd ?? code.length) - (b.bodyStart ?? 0)))[0];
    const declaredType = match[3]?.trim();
    const inferredType = declaredType ?? inferLiteralType(code, pattern.lastIndex);
    const kind = match[1] === "const" ? SymbolKind.Constant : SymbolKind.Variable;
    symbols.push(makeSymbol(doc, {
      name,
      kind,
      role: "binding",
      start,
      nameStart,
      nameEnd: nameStart + name.length,
      end: nameStart + name.length,
      detail: inferredType ? (kind === SymbolKind.Constant ? "const " : "let ") + name + ": " + inferredType : name,
      type: inferredType,
      scopeStart: scope?.start ?? proc?.bodyStart,
      scopeEnd: scope?.end ?? proc?.bodyEnd,
      isPublic: match[1] === "const" || match[1] === "static" || match[1] === "global" ? /\b(?:export|pub)\b/.test(code.slice(Math.max(0, start - 40), start)) : false,
    }));
  }

  for (const procedure of procedures) {
    if (procedure.bodyStart === undefined || procedure.bodyEnd === undefined) continue;
    for (const parameter of procedure.parameters) {
      if (parameter.nameStart === undefined || parameter.nameEnd === undefined) continue;
      symbols.push(makeSymbol(doc, {
        name: parameter.name,
        kind: SymbolKind.Variable,
        role: "parameter",
        start: parameter.nameStart,
        nameStart: parameter.nameStart,
        nameEnd: parameter.nameEnd,
        end: parameter.nameEnd,
        detail: parameter.type ? "paramètre: " + parameter.type : "paramètre",
        type: parameter.type,
        documentation: parameter.documentation ?? parameterDoc(procedure.documentation, parameter.name),
        scopeStart: procedure.bodyStart,
        scopeEnd: procedure.bodyEnd,
        containerName: procedure.name,
      }));
    }
  }
}

function addModuleSymbols(doc: TextDocument, code: string, symbols: SourceSymbol[]): void {
  const pattern = /\b(?:use|pull|space)\s+([A-Za-z_]\w*(?:(?:::|\/|\.)[A-Za-z_]\w*)*)/g;
  let match: RegExpExecArray | null;
  while ((match = pattern.exec(code))) {
    const path = match[1];
    if (!path) continue;
    const name = path.split(/::|\/|\./).pop() ?? path;
    const nameStart = match.index + match[0].lastIndexOf(name);
    const isNamespace = match[0].startsWith("space");
    const open = isNamespace ? code.indexOf("{", pattern.lastIndex) : -1;
    const semicolon = isNamespace ? code.indexOf(";", pattern.lastIndex) : -1;
    const hasBlock = open >= 0 && (semicolon < 0 || open < semicolon);
    const bodyStart = isNamespace ? hasBlock ? open : semicolon >= 0 ? semicolon + 1 : nameStart + name.length : undefined;
    const bodyEnd = isNamespace ? hasBlock && bodyStart !== undefined ? matchingBrace(code, bodyStart) : code.length : undefined;
    symbols.push(makeSymbol(doc, {
      name,
      kind: SymbolKind.Module,
      role: "module",
      start: match.index,
      nameStart,
      nameEnd: nameStart + name.length,
      end: bodyEnd !== undefined && bodyEnd >= 0 ? hasBlock ? bodyEnd + 1 : bodyEnd : nameStart + name.length,
      detail: path,
      documentation: isNamespace ? "Espace de noms Vitte " + path + "." : "Module importé " + path + ".",
      type: path,
      ...(bodyStart !== undefined && bodyEnd !== undefined && bodyEnd >= 0 ? { bodyStart, bodyEnd } : {}),
    }));
    if (bodyEnd !== undefined && bodyEnd >= 0) pattern.lastIndex = bodyEnd + 1;
  }
}

function addImplementationContainers(doc: TextDocument, code: string, text: string, symbols: SourceSymbol[]): void {
  const pattern = /\bimpl\b/g;
  let match: RegExpExecArray | null;
  while ((match = pattern.exec(code))) {
    const open = code.indexOf("{", pattern.lastIndex);
    const semicolon = code.indexOf(";", pattern.lastIndex);
    if (open < 0 || (semicolon >= 0 && semicolon < open)) continue;
    const close = matchingBrace(code, open);
    if (close < 0) continue;
    const header = text.slice(match.index, open);
    const target = /\bfor\s+([A-Za-z_]\w*)/.exec(header)?.[1] ?? /\bimpl\s*(?:<[^>]*>\s*)?([A-Za-z_]\w*)/.exec(header)?.[1];
    const trait = /\bimpl\s*(?:<[^>]*>\s*)?([A-Za-z_]\w*)\s+for\b/.exec(header)?.[1];
    if (!target) continue;
    const display = "impl " + (trait ? trait + " for " : "") + target;
    const nameStart = match.index;
    symbols.push(makeSymbol(doc, {
      name: display,
      kind: SymbolKind.Class,
      role: "implementation",
      start: match.index,
      nameStart,
      nameEnd: nameStart + 4,
      end: close + 1,
      detail: trait ? "impl " + trait + " for " + target : "impl " + target,
      documentation: trait ? "Implémentation de " + trait + " pour " + target + "." : "Bloc d’implémentation de " + target + ".",
      ownerType: target,
      implementedTrait: trait,
      bodyStart: open,
      bodyEnd: close,
    }));
    pattern.lastIndex = close + 1;
  }
}

function assignContainers(doc: TextDocument, symbols: SourceSymbol[]): SourceSymbol[] {
  return symbols.map((symbol) => {
    const parent = symbols
      .filter((candidate) => candidate !== symbol && candidate.bodyStart !== undefined && candidate.bodyEnd !== undefined
        && candidate.bodyStart < symbol.start && symbol.end <= candidate.bodyEnd)
      .sort((left, right) => ((left.bodyEnd ?? 0) - (left.bodyStart ?? 0)) - ((right.bodyEnd ?? 0) - (right.bodyStart ?? 0)))[0];
    if (!parent) return symbol;
    if ((symbol.role === "field" || symbol.role === "variant") && symbol.ownerType && parent.name !== symbol.ownerType) return symbol;
    return { ...symbol, containerName: parent.name };
  });
}

function buildOutline(symbols: readonly SourceSymbol[]): DocumentSymbol[] {
  const nodes = new Map<SourceSymbol, DocumentSymbol>();
  for (const symbol of symbols) {
    if (symbol.role === "module" && !symbol.documentation?.startsWith("Espace de noms")) continue;
    nodes.set(symbol, {
      name: symbol.name,
      kind: symbol.kind,
      range: symbol.range,
      selectionRange: symbol.selectionRange,
      ...(symbol.detail ? { detail: symbol.detail } : {}),
      children: [],
    });
  }
  const roots: DocumentSymbol[] = [];
  for (const [symbol, node] of nodes) {
    const parentSymbol = symbols
      .filter((candidate) => nodes.has(candidate) && candidate !== symbol
        && ((candidate.bodyStart !== undefined && candidate.bodyEnd !== undefined && candidate.bodyStart < symbol.start && symbol.end <= candidate.bodyEnd)
          || (symbol.containerName === candidate.name && candidate.role === "procedure")))
      .sort((left, right) => ((left.bodyEnd ?? 0) - (left.bodyStart ?? 0)) - ((right.bodyEnd ?? 0) - (right.bodyStart ?? 0)))[0];
    if (parentSymbol) nodes.get(parentSymbol)?.children?.push(node);
    else roots.push(node);
  }
  const sortTree = (items: DocumentSymbol[]): void => {
    items.sort((left, right) => left.range.start.line - right.range.start.line || left.range.start.character - right.range.start.character);
    for (const item of items) if (item.children) sortTree(item.children);
  };
  sortTree(roots);
  return roots;
}

function makeSymbol(doc: TextDocument, input: Omit<SourceSymbol, "location" | "range" | "selectionRange">): SourceSymbol {
  const textLength = doc.getText().length;
  const start = Math.max(0, Math.min(input.start, textLength));
  const end = Math.max(start, Math.min(input.end, textLength));
  const nameStart = Math.max(start, Math.min(input.nameStart, textLength));
  const nameEnd = Math.max(nameStart, Math.min(input.nameEnd, textLength));
  const range = Range.create(doc.positionAt(start), doc.positionAt(end));
  const selectionRange = Range.create(doc.positionAt(nameStart), doc.positionAt(nameEnd));
  return { ...input, start, end, nameStart, nameEnd, range, selectionRange, location: Location.create(doc.uri, selectionRange) };
}

function braceScopes(code: string): { start: number; end: number }[] {
  const scopes: { start: number; end: number }[] = [];
  const stack: number[] = [];
  for (let i = 0; i < code.length; i++) {
    if (code[i] === "{") stack.push(i);
    else if (code[i] === "}") {
      const start = stack.pop();
      if (start !== undefined) scopes.push({ start, end: i });
    }
  }
  return scopes;
}

function innermostScope(scopes: { start: number; end: number }[], offset: number): { start: number; end: number } | undefined {
  return scopes.filter((scope) => scope.start < offset && offset <= scope.end).sort((left, right) => right.start - left.start)[0];
}

function matchingBrace(code: string, open: number): number {
  let depth = 0;
  for (let i = open; i < code.length; i++) {
    if (code[i] === "{") depth++;
    else if (code[i] === "}" && --depth === 0) return i;
  }
  return -1;
}

function inferLiteralType(code: string, after: number): string | undefined {
  const end = code.indexOf("\n", after);
  const value = code.slice(after, end < 0 ? code.length : end).replace(/^\s*=\s*/, "").trim();
  if (/^(?:true|false)\b/.test(value)) return "bool";
  if (value.startsWith('"')) return "string";
  if (value.startsWith("'")) return "char";
  if (/^-?\d/.test(value)) return "int";
  const ctor = /^([A-Za-z_]\w*)\s*(?:\{|::)/.exec(value);
  return ctor?.[1];
}

function parameterDoc(doc: string | undefined, name: string): string | undefined {
  if (!doc) return undefined;
  const escaped = name.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  return new RegExp("^\\s*(?:[-*]\\s*)?" + escaped + "\\s*:\\s*(.+)$", "m").exec(doc)?.[1]?.trim();
}

function typeMatchesOwner(procedure: ProcedureSymbol, owner: string | undefined): boolean {
  return !!owner && procedure.ownerType === owner;
}

function isIdentifier(value: string | undefined): boolean {
  return value !== undefined && /[A-Za-z0-9_]/.test(value);
}

function only<T>(candidates: readonly T[]): T | undefined {
  return candidates.length === 1 ? candidates[0] : undefined;
}
