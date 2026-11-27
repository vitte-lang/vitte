import { CompletionItemKind } from "vscode-languageserver/node";
import type { TextDocument } from "vscode-languageserver-textdocument";
import { getDocstringAtLine } from "./docstrings.js";
import { maskNonCode } from "./lexical.js";

export interface ProcedureParameter {
  name: string;
  type?: string;
  mode?: string;
  documentation?: string;
}

export interface ProcedureSymbol {
  name: string;
  parameters: ProcedureParameter[];
  generics: string[];
  returnType?: string;
  documentation?: string;
  isPublic: boolean;
  start: number;
  bodyStart?: number;
  bodyEnd?: number;
  declarationEnd: number;
  ownerType?: string;
  scopeGenerics?: string[];
  isInstanceMethod?: boolean;
  traitOwner?: string;
  implementedTrait?: string;
  genericTraitBounds?: Record<string, string[]>;
}

export interface FormFieldSymbol {
  name: string;
  type: string;
  documentation?: string;
}

export interface PickVariantSymbol {
  name: string;
  payload: string[];
  documentation?: string;
}

export interface TypeSymbol {
  name: string;
  kind: "form" | "pick" | "trait" | "type" | "opaque";
  generics: string[];
  fields: FormFieldSymbol[];
  variants: PickVariantSymbol[];
  methods: ProcedureSymbol[];
  aliasTarget?: string;
  documentation?: string;
  isPublic: boolean;
  start: number;
  bodyStart?: number;
  bodyEnd?: number;
  declarationEnd: number;
}

export interface LocalBinding {
  name: string;
  type?: string;
  kind: CompletionItemKind;
  detail: string;
  start: number;
}

export interface DocumentCompletionModel {
  procedures: ProcedureSymbol[];
  types: TypeSymbol[];
  bindings: LocalBinding[];
  genericParameters: string[];
}

interface BraceBlock {
  open: number;
  close: number;
  parent?: number;
}

export function parseCompletionModel(doc: TextDocument, cursorOffset = doc.getText().length): DocumentCompletionModel {
  const text = doc.getText();
  const code = maskNonCode(text);
  const blocks = findBraceBlocks(code);
  const procedures = parseProcedures(doc, text, code);
  const types = parseTypes(doc, text, code);
  attachTraitMethods(types, procedures);
  resolveLocalTypeAliases(types);
  attachImplementationMethods(text, code, procedures);
  const bindings = parseVisibleBindings(text, code, cursorOffset, blocks, procedures);
  const genericParameters = parseVisibleGenericParameters(code, cursorOffset, procedures, types);
  return { procedures, types, bindings, genericParameters };
}

function parseProcedures(doc: TextDocument, text: string, code: string): ProcedureSymbol[] {
  const result: ProcedureSymbol[] = [];
  const declaration = /\b(?:proc|intrinsic)\s+([A-Za-z_]\w*)/g;
  let match: RegExpExecArray | null;
  while ((match = declaration.exec(code))) {
    const name = match[1];
    if (!name || !isCodeAt(code, match.index)) continue;
    let cursor = declaration.lastIndex;
    while (/\s/.test(code[cursor] ?? "")) cursor++;
    let generics: string[] = [];
    let genericTraitBounds: Record<string, string[]> = {};
    if (code[cursor] === "<") {
      const genericEnd = findClosing(code, cursor, "<", ">");
      if (genericEnd === undefined) continue;
      const genericText = text.slice(cursor + 1, genericEnd);
      generics = parseGenericNames(genericText);
      genericTraitBounds = parseGenericTraitBounds(genericText);
      cursor = genericEnd + 1;
    }
    while (/\s/.test(code[cursor] ?? "")) cursor++;
    if (code[cursor] !== "(") continue;
    const open = cursor;
    const close = findClosing(code, open, "(", ")");
    if (close === undefined) continue;
    const parameters = splitTopLevel(text.slice(open + 1, close)).map(parseParameter).filter(isPresent);
    const post = close + 1;
    const body = findDeclarationBody(code, post);
    const end = body ? findClosing(code, body.open, "{", "}") : undefined;
    const declarationEnd = body ? (end ?? code.length) : findDeclarationEnd(code, post);
    const returnType = readReturnType(text, code, close + 1);
    const line = doc.positionAt(match.index).line;
    result.push({
      name,
      parameters,
      generics,
      genericTraitBounds,
      returnType,
      documentation: getDocstringAtLine(doc, line) ?? undefined,
      isPublic: isPublicDeclaration(code, match.index),
      start: match.index,
      bodyStart: body?.open,
      bodyEnd: end,
      declarationEnd,
    });
    declaration.lastIndex = close + 1;
  }
  return result;
}

