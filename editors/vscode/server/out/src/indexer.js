"use strict";
// indexer.ts — indexation robuste des symboles pour Vitte/Vit (LSP prêt, sans dépendance runtime)
Object.defineProperty(exports, "__esModule", { value: true });
exports.SK = void 0;
exports.indexDocument = indexDocument;
exports.indexText = indexText;
exports.updateDocument = updateDocument;
exports.removeDocument = removeDocument;
exports.clearIndex = clearIndex;
exports.getIndex = getIndex;
exports.getDocumentIndex = getDocumentIndex;
exports.searchWorkspaceSymbols = searchWorkspaceSymbols;
exports.toWorkspaceSymbols = toWorkspaceSymbols;
exports.toDocumentSymbols = toDocumentSymbols;
exports.symbolsAtPosition = symbolsAtPosition;
exports.findDefinition = findDefinition;
exports.findReferences = findReferences;
// Import minimal côté serveur LSP.
const vscode_languageserver_textdocument_1 = require("vscode-languageserver-textdocument");
/* ============================================================================
 * Types et constantes
 * ========================================================================== */
// Utiliser "enum" plutôt que "const enum" pour éviter les soucis de transpilation TS dans certains toolchains.
var SK;
(function (SK) {
    SK[SK["Module"] = 2] = "Module";
    SK[SK["Namespace"] = 3] = "Namespace";
    SK[SK["Class"] = 5] = "Class";
    SK[SK["Method"] = 6] = "Method";
    SK[SK["Property"] = 7] = "Property";
    SK[SK["Field"] = 8] = "Field";
    SK[SK["Constructor"] = 9] = "Constructor";
    SK[SK["Enum"] = 10] = "Enum";
    SK[SK["Interface"] = 11] = "Interface";
    SK[SK["Function"] = 12] = "Function";
    SK[SK["Variable"] = 13] = "Variable";
    SK[SK["Constant"] = 14] = "Constant";
    SK[SK["Struct"] = 23] = "Struct";
    SK[SK["TypeParameter"] = 26] = "TypeParameter";
})(SK || (exports.SK = SK = {}));
/** Index global: uri -> liste de symboles */
const INDEX = new Map();
const DECLARATIONS = new Map([
    ["module", { kind: SK.Namespace, expectBody: true, allowBodyless: true }],
    ["struct", { kind: SK.Struct, expectBody: true }],
    ["enum", { kind: SK.Enum, expectBody: true }],
    ["union", { kind: SK.Struct, expectBody: true }],
    ["type", { kind: SK.Interface, expectBody: false }],
    ["fn", { kind: SK.Function, expectBody: true, allowBodyless: true }],
    ["const", { kind: SK.Constant, expectBody: false }],
    ["static", { kind: SK.Variable, expectBody: false }],
]);
const MODIFIERS = new Set(["pub"]);
class SymbolParser {
    constructor(doc) {
        this.doc = doc;
        this.symbols = [];
        this.braceStack = [];
        this.pendingBodies = [];
        this.activeSymbols = [];
        this.pos = 0;
        this.text = doc.getText();
        this.masked = maskNonCode(this.text);
        this.length = this.masked.length;
    }
    parse() {
        while (this.pos < this.length) {
            const ch = this.masked[this.pos];
            if (isWhitespace(ch)) {
                this.pos++;
                continue;
            }
            if (ch === "#") {
                this.skipAttribute();
                continue;
            }
            if (ch === "{") {
                this.handleOpenBrace();
                this.pos++;
                continue;
            }
            if (ch === "}") {
                this.handleCloseBrace();
                this.pos++;
                continue;
            }
            if (ch === ";") {
                this.handleSemicolon();
                this.pos++;
                continue;
            }
            if (isIdentifierStart(ch)) {
                const keywordStart = this.pos;
                const word = this.readWord();
                if (!word) {
                    this.pos++;
                    continue;
                }
                const lower = word.toLowerCase();
                if (MODIFIERS.has(lower)) {
                    this.skipModifierTail(lower);
                    continue;
                }
                const decl = DECLARATIONS.get(lower);
                if (decl) {
                    this.handleDeclaration(lower, keywordStart, decl);
                    continue;
                }
                continue;
            }
            this.pos++;
        }
        return this.symbols;
    }
    handleDeclaration(keyword, startOffset, info) {
        const name = this.readSymbolName(keyword);
        if (!name)
            return;
        const startPos = this.doc.positionAt(startOffset);
        const sym = {
            name,
            kind: info.kind,
            uri: this.doc.uri,
            line: startPos.line,
            character: startPos.character,
        };
        const container = this.currentContainer();
        if (container) {
            sym.containerName = container.name;
        }
        const index = this.symbols.push(sym) - 1;
        if (info.expectBody) {
            const expectation = this.probeBodyOffset(info, this.pos);
            if (expectation.kind === "body") {
                this.pendingBodies.push({
                    symbolIndex: index,
                    allowBodyless: !!info.allowBodyless,
                    startOffset,
                    bodyOffset: expectation.offset,
                });
            }
            else if (expectation.kind === "unknown") {
                this.pendingBodies.push({
                    symbolIndex: index,
                    allowBodyless: !!info.allowBodyless,
                    startOffset,
                });
            }
        }
    }
    readSymbolName(keyword) {
        switch (keyword) {
            case "module":
                return this.readQualifiedName();
            case "struct":
            case "enum":
            case "union":
            case "type":
                this.skipTrivia();
                return this.readIdentifierToken();
            case "fn":
                this.skipTrivia();
                return this.readFunctionName();
            case "const":
            case "static":
                return this.readConstName();
            default:
                return undefined;
        }
    }
    readQualifiedName() {
        const segments = [];
        while (true) {
            this.skipTrivia();
            const segment = this.readIdentifierToken();
            if (!segment)
                break;
            segments.push(segment);
            const saved = this.pos;
            this.skipTrivia();
            if (this.masked[this.pos] === ":" && this.masked[this.pos + 1] === ":") {
                this.pos += 2;
                continue;
            }
            this.pos = saved;
            break;
        }
        return segments.length ? segments.join("::") : undefined;
    }
    readFunctionName() {
        return this.readIdentifierToken();
    }
    readConstName() {
        this.skipTrivia();
        return this.readIdentifierToken();
    }
    readIdentifierToken() {
        if (!isIdentifierStart(this.masked[this.pos]))
            return undefined;
        const start = this.pos;
        this.pos++;
        while (this.pos < this.length && isIdentifierPart(this.masked[this.pos])) {
            this.pos++;
        }
        const raw = this.text.slice(start, this.pos);
        const trimmed = raw.trim();
        return trimmed ? trimmed : undefined;
    }
    readWord() {
        const start = this.pos;
        while (this.pos < this.length && isIdentifierPart(this.masked[this.pos])) {
            this.pos++;
        }
        return this.masked.slice(start, this.pos);
    }
    skipModifierTail(word) {
        if (word === "pub") {
            this.skipTrivia();
            if (this.masked[this.pos] === "(") {
                this.pos = this.skipBalancedFrom(this.pos, "(", ")");
                this.skipTrivia();
            }
        }
    }
    skipBalancedFrom(index, open, close) {
        let depth = 0;
        let i = index;
        while (i < this.length) {
            const ch = this.masked[i];
            if (ch === open)
                depth++;
            else if (ch === close) {
                depth--;
                if (depth === 0)
                    return i + 1;
            }
            i++;
        }
        return this.length;
    }
    skipAttribute() {
        let i = this.pos + 1;
        if (i < this.length && this.masked[i] === "!") {
            i++;
        }
        while (i < this.length && isWhitespace(this.masked[i]))
            i++;
        if (i >= this.length || this.masked[i] !== "[") {
            this.pos++;
            return;
        }
        const end = this.skipBalancedFrom(i, "[", "]");
        this.pos = Math.max(end, i + 1);
    }
    skipTrivia() {
        while (this.pos < this.length && isWhitespace(this.masked[this.pos])) {
            this.pos++;
        }
    }
    currentContainer() {
        if (!this.activeSymbols.length)
            return undefined;
        return this.symbols[this.activeSymbols[this.activeSymbols.length - 1]];
    }
    handleOpenBrace() {
        const currentOffset = this.pos;
        let matchedIndex = -1;
        for (let i = this.pendingBodies.length - 1; i >= 0; i--) {
            const pending = this.pendingBodies[i];
            if (pending.bodyOffset !== undefined && pending.bodyOffset !== currentOffset) {
                continue;
            }
            matchedIndex = i;
            break;
        }
        let symbolIndex = null;
        if (matchedIndex >= 0) {
            const [pending] = this.pendingBodies.splice(matchedIndex, 1);
            symbolIndex = pending.symbolIndex;
            this.activeSymbols.push(symbolIndex);
        }
        this.braceStack.push({ symbolIndex, offset: currentOffset });
    }
    handleCloseBrace() {
        const frame = this.braceStack.pop();
        if (!frame)
            return;
        if (frame.symbolIndex != null) {
            const pos = this.doc.positionAt(this.pos);
            const sym = this.symbols[frame.symbolIndex];
            sym.endLine = pos.line;
            sym.endCharacter = pos.character + 1;
            const idx = this.activeSymbols.lastIndexOf(frame.symbolIndex);
            if (idx >= 0)
                this.activeSymbols.splice(idx, 1);
        }
    }
    handleSemicolon() {
        const current = this.pos;
        for (let i = this.pendingBodies.length - 1; i >= 0; i--) {
            const pending = this.pendingBodies[i];
            if (!pending.allowBodyless)
                continue;
            if (pending.bodyOffset !== undefined && pending.bodyOffset <= current)
                continue;
            this.pendingBodies.splice(i, 1);
            break;
        }
    }
    matchKeywordAt(pos, keyword) {
        if (pos + keyword.length > this.length)
            return false;
        const slice = this.masked.slice(pos, pos + keyword.length).toLowerCase();
        if (slice !== keyword)
            return false;
        const before = pos > 0 ? this.masked[pos - 1] : "";
        const after = pos + keyword.length < this.length ? this.masked[pos + keyword.length] : "";
        if ((before && isIdentifierPart(before)) || (after && isIdentifierPart(after))) {
            return false;
        }
        return true;
    }
    probeBodyOffset(info, start) {
        let i = start;
        let depthParen = 0;
        let depthAngle = 0;
        let depthBracket = 0;
        while (i < this.length) {
            const ch = this.masked[i];
            if (ch === "(") {
                depthParen++;
                i++;
                continue;
            }
            if (ch === ")") {
                if (depthParen > 0)
                    depthParen--;
                i++;
                continue;
            }
            if (ch === "<") {
                depthAngle++;
                i++;
                continue;
            }
            if (ch === ">") {
                if (depthAngle > 0)
                    depthAngle--;
                i++;
                continue;
            }
            if (ch === "[") {
                depthBracket++;
                i++;
                continue;
            }
            if (ch === "]") {
                if (depthBracket > 0)
                    depthBracket--;
                i++;
                continue;
            }
            if (isWhitespace(ch)) {
                i++;
                continue;
            }
            if (!depthParen && !depthAngle && !depthBracket) {
                if (ch === "{")
                    return { kind: "body", offset: i };
                if (ch === ";") {
                    return info.allowBodyless ? { kind: "none" } : { kind: "unknown" };
                }
                if (ch === "=") {
                    return { kind: "none" };
                }
            }
            i++;
        }
        return { kind: "unknown" };
    }
}
function extract(doc) {
    const parser = new SymbolParser(doc);
    const symbols = parser.parse();
    return dedupe(symbols, (s) => `${s.kind}:${s.name}:${s.line}:${s.character}`);
}
/* ============================================================================
 * API d’index
 * ========================================================================== */
