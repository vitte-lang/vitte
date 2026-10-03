/** Canonical Vitte vocabulary mirrored from grammar/vitte.ebnf. */

export const KEYWORDS = [
  "space",
  "use",
  "export",
  "share",
  "const",
  "static",
  "global",
  "region",
  "type",
  "opaque",
  "extern",
  "form",
  "pick",
  "trait",
  "impl",
  "proc",
  "intrinsic",
  "compiler",
  "query",
  "pass",
  "backend",
  "diagnostic",
  "macro",
  "comptime",
  "static_assert",
  "test",
  "bench",
  "entry",
  "pub",
  "priv",
  "unsafe",
  "async",
  "inline",
  "noinline",
  "naked",
  "interrupt",
  "where",
  "effects",
  "requires",
  "operator",
  "noexcept",
  "as",
  "for",
  "at",
  "let",
  "set",
  "give",
  "try",
  "defer",
  "asm",
  "emit",
  "assert",
  "panic",
  "unreachable",
  "if",
  "elif",
  "else",
  "while",
  "loop",
  "in",
  "break",
  "continue",
  "select",
  "when",
  "match",
  "case",
  "with",
  "critical",
  "mut",
  "owned",
  "borrow",
  "move",
  "await",
  "not",
  "and",
  "or",
  "is",
  "self",
  "ref",
  "dyn",
  "sizeof",
  "alignof",
  "offsetof",
  "typeof",
  "nameof",
  "map",
  "resource",
  "volatile",
  "atomic",
  "user",
  "kernel",
  "phys",
  "mmio",
  "dma",
] as const;
export type Keyword = typeof KEYWORDS[number];

export const DECL_KEYWORDS = [
  "space",
  "use",
  "export",
  "share",
  "const",
  "static",
  "global",
  "region",
  "type",
  "opaque",
  "extern",
  "proc",
  "intrinsic",
  "form",
  "pick",
  "trait",
  "impl",
  "entry",
  "macro",
  "compiler",
  "query",
  "pass",
  "backend",
  "diagnostic",
  "comptime",
  "static_assert",
  "test",
  "bench",
] as const;

export const CONTEXTUAL_KEYWORDS = [
  "C",
  "sysv64",
  "win64",
  "package",
  "module",
  "super",
  "stack",
  "heap",
  "arena",
  "Self",
] as const;
export type ContextualKeyword = typeof CONTEXTUAL_KEYWORDS[number];

export const DEPRECATED_KEYWORDS = ["class", "union", "bits", "flags"] as const;

export const BOOL_LITERALS = ["true", "false"] as const;
export type BoolLiteral = typeof BOOL_LITERALS[number];

export const NIL_LITERALS = ["null"] as const;
export type NilLiteral = typeof NIL_LITERALS[number];

export const PRIMITIVE_TYPES = [
  "void", "never", "unit", "bool", "char", "rune", "str", "string", "bytes", "cstr", "int",
  "i8", "i16", "i32", "i64", "i128",
  "u8", "u16", "u32", "u64", "u128",
  "usize", "isize", "intptr", "uintptr",
  "f16", "f32", "f64", "f128",
  "c_char", "c_int", "c_uint", "c_long", "c_ulong", "c_void",
  "TokenId", "NodeId", "DefId", "HirId", "MirId", "TypeId", "SymbolId", "ScopeId", "BlockId", "ValueId", "InstrId",
] as const;
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
  "<<=", ">>=", "..=", "...", "??", "&&", "||", "==", "!=", "<=", ">=", "<<", ">>",
  "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "->", "=>", "::", "..",
  "=", "+", "-", "*", "/", "%", "<", ">", "&", "|", "^", "~", "?", ".", ",", ":", ";",
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
  ...DEPRECATED_KEYWORDS,
  ...BOOL_LITERALS,
  ...NIL_LITERALS,
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
  DEPRECATED_KEYWORDS,
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
