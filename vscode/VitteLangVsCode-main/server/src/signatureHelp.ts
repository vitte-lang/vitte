import { MarkupKind } from "vscode-languageserver/node";
import type {
  ParameterInformation,
  Position,
  SignatureHelp,
  SignatureInformation,
} from "vscode-languageserver/node";
import type { TextDocument } from "vscode-languageserver-textdocument";
import { maskNonCode } from "./lexical.js";
import { parseCompletionModel, type ProcedureParameter, type ProcedureSymbol } from "./completionModel.js";
import { importedCompletionModel, readModuleExports, type ImportedCompletionSymbol, type CompletionEnvironment } from "./moduleResolver.js";
import { COMPILER_BUILTINS } from "./callableFacts.js";

export function provideSignatureHelp(
  doc: TextDocument,
  position: Position,
  importedSymbols: readonly ImportedCompletionSymbol[] = [],
): SignatureHelp | undefined {
  const text = doc.getText();
  const cursor = doc.offsetAt(position);
  const code = maskNonCode(text);
  const constructorHelp = formInitializerHelp(doc, code, cursor, importedSymbols);
  if (constructorHelp) return constructorHelp;

  const model = parseCompletionModel(doc, cursor);
  for (const openParen of findActiveCallParens(code, cursor)) {
    const callPrefix = code.slice(0, openParen);
    if (/\b(?:proc|intrinsic)\s+[A-Za-z_]\w*(?:\s*<[^{}]*>)?\s*$/.test(callPrefix)) continue;
    const normalizedPrefix = stripExplicitGenericArguments(callPrefix.trim());
    const callMatch = /([A-Za-z_]\w*(?:(?:::|\.)[A-Za-z_]\w*)*)\s*$/.exec(normalizedPrefix);
    const qualifiedName = callMatch?.[1];
    const calledName = qualifiedName?.split(/::|\./).pop();
    if (!calledName) continue;

    const methodCall = qualifiedName?.includes(".") ? qualifiedName.slice(0, qualifiedName.lastIndexOf(".")) : undefined;
    const rawReceiverType = model.bindings.find((candidate) => candidate.name === methodCall)?.type?.match(/[A-Za-z_]\w*/)?.[0];
    const receiverType = resolveReceiverTypeName(rawReceiverType, model.types, importedSymbols);
    const isModuleQualified = Boolean(qualifiedName?.includes("::"));
    const isQualified = Boolean(isModuleQualified || methodCall);
    const scopeQualifier = isModuleQualified ? qualifiedName?.slice(0, qualifiedName.lastIndexOf("::")) : undefined;
    const isTypeQualified = scopeQualifier !== undefined && [
      ...importedSymbols.filter((symbol) => symbol.name === scopeQualifier && symbol.type),
      ...model.types.filter((type) => type.name === scopeQualifier),
    ].length > 0;
    const procedureMatchesScope = (procedure: ProcedureSymbol): boolean => {
      if (methodCall || isTypeQualified) return false;
      if (isModuleQualified) return procedure.ownerType === undefined && procedure.traitOwner === undefined;
      return procedure.ownerType === undefined && procedure.traitOwner === undefined;
    };
    const procedures: ProcedureSymbol[] = [
      ...model.procedures.filter((procedure) => procedure.name === calledName && procedureMatchesScope(procedure)),
      ...importedSymbols.flatMap((symbol) => symbol.procedure && symbol.name === calledName && procedureMatchesScope(symbol.procedure) ? [{ ...symbol.procedure, name: symbol.name }] : []),
      ...visibleTraitMethods(model, importedSymbols, rawReceiverType, receiverType, cursor).filter((procedure) => procedure.name === calledName),
    ];
    const binding = model.bindings.find((candidate) => candidate.name === calledName && candidate.type?.startsWith("proc"));
    if (procedures.length === 0 && binding?.type && !isQualified) {
      const callable = callableValueSignature(calledName, binding.type);
      if (callable) procedures.push(callable);
    }
    const builtin = COMPILER_BUILTINS.find((candidate) => candidate.name === calledName);
    if (procedures.length === 0 && builtin && !isQualified) {
      procedures.push({
        name: builtin.name,
        parameters: builtin.parameters.map(parseParameterLabel),
        generics: [],
        returnType: builtin.returnType,
        documentation: builtin.documentation,
        isPublic: true,
        start: 0,
        declarationEnd: 0,
      });
    }
    if (procedures.length === 0) continue;

    const activeParameter = countActiveParameter(code, openParen + 1, cursor);
    const signatures = procedures.map(signatureInformation);
    const firstCompatible = signatures.findIndex((signature) => (signature.parameters?.length ?? 0) > activeParameter);
    const activeSignature = firstCompatible >= 0 ? firstCompatible : 0;
    const activeParameters = signatures[activeSignature]?.parameters ?? [];
    const active = activeParameters.length > 0 ? Math.min(activeParameter, activeParameters.length - 1) : undefined;
    return { signatures, activeSignature, ...(active !== undefined ? { activeParameter: active } : {}) };
  }
  return undefined;
}