function parseTypes(doc: TextDocument, text: string, code: string): TypeSymbol[] {
  const result: TypeSymbol[] = [];
  const declaration = /\b(form|pick|trait|type|opaque)\s+([A-Za-z_]\w*)/g;
  let match: RegExpExecArray | null;
  while ((match = declaration.exec(code))) {
    const kind = match[1] as TypeSymbol["kind"] | undefined;
    const name = match[2];
    if (!kind || !name || !isCodeAt(code, match.index)) continue;
    let cursor = declaration.lastIndex;
    while (/\s/.test(code[cursor] ?? "")) cursor++;
    let generics: string[] = [];
    if (code[cursor] === "<") {
      const genericEnd = findClosing(code, cursor, "<", ">");
      if (genericEnd !== undefined) {
        generics = parseGenericNames(text.slice(cursor + 1, genericEnd));
        cursor = genericEnd + 1;
      }
    }
    const body = kind === "form" || kind === "pick" || kind === "trait"
      ? findDeclarationBody(code, cursor)
      : undefined;
    const bodyEnd = body ? findClosing(code, body.open, "{", "}") : undefined;
    const declarationEnd = body ? (bodyEnd ?? code.length) : findDeclarationEnd(code, cursor);
    const line = doc.positionAt(match.index).line;
    const typeSymbol: TypeSymbol = {
      name,
      kind,
      generics,
      fields: [],
      variants: [],
      methods: [],
      documentation: getDocstringAtLine(doc, line) ?? undefined,
      isPublic: isPublicDeclaration(code, match.index),
      start: match.index,
      bodyStart: body?.open,
      bodyEnd,
      declarationEnd,
      aliasTarget: kind === "type" ? readTypeAliasTarget(text, code, cursor) : undefined,
    };
    if (body && bodyEnd !== undefined && kind === "form") {
      typeSymbol.fields = parseFields(doc, text, code, body.open + 1, bodyEnd);
    } else if (body && bodyEnd !== undefined && kind === "pick") {
      typeSymbol.variants = parseVariants(doc, text, code, body.open + 1, bodyEnd);
    }
    result.push(typeSymbol);
    if (bodyEnd !== undefined) declaration.lastIndex = bodyEnd + 1;
  }
  return result;
}

function parseFields(doc: TextDocument, text: string, code: string, start: number, end: number): FormFieldSymbol[] {
  const fields: FormFieldSymbol[] = [];
  const body = text.slice(start, end);
  const codeBody = code.slice(start, end);
  for (const segment of splitTopLevelFields(body, codeBody)) {
    const match = /^\s*([A-Za-z_]\w*)\s*:\s*(.+?)\s*$/.exec(segment.text);
    const name = match?.[1];
    const type = match?.[2]?.trim();
    if (!name || !type) continue;
    const offset = start + segment.start + segment.text.indexOf(name);
    fields.push({
      name,
      type,
      documentation: getDocstringAtLine(doc, doc.positionAt(offset).line) ?? undefined,
    });
  }
  return dedupeByName(fields);
}