/** Indexe un document (remplace l’entrée précédente). */
function indexDocument(doc) {
    INDEX.set(doc.uri, extract(doc));
}
/** Indexe une chaîne pour un uri donné. */
function indexText(uri, text) {
    const doc = vscode_languageserver_textdocument_1.TextDocument.create(uri, "vitte", 0, text);
    INDEX.set(uri, extract(doc));
}
/** Réindexe un document (alias plus explicite). */
function updateDocument(doc) {
    INDEX.set(doc.uri, extract(doc));
}
/** Supprime un document de l’index. */
function removeDocument(uri) {
    INDEX.delete(uri);
}
/** Vide l’index. */
function clearIndex() {
    INDEX.clear();
}
/** Récupère l’index brut (lecture seule). */
function getIndex() {
    return INDEX;
}
/** Récupère les symboles d’un document. */
function getDocumentIndex(uri) {
    return INDEX.get(uri) ?? [];
}
/* ============================================================================
 * Requêtes et conversions LSP
 * ========================================================================== */
/** Recherche globale par fuzzy + préfixe, tri par score et nature. */
function searchWorkspaceSymbols(query, limit = 400) {
    const q = query.trim().toLowerCase();
    const all = flattenIndex();
    if (!q)
        return all.slice(0, limit);
    const scored = all
        .map((s) => ({ s, score: fuzzyScore(s.name.toLowerCase(), q) }))
        .filter((x) => x.score > 0)
        .sort((a, b) => b.score - a.score ||
        rankKind(b.s.kind) - rankKind(a.s.kind) ||
        a.s.name.localeCompare(b.s.name))
        .slice(0, limit)
        .map((x) => x.s);
    return scored;
}
/** Conversion vers SymbolInformation[] (Workspace Symbols). */
function toWorkspaceSymbols(syms) {
    return syms.map((s) => ({
        name: s.name,
        kind: toLspKind(s.kind),
        location: {
            uri: s.uri,
            range: lspRange(s.line, s.character, s.endLine ?? s.line, s.endCharacter ?? s.character),
        },
        containerName: s.containerName,
    }));
}
/** Conversion hiérarchique DocumentSymbol[] pour un document. */
function toDocumentSymbols(doc) {
    const list = getDocumentIndex(doc.uri);
    // Simple regroupement par containerName. Les doublons de nom sont pris tels quels.
    const byName = new Map();
    const roots = [];
    for (const s of list) {
        const ds = {
            name: s.name,
            kind: toLspKind(s.kind),
            range: lspRange(s.line, s.character, s.endLine ?? s.line, s.endCharacter ?? s.character),
            selectionRange: lspRange(s.line, s.character, s.line, Math.max(s.character, s.character + s.name.length)),
            children: [],
        };
        if (s.containerName && byName.has(s.containerName)) {
            byName.get(s.containerName).children.push(ds);
        }
        else {
            roots.push(ds);
        }
        byName.set(s.name, ds);
    }
    return roots;
}
/** Renvoie les symboles à une position. */
function symbolsAtPosition(uri, pos) {
    const list = getDocumentIndex(uri);
    return list.filter((s) => inRange(pos, s));
}
/** Définition naïve: symbole portant le même nom, priorisant le même fichier. */
function findDefinition(uri, name) {
    const local = getDocumentIndex(uri).find((s) => s.name === name);
    if (local)
        return local;
    for (const [, list] of INDEX) {
        const hit = list.find((s) => s.name === name);
        if (hit)
            return hit;
    }
    return undefined;
}
/** Références naïves: symboles du même nom dans l’index. */
function findReferences(_uri, name, limit = 500) {
    const acc = [];
    for (const list of INDEX.values()) {
        for (const s of list) {
            if (s.name === name) {
                acc.push(s);
                if (acc.length >= limit)
                    return acc;
            }
        }
    }
    return acc;
}
/* ============================================================================
 * Utilitaires internes
 * ========================================================================== */