export async function provideSignatureHelpWithImports(
  doc: TextDocument,
  position: Position,
  environment: CompletionEnvironment,
): Promise<SignatureHelp | undefined> {
  const imports = await importedCompletionModel(doc, environment);
  const direct = provideSignatureHelp(doc, position, imports.symbols);
  if (direct) return direct;
  const code = maskNonCode(doc.getText());
  const cursor = doc.offsetAt(position);
  for (const openParen of findActiveCallParens(code, cursor)) {
    const prefix = stripExplicitGenericArguments(code.slice(0, openParen).trim());
    const callName = /([A-Za-z_]\w*(?:(?:::|\.)[A-Za-z_]\w*)*)\s*$/.exec(prefix)?.[1];
    if (!callName?.includes("::")) continue;
    const separator = callName.lastIndexOf("::");
    const qualifier = callName.slice(0, separator);
    const alias = imports.modules.find((candidate) => candidate.alias === qualifier || qualifier.startsWith(candidate.alias + "::"));
    const modulePath = alias ? alias.modulePath + qualifier.slice(alias.alias.length) : qualifier;
    const exports = await readModuleExports(modulePath, doc.uri, environment);
    const help = provideSignatureHelp(doc, position, [...imports.symbols, ...exports.symbols]);
    if (help) return help;
  }
  return undefined;
}

function signatureInformation(procedure: ProcedureSymbol): SignatureInformation {
  const generic = procedure.generics.length ? "<" + procedure.generics.join(", ") + ">" : "";
  const parameterLabels = procedure.parameters.map(parameterLabel);
  const label = "proc " + procedure.name + generic + "(" + parameterLabels.join(", ") + ")" + (procedure.returnType ? " -> " + procedure.returnType : "");
  const parameters: ParameterInformation[] = [];
  let offset = label.indexOf("(") + 1;
  for (let index = 0; index < procedure.parameters.length; index++) {
    const parameter = procedure.parameters[index];
    const text = parameterLabels[index] ?? parameter.name;
    const documentation = parameter.documentation ?? parameterDocumentation(procedure.documentation, parameter.name);
    parameters.push({
      label: [offset, offset + text.length],
      ...(documentation ? { documentation: { kind: MarkupKind.Markdown, value: documentation } } : {}),
    });
    offset += text.length + 2;
  }
  return {
    label,
    ...(procedure.documentation ? { documentation: { kind: MarkupKind.Markdown, value: procedure.documentation } } : {}),
    parameters,
  };
}

function parameterLabel(parameter: ProcedureParameter): string {
  return (parameter.mode ? parameter.mode + " " : "") + parameter.name + (parameter.type ? ": " + parameter.type : "");
}

function parameterDocumentation(docstring: string | undefined, name: string): string | undefined {
  if (!docstring) return undefined;
  const escaped = escapeRegex(name);
  const pattern = new RegExp("^\\s*(?:[-*]\\s*)?" + escaped + "\\s*:\\s*(.+)$", "m");
  return pattern.exec(docstring)?.[1]?.trim();
}