function parseVariants(doc: TextDocument, text: string, code: string, start: number, end: number): PickVariantSymbol[] {
  const variants: PickVariantSymbol[] = [];
  const body = text.slice(start, end);
  const codeBody = code.slice(start, end);
  for (const segment of splitTopLevelFields(body, codeBody)) {
    const match = /^\s*([A-Za-z_]\w*)\s*(?:\(([\s\S]*)\))?\s*$/.exec(segment.text);
    const name = match?.[1];
    if (!name) continue;
    const payloadText = match[2] ?? "";
    const payload = payloadText.trim()
      ? splitTopLevel(payloadText).map((item) => item.trim()).filter(Boolean)
      : [];
    const offset = start + segment.start + segment.text.indexOf(name);
    variants.push({ name, payload, documentation: getDocstringAtLine(doc, doc.positionAt(offset).line) ?? undefined });
  }
  return dedupeByName(variants);
}

function parseVisibleBindings(
  text: string,
  code: string,
  cursorOffset: number,
  blocks: BraceBlock[],
  procedures: ProcedureSymbol[],
): LocalBinding[] {
  const activeBlocks = blocks.filter((block) => block.open < cursorOffset && cursorOffset <= block.close);
  const activeIds = new Set(activeBlocks.map((block) => block.open));
  const depthById = new Map(activeBlocks.map((block, depth) => [block.open, depth]));
  const candidates: LocalBinding[] = [];
  const declaration = /\b(let|const|static|global|share|make)\s+(?:mut\s+)?([A-Za-z_]\w*)(?:\s*:\s*([^=;,\n]+))?/g;
  let match: RegExpExecArray | null;
  while ((match = declaration.exec(code))) {
    const name = match[2];
    if (!name || match.index >= cursorOffset) continue;
    const block = innermostBlock(blocks, match.index);
    const blockId = block?.open ?? -1;
    if (block && !activeIds.has(blockId)) continue;
    const kind = match[1] === "const" || match[1] === "static" || match[1] === "global"
      ? CompletionItemKind.Constant
      : CompletionItemKind.Variable;
    const annotatedType = match[3]?.trim();
    const initializerType = inferInitializerType(text, code, declaration.lastIndex, name);
    candidates.push({
      name,
      type: annotatedType || initializerType,
      kind,
      detail: `${kind === CompletionItemKind.Constant ? "constante" : "liaison locale"}${annotatedType ? `: ${annotatedType}` : initializerType ? `: ${initializerType}` : ""}`,
      start: match.index,
    });
  }

  for (const procedure of procedures) {
    if (procedure.bodyStart === undefined || cursorOffset < procedure.bodyStart || (procedure.bodyEnd !== undefined && cursorOffset > procedure.bodyEnd)) continue;
    for (const parameter of procedure.parameters) {
      const type = parameter.type;
      candidates.push({
        name: parameter.name,
        type,
        kind: CompletionItemKind.Variable,
        detail: `paramètre${type ? `: ${type}` : ""}`,
        start: procedure.bodyStart + 1,
      });
    }
  }

  const best = new Map<string, LocalBinding & { depth: number }>();
  for (const candidate of candidates) {
    const block = innermostBlock(blocks, candidate.start);
    const depth = block ? depthById.get(block.open) ?? -1 : -1;
    const existing = best.get(candidate.name);
    if (!existing || depth > existing.depth || (depth === existing.depth && candidate.start >= existing.start)) {
      best.set(candidate.name, { ...candidate, depth });
    }
  }
  return Array.from(best.values()).sort((left, right) => right.depth - left.depth || right.start - left.start);
}

function parseVisibleGenericParameters(
  code: string,
  cursorOffset: number,
  procedures: ProcedureSymbol[],
  types: TypeSymbol[],
): string[] {
  const visible = new Set<string>();
  for (const procedure of procedures) {
    const scopeEnd = procedure.bodyEnd ?? procedure.declarationEnd;
    if (procedure.start <= cursorOffset && cursorOffset <= scopeEnd) {
      [...procedure.generics, ...(procedure.scopeGenerics ?? [])].forEach((generic) => visible.add(generic));
    }
  }
  for (const type of types) {
    const scopeEnd = type.bodyEnd ?? type.declarationEnd;
    if (type.start <= cursorOffset && cursorOffset <= scopeEnd) {
      type.generics.forEach((generic) => visible.add(generic));
    }
  }
  return Array.from(visible);
}

