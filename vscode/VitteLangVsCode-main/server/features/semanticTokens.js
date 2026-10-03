"use strict";
/**
 * Vitte Language Server — Semantic Tokens Feature
 * ------------------------------------------------
 * Provides a minimal, deterministic tokenizer for Vitte source files.
 * No LSP side effects — pure computation only.
 *
 * Supported token types:
 * - keyword
 * - string
 * - number
 * - function
 * - variable
 * - type
 * - comment
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.legend = void 0;
exports.tokenize = tokenize;
exports.emptyTokens = emptyTokens;
// Legend kept stable for client expectations.
exports.legend = {
    tokenTypes: [
        'namespace', // 0
        'type', // 1
        'class', // 2
        'enum', // 3
        'interface', // 4
        'struct', // 5
        'typeParameter', // 6
        'parameter', // 7
        'variable', // 8
        'property', // 9
        'enumMember', // 10
        'event', // 11
        'function', // 12
        'method', // 13
        'macro', // 14
        'keyword', // 15
        'modifier', // 16
        'comment', // 17
        'string', // 18
        'number', // 19
        'regexp', // 20
        'operator' // 21
    ],
    tokenModifiers: [
        'declaration', // 0 -> 1<<0
        'definition', // 1
        'readonly', // 2
        'static', // 3
        'deprecated', // 4
        'abstract', // 5
        'async', // 6
        'modification', // 7
        'documentation', // 8
        'defaultLibrary' // 9
    ]
};
// Fast index lookup.
const TI = Object.fromEntries(exports.legend.tokenTypes.map((t, i) => [t, i]));
const TM = Object.fromEntries(exports.legend.tokenModifiers.map((t, i) => [t, 1 << i]));
const KEYWORDS = new Set([
    'fn', 'struct', 'enum', 'trait', 'impl', 'let', 'const', 'mut', 'pub', 'use', 'mod', 'return', 'if', 'else', 'while', 'for', 'match',
    'break', 'continue', 'async', 'await', 'yield', 'where', 'type', 'alias', 'true', 'false', 'in', 'of'
]);
// Simple operator / punct set (we may ignore emitting tokens for these for now).
const OPERATORS = new Set(['=>', '->', '==', '!=', '<=', '>=', '&&', '||', '::', ':=', '+=', '-=', '*=', '/=', '%=',
    '+', '-', '*', '/', '%', '<', '>', '=', '!', '&', '|', '^', '~', ':', '.', ';', ',', '(', ')', '[', ']', '{', '}']);
/** Encodes one token into the semantic tokens `data` using delta encoding. */
function pushToken(data, state, line, start, length, tokenType, modifiers = 0) {
    const deltaLine = line - state.prevLine;
    const deltaStart = deltaLine === 0 ? start - state.prevChar : start;
    data.push(deltaLine, deltaStart, Math.max(0, length), tokenType, modifiers);
    state.prevLine = line;
    state.prevChar = start;
}
/** Classifies an identifier using its immediate syntactic context. */
function classifyIdent(ident, prevSigToken, nextSigChar) {
    // Keywords.
    if (KEYWORDS.has(ident)) {
        return { type: TI['keyword'], mods: 0 };
    }
    // Contextual declarations: name after fn/struct/enum/trait/type/let/const.
    if (prevSigToken === 'fn') {
        return { type: TI['function'], mods: TM['declaration'] };
    }
    if (prevSigToken === 'struct' || prevSigToken === 'enum' || prevSigToken === 'trait' || prevSigToken === 'type') {
        return { type: TI['type'], mods: TM['declaration'] };
    }
    if (prevSigToken === 'let' || prevSigToken === 'const') {
        let mods = TM['declaration'];
        if (prevSigToken === 'const')
            mods |= TM['readonly'];
        return { type: TI['variable'], mods };
    }
    // Heuristic: UpperCamel → type-ish.
    if (/^[A-Z][A-Za-z0-9_]*$/.test(ident)) {
        return { type: TI['type'], mods: 0 };
    }
    // Function hint: next significant char is '('.
    if (nextSigChar === '(') {
        return { type: TI['function'], mods: 0 };
    }
    // Default: variable/identifier.
    return { type: TI['variable'], mods: 0 };
}
/** Returns the next non‑space char from a given index in a string, if any. */
function nextNonSpace(s, from) {
    for (let i = from; i < s.length; i++) {
        const c = s[i];
        if (c !== ' ' && c !== '\t')
            return c;
    }
    return null;
}
/** Tokenizes the whole document into `SemanticTokens`. */
function tokenize(text) {
    const data = [];
    const lines = text.split(/\r?\n/);
    const state = { prevLine: 0, prevChar: 0 };
    let inBlockComment = false;
    for (let lineNo = 0; lineNo < lines.length; lineNo++) {
        const line = lines[lineNo];
        let i = 0;
        // Track last significant keyword for contextual declarations on this line.
        let lastSigKeyword = null;
        let pendingAsync = false; // Marks async before fn.
        const L = line.length;
        while (i < L) {
            // Handle multiline block comments.
            if (inBlockComment) {
                const end = line.indexOf('*/', i);
                if (end === -1) {
                    pushToken(data, state, lineNo, i, L - i, TI['comment']);
                    i = L;
                    continue;
                }
                else {
                    pushToken(data, state, lineNo, i, end + 2 - i, TI['comment']);
                    i = end + 2;
                    inBlockComment = false;
                    continue;
                }
            }
            const ch = line[i];
            // Whitespace.
            if (ch === ' ' || ch === '\t') {
                i++;
                continue;
            }
            // Line comment `//`.
            if (ch === '/' && i + 1 < L && line[i + 1] === '/') {
                pushToken(data, state, lineNo, i, L - i, TI['comment']);
                break; // Rest of the line is a comment.
            }
            // Block comment start `/*`.
            if (ch === '/' && i + 1 < L && line[i + 1] === '*') {
                const end = line.indexOf('*/', i + 2);
                if (end === -1) {
                    // Continues to next lines.
                    pushToken(data, state, lineNo, i, L - i, TI['comment']);
                    inBlockComment = true;
                    break;
                }
                else {
                    pushToken(data, state, lineNo, i, end + 2 - i, TI['comment']);
                    i = end + 2;
                    continue;
                }
            }
            // String literal starting with ". Support escapes, single line.
            if (ch === '"') {
                let j = i + 1;
                while (j < L) {
                    if (line[j] === '\\') {
                        j += 2;
                        continue;
                    }
                    if (line[j] === '"') {
                        j++;
                        break;
                    }
                    j++;
                }
                pushToken(data, state, lineNo, i, Math.max(1, j - i), TI['string']);
                i = j;
                continue;
            }
            // Number literal.
            if (/[0-9]/.test(ch)) {
                let j = i + 1;
                if (ch === '0' && j < L && /[xob]/i.test(line[j])) {
                    // 0x, 0o, 0b
                    j++;
                    while (j < L && /[0-9A-Fa-f_]/.test(line[j]))
                        j++;
                }
                else {
                    while (j < L && /[0-9_]/.test(line[j]))
                        j++;
                    if (j < L && line[j] === '.') {
                        j++;
                        while (j < L && /[0-9_]/.test(line[j]))
                            j++;
                    }
                    if (j < L && /[eE]/.test(line[j])) {
                        j++;
                        if (j < L && /[+-]/.test(line[j]))
                            j++;
                        while (j < L && /[0-9_]/.test(line[j]))
                            j++;
                    }
                }
                pushToken(data, state, lineNo, i, j - i, TI['number']);
                i = j;
                continue;
            }
            // Identifier or keyword.
            if (/[A-Za-z_]/.test(ch)) {
                let j = i + 1;
                while (j < L && /[A-Za-z0-9_]/.test(line[j]))
                    j++;
                const ident = line.slice(i, j);
                // Track async before fn to add modifier to the function decl.
                if (ident === 'async') {
                    pendingAsync = true;
                    lastSigKeyword = 'async';
                }
                if (KEYWORDS.has(ident)) {
                    pushToken(data, state, lineNo, i, j - i, TI['keyword']);
                    // Maintain contextual keyword for upcoming declarations.
                    if (ident === 'fn' || ident === 'struct' || ident === 'enum' || ident === 'trait' || ident === 'type' || ident === 'let' || ident === 'const') {
                        lastSigKeyword = ident;
                    }
                    i = j;
                    continue;
                }
                // Contextual classification.
                const nextChar = nextNonSpace(line, j);
                const cls = classifyIdent(ident, lastSigKeyword, nextChar);
                let mods = cls.mods;
                if (mods & TM['declaration']) {
                    // If `async` precedes `fn`, mark async on the declaration token (function name).
                    if (lastSigKeyword === 'fn' && pendingAsync)
                        mods |= TM['async'];
                    // Reset after using declaration context.
                    lastSigKeyword = null;
                    pendingAsync = false;
                }
                pushToken(data, state, lineNo, i, j - i, cls.type, mods);
                i = j;
                continue;
            }
            // Operators/punct — we currently skip emitting them to keep noise low.
            // If needed, enable the following to colorize operators as well.
            /*
            let matchedOp: string | null = null;
            for (const op of OPERATORS) {
              if (line.startsWith(op, i)) { matchedOp = op; break; }
            }
            if (matchedOp) {
              pushToken(data, state, lineNo, i, matchedOp.length, TI['operator']);
              i += matchedOp.length;
              continue;
            }
            */
            // Fallback: unrecognized single char.
            i++;
        }
    }
    return { data };
}
/** Provides an empty SemanticTokens result (for fallback or invalid documents). */
function emptyTokens() {
    return { data: [] };
}
//# sourceMappingURL=semanticTokens.js.map