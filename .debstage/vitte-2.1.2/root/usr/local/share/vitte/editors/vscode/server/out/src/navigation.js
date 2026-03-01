"use strict";
// navigation.ts — symboles, définitions, références, rename, workspace symbols
// Version enrichie :
// - Parsing tolérant qui ignore commentaires/chaînes
// - Hiérarchie de symboles via pile d’accolades
// - Règles étendues (module, import, fn, struct, enum, union, type, const/static, field)
// - Références/rename hors commentaires/chaînes
// - Cache par document/version
// - Workspace symbols avec fuzzy match et scoring stable
// - API additionnelle: prepareRename, indexDocument
Object.defineProperty(exports, "__esModule", { value: true });
exports.documentSymbols = documentSymbols;
exports.symbolOutline = symbolOutline;
exports.definitionAtPosition = definitionAtPosition;
exports.referencesAtPosition = referencesAtPosition;
exports.prepareRename = prepareRename;
exports.renameSymbol = renameSymbol;
exports.workspaceSymbols = workspaceSymbols;
exports.indexDocForTests = indexDocForTests;
const node_1 = require("vscode-languageserver/node");
/* -------------------------- Masque code (lexer light) ---------------------- */
// Construit un masque booléen de positions « valides » (hors commentaires et chaînes)
// Supporte //, /* */, "...", '...', r"..." et r#"..."# (style Rust/Vitte). Sans interpolation.
function buildCodeMask(text) {
    const n = text.length;
    const mask = new Uint8Array(n);
    let i = 0;
    const setCode = (from, to) => {
        for (let k = from; k < to; k++)
            mask[k] = 1;
    };
    while (i < n) {
        const c = text.charCodeAt(i);
        const c2 = i + 1 < n ? text.charCodeAt(i + 1) : 0;
        // Line comment #... (ignore attributes #[...])
        if (c === 0x23 /* # */ && c2 !== 0x5b /* [ */) {
            i += 1;
            while (i < n && text.charCodeAt(i) !== 0x0a)
                i++;
            continue;
        }
        // Line comment
        if (c === 0x2f /*/ */ && c2 === 0x2f) {
            i += 2;
            while (i < n && text.charCodeAt(i) !== 0x0a)
                i++;
            continue;
        }
        // Block comment (supports nesting depth 1 minimal)
        if (c === 0x2f && c2 === 0x2a /* * */) {
            i += 2;
            while (i < n) {
                if (text.charCodeAt(i) === 0x2a && i + 1 < n && text.charCodeAt(i + 1) === 0x2f) {
                    i += 2;
                    break;
                }
                i++;
            }
            continue;
        }
        // Raw string r#*" ... "#*
        if (c === 0x72 /* r */) {
            let j = i + 1;
            let hashes = 0;
            while (j < n && text.charCodeAt(j) === 0x23 /* # */) {
                hashes++;
                j++;
            }
            if (j < n && text.charCodeAt(j) === 0x22 /* " */) {
                // found r#*"
                j++;
                // consume until closing "#*
                for (; j < n; j++) {
                    if (text.charCodeAt(j) === 0x22 /* " */) {
                        let k = j + 1;
                        let ok = true;
                        for (let h = 0; h < hashes; h++) {
                            if (k >= n || text.charCodeAt(k) !== 0x23) {
                                ok = false;
                                break;
                            }
                            k++;
                        }
                        if (ok) {
                            i = k;
                            break;
                        }
                    }
                }
                continue;
            }
        }
        // Normal string "..." ou '...'
        if (c === 0x22 || c === 0x27) {
            const quote = c;
            i++;
            while (i < n) {
                if (text.charCodeAt(i) === 0x5c /* \\ */) {
                    i += 2;
                    continue;
                }
                if (text.charCodeAt(i) === quote) {
                    i++;
                    break;
                }
                i++;
            }
            continue;
        }
        // Code token
        const start = i;
        // Avance jusqu’à prochain début de com/str pour marquer code en bloc
        while (i < n) {
            const a = text.charCodeAt(i);
            const b = i + 1 < n ? text.charCodeAt(i + 1) : 0;
            if ((a === 0x2f && (b === 0x2f || b === 0x2a)) ||
                a === 0x22 ||
                a === 0x27 ||
                a === 0x72 ||
                (a === 0x23 && b !== 0x5b))
                break;
            i++;
        }
        setCode(start, i);
    }
    return mask;
}
function isIdentChar(ch) {
    return (ch >= 48 && ch <= 57) || // 0-9
        (ch >= 65 && ch <= 90) || // A-Z
        (ch >= 97 && ch <= 122) || // a-z
        ch === 95; // _
}
function wordAt(doc, pos) {
    const text = doc.getText();
    const off = doc.offsetAt(pos);
    let s = off, e = off;
    while (s > 0 && isIdentChar(text.charCodeAt(s - 1)))
        s--;
    while (e < text.length && isIdentChar(text.charCodeAt(e)))
        e++;
    return e > s ? text.slice(s, e) : null;
}
function escapeRx(s) {
    return s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}
