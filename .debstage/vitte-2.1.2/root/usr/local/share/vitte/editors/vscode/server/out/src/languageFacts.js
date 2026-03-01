"use strict";
/**
 * languageFacts.ts — Autorité centrale sur le vocabulaire Vitte.
 * Réunit les mots-clés, types, littéraux et symboles réservés pour aligner
 * complétion, lint, hover et semantic tokens.
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.LanguageFacts = exports.RESERVED_WORDS = exports.BRACKET_SET = exports.OPERATOR_SET = exports.PRIMITIVE_TYPE_SET = exports.NIL_LITERAL_SET = exports.BOOL_LITERAL_SET = exports.CONTEXTUAL_KEYWORD_SET = exports.KEYWORD_SET = exports.BRACKETS = exports.OPERATORS = exports.PRIMITIVE_TYPES = exports.NIL_LITERALS = exports.BOOL_LITERALS = exports.CONTEXTUAL_KEYWORDS = exports.KEYWORDS = void 0;
exports.isIdentifierStart = isIdentifierStart;
exports.isIdentifierPart = isIdentifierPart;
exports.isKeyword = isKeyword;
exports.isContextualKeyword = isContextualKeyword;
exports.isBoolLiteral = isBoolLiteral;
exports.isNilLiteral = isNilLiteral;
exports.isPrimitiveType = isPrimitiveType;
exports.isReserved = isReserved;
exports.classifyWord = classifyWord;
exports.scanIdentifierAt = scanIdentifierAt;
exports.suggestCompletions = suggestCompletions;
// 🟩 Mots-clés de base (réservés au niveau grammaire)
exports.KEYWORDS = [
    "space", "module", "import", "use", "export", "pub",
    "program", "service", "kernel", "driver", "tool", "scenario", "pipeline",
    "struct", "enum", "union", "typedef", "type", "form",
    "fn", "proc", "extern", "inline", "static",
    "let", "mut", "const",
    "as", "where",
    "if", "elif", "else", "while", "for", "in", "loop", "match",
    "break", "continue", "return", "give",
    "and", "or", "not",
    "mod", "field", "set", "say", "do", "ret", "when", "scn", "prog",
    "from", "to", "step",
];
// 🟩 Mots-clés contextuels (acceptés selon contexte — ne bloquent pas les idents)
exports.CONTEXTUAL_KEYWORDS = [];
// 🟩 Littéraux
exports.BOOL_LITERALS = ["true", "false"];
exports.NIL_LITERALS = ["nil", "null"];
// 🟩 Types primitifs (réservés côté type-lexer)
exports.PRIMITIVE_TYPES = [
    "bool", "char", "string", "str", "byte", "bytes",
    "int", "uint", "float", "double", "void", "any", "unknown", "Unit",
    "i8", "i16", "i32", "i64", "i128", "isize",
    "u8", "u16", "u32", "u64", "u128", "usize",
    "f16", "f32", "f64", "f128"
];
// 🟩 Opérateurs et symboles significatifs (pour tokenizer/semantic)
exports.OPERATORS = [
    "=", "+", "-", "*", "/", "%",
    "==", "!=", "<", "<=", ">", ">=",
    "&&", "||", "!",
    "&", "|", "^", "~",
    "<<", ">>", "..", "..=",
    "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
    "->", "=>", "::", "?",
];
exports.BRACKETS = ["(", ")", "[", "]", "{", "}"];
// 🟩 Ensembles (lookup O(1))
exports.KEYWORD_SET = new Set(exports.KEYWORDS);
exports.CONTEXTUAL_KEYWORD_SET = new Set(exports.CONTEXTUAL_KEYWORDS);
exports.BOOL_LITERAL_SET = new Set(exports.BOOL_LITERALS);
exports.NIL_LITERAL_SET = new Set(exports.NIL_LITERALS);
exports.PRIMITIVE_TYPE_SET = new Set(exports.PRIMITIVE_TYPES);
exports.OPERATOR_SET = new Set(exports.OPERATORS);
exports.BRACKET_SET = new Set(exports.BRACKETS);
// 🟩 Réservés (union globale pour filtres/complétion)
exports.RESERVED_WORDS = new Set([
    ...exports.KEYWORDS,
    ...exports.BOOL_LITERALS,
    ...exports.NIL_LITERALS,
    ...exports.PRIMITIVE_TYPES,
]);
// 🟩 Helpers d’identifiants (ASCII sûr ; un lexer Unicode pourra remplacer)
function isIdentifierStart(ch) {
    return /[A-Za-z_]/.test(ch);
}
function isIdentifierPart(ch) {
    return /[A-Za-z0-9_]/.test(ch);
}
// 🟩 Classification rapide
function isKeyword(value) {
    return exports.KEYWORD_SET.has(value);
}
function isContextualKeyword(value) {
    return exports.CONTEXTUAL_KEYWORD_SET.has(value);
}
function isBoolLiteral(value) {
    return exports.BOOL_LITERAL_SET.has(value);
}
function isNilLiteral(value) {
    return exports.NIL_LITERAL_SET.has(value);
}
function isPrimitiveType(value) {
    return exports.PRIMITIVE_TYPE_SET.has(value);
}
function isReserved(value) {
    return exports.RESERVED_WORDS.has(value);
}
/** 🟩 Retourne une classification basique pour un mot/lexème. */
function classifyWord(value) {
    if (isKeyword(value))
        return "keyword";
    if (isContextualKeyword(value))
        return "contextualKeyword";
    if (isPrimitiveType(value))
        return "type";
    if (isBoolLiteral(value))
        return "bool";
    if (isNilLiteral(value))
        return "nil";
    if (exports.OPERATOR_SET.has(value))
        return "operator";
    if (exports.BRACKET_SET.has(value))
        return "bracket";
    return "identifier";
}
// 🟩 Scan d’un identifiant depuis une position (utilitaire hover/rename)
function scanIdentifierAt(text, offset) {
    if (offset < 0 || offset >= text.length)
        return null;
    let s = offset;
    let e = offset;
    while (s > 0 && isIdentifierPart(text[s - 1]))
        s--;
    while (e < text.length && isIdentifierPart(text[e]))
        e++;
    if (s === e || !isIdentifierStart(text[s]))
        return null;
    return { start: s, end: e };
}
// 🟩 Suggestions de complétion simples (priorité: mots-clés > types > bool/nil)
function suggestCompletions(prefix) {
    const p = prefix || "";
    const startWith = (s) => s.startsWith(p);
    const kw = exports.KEYWORDS.filter(startWith);
    const ty = exports.PRIMITIVE_TYPES.filter(startWith);
    const lit = [...exports.BOOL_LITERALS, ...exports.NIL_LITERALS].filter(startWith);
    return [...kw, ...ty, ...lit];
}
// 🟩 Export compact des faits (pratique pour le client)
exports.LanguageFacts = {
    KEYWORDS: exports.KEYWORDS,
    CONTEXTUAL_KEYWORDS: exports.CONTEXTUAL_KEYWORDS,
    BOOL_LITERALS: exports.BOOL_LITERALS,
    NIL_LITERALS: exports.NIL_LITERALS,
    PRIMITIVE_TYPES: exports.PRIMITIVE_TYPES,
    OPERATORS: exports.OPERATORS,
    BRACKETS: exports.BRACKETS,
    isKeyword,
    isContextualKeyword,
    isBoolLiteral,
    isNilLiteral,
    isPrimitiveType,
    isReserved,
    classifyWord,
    suggestCompletions,
    scanIdentifierAt,
};
exports.default = exports.LanguageFacts;
//# sourceMappingURL=languageFacts.js.map