function callableValueSignature(name: string, type: string): ProcedureSymbol | undefined {
  const match = /^proc\s*\((.*)\)\s*(?:->\s*(.+))?$/.exec(type.trim());
  if (!match) return undefined;
  const parameters = splitTopLevel(match[1] ?? "").map((parameter, index) => {
    const part = parameter.trim();
    const colon = findTopLevelColon(part);
    if (colon >= 0) return { name: part.slice(0, colon).trim(), type: part.slice(colon + 1).trim() };
    return { name: "arg" + (index + 1), type: part };
  });
  return { name, parameters, generics: [], returnType: match[2]?.trim(), isPublic: false, start: 0, declarationEnd: 0 };
}

function resolveReceiverTypeName(
  name: string | undefined,
  types: ReturnType<typeof parseCompletionModel>["types"],
  importedSymbols: readonly ImportedCompletionSymbol[],
): string | undefined {
  let current = name;
  const seen = new Set<string>();
  while (current && !seen.has(current)) {
    seen.add(current);
    const local = types.find((type) => type.name === current);
    if (local?.aliasTarget) {
      current = /[A-Za-z_]\w*/.exec(local.aliasTarget)?.[0];
      continue;
    }
    const imported = importedSymbols.find((symbol) => symbol.name === current)?.type;
    if (imported) return imported.name;
    return current;
  }
  return current;
}

function visibleTraitMethods(
  model: ReturnType<typeof parseCompletionModel>,
  importedSymbols: readonly ImportedCompletionSymbol[],
  receiverType: string | undefined,
  ownerType: string | undefined,
  offset: number,
): ProcedureSymbol[] {
  const activeProcedure = model.procedures
    .filter((procedure) => procedure.bodyStart !== undefined && procedure.bodyStart <= offset && (procedure.bodyEnd === undefined || offset <= procedure.bodyEnd))
    .sort((left, right) => right.start - left.start)[0];
  const genericBounds = receiverType ? activeProcedure?.genericTraitBounds?.[receiverType] ?? [] : [];
  const traitDefinitions = [
    ...model.types.filter((type) => type.kind === "trait").map((type) => ({ visibleName: type.name, type })),
    ...importedSymbols.flatMap((symbol) => symbol.type?.kind === "trait" ? [{ visibleName: symbol.name, type: symbol.type }] : []),
  ];
  const implementations = [...model.procedures, ...importedSymbols.flatMap((symbol) => symbol.procedure ? [symbol.procedure] : [])];
  const methods = new Map<string, ProcedureSymbol>();
  for (const definition of traitDefinitions) {
    const constrained = genericBounds.includes(definition.visibleName) || genericBounds.includes(definition.type.name);
    const implemented = Boolean(ownerType && implementations.some((procedure) => procedure.ownerType === ownerType && procedure.implementedTrait === definition.type.name));
    if (!constrained && !implemented) continue;
    for (const method of definition.type.methods) methods.set(method.name, method);
  }
  return Array.from(methods.values());
}

function parseParameterLabel(value: string): ProcedureParameter {
  const colon = findTopLevelColon(value);
  return colon < 0 ? { name: value } : { name: value.slice(0, colon).trim(), type: value.slice(colon + 1).trim() };
}

