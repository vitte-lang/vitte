import { InlayHintKind } from "vscode-languageserver/node";
import type { InlayHint, Range as LspRange } from "vscode-languageserver/node";
import type { TextDocument } from "vscode-languageserver-textdocument";
import { maskNonCode, scanLexical } from "./lexical.js";
import { sourceIndex } from "./sourceSymbols.js";
import { importedCompletionModel, type CompletionEnvironment } from "./moduleResolver.js";
import { provideSignatureHelp } from "./signatureHelp.js";

export interface InlayHintPrefs {
  parameterHints: boolean;
  typeHints: boolean;
  returnHints: boolean;
  aliasHints: boolean;
}

export async function provideInlayHints(
  doc: TextDocument,
  range: LspRange,
  prefs: Partial<InlayHintPrefs> | undefined,
  environment: CompletionEnvironment,
): Promise<InlayHint[]> {
  const cfg: InlayHintPrefs = {
    parameterHints: prefs?.parameterHints ?? true,
    typeHints: prefs?.typeHints ?? true,
    returnHints: prefs?.returnHints ?? true,
    aliasHints: prefs?.aliasHints ?? true,
  };
  const text = doc.getText();
  const code = maskNonCode(text);
  const lexical = scanLexical(text);
  const start = doc.offsetAt(range.start);
  const end = doc.offsetAt(range.end);
  const hints: InlayHint[] = [];
  if (cfg.typeHints) addLiteralTypeHints(doc, text, code, start, end, hints);
  if (cfg.parameterHints) {
    const imports = await importedCompletionModel(doc, environment);
    addUniqueParameterHints(doc, text, code, lexical.mask, lexical.spans, start, end, imports.symbols, hints);
  }
  // Return and import alias inference is deliberately omitted: the active
  // frontend does not expose the inferred return/alias model needed to prove it.
  return hints.sort((left, right) => doc.offsetAt(left.position) - doc.offsetAt(right.position)).slice(0, 160);
}

function addLiteralTypeHints(
  doc: TextDocument,
  text: string,
  code: string,
  start: number,
  end: number,
  hints: InlayHint[],
): void {
  const types = sourceIndex(doc).symbols.filter((symbol) =>
    (symbol.role === "binding" || symbol.role === "global") && symbol.type && symbol.nameEnd >= start && symbol.nameEnd <= end,
  );
  for (const symbol of types) {
    const lineEndIndex = code.indexOf("\n", symbol.nameEnd);
    const lineEnd = lineEndIndex < 0 ? code.length : lineEndIndex;
    const declaration = code.slice(symbol.start, lineEnd);
    const escapedName = escapeRegExp(symbol.name);
    const match = new RegExp(`^(?:let|make|const|static|global|share)\\s+(?:mut\\s+)?${escapedName}\\s*=`).exec(declaration);
    if (!match) continue;
    const expressionStart = symbol.start + match[0].length + declaration.indexOf(match[0]);
    const parsed = literalAt(text, expressionStart, lineEnd);
    const inferred = parsed?.type;
    if (!inferred || inferred !== symbol.type) continue;
    hints.push({
      position: doc.positionAt(symbol.nameEnd),
      label: `: ${inferred}`,
      kind: InlayHintKind.Type,
      paddingLeft: true,
    });
  }
}

