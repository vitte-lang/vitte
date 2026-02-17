/**
 * languageFacts.ts — Autorité centrale sur le vocabulaire Vitte.
 * Réunit les mots-clés, types, littéraux et symboles réservés pour aligner
 * complétion, lint, hover et semantic tokens.
 */
export declare const KEYWORDS: readonly ["space", "module", "import", "use", "export", "pub", "program", "service", "kernel", "driver", "tool", "scenario", "pipeline", "struct", "enum", "union", "typedef", "type", "form", "fn", "proc", "extern", "inline", "static", "let", "mut", "const", "as", "where", "if", "elif", "else", "while", "for", "in", "loop", "match", "break", "continue", "return", "give", "and", "or", "not", "mod", "field", "set", "say", "do", "ret", "when", "scn", "prog", "from", "to", "step"];
export type Keyword = typeof KEYWORDS[number];
export declare const CONTEXTUAL_KEYWORDS: readonly [];
export type ContextualKeyword = typeof CONTEXTUAL_KEYWORDS[number];
export declare const BOOL_LITERALS: readonly ["true", "false"];
export declare const NIL_LITERALS: readonly ["nil", "null"];
export type BoolLiteral = typeof BOOL_LITERALS[number];
export type NilLiteral = typeof NIL_LITERALS[number];
export declare const PRIMITIVE_TYPES: readonly ["bool", "char", "string", "str", "byte", "bytes", "int", "uint", "float", "double", "void", "any", "unknown", "Unit", "i8", "i16", "i32", "i64", "i128", "isize", "u8", "u16", "u32", "u64", "u128", "usize", "f16", "f32", "f64", "f128"];
export type PrimitiveType = typeof PRIMITIVE_TYPES[number];
export declare const OPERATORS: readonly ["=", "+", "-", "*", "/", "%", "==", "!=", "<", "<=", ">", ">=", "&&", "||", "!", "&", "|", "^", "~", "<<", ">>", "..", "..=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "->", "=>", "::", "?"];
export declare const BRACKETS: readonly ["(", ")", "[", "]", "{", "}"];
export declare const KEYWORD_SET: ReadonlySet<string>;
export declare const CONTEXTUAL_KEYWORD_SET: ReadonlySet<string>;
export declare const BOOL_LITERAL_SET: ReadonlySet<string>;
export declare const NIL_LITERAL_SET: ReadonlySet<string>;
export declare const PRIMITIVE_TYPE_SET: ReadonlySet<string>;
export declare const OPERATOR_SET: ReadonlySet<string>;
export declare const BRACKET_SET: ReadonlySet<string>;
export declare const RESERVED_WORDS: ReadonlySet<string>;
export declare function isIdentifierStart(ch: string): boolean;
export declare function isIdentifierPart(ch: string): boolean;
export declare function isKeyword(value: string): boolean;
export declare function isContextualKeyword(value: string): boolean;
export declare function isBoolLiteral(value: string): boolean;
export declare function isNilLiteral(value: string): boolean;
export declare function isPrimitiveType(value: string): boolean;
export declare function isReserved(value: string): boolean;
export type TokenKind = "keyword" | "contextualKeyword" | "type" | "bool" | "nil" | "operator" | "bracket" | "identifier";
/** 🟩 Retourne une classification basique pour un mot/lexème. */
export declare function classifyWord(value: string): TokenKind;
export declare function scanIdentifierAt(text: string, offset: number): {
    start: number;
    end: number;
} | null;
export declare function suggestCompletions(prefix: string): string[];
export declare const LanguageFacts: {
    readonly KEYWORDS: readonly ["space", "module", "import", "use", "export", "pub", "program", "service", "kernel", "driver", "tool", "scenario", "pipeline", "struct", "enum", "union", "typedef", "type", "form", "fn", "proc", "extern", "inline", "static", "let", "mut", "const", "as", "where", "if", "elif", "else", "while", "for", "in", "loop", "match", "break", "continue", "return", "give", "and", "or", "not", "mod", "field", "set", "say", "do", "ret", "when", "scn", "prog", "from", "to", "step"];
    readonly CONTEXTUAL_KEYWORDS: readonly [];
    readonly BOOL_LITERALS: readonly ["true", "false"];
    readonly NIL_LITERALS: readonly ["nil", "null"];
    readonly PRIMITIVE_TYPES: readonly ["bool", "char", "string", "str", "byte", "bytes", "int", "uint", "float", "double", "void", "any", "unknown", "Unit", "i8", "i16", "i32", "i64", "i128", "isize", "u8", "u16", "u32", "u64", "u128", "usize", "f16", "f32", "f64", "f128"];
    readonly OPERATORS: readonly ["=", "+", "-", "*", "/", "%", "==", "!=", "<", "<=", ">", ">=", "&&", "||", "!", "&", "|", "^", "~", "<<", ">>", "..", "..=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "->", "=>", "::", "?"];
    readonly BRACKETS: readonly ["(", ")", "[", "]", "{", "}"];
    readonly isKeyword: typeof isKeyword;
    readonly isContextualKeyword: typeof isContextualKeyword;
    readonly isBoolLiteral: typeof isBoolLiteral;
    readonly isNilLiteral: typeof isNilLiteral;
    readonly isPrimitiveType: typeof isPrimitiveType;
    readonly isReserved: typeof isReserved;
    readonly classifyWord: typeof classifyWord;
    readonly suggestCompletions: typeof suggestCompletions;
    readonly scanIdentifierAt: typeof scanIdentifierAt;
};
export default LanguageFacts;
