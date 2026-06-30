/**
 * Autorité locale sur le vocabulaire Vitte.
 * Aligné sur editors/grammar/vitte_highlight_grammar.json du dépôt principal.
 */

export const KEYWORDS = [
  "space",
  "pull",
  "use",
  "export",
  "share",
  "form",
  "field",
  "pick",
  "case",
  "trait",
  "type",
  "const",
  "macro",
  "proc",
  "entry",
  "at",
  "asm",
  "unsafe",
  "match",
  "let",
  "make",
  "set",
  "give",
  "emit",
  "if",
  "else",
  "otherwise",
  "select",
  "when",
  "is",
  "loop",
  "for",
  "in",
  "break",
  "continue",
  "return",
  "not",
  "and",
  "or",
  "as",
  "all",
] as const;
export type Keyword = typeof KEYWORDS[number];

export const DECL_KEYWORDS = [
  "proc",
  "form",
  "pick",
  "trait",
  "entry",
  "macro",
  "type",
  "field",
  "case",
  "export",
] as const;

export const CONTEXTUAL_KEYWORDS = [] as const;
export type ContextualKeyword = typeof CONTEXTUAL_KEYWORDS[number];

export const BOOL_LITERALS = ["true", "false"] as const;
export type BoolLiteral = typeof BOOL_LITERALS[number];

export const NIL_LITERALS = [] as const;
export type NilLiteral = typeof NIL_LITERALS[number];

export const PRIMITIVE_TYPES = ["bool", "string", "int"] as const;
export type PrimitiveType = typeof PRIMITIVE_TYPES[number];

export const BUILTIN_FUNCTIONS = [
  "print",
  "println",
  "len",
  "size",
  "assert",
  "panic",
  "range",
  "map",
  "filter",
  "reduce",
  "open",
  "read",
  "write",
  "close",
  "slice",
  "as_bytes",
  "push",
  "pop",
  "clear",
  "iter",
] as const;

export const OPERATORS = [
  "=",
  "+",
  "-",
  "*",
  "/",
  "%",
  "==",
  "!=",
  "<",
  "<=",
  ">",
  ">=",
  "->",
  "=>",
  ".",
  ",",
  ":",
  ";",
] as const;

export const BRACKETS = ["(", ")", "[", "]", "{", "}"] as const;

export const KEYWORD_SET: ReadonlySet<string> = new Set(KEYWORDS);
export const DECL_KEYWORD_SET: ReadonlySet<string> = new Set(DECL_KEYWORDS);
export const CONTEXTUAL_KEYWORD_SET: ReadonlySet<string> = new Set(CONTEXTUAL_KEYWORDS);
export const BOOL_LITERAL_SET: ReadonlySet<string> = new Set(BOOL_LITERALS);
export const NIL_LITERAL_SET: ReadonlySet<string> = new Set(NIL_LITERALS);
export const PRIMITIVE_TYPE_SET: ReadonlySet<string> = new Set(PRIMITIVE_TYPES);
export const BUILTIN_FUNCTION_SET: ReadonlySet<string> = new Set(BUILTIN_FUNCTIONS);
export const OPERATOR_SET: ReadonlySet<string> = new Set(OPERATORS);
export const BRACKET_SET: ReadonlySet<string> = new Set(BRACKETS);

export const RESERVED_WORDS: ReadonlySet<string> = new Set([
  ...KEYWORDS,
  ...BOOL_LITERALS,
  ...PRIMITIVE_TYPES,
]);

export function isIdentifierStart(ch: string): boolean {
  return /[A-Za-z_]/.test(ch);
}

export function isIdentifierPart(ch: string): boolean {
  return /[A-Za-z0-9_]/.test(ch);
}

export function isKeyword(value: string): boolean {
  return KEYWORD_SET.has(value);
}

export function isContextualKeyword(value: string): boolean {
  return CONTEXTUAL_KEYWORD_SET.has(value);
}

export function isBoolLiteral(value: string): boolean {
  return BOOL_LITERAL_SET.has(value);
}

export function isNilLiteral(value: string): boolean {
  return NIL_LITERAL_SET.has(value);
}

export function isPrimitiveType(value: string): boolean {
  return PRIMITIVE_TYPE_SET.has(value);
}

export function isReserved(value: string): boolean {
  return RESERVED_WORDS.has(value);
}

export type TokenKind =
  | "keyword"
  | "contextualKeyword"
  | "type"
  | "bool"
  | "operator"
  | "bracket"
  | "identifier";

export function classifyWord(value: string): TokenKind {
  if (isKeyword(value)) return "keyword";
  if (isContextualKeyword(value)) return "contextualKeyword";
  if (isPrimitiveType(value)) return "type";
  if (isBoolLiteral(value)) return "bool";
  if (OPERATOR_SET.has(value)) return "operator";
  if (BRACKET_SET.has(value)) return "bracket";
  return "identifier";
}

export function scanIdentifierAt(text: string, offset: number): { start: number; end: number } | null {
  if (offset < 0 || offset >= text.length) return null;
  let s = offset;
  let e = offset;
  while (s > 0 && isIdentifierPart(text[s - 1])) s--;
  while (e < text.length && isIdentifierPart(text[e])) e++;
  if (s === e || !isIdentifierStart(text[s])) return null;
  return { start: s, end: e };
}

export function suggestCompletions(prefix: string): string[] {
  const p = prefix || "";
  const startWith = (s: string) => s.startsWith(p);
  const kw = KEYWORDS.filter(startWith);
  const ty = PRIMITIVE_TYPES.filter(startWith);
  const lit = [...BOOL_LITERALS].filter(startWith);
  return [...kw, ...ty, ...lit];
}

export const LanguageFacts = {
  KEYWORDS,
  DECL_KEYWORDS,
  CONTEXTUAL_KEYWORDS,
  BOOL_LITERALS,
  NIL_LITERALS,
  PRIMITIVE_TYPES,
  BUILTIN_FUNCTIONS,
  OPERATORS,
  BRACKETS,
  isKeyword,
  isContextualKeyword,
  isBoolLiteral,
  isNilLiteral,
  isPrimitiveType,
  isReserved,
  classifyWord,
  suggestCompletions,
  scanIdentifierAt,
} as const;

export default LanguageFacts;