function addUniqueParameterHints(
  doc: TextDocument,
  text: string,
  code: string,
  mask: Uint8Array,
  spans: ReturnType<typeof scanLexical>["spans"],
  start: number,
  end: number,
  importedSymbols: Awaited<ReturnType<typeof importedCompletionModel>>["symbols"],
  hints: InlayHint[],
): void {
  const callOpen = /\b[A-Za-z_]\w*(?:(?:::|\.)[A-Za-z_]\w*)*\s*\(/g;
  let match: RegExpExecArray | null;
  let calls = 0;
  while ((match = callOpen.exec(code)) && calls++ < 120) {
    const open = match.index + match[0].lastIndexOf("(");
    if (!mask[open]) continue;
    const prefix = code.slice(Math.max(0, match.index - 24), match.index);
    if (/\b(?:proc|intrinsic|if|while|for|match|select|when)\s*$/.test(prefix)) continue;
    const segments = topLevelArguments(code, mask, open);
    const firstIndex = segments.findIndex((segment) => findArgumentStart(text, spans, segment.start, segment.end) < segment.end);
    if (firstIndex < 0) continue;
    const firstSegment = segments[firstIndex];
    if (!firstSegment) continue;
    const firstStart = findArgumentStart(text, spans, firstSegment.start, firstSegment.end);
    if (/^[A-Za-z_]\w*\s*=/.test(code.slice(firstStart, firstSegment.end))) continue;
    const help = provideSignatureHelp(doc, doc.positionAt(firstStart), importedSymbols);
    if (help?.signatures.length !== 1 || help.activeParameter === undefined) continue;
    const signature = help.signatures[0];
    if (!signature) continue;
    for (let segmentIndex = firstIndex; segmentIndex < segments.length; segmentIndex++) {
      const segment = segments[segmentIndex];
      if (!segment) continue;
      const argumentStart = findArgumentStart(text, spans, segment.start, segment.end);
      if (argumentStart >= segment.end || argumentStart < start || argumentStart > end) continue;
      if (/^[A-Za-z_]\w*\s*=/.test(code.slice(argumentStart, segment.end))) continue;
      const parameterIndex = help.activeParameter + segmentIndex - firstIndex;
      const parameter = signature.parameters?.[parameterIndex];
      if (!parameter) continue;
      const parameterLabel = typeof parameter.label === "string"
        ? parameter.label
        : signature.label.slice(parameter.label[0], parameter.label[1]);
      const name = parameterLabel.replace(/^(?:mut|in|out)\s+/, "").split(/\s*:/, 1)[0]?.trim();
      if (!name || !/^[A-Za-z_]\w*$/.test(name)) continue;
      hints.push({
        position: doc.positionAt(argumentStart),
        label: `${name}:`,
        kind: InlayHintKind.Parameter,
        paddingRight: true,
      });
    }
  }
}

function topLevelArguments(code: string, mask: Uint8Array, open: number): { start: number; end: number }[] {
  const result: { start: number; end: number }[] = [];
  let paren = 0, bracket = 0, brace = 0, argumentStart = open + 1;
  for (let index = open + 1; index < code.length; index++) {
    if (!mask[index]) continue;
    const char = code[index];
    if (char === "(" ) paren++;
    else if (char === ")") {
      if (paren === 0 && bracket === 0 && brace === 0) {
        if (index > argumentStart || result.length > 0) result.push({ start: argumentStart, end: index });
        return result;
      }
      paren = Math.max(0, paren - 1);
    } else if (char === "[") bracket++;
    else if (char === "]") bracket = Math.max(0, bracket - 1);
    else if (char === "{") brace++;
    else if (char === "}") brace = Math.max(0, brace - 1);
    else if (char === "," && paren === 0 && bracket === 0 && brace === 0) {
      result.push({ start: argumentStart, end: index });
      argumentStart = index + 1;
    }
  }
  if (argumentStart < code.length) result.push({ start: argumentStart, end: code.length });
  return result;
}

function findArgumentStart(text: string, spans: ReturnType<typeof scanLexical>["spans"], start: number, end: number): number {
  let offset = start;
  while (offset < end) {
    while (offset < end && /\s/.test(text[offset] ?? "")) offset++;
    const comment = spans.find((span) => span.kind === "comment" && span.start <= offset && offset < span.end);
    if (comment) { offset = comment.end; continue; }
    break;
  }
  return offset;
}

function literalAt(text: string, start: number, limit: number): { type: string; end: number } | undefined {
  let offset = start;
  while (offset < limit && /\s/.test(text[offset] ?? "")) offset++;
  const rest = text.slice(offset, limit);
  const quoted = /^(?:"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])')/.exec(rest);
  let type: string | undefined;
  let length = 0;
  if (quoted) {
    type = quoted[0].startsWith('"') ? "string" : "char";
    length = quoted[0].length;
  } else {
    const token = /^(?:true|false|(?:0[xX][0-9A-Fa-f_]+|0[bB][01_]+|0[oO][0-7_]+|\d[\d_]*)(?:\.\d[\d_]*)?(?:[eE][+-]?\d[\d_]*)?)/.exec(rest)?.[0];
    if (token) {
      type = token === "true" || token === "false" ? "bool" : token.includes(".") || /[eE]/.test(token) ? "float" : "int";
      length = token.length;
    }
  }
  if (!type) return undefined;
  let after = offset + length;
  while (after < limit && /\s/.test(text[after] ?? "")) after++;
  if (after < limit && text[after] !== ";") return undefined;
  return { type, end: offset + length };
}

function escapeRegExp(value: string): string {
  return value.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}
