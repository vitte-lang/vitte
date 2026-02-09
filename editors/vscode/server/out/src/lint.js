"use strict";
// lint.ts — Linter minimal pour Vitte/Vit (LSP), robuste, configurable, directives inline
Object.defineProperty(exports, "__esModule", { value: true });
exports.lintText = lintText;
exports.lintToPublishable = lintToPublishable;
const node_1 = require("vscode-languageserver/node");
const languageFacts_js_1 = require("./languageFacts.js");
const DEFAULTS = {
    maxLineLength: 120,
    allowTabs: false,
    allowTrailingWhitespace: false,
};
/* =============================== Regex ==================================== */
const rxTrailingWS = /[ \t]+$/;
const rxTab = /\t/;
const rxTodo = /(^|\s)(TODO|FIXME|XXX)(:|\b)/;
const rxLineEndSemicolon = /;\s*$/;
const rxIdent = /[A-Za-z_][A-Za-z0-9_]*/g;
const rxNumber = /(?:0x[0-9A-Fa-f](?:[0-9A-Fa-f_])*)|(?:0b[01](?:[01_])*)|(?:\d(?:[\d_])*(?:\.(?:\d(?:[\d_])*))?(?:[eE][+-]?\d(?:[\d_])*)?)/g;
const rxString = /"(?:\\.|[^"\\])*"/g;
const rxChar = /'(?:\\.|[^'\\])'/g;
const rxCommentLine = /\/\/[^\n]*/g;
const rxCommentDoc = /\/\/![^\n]*/g;
const rxCommentBlock = /\/\*[\s\S]*?\*\//g;
const rxCommentHash = /#(?!\[)[^\n]*/g;
/* ============================ Directives ================================== */
/**
 * Directives supportées:
 *  - Ligne:  // vitte-lint:disable-next-line rule1,rule2
 *  - Bloc:   /* vitte-lint:disable rule1,rule2 *\/ ... /* vitte-lint:enable *\/
 *  - Pragma: // vitte-lint maxLineLength=100 allowTabs=true allowTrailingWhitespace=true
 */