function formInitializerHelp(
  doc: TextDocument,
  code: string,
  cursor: number,
  importedSymbols: readonly ImportedCompletionSymbol[],
): SignatureHelp | undefined {
  const open = findUnmatchedBrace(code, cursor);
  if (open === undefined) return undefined;
  const prefix = code.slice(0, open);
  const typeMatch = /([A-Za-z_]\w*)\s*$/.exec(prefix);
  const name = typeMatch?.[1];
  if (!name) return undefined;
  const model = parseCompletionModel(doc, cursor);
  const localType = model.types.find((candidate) => candidate.name === name);
  const type = localType ?? importedSymbols.find((symbol) => symbol.name === name)?.type;
  if (!type || !(type.kind === "form" || (type.kind === "type" && type.aliasTarget && type.fields.length > 0)) || type.fields.length === 0) return undefined;
  const fields = type.fields;
  if (fields.length === 0) return undefined;

  const beforeCursor = code.slice(open + 1, cursor);
  const valueField = /([A-Za-z_]\w*)\s*:\s*[^,]*$/.exec(beforeCursor)?.[1];
  const fieldName = /(?:^|,)\s*([A-Za-z_]\w*)\s*$/.exec(beforeCursor)?.[1];
  const selectedField = valueField ?? fieldName;
  const usedFields = new Set(Array.from(beforeCursor.matchAll(/\b([A-Za-z_]\w*)\s*:/g), (match) => match[1]).filter(isPresent));
  const firstMissing = fields.findIndex((field) => !usedFields.has(field.name));
  const activeField = selectedField ? fields.findIndex((field) => field.name === selectedField) : firstMissing;
  const label = type.name + " { " + fields.map((field) => field.name + ": " + field.type).join(", ") + " }";
  const parameters: ParameterInformation[] = [];
  let offset = label.indexOf("{") + 2;
  for (const field of fields) {
    const fieldLabel = field.name + ": " + field.type;
    parameters.push({
      label: [offset, offset + fieldLabel.length],
      ...(field.documentation ? { documentation: { kind: MarkupKind.Markdown, value: field.documentation } } : {}),
    });
    offset += fieldLabel.length + 2;
  }
  return {
    signatures: [{ label, ...(type.documentation ? { documentation: { kind: MarkupKind.Markdown, value: type.documentation } } : {}), parameters }],
    activeSignature: 0,
    activeParameter: Math.max(0, Math.min(activeField < 0 ? 0 : activeField, parameters.length - 1)),
  };
}

function findActiveCallParens(code: string, cursor: number): number[] {
  const opens: number[] = [];
  for (let index = 0; index < cursor && index < code.length; index++) {
    if (code[index] === "(") opens.push(index);
    else if (code[index] === ")" && opens.length > 0) opens.pop();
  }
  return opens.reverse();
}

function findUnmatchedBrace(code: string, cursor: number): number | undefined {
  const opens: number[] = [];
  for (let index = 0; index < cursor && index < code.length; index++) {
    if (code[index] === "{") opens.push(index);
    else if (code[index] === "}" && opens.length > 0) opens.pop();
  }
  return opens[opens.length - 1];
}

function countActiveParameter(code: string, start: number, end: number): number {
  const nested: string[] = [];
  let parameter = 0;
  for (let index = start; index < end; index++) {
    const character = code[index];
    if (character === "(" || character === "[" || character === "{") nested.push(character);
    else if (character === ")" || character === "]" || character === "}") {
      const expected = character === ")" ? "(" : character === "]" ? "[" : "{";
      if (nested[nested.length - 1] === expected) nested.pop();
    } else if (character === "," && nested.length === 0) parameter++;
  }
  return parameter;
}

function stripExplicitGenericArguments(prefix: string): string {
  const match = /^([\s\S]*?)([A-Za-z_]\w*)\s*<[^<>]*>$/.exec(prefix);
  return match ? (match[1] ?? "") + (match[2] ?? "") : prefix;
}

function splitTopLevel(value: string): string[] {
  const parts: string[] = [];
  const nested: string[] = [];
  let angleDepth = 0;
  let start = 0;
  for (let index = 0; index < value.length; index++) {
    const character = value[index];
    if (character === "(" || character === "[" || character === "{") nested.push(character);
    else if (character === ")" || character === "]" || character === "}") nested.pop();
    else if (nested.length === 0 && character === "<") angleDepth++;
    else if (nested.length === 0 && character === ">" && angleDepth > 0) angleDepth--;
    else if (character === "," && nested.length === 0 && angleDepth === 0) {
      parts.push(value.slice(start, index));
      start = index + 1;
    }
  }
  if (start < value.length) parts.push(value.slice(start));
  return parts;
}

function findTopLevelColon(value: string): number {
  let depth = 0;
  for (let index = 0; index < value.length; index++) {
    const character = value[index];
    if (character === "(" || character === "[" || character === "{" || character === "<") depth++;
    else if (character === ")" || character === "]" || character === "}" || character === ">") depth = Math.max(0, depth - 1);
    else if (character === ":" && depth === 0) return index;
  }
  return -1;
}

function escapeRegex(value: string): string {
  return value.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function isPresent<T>(value: T | undefined): value is T { return value !== undefined; }