function attachImplementationMethods(text: string, code: string, procedures: ProcedureSymbol[]): void {
  const implementation = /\bimpl\b/g;
  let match: RegExpExecArray | null;
  while ((match = implementation.exec(code))) {
    const body = findDeclarationBody(code, implementation.lastIndex);
    if (!body) continue;
    const end = findClosing(code, body.open, "{", "}");
    if (end === undefined) continue;
    const header = text.slice(match.index, body.open);
    const forMatch = /\bfor\s+([A-Za-z_]\w*(?:\s*<[^>{}]*>)?)/.exec(header);
    const directType = /^\s*impl\s*(?:<[^>{}]*>\s*)?([A-Za-z_]\w*(?:\s*<[^>{}]*>)?)/.exec(header)?.[1];
    const implementedTrait = /^\s*impl\s*(?:<[^>{}]*>\s*)?([A-Za-z_]\w*(?:\s*<[^>{}]*>)?)\s+for\b/.exec(header)?.[1]?.replace(/\s*<.*$/, "");
    const typeExpression = forMatch?.[1] ?? directType;
    const ownerType = typeExpression?.replace(/\s*<.*$/, "").trim();
    const implGenerics = /\bimpl\s*<([^>{}]*)>/.exec(header)?.[1];
    const typeGenerics = /<([^>{}]*)>/.exec(typeExpression ?? "")?.[1];
    const scopeGenerics = [...parseGenericNames(implGenerics ?? ""), ...parseGenericNames(typeGenerics ?? "")];
    if (ownerType) {
      for (const procedure of procedures) {
        if (procedure.start < body.open || procedure.start > end) continue;
        procedure.ownerType = ownerType;
        procedure.implementedTrait = implementedTrait;
        procedure.scopeGenerics = Array.from(new Set([...(procedure.scopeGenerics ?? []), ...scopeGenerics]));
        procedure.isInstanceMethod = Boolean(implementedTrait);
      }
    }
    implementation.lastIndex = end + 1;
  }
}

function attachTraitMethods(types: TypeSymbol[], procedures: ProcedureSymbol[]): void {
  for (const type of types.filter((candidate) => candidate.kind === "trait" && candidate.bodyStart !== undefined && candidate.bodyEnd !== undefined)) {
    for (const procedure of procedures) {
      if (procedure.start < (type.bodyStart ?? 0) || procedure.start > (type.bodyEnd ?? -1)) continue;
      procedure.traitOwner = type.name;
      procedure.isInstanceMethod = true;
      type.methods.push(procedure);
    }
  }
}

function parseParameter(raw: string): ProcedureParameter | undefined {
  const value = raw.trim();
  if (!value) return undefined;
  const modeMatch = /^(mut|move|owned|borrow)\s+/.exec(value);
  const withoutMode = modeMatch ? value.slice(modeMatch[0].length) : value;
  const colon = findTopLevelCharacter(withoutMode, ":");
  const name = /[A-Za-z_]\w*$/.exec((colon < 0 ? withoutMode : withoutMode.slice(0, colon)).trim())?.[0];
  if (!name) return undefined;
  return { name, type: colon >= 0 ? withoutMode.slice(colon + 1).trim() : undefined, mode: modeMatch?.[1] };
}

function parseGenericNames(value: string): string[] {
  return splitTopLevel(value).map((part) => /^([A-Za-z_]\w*)/.exec(part.trim().replace(/^(?:const)\s+/, ""))?.[1]).filter(isPresent);
}

function parseGenericTraitBounds(value: string): Record<string, string[]> {
  const result: Record<string, string[]> = {};
  for (const part of splitTopLevel(value)) {
    const constraint = /^([A-Za-z_]\w*)\s*:\s*([\s\S]+)$/.exec(part.trim());
    if (!constraint?.[1] || !constraint[2] || /^const\s+/.test(part.trim())) continue;
    const bounds = constraint[2].split("+").map((bound) => bound.trim()).filter(Boolean);
    result[constraint[1]] = bounds;
  }
  return result;
}

function readReturnType(text: string, code: string, start: number): string | undefined {
  let cursor = start;
  while (/\s/.test(code[cursor] ?? "")) cursor++;
  if (!code.startsWith("->", cursor)) return undefined;
  cursor += 2;
  const typeStart = cursor;
  const stack: string[] = [];
  let angleDepth = 0;
  while (cursor < code.length) {
    const character = code[cursor];
    if (character === "(" || character === "[" || character === "{") {
      if (character === "{" && stack.length === 0 && angleDepth === 0) break;
      stack.push(character);
    } else if (character === ")" || character === "]" || character === "}") {
      if (stack.length === 0) break;
      stack.pop();
    } else if (stack.length === 0 && character === "<") angleDepth++;
    else if (stack.length === 0 && character === ">" && angleDepth > 0) angleDepth--;
    else if (stack.length === 0 && angleDepth === 0 && (character === ";" || character === "{")) break;
    cursor++;
  }
  return text.slice(typeStart, cursor).trim() || undefined;
}

function findDeclarationBody(code: string, start: number): { open: number } | undefined {
  for (let cursor = start; cursor < code.length; cursor++) {
    if (code[cursor] === "{") return { open: cursor };
    if (code[cursor] === ";") return undefined;
  }
  return undefined;
}

function findDeclarationEnd(code: string, start: number): number {
  for (let cursor = start; cursor < code.length; cursor++) {
    if (code[cursor] === ";") return cursor;
    if (code[cursor] === "\n" || code[cursor] === "\r") return cursor;
  }
  return code.length;
}

function readTypeAliasTarget(text: string, code: string, start: number): string | undefined {
  const equals = code.indexOf("=", start);
  if (equals < 0) return undefined;
  const end = findDeclarationEnd(code, equals + 1);
  return text.slice(equals + 1, end).trim() || undefined;
}

function resolveLocalTypeAliases(types: TypeSymbol[]): void {
  let pass = 0;
  let changed = true;
  while (changed && pass++ < types.length) {
    changed = false;
    for (const type of types) {
      if (!type.aliasTarget || type.fields.length > 0 || type.variants.length > 0) continue;
      const targetName = /^([A-Za-z_]\w*)/.exec(type.aliasTarget)?.[1];
      const target = types.find((candidate) => candidate.name === targetName && candidate !== type);
      if (!target) continue;
      type.fields = target.fields;
      type.variants = target.variants;
      type.methods = target.methods;
      changed = true;
    }
    if (!changed) break;
  }
}

function findBraceBlocks(code: string): BraceBlock[] {
  const blocks: BraceBlock[] = [];
  const stack: BraceBlock[] = [];
  for (let offset = 0; offset < code.length; offset++) {
    if (code[offset] === "{") {
      const parent = stack.length > 0 ? stack[stack.length - 1]?.open : undefined;
      const block: BraceBlock = { open: offset, close: code.length, parent };
      blocks.push(block);
      stack.push(block);
    } else if (code[offset] === "}" && stack.length > 0) {
      const block = stack.pop();
      if (block) block.close = offset;
    }
  }
  return blocks;
}

function innermostBlock(blocks: BraceBlock[], offset: number): BraceBlock | undefined {
  let best: BraceBlock | undefined;
  for (const block of blocks) {
    if (block.open < offset && offset <= block.close && (!best || block.open > best.open)) best = block;
  }
  return best;
}

function findClosing(code: string, open: number, left: string, right: string): number | undefined {
  let depth = 0;
  for (let index = open; index < code.length; index++) {
    if (code[index] === left) depth++;
    else if (code[index] === right && --depth === 0) return index;
  }
  return undefined;
}

function splitTopLevel(value: string): string[] {
  const parts: string[] = [];
  const stack: string[] = [];
  let angleDepth = 0;
  let start = 0;
  for (let index = 0; index < value.length; index++) {
    const character = value[index];
    if (character === "(" || character === "[" || character === "{") stack.push(character);
    else if (character === ")" || character === "]" || character === "}") stack.pop();
    else if (stack.length === 0 && character === "<") angleDepth++;
    else if (stack.length === 0 && character === ">" && angleDepth > 0) angleDepth--;
    else if (character === "," && stack.length === 0 && angleDepth === 0) {
      parts.push(value.slice(start, index));
      start = index + 1;
    }
  }
  if (start < value.length) parts.push(value.slice(start));
  return parts;
}

function splitTopLevelFields(text: string, code: string): { text: string; start: number }[] {
  const result: { text: string; start: number }[] = [];
  const stack: string[] = [];
  let angleDepth = 0;
  let start = 0;
  for (let index = 0; index < code.length; index++) {
    const character = code[index];
    if (character === "(" || character === "[" || character === "{") stack.push(character);
    else if (character === ")" || character === "]" || character === "}") stack.pop();
    else if (stack.length === 0 && character === "<") angleDepth++;
    else if (stack.length === 0 && character === ">" && angleDepth > 0) angleDepth--;
    else if ((character === "," || character === ";" || character === "\n" || character === "\r") && stack.length === 0 && angleDepth === 0) {
      result.push({ text: text.slice(start, index), start });
      start = index + 1;
    }
  }
  if (start < text.length) result.push({ text: text.slice(start), start });
  return result;
}

function findTopLevelCharacter(value: string, expected: string): number {
  const stack: string[] = [];
  let angleDepth = 0;
  for (let index = 0; index < value.length; index++) {
    const character = value[index];
    if (character === "(" || character === "[" || character === "{") stack.push(character);
    else if (character === ")" || character === "]" || character === "}") stack.pop();
    else if (stack.length === 0 && character === "<") angleDepth++;
    else if (stack.length === 0 && character === ">" && angleDepth > 0) angleDepth--;
    else if (character === expected && stack.length === 0 && angleDepth === 0) return index;
  }
  return -1;
}

function inferInitializerType(text: string, code: string, afterDeclaration: number, name: string): string | undefined {
  const lineEnd = code.indexOf("\n", afterDeclaration);
  const end = lineEnd < 0 ? code.length : lineEnd;
  const line = text.slice(afterDeclaration, end);
  const equals = line.indexOf("=");
  if (equals < 0) return undefined;
  const value = line.slice(equals + 1).trim();
  if (/^(?:true|false)\b/.test(value)) return "bool";
  if (value.startsWith("\"") || value.startsWith("'")) return value.startsWith("'") ? "char" : "string";
  if (/^-?(?:0[xX][0-9A-Fa-f_]+|0[bB][01_]+|0[oO][0-7_]+|[0-9][0-9_]*(?:\.[0-9][0-9_]*)?(?:[eE][+-]?[0-9_]+)?)\b/.test(value)) return "int";
  const constructor = /^([A-Za-z_]\w*)\s*(?:\{|::)/.exec(value);
  if (constructor?.[1]) return constructor[1];
  if (/^proc\s*\(/.test(value)) return value.slice(0, 80);
  void name;
  return undefined;
}

function isPublicDeclaration(code: string, start: number): boolean {
  const boundary = Math.max(code.lastIndexOf(";", start), code.lastIndexOf("}", start), code.lastIndexOf("{", start), code.lastIndexOf("\n", start)) + 1;
  return /\b(?:pub|export)(?:\s*\([^)]*\))?\s*$/.test(code.slice(boundary, start)) || /\b(?:pub|export)(?:\s*\([^)]*\))?\s+(?:async\s+|unsafe\s+|comptime\s+)*$/.test(code.slice(boundary, start));
}

function dedupeByName<T extends { name: string }>(values: T[]): T[] {
  const seen = new Set<string>();
  return values.filter((value) => {
    if (seen.has(value.name)) return false;
    seen.add(value.name);
    return true;
  });
}

function isCodeAt(code: string, offset: number): boolean {
  return code[offset] !== " ";
}

function isPresent<T>(value: T | undefined): value is T {
  return value !== undefined;
}