const RULES = {
    Tabs: "format.tabs",
    TrailingWhitespace: "format.trailingWhitespace",
    LineLength: "format.lineLength",
    Todo: "note.todo",
    Brackets: "syntax.brackets",
    Number: "lex.number",
    IdentKeywordShadow: "style.ident.keywordShadow",
    Semicolon: "style.semicolon",
    StyleTypeName: "style.typeName",
    StyleFnName: "style.fnName",
    StyleConstName: "style.constName",
    StyleVarName: "style.varName",
    StyleModulePath: "style.modulePath",
    StyleFieldName: "style.fieldName",
    StyleUsePath: "style.usePath",
};
/** Fix TS2345: disposer d’un garde de type pour convertir un `string` en `RuleId`. */
const ALL_RULES = Object.values(RULES);
function isRuleId(x) {
    // cast en readonly string[] uniquement pour l’opération includes, tout en conservant RuleId côté retour
    return ALL_RULES.includes(x);
}
/* =============================== Core ===================================== */
function lintText(text, uri = "file://unknown", opts = {}) {
    const pragmas = parsePragmas(text);
    const cfg = { ...DEFAULTS, ...pragmas.overrides, ...opts };
    const { lineDisables, blockDisables } = collectDisables(text);
    const diags = [];
    const lines = text.split(/\r?\n/);
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        if (!isDisabled(i, RULES.Tabs, lineDisables, blockDisables)) {
            const m = rxTab.exec(line);
            if (!cfg.allowTabs && m) {
                diags.push(diag(i, m.index, i, m.index + 1, "Tabulation détectée. Utiliser des espaces.", node_1.DiagnosticSeverity.Warning, uri, RULES.Tabs));
            }
        }
        if (!isDisabled(i, RULES.TrailingWhitespace, lineDisables, blockDisables)) {
            const trailing = rxTrailingWS.exec(line);
            if (!cfg.allowTrailingWhitespace && trailing) {
                const start = trailing.index ?? (line.length - trailing[0].length);
                diags.push(diag(i, start, i, line.length, "Espaces en fin de ligne.", node_1.DiagnosticSeverity.Hint, uri, RULES.TrailingWhitespace));
            }
        }
        if (!isDisabled(i, RULES.LineLength, lineDisables, blockDisables)) {
            if (line.length > cfg.maxLineLength) {
                diags.push(diag(i, cfg.maxLineLength, i, line.length, `Ligne trop longue (${line.length} > ${cfg.maxLineLength}).`, node_1.DiagnosticSeverity.Information, uri, RULES.LineLength));
            }
        }
        if (!isDisabled(i, RULES.Todo, lineDisables, blockDisables)) {
            const todo = rxTodo.exec(line);
            if (todo) {
                const idx = todo.index ?? 0;
                diags.push(diag(i, idx, i, idx + todo[0].length, "Marqueur de tâche détecté.", node_1.DiagnosticSeverity.Hint, uri, RULES.Todo));
            }
        }
    }
    const stripped = stripNonCode(text);
    diags.push(...checkBrackets(stripped, uri, lineDisables, blockDisables));
    diags.push(...checkIdentifiersAndKeywords(stripped, uri, lineDisables, blockDisables));
    diags.push(...checkSemicolonHeuristics(stripped, uri, lineDisables, blockDisables));
    diags.push(...checkStyleConventions(stripped, uri, lineDisables, blockDisables));
    diags.push(...checkModulePaths(stripped, uri, lineDisables, blockDisables));
    return diags;
}
/* ============================= Helpers ==================================== */
function diag(line, chStart, lineEnd, chEnd, message, severity, uri, code) {
    return {
        range: node_1.Range.create(node_1.Position.create(line, chStart), node_1.Position.create(lineEnd, chEnd)),
        message,
        severity,
        source: "vitte-lint",
        code,
    };
}
function stripNonCode(src) {
    return src
        .replace(rxCommentBlock, (m) => " ".repeat(m.length))
        .replace(rxCommentDoc, (m) => " ".repeat(m.length))
        .replace(rxCommentLine, (m) => " ".repeat(m.length))
        .replace(rxCommentHash, (m) => " ".repeat(m.length))
        .replace(rxString, (m) => " ".repeat(m.length))
        .replace(rxChar, (m) => " ".repeat(m.length));
}
/* ============================ Brackets ==================================== */
function checkBrackets(text, uri, lineDisables, blockDisables) {
    const diags = [];
    const pairs = { ")": "(", "]": "[", "}": "{" };
    const openers = new Set(["(", "[", "{"]);
    const stack = [];
    const lines = text.split(/\r?\n/);
    for (let i = 0; i < lines.length; i++) {
        const L = lines[i];
        for (let j = 0; j < L.length; j++) {
            const c = L[j];
            if (openers.has(c))
                stack.push({ ch: c, line: i, col: j });
            else if (c in pairs) {
                if (stack.length === 0) {
                    if (!isDisabled(i, RULES.Brackets, lineDisables, blockDisables)) {
                        diags.push(diag(i, j, i, j + 1, `Fermeture inattendue '${c}'.`, node_1.DiagnosticSeverity.Error, uri, RULES.Brackets));
                    }
                }
                else {
                    const top = stack.pop();
                    if (pairs[c] !== top.ch) {
                        if (!isDisabled(i, RULES.Brackets, lineDisables, blockDisables)) {
                            diags.push(diag(i, j, i, j + 1, `Attendu '${matchingCloser(top.ch)}' avant '${c}'.`, node_1.DiagnosticSeverity.Error, uri, RULES.Brackets));
                        }
                    }
                }
            }
        }
    }
    for (const unclosed of stack) {
        if (!isDisabled(unclosed.line, RULES.Brackets, lineDisables, blockDisables)) {
            diags.push(diag(unclosed.line, unclosed.col, unclosed.line, unclosed.col + 1, `Délimiteur non fermé '${unclosed.ch}'.`, node_1.DiagnosticSeverity.Error, uri, RULES.Brackets));
        }
    }
    return diags;
}
function matchingCloser(open) {
    if (open === "(")
        return ")";
    if (open === "[")
        return "]";
    return "}";
}
/* ===================== Idents, nombres, mots-clés ========================= */
function checkIdentifiersAndKeywords(text, uri, lineDisables, blockDisables) {
    const diags = [];
    const lines = text.split(/\r?\n/);
    for (let i = 0; i < lines.length; i++) {
        const L = lines[i];
        if (!isDisabled(i, RULES.Number, lineDisables, blockDisables)) {
            for (const m of L.matchAll(rxNumber)) {
                const tok = m[0];
                if (tok.includes("__") || tok.startsWith("_") || tok.endsWith("_")) {
                    const s = m.index ?? 0;
                    diags.push(diag(i, s, i, s + tok.length, "Littéral numérique invalide (underscore mal placé).", node_1.DiagnosticSeverity.Warning, uri, RULES.Number));
                }
            }
        }
        if (!isDisabled(i, RULES.IdentKeywordShadow, lineDisables, blockDisables)) {
            for (const m of L.matchAll(rxIdent)) {
                const tok = m[0];
                const idx = m.index ?? 0;
                if (languageFacts_js_1.RESERVED_WORDS.has(tok))
                    continue;
                for (const kw of languageFacts_js_1.RESERVED_WORDS) {
                    if (tok === kw)
                        continue;
                    if (tok.startsWith(kw) && tok.length > kw.length && /[A-Za-z0-9_]/.test(tok[kw.length]))
                        continue;
                    if (tok.endsWith(kw) && tok.length > kw.length) {
                        diags.push(diag(i, idx, i, idx + tok.length, `Éviter d'incorporer le mot-clé '${kw}' dans un identifiant.`, node_1.DiagnosticSeverity.Hint, uri, RULES.IdentKeywordShadow));
                        break;
                    }
                }
            }
        }
    }
    return diags;
}
/* ======================== Heuristique point-virgule ======================= */
function checkSemicolonHeuristics(text, uri, lineDisables, blockDisables) {
    const diags = [];
    const lines = text.split(/\r?\n/);
    const starters = /^(?:\s*)(?:let|const|return|break|continue|type)\b/;
    const blockEnders = /^(?:\s*)[}\])]\s*$/;
    for (let i = 0; i < lines.length; i++) {
        if (isDisabled(i, RULES.Semicolon, lineDisables, blockDisables))
            continue;
        const L = lines[i];
        if (blockEnders.test(L))
            continue;
        if (/{\s*$/.test(L))
            continue;
        if (/\b(if|while|for|match)\b/.test(L))
            continue;
        if (starters.test(L)) {
            if (!rxLineEndSemicolon.test(L)) {
                const col = Math.max(0, L.length - 1);
                diags.push(diag(i, col, i, L.length, "Point-virgule probablement attendu ici.", node_1.DiagnosticSeverity.Hint, uri, RULES.Semicolon));
            }
        }
    }
    return diags;
}
/* ============================ Directives utils ============================ */
function parsePragmas(text) {
    const lines = text.split(/\r?\n/);
    const overrides = {};
    for (let i = 0; i < Math.min(20, lines.length); i++) {
        const L = lines[i].trim();
        if (!L.startsWith("//"))
            continue;
        const m = /\/\/\s*vitte-lint\s+(.*)$/i.exec(L);
        if (!m)
            continue;
        const kvs = m[1].split(/\s+/);
        for (const kv of kvs) {
            const [k, v] = kv.split("=");
            if (!k || v === undefined)
                continue;
            if (k === "maxLineLength") {
                const n = Number(v);
                if (Number.isFinite(n) && n > 0)
                    overrides.maxLineLength = n;
            }
            else if (k === "allowTabs") {
                overrides.allowTabs = v === "true";
            }
            else if (k === "allowTrailingWhitespace") {
                overrides.allowTrailingWhitespace = v === "true";
            }
        }
    }
    return { overrides };
}
function collectDisables(text) {
    const lines = text.split(/\r?\n/);
    const lineDisables = new Map();
    const block = { byRule: new Map() };
    // Ligne: // vitte-lint:disable-next-line ruleA,ruleB
    for (let i = 0; i < lines.length; i++) {
        const m = /\/\/\s*vitte-lint:disable-next-line\s+([A-Za-z0-9_.,-\s]+)/i.exec(lines[i]);
        if (!m)
            continue;
        const rules = splitRules(m[1]);
        const target = i + 1;
        if (!lineDisables.has(target))
            lineDisables.set(target, new Set());
        const set = lineDisables.get(target);
        for (const r of rules)
            set.add(mapRuleAlias(r));
    }
    // Bloc: /* vitte-lint:disable rule */ ... /* vitte-lint:enable */
    const blockRx = /\/\*\s*vitte-lint:(disable|enable)\s*([^*]*)\*\//gi;
    const events = [];
    for (let i = 0; i < lines.length; i++) {
        let m;
        const L = lines[i];
        blockRx.lastIndex = 0;
        while ((m = blockRx.exec(L))) {
            const kind = m[1].toLowerCase();
            const rules = splitRules(m[2] ?? "");
            events.push({ line: i, kind, rules });
        }
    }
    const openByRule = new Map();
    for (const ev of events) {
        for (const r of ev.rules) {
            const rule = mapRuleAlias(r);
            if (ev.kind === "disable") {
                openByRule.set(rule, ev.line);
            }
            else {
                const start = openByRule.get(rule);
                if (start !== undefined) {
                    addRange(block, rule, start, ev.line);
                    openByRule.delete(rule);
                }
            }
        }
    }
    for (const [rule, start] of openByRule)
        addRange(block, rule, start, lines.length - 1);
    return { lineDisables, blockDisables: block };
}
function splitRules(s) {
    return s
        .split(/[,\s]+/)
        .map(x => x.trim())
        .filter(Boolean);
}
function mapRuleAlias(name) {
    switch (name) {
        case "tabs": return RULES.Tabs;
        case "trailing-ws":
        case "trailingWhitespace": return RULES.TrailingWhitespace;
        case "line-length":
        case "lineLength": return RULES.LineLength;
        case "todo": return RULES.Todo;
        case "brackets": return RULES.Brackets;
        case "number": return RULES.Number;
        case "ident-shadow":
        case "keyword-shadow": return RULES.IdentKeywordShadow;
        case "semicolon": return RULES.Semicolon;
        case "type-name":
        case "typeName": return RULES.StyleTypeName;
        case "fn-name":
        case "fnName": return RULES.StyleFnName;
        case "const-name":
        case "constName": return RULES.StyleConstName;
        case "var-name":
        case "varName": return RULES.StyleVarName;
        case "module-path":
        case "modulePath": return RULES.StyleModulePath;
        case "field-name":
        case "fieldName": return RULES.StyleFieldName;
        case "use-path":
        case "usePath": return RULES.StyleUsePath;
        default:
            return isRuleId(name) ? name : RULES.LineLength;
    }
}
function addRange(store, rule, start, end) {
    if (!store.byRule.has(rule))
        store.byRule.set(rule, []);
    store.byRule.get(rule).push({ start, end });
}
function isDisabled(line, rule, lineDisables, blockDisables) {
    if (lineDisables.get(line)?.has(rule))
        return true;
    const ranges = blockDisables.byRule.get(rule);
    if (!ranges)
        return false;
    for (const r of ranges)
        if (line >= r.start && line <= r.end)
            return true;
    return false;
}
/* ============================= LSP Bridge ================================= */
function lintToPublishable(text, uri, options) {
    return lintText(text, uri, options);
}
/* ========================= Style conventions (Vitte) ====================== */
function checkStyleConventions(text, uri, lineDisables, blockDisables) {
    const diags = [];
    const lines = text.split(/\r?\n/);
    const rxTypeDecl = /\b(?:struct|form|enum|union|type)\s+([A-Za-z_]\w*)/g;
    const rxFnDecl = /\b(?:fn|proc)\s+([A-Za-z_]\w*)/g;
    const rxConstDecl = /\b(?:const|static)\s+([A-Za-z_]\w*)/g;
    const rxVarDecl = /\blet\s+([A-Za-z_]\w*)/g;
    const rxUseDecl = /\buse\s+([A-Za-z0-9_./:]+)/g;
    for (let i = 0; i < lines.length; i++) {
        const L = lines[i];
        if (!isDisabled(i, RULES.StyleTypeName, lineDisables, blockDisables)) {
            for (const m of L.matchAll(rxTypeDecl)) {
                const name = m[1];
                if (!/^[A-Z][A-Za-z0-9]*$/.test(name)) {
                    const idx = (m.index ?? 0) + m[0].lastIndexOf(name);
                    diags.push(diag(i, idx, i, idx + name.length, "Nom de type conseillé en PascalCase (ex: UserProfile).", node_1.DiagnosticSeverity.Hint, uri, RULES.StyleTypeName));
                }
            }
        }
        if (!isDisabled(i, RULES.StyleFnName, lineDisables, blockDisables)) {
            for (const m of L.matchAll(rxFnDecl)) {
                const name = m[1];
                if (!/^[a-z][a-z0-9_]*$/.test(name)) {
                    const idx = (m.index ?? 0) + m[0].lastIndexOf(name);
                    diags.push(diag(i, idx, i, idx + name.length, "Nom de fonction conseillé en snake_case (ex: read_users).", node_1.DiagnosticSeverity.Hint, uri, RULES.StyleFnName));
                }
            }
        }
        if (!isDisabled(i, RULES.StyleConstName, lineDisables, blockDisables)) {
            for (const m of L.matchAll(rxConstDecl)) {
                const name = m[1];
                if (!/^[A-Z][A-Z0-9_]*$/.test(name)) {
                    const idx = (m.index ?? 0) + m[0].lastIndexOf(name);
                    diags.push(diag(i, idx, i, idx + name.length, "Nom de constante conseillé en UPPER_SNAKE_CASE (ex: MAX_SIZE).", node_1.DiagnosticSeverity.Hint, uri, RULES.StyleConstName));
                }
            }
        }
        if (!isDisabled(i, RULES.StyleVarName, lineDisables, blockDisables)) {
            for (const m of L.matchAll(rxVarDecl)) {
                const name = m[1];
                if (!/^[a-z][a-z0-9_]*$/.test(name)) {
                    const idx = (m.index ?? 0) + m[0].lastIndexOf(name);
                    diags.push(diag(i, idx, i, idx + name.length, "Nom de variable conseillé en snake_case (ex: file_count).", node_1.DiagnosticSeverity.Hint, uri, RULES.StyleVarName));
                }
            }
        }
    }
    return diags;
}
function checkModulePaths(text, uri, lineDisables, blockDisables) {
    const diags = [];
    const lines = text.split(/\r?\n/);
    const rxSpace = /\bspace\s+([A-Za-z0-9_./-]+)/g;
    const rxModule = /\bmodule\s+([A-Za-z0-9_./: -]+)/g;
    for (let i = 0; i < lines.length; i++) {
        const L = lines[i];
        if (!isDisabled(i, RULES.StyleModulePath, lineDisables, blockDisables)) {
            for (const m of L.matchAll(rxSpace)) {
                const name = m[1];
                if (!/^[a-z0-9_./-]+$/.test(name)) {
                    const idx = (m.index ?? 0) + m[0].lastIndexOf(name);
                    diags.push(diag(i, idx, i, idx + name.length, "Chemin d’espace conseillé en minuscules (ex: std/os/user).", node_1.DiagnosticSeverity.Hint, uri, RULES.StyleModulePath));
                }
            }
            for (const m of L.matchAll(rxModule)) {
                const name = m[1].trim();
                if (!/^[a-z0-9_./:]+$/.test(name)) {
                    const idx = (m.index ?? 0) + m[0].lastIndexOf(name);
                    diags.push(diag(i, idx, i, idx + name.length, "Chemin de module conseillé en minuscules.", node_1.DiagnosticSeverity.Hint, uri, RULES.StyleModulePath));
                }
            }
        }
        if (!isDisabled(i, RULES.StyleFieldName, lineDisables, blockDisables)) {
            const m = /(^|\s)([A-Za-z_]\w*)\s*:\s*[^;{},\n]+/.exec(L);
            if (m) {
                const name = m[2];
                if (!/^[a-z][a-z0-9_]*$/.test(name)) {
                    const idx = (m.index ?? 0) + m[0].lastIndexOf(name);
                    diags.push(diag(i, idx, i, idx + name.length, "Nom de champ conseillé en snake_case (ex: user_id).", node_1.DiagnosticSeverity.Hint, uri, RULES.StyleFieldName));
                }
            }
        }
        if (!isDisabled(i, RULES.StyleUsePath, lineDisables, blockDisables)) {
            for (const m of L.matchAll(/\buse\s+([A-Za-z0-9_./:]+)/g)) {
                const name = m[1];
                if (/::/.test(name)) {
                    const idx = (m.index ?? 0) + m[0].lastIndexOf(name);
                    diags.push(diag(i, idx, i, idx + name.length, "Chemin use conseillé avec '/' et '.' (éviter '::').", node_1.DiagnosticSeverity.Hint, uri, RULES.StyleUsePath));
                    continue;
                }
                if (!/^[a-z0-9_./]+(?:\.[A-Za-z_][A-Za-z0-9_]*)*$/.test(name)) {
                    const idx = (m.index ?? 0) + m[0].lastIndexOf(name);
                    diags.push(diag(i, idx, i, idx + name.length, "Chemin use conseillé en minuscules (ex: std/core/types.i32).", node_1.DiagnosticSeverity.Hint, uri, RULES.StyleUsePath));
                }
            }
        }
    }
    return diags;
}
//# sourceMappingURL=lint.js.map