function posKey(p) { return `${p.line}:${p.character}`; }
function dedupeBy(arr, keyFn) {
    const seen = new Set();
    const out = [];
    for (const v of arr) {
        const k = keyFn(v);
        if (seen.has(k))
            continue;
        seen.add(k);
        out.push(v);
    }
    return out;
}
function isValidIdent(s) { return /^[A-Za-z_]\w*$/.test(s); }
/* ------------------------------ Règles symboles ---------------------------- */
// Les regex sont évaluées uniquement sur les positions mask==1
const RULES = [
    { rx: /\bmodule\s+([A-Za-z_][\w./:]*)/g, kind: node_1.SymbolKind.Namespace, nameGroup: 1 },
    { rx: /\bspace\s+([A-Za-z_][\w./:]*)/g, kind: node_1.SymbolKind.Namespace, nameGroup: 1 },
    { rx: /\bimport\s+([A-Za-z_][\w./:]*(?:::\*)?)/g, kind: node_1.SymbolKind.Namespace, nameGroup: 1 },
    { rx: /\buse\s+([A-Za-z_][\w./:]*(?:::\*)?)/g, kind: node_1.SymbolKind.Namespace, nameGroup: 1 },
    { rx: /\b(?:pub\s+)?(?:fn|proc)\s+([A-Za-z_]\w*)\s*\(/g, kind: node_1.SymbolKind.Function, nameGroup: 1 },
    { rx: /\b(?:pub\s+)?struct\s+([A-Za-z_]\w*)/g, kind: node_1.SymbolKind.Struct, nameGroup: 1 },
    { rx: /\b(?:pub\s+)?form\s+([A-Za-z_]\w*)/g, kind: node_1.SymbolKind.Struct, nameGroup: 1 },
    { rx: /\b(?:pub\s+)?enum\s+([A-Za-z_]\w*)/g, kind: node_1.SymbolKind.Enum, nameGroup: 1 },
    { rx: /\b(?:pub\s+)?union\s+([A-Za-z_]\w*)/g, kind: node_1.SymbolKind.Struct, nameGroup: 1 },
    { rx: /\b(?:pub\s+)?type\s+([A-Za-z_]\w*)/g, kind: node_1.SymbolKind.TypeParameter, nameGroup: 1 },
    { rx: /\b(?:pub\s+)?const\s+([A-Za-z_]\w*)/g, kind: node_1.SymbolKind.Constant, nameGroup: 1 },
    { rx: /\b(?:pub\s+)?static\s+([A-Za-z_]\w*)/g, kind: node_1.SymbolKind.Variable, nameGroup: 1 },
    // champs de struct/form: name: Type
    { rx: /(^|\s)([A-Za-z_]\w*)\s*:\s*[^;{},\n]+(?=,|\n|\r|\})/g, kind: node_1.SymbolKind.Field, nameGroup: 2 },
];
function* iterMaskedMatches(rx, text, mask) {
    rx.lastIndex = 0;
    let m;
    while ((m = rx.exec(text))) {
        const idx = m.index ?? 0;
        // Vérifie que le début du match tombe sur zone code
        if (mask[idx]) {
            yield m;
        }
        if (m[0].length === 0)
            rx.lastIndex++;
    }
}
/* ------------------------ Collecte + hiérarchie outline -------------------- */
function collectFlatSymbols(doc) {
    const text = doc.getText();
    const mask = buildCodeMask(text);
    const out = [];
    for (const { rx, kind, nameGroup } of RULES) {
        for (const m of iterMaskedMatches(rx, text, mask)) {
            const name = m[nameGroup];
            if (!name)
                continue;
            const start = doc.positionAt(m.index ?? 0);
            const end = doc.positionAt((m.index ?? 0) + m[0].length);
            const nameOffset = (m.index ?? 0) + m[0].indexOf(name);
            const nameStart = doc.positionAt(nameOffset);
            const nameEnd = doc.positionAt(nameOffset + name.length);
            out.push({
                name,
                kind,
                range: node_1.Range.create(start, end),
                selectionRange: node_1.Range.create(nameStart, nameEnd),
            });
        }
    }
    // Déduplication conservatrice
    return dedupeBy(out, s => `${s.kind}:${s.name}:${posKey(s.selectionRange.start)}`);
}
function buildOutline(doc, flat) {
    // Trie par position pour associer via une pile d’accolades
    const text = doc.getText();
    const nodes = flat
        .map(s => ({
        name: s.name,
        kind: s.kind,
        range: s.range,
        selectionRange: s.selectionRange,
        children: [],
    }))
        .sort((a, b) => {
        const da = doc.offsetAt(a.range.start);
        const db = doc.offsetAt(b.range.start);
        return da - db;
    });
    // Pile de conteneurs basée sur accolades
    const root = { name: "<root>", kind: node_1.SymbolKind.Namespace, range: node_1.Range.create(node_1.Position.create(0, 0), node_1.Position.create(0, 0)), selectionRange: node_1.Range.create(node_1.Position.create(0, 0), node_1.Position.create(0, 0)), children: [] };
    const stack = [root];
    // Prépare positions des '{' et '}' sur zones code
    const mask = buildCodeMask(text);
    const opens = [];
    for (let i = 0; i < text.length; i++) {
        if (!mask[i])
            continue;
        const ch = text.charCodeAt(i);
        if (ch === 0x7b /* { */)
            opens.push(i);
        else if (ch === 0x7d /* } */) {
            if (opens.length)
                opens.pop();
        }
    }
    // Index rapide: map startOffset->depth at that point
    // Simplification: nous utilisons l’ordre croissant + heuristique par proximité
    function pushToBestContainer(node) {
        // Heuristique: rattacher au dernier symbole dont la position est avant node et qui n’est pas clos avant.
        for (let j = stack.length - 1; j >= 0; j--) {
            const top = stack[j];
            const topStart = doc.offsetAt(top.range.start);
            const nodeStart = doc.offsetAt(node.range.start);
            if (nodeStart >= topStart) {
                top.children.push(node);
                stack.push(node);
                return;
            }
        }
        root.children.push(node);
        stack.push(node);
    }
    // On parcourt les nodes et simule la fermeture quand on rencontre '}' avant prochain node
    const bracesPositions = [];
    const n = text.length;
    for (let i = 0; i < n; i++)
        if (mask[i] && (text[i] === '{' || text[i] === '}'))
            bracesPositions.push(i);
    let bp = 0;
    for (const node of nodes) {
        const nodeStart = doc.offsetAt(node.range.start);
        // ferme conteneurs jusqu’à la position courante
        while (bp < bracesPositions.length && bracesPositions[bp] < nodeStart) {
            const ch = text[bracesPositions[bp]];
            if (ch === '}') {
                if (stack.length > 1)
                    stack.pop();
            }
            bp++;
        }
        pushToBestContainer(node);
    }
    // Ferme tout ce qui reste
    while (bp < bracesPositions.length) {
        if (text[bracesPositions[bp]] === '}' && stack.length > 1)
            stack.pop();
        bp++;
    }
    return root.children;
}
/* ------------------------------- Index & cache ----------------------------- */
const docCache = new WeakMap();
function indexDocument(doc) {
    const cached = docCache.get(doc);
    if (cached && cached.version === doc.version)
        return cached.index;
    const flat = collectFlatSymbols(doc);
    const outline = buildOutline(doc, flat);
    const byName = new Map();
    for (const s of flat) {
        const arr = byName.get(s.name) ?? [];
        arr.push(s);
        byName.set(s.name, arr);
    }
    const index = { flat, outline, byName };
    docCache.set(doc, { version: doc.version, index });
    return index;
}
/* --------------------------------- API doc --------------------------------- */
function documentSymbols(doc) {
    return indexDocument(doc).outline;
}
function symbolOutline(doc) {
    return documentSymbols(doc);
}
/* --------------------------- Définitions / refs ---------------------------- */
function definitionAtPosition(doc, pos, uri) {
    const word = wordAt(doc, pos);
    if (!word)
        return [];
    const { byName } = indexDocument(doc);
    const defs = byName.get(word) ?? [];
    // Tri par priorité de kind puis proximité
    const baseOff = doc.offsetAt(pos);
    defs.sort((a, b) => kindPriority(a.kind) - kindPriority(b.kind) || Math.abs(doc.offsetAt(a.selectionRange.start) - baseOff) - Math.abs(doc.offsetAt(b.selectionRange.start) - baseOff));
    return defs.map(d => node_1.Location.create(uri, d.selectionRange));
}
function referencesAtPosition(doc, pos, uri) {
    const word = wordAt(doc, pos);
    if (!word)
        return [];
    const text = doc.getText();
    const mask = buildCodeMask(text);
    const out = [];
    const re = new RegExp(`(?<![A-Za-z0-9_])${escapeRx(word)}(?![A-Za-z0-9_])`, "g");
    re.lastIndex = 0;
    let m;
    while ((m = re.exec(text))) {
        const idx = m.index ?? 0;
        if (!mask[idx]) {
            if (m[0].length === 0)
                re.lastIndex++;
            continue;
        }
        const start = doc.positionAt(idx);
        const end = doc.positionAt(idx + m[0].length);
        out.push(node_1.Location.create(uri, node_1.Range.create(start, end)));
        if (m[0].length === 0)
            re.lastIndex++;
    }
    return out;
}
/* --------------------------------- Rename ---------------------------------- */
function prepareRename(doc, pos) {
    const name = wordAt(doc, pos);
    if (!name || !isValidIdent(name))
        return null;
    const off = doc.offsetAt(pos);
    const start = doc.positionAt(off - (name.length - (name.lastIndexOf(name) + 1)));
    const end = doc.positionAt(doc.offsetAt(start) + name.length);
    return { range: node_1.Range.create(start, end), placeholder: name };
}
function renameSymbol(doc, pos, newName) {
    const old = wordAt(doc, pos);
    if (!old || !isValidIdent(newName))
        return [];
    const text = doc.getText();
    const mask = buildCodeMask(text);
    const edits = [];
    const re = new RegExp(`(?<![A-Za-z0-9_])${escapeRx(old)}(?![A-Za-z0-9_])`, "g");
    let m;
    while ((m = re.exec(text))) {
        const idx = m.index ?? 0;
        if (!mask[idx]) {
            if (m[0].length === 0)
                re.lastIndex++;
            continue;
        }
        const start = doc.positionAt(idx);
        const end = doc.positionAt(idx + m[0].length);
        edits.push({ range: node_1.Range.create(start, end), newText: newName });
        if (m[0].length === 0)
            re.lastIndex++;
    }
    return edits;
}
/* ----------------------------- Workspace symbols --------------------------- */
function workspaceSymbols(query, openDocs, limit = 200) {
    const q = query.trim();
    const result = [];
    let seq = 0;
    for (const { uri, doc } of openDocs) {
        const { flat } = indexDocument(doc);
        for (const s of flat) {
            const score = q ? fuzzyScore(s.name, q) : 1;
            if (q && score <= 0)
                continue;
            result.push({
                ws: { name: s.name, kind: s.kind, location: node_1.Location.create(uri, s.selectionRange) },
                score,
                idx: seq++,
            });
            if (result.length >= limit * 4)
                break; // cap intermédiaire avant tri
        }
    }
    result.sort((a, b) => b.score - a.score || a.idx - b.idx);
    return result.slice(0, limit).map(r => r.ws);
}
/* --------------------------------- Helpers -------------------------------- */
function kindPriority(k) {
    switch (k) {
        case node_1.SymbolKind.Function: return 0;
        case node_1.SymbolKind.Method: return 1;
        case node_1.SymbolKind.Variable: return 2;
        case node_1.SymbolKind.Constant: return 3;
        case node_1.SymbolKind.Struct: return 4;
        case node_1.SymbolKind.Class: return 5;
        case node_1.SymbolKind.Enum: return 6;
        case node_1.SymbolKind.Interface: return 7;
        case node_1.SymbolKind.Namespace: return 8;
        case node_1.SymbolKind.Field: return 9;
        default: return 10;
    }
}
function fuzzyScore(name, query) {
    // subsequence scoring sensible à la casse avec bonus pour débuts de mots et camelCase
    let i = 0, j = 0, score = 0, lastMatch = -2;
    while (i < name.length && j < query.length) {
        const nc = name[i];
        const qc = query[j];
        if (nc.toLowerCase() === qc.toLowerCase()) {
            let s = 1;
            if (i === 0 || !isIdentChar(name.charCodeAt(i - 1)))
                s += 2; // début de mot
            if (nc === qc)
                s += 1; // casse exacte
            if (i === lastMatch + 1)
                s += 2; // consécutif
            score += s;
            lastMatch = i;
            j++;
            i++;
        }
        else {
            i++;
        }
    }
    return j === query.length ? score : -1;
}
/* --------------------------------- Exports -------------------------------- */
function indexDocForTests(doc) { return indexDocument(doc); }
//# sourceMappingURL=navigation.js.map