function inRange(pos, s) {
    const startOk = pos.line > s.line || (pos.line === s.line && pos.character >= s.character);
    const endLine = s.endLine ?? s.line;
    const endChar = s.endCharacter ?? s.character;
    const endOk = pos.line < endLine || (pos.line === endLine && pos.character <= endChar);
    return startOk && endOk;
}
function flattenIndex() {
    const acc = [];
    for (const list of INDEX.values())
        acc.push(...list);
    return acc;
}
function toLspKind(k) {
    // Les valeurs numériques de SK sont alignées avec LSP. Cast sûr.
    return k;
}
function lspRange(sl, sc, el, ec) {
    return {
        start: { line: sl, character: sc },
        end: { line: el, character: ec },
    };
}
/** Fuzzy score simple. Favorise le préfixe et les sous-séquences denses. */
function fuzzyScore(candidate, query) {
    if (candidate === query)
        return 1000;
    if (candidate.startsWith(query))
        return 800 - Math.min(200, candidate.length - query.length);
    let qi = 0;
    let streak = 0;
    let score = 0;
    for (let i = 0; i < candidate.length && qi < query.length; i++) {
        if (candidate[i] === query[qi]) {
            qi++;
            streak++;
            score += 5 + Math.min(10, streak);
        }
        else {
            streak = 0;
        }
    }
    return qi === query.length ? 300 + score : 0;
}
/** Ordre de priorité par nature de symbole. */
function rankKind(k) {
    switch (k) {
        case SK.Namespace:
        case SK.Module: return 6;
        case SK.Class:
        case SK.Struct:
        case SK.Interface:
        case SK.Enum: return 5;
        case SK.Function:
        case SK.Method:
        case SK.Constructor: return 4;
        case SK.Property:
        case SK.Field: return 3;
        case SK.Variable:
        case SK.Constant: return 2;
        default: return 1;
    }
}
/** Déduplication générique. */
function dedupe(arr, key) {
    const seen = new Set();
    const out = [];
    for (const x of arr) {
        const k = key(x);
        if (seen.has(k))
            continue;
        seen.add(k);
        out.push(x);
    }
    return out;
}
function maskNonCode(src) {
    const chars = Array.from(src);
    const len = chars.length;
    let i = 0;
    const blank = (idx) => {
        if (idx >= 0 && idx < len && chars[idx] !== "\n") {
            chars[idx] = " ";
        }
    };
    while (i < len) {
        const ch = chars[i];
        if (ch === "/" && i + 1 < len && chars[i + 1] === "/") {
            blank(i);
            blank(i + 1);
            i += 2;
            while (i < len && chars[i] !== "\n") {
                blank(i);
                i++;
            }
            continue;
        }
        if (ch === "/" && i + 1 < len && chars[i + 1] === "*") {
            blank(i);
            blank(i + 1);
            i += 2;
            while (i + 1 < len && !(chars[i] === "*" && chars[i + 1] === "/")) {
                if (chars[i] !== "\n")
                    chars[i] = " ";
                i++;
            }
            if (i < len) {
                blank(i);
                i++;
            }
            if (i < len) {
                blank(i);
                i++;
            }
            continue;
        }
        if (ch === "'" || ch === "\"") {
            const quote = ch;
            blank(i);
            i++;
            while (i < len) {
                const current = chars[i];
                if (current === "\n") {
                    i++;
                    break;
                }
                blank(i);
                if (current === quote && chars[i - 1] !== "\\") {
                    i++;
                    break;
                }
                i++;
            }
            continue;
        }
        if (ch === "r") {
            let j = i + 1;
            while (j < len && chars[j] === "#")
                j++;
            if (j < len && chars[j] === "\"") {
                const hashCount = j - (i + 1);
                const terminator = "\"" + "#".repeat(hashCount);
                blank(i);
                for (let t = i + 1; t <= j; t++)
                    blank(t);
                let k = j + 1;
                while (k < len) {
                    if (chars[k] === "\n") {
                        k++;
                        continue;
                    }
                    if (src.startsWith(terminator, k)) {
                        for (let t = 0; t < terminator.length; t++) {
                            blank(k + t);
                        }
                        k += terminator.length;
                        break;
                    }
                    blank(k);
                    k++;
                }
                i = k;
                continue;
            }
        }
        i++;
    }
    return chars.join("");
}
function isWhitespace(ch) {
    return ch === " " || ch === "\t" || ch === "\r" || ch === "\n" || ch === "\f";
}
function isIdentifierStart(ch) {
    if (!ch)
        return false;
    return /[A-Za-z_]/.test(ch);
}
function isIdentifierPart(ch) {
    if (!ch)
        return false;
    return /[A-Za-z0-9_]/.test(ch);
}
//# sourceMappingURL=indexer.js.map