"use strict";
// formatting.ts — formateur LSP déterministe, idempotent et paramétrable
Object.defineProperty(exports, "__esModule", { value: true });
exports.provideFormattingEdits = provideFormattingEdits;
exports.formatDocument = formatDocument;
const vscode_languageserver_1 = require("vscode-languageserver");
/* ============================================================================
 * API principale
 * ========================================================================== */
function provideFormattingEdits(doc, options = {
    tabSize: 2,
    insertSpaces: true,
    trimTrailingWhitespace: true,
    insertFinalNewline: true,
    trimFinalNewlines: true,
    normalizeEOL: "lf",
    maxConsecutiveBlankLines: 2,
    ensureSpaceAroundOperators: true,
    spaceAfterComma: true,
    spaceAroundColon: "right",
    normalizeQuotes: "preserve",
    keepIndentInsideStrings: true,
    alignInlineComments: true,
    alignEquals: true,
    braceStyle: "attach",
    newlineBeforeElse: false,
    wrapCommentsAt: 100,
}) {
    const original = doc.getText();
    const lines = splitLines(original);
    const outLines = formatLines(lines, options);
    let text = joinWithEOL(outLines, options.normalizeEOL ?? "lf");
    // Trim final newlines
    if (options.trimFinalNewlines) {
        text = text.replace(/\n+$/g, "\n");
    }
    // Enforce final newline
    if (options.insertFinalNewline && !text.endsWith("\n")) {
        text += "\n";
    }
    // Early exit si inchangé (après normalisation EOL)
    const normalizedOrig = joinWithEOL(splitLines(original), options.normalizeEOL ?? "lf");
    if (normalizedOrig === text)
        return [];
    return [vscode_languageserver_1.TextEdit.replace(fullRange(doc), text)];
}
function formatDocument(doc, options) {
    // Ne plus passer {} : on laisse le paramètre non défini pour déclencher les valeurs par défaut
    return provideFormattingEdits(doc, options);
}
/* ============================================================================
 * Formatage par passes
 * ========================================================================== */
function formatLines(src, opt) {
    const tabSize = opt.tabSize ?? 2;
    const indentUnit = (opt.insertSpaces ?? true) ? " ".repeat(tabSize) : "\t";
    // Pass 1: normalisation indentation + espaces élémentaires + style d’accolades
    let lines = passIndentAndSpacing(src, indentUnit, tabSize, opt);
    // Pass 2: limitation des lignes vides
    lines = limitBlankRuns(lines, opt.maxConsecutiveBlankLines ?? 2);
    // Pass 3: alignement facultatif des commentaires inline
    if (opt.alignInlineComments) {
        lines = alignEndOfLineComments(lines);
    }
    // Pass 4: alignement simple des '=' sur blocs contigus
    if (opt.alignEquals) {
        lines = alignEqualsBlocks(lines);
    }
    // Pass 5: wrap des commentaires
    if (opt.wrapCommentsAt && opt.wrapCommentsAt > 10) {
        lines = wrapCommentLines(lines, opt.wrapCommentsAt);
    }
    return lines;
}
/* ============================================================================
 * Pass 1 — indentation, espaces, quotes, comma/colon, style accolades
 * ========================================================================== */
function passIndentAndSpacing(src, indentUnit, tabSize, opt) {
    const out = [];
    let indentLevel = 0;
    for (let raw of src) {
        // EOL retiré au split
        // Normalisation indentation hard/soft
        raw = normalizeIndent(raw, indentUnit, tabSize, (opt.insertSpaces ?? true));
        // Brace style: détecter pré-décrément si ligne commence par fermante
        const preDec = startsWithClosingBracket(raw) ? 1 : 0;
        // Appliquer indentation
        let line = applyIndentHeuristic(raw, Math.max(0, indentLevel - preDec), indentUnit);
        // Normaliser quotes simples/doubles (hors chaînes déjà en cours)
        if (opt.normalizeQuotes && opt.normalizeQuotes !== "preserve") {
            line = normalizeQuotesInLine(line, opt.normalizeQuotes);
        }
        // Espaces autour des opérateurs (hors chaînes)
        if (opt.ensureSpaceAroundOperators) {
            line = ensureSpacesAroundOpsOutsideStrings(line);
        }
        // Espaces après virgules
        if (opt.spaceAfterComma) {
            line = ensureSpaceAfterCommasOutsideStrings(line);
        }
        // Espaces autour des ':'
        if (opt.spaceAroundColon && opt.spaceAroundColon !== "none") {
            line = ensureSpaceAroundColonOutsideStrings(line, opt.spaceAroundColon);
        }
        // Style des accolades
        line = applyBraceStyle(line, opt.braceStyle ?? "attach");
        // Newline before else
        if (opt.newlineBeforeElse) {
            line = applyNewlineBeforeElse(line);
        }
        // Trim trailing whitespace
        if (opt.trimTrailingWhitespace ?? true) {
            line = line.replace(/[ \t]+$/g, "");
        }
        out.push(line);
        // Ajuster indentLevel pour la ligne suivante
        indentLevel += netBracketDelta(raw);
    }
    return out;
}
/* ============================================================================
 * Limitation des suites de lignes vides
 * ========================================================================== */
function limitBlankRuns(lines, maxBlank) {
    const out = [];
    let run = 0;
    for (const l of lines) {
        if (/^\s*$/.test(l)) {
            run++;
            if (run <= maxBlank)
                out.push("");
        }
        else {
            run = 0;
            out.push(l);
        }
    }
    return out;
}
/* ============================================================================
 * Indentation utilitaires
 * ========================================================================== */
function normalizeIndent(line, indentUnit, tabSize, useSpaces) {
    const m = /^([ \t]*)/.exec(line);
    const lead = m ? m[1] : "";
    const body = line.slice(lead.length);
    if (useSpaces) {
        const expanded = lead.replace(/\t/g, " ".repeat(tabSize));
        return expanded + body;
    }
    else {
        const width = visualWidth(lead, tabSize);
        const tabs = "\t".repeat(Math.floor(width / tabSize));
        const spaces = " ".repeat(width % tabSize);
        return tabs + spaces + body;
    }
}
function applyIndentHeuristic(line, level, indentUnit) {
    const body = line.replace(/^[ \t]+/, "");
    return indentUnit.repeat(Math.max(0, level)) + body;
}
function netBracketDelta(line) {
    const s = stripStrings(line);
    const opens = s.match(/[{\[(]/g)?.length ?? 0;
    const closes = s.match(/[}\])]/g)?.length ?? 0;
    return opens - closes;
}
function startsWithClosingBracket(line) {
    const s = line.replace(/^[ \t]+/, "");
    return /^[}\])]/.test(s);
}
/* ============================================================================
 * Espaces — opérateurs, virgules, colon
 * ========================================================================== */
function ensureSpacesAroundOpsOutsideStrings(line) {
    const ops = [
        "\\+=", "-=", "\\*=", "/=", "%=", "<<=", ">>=", "&=", "\\^=", "\\|=",
        "===", "!==", "==", "!=", "<=", ">=", "&&", "\\|\\|", "<<", ">>",
        "\\+", "-", "\\*", "/", "%", "<", ">", "=", "&", "\\|", "\\^",
    ];
    const rx = new RegExp(`\\s*(${ops.join("|")})\\s*`, "g");
    return rewriteOutsideStrings(line, (chunk) => chunk.replace(rx, " $1 ").replace(/ {2,}/g, " "));
}
function ensureSpaceAfterCommasOutsideStrings(line) {
    return rewriteOutsideStrings(line, (chunk) => chunk.replace(/\s*,\s*/g, ", ").replace(/, +([,\]\)}])/g, ",$1"));
}
function ensureSpaceAroundColonOutsideStrings(line, mode) {
    return rewriteOutsideStrings(line, (chunk) => {
        switch (mode) {
            case "left": return chunk.replace(/\s*:\s*/g, " :");
            case "both": return chunk.replace(/\s*:\s*/g, " : ");
            case "right":
            default: return chunk.replace(/\s*:\s*/g, ": ");
        }
    });
}
/* ============================================================================
 * Quotes normalisation (hors chaînes)
 * ========================================================================== */
function normalizeQuotesInLine(line, target) {
    return rewriteStrings(line, (content, quote) => {
        if (target === "double" && quote === "'") {
            if (!content.includes("\""))
                return `"${content.replace(/"/g, "\\\"")}"`;
        }
        if (target === "single" && quote === "\"") {
            if (!content.includes("'"))
                return `'${content.replace(/'/g, "\\'")}'`;
        }
        return `${quote}${content}${quote}`;
    });
}
/* ============================================================================
 * Style d’accolades et else
 * ========================================================================== */
function applyBraceStyle(line, style) {
    if (style === "attach") {
        return line.replace(/\)\s*\{\s*$/, ") {");
    }
    else {
        return line.replace(/\)\s*\{\s*$/, ")\n{");
    }
}
function applyNewlineBeforeElse(line) {
    if (!/^\s*else\b/.test(line) && /}\s*else\b/.test(line)) {
        return line.replace(/}\s*else\b/g, "}\nelse");
    }
    return line;
}
/* ============================================================================
 * Alignements
 * ========================================================================== */
function alignEndOfLineComments(lines) {
    const info = [];
    let maxCode = 0;
    for (let i = 0; i < lines.length; i++) {
        const { code, comment } = splitCodeAndComment(lines[i]);
        if (comment) {
            const codeTrim = code.replace(/[ \t]+$/g, "");
            maxCode = Math.max(maxCode, codeTrim.length);
            info.push({ idx: i, code: codeTrim, comment });
        }
    }
    if (info.length === 0)
        return lines;
    const out = [...lines];
    for (const { idx, code, comment } of info) {
        const pad = " ".repeat(Math.max(1, maxCode - code.length + 1));
        out[idx] = code + pad + comment;
    }
    return out;
}
function alignEqualsBlocks(lines) {
    const out = [...lines];
    let i = 0;
    while (i < out.length) {
        const block = [];
        while (i < out.length) {
            const l = out[i];
            if (/^\s*$/.test(l))
                break;
            const sc = stripStrings(l);
            if (sc.includes("=") && !/^\s*\/\//.test(sc))
                block.push(i);
            else if (block.length > 0)
                break;
            i++;
        }
        if (block.length >= 2) {
            let col = 0;
            for (const idx of block) {
                const before = out[idx].split("=")[0] ?? "";
                const w = visualWidth(before, 2);
                col = Math.max(col, w);
            }
            for (const idx of block) {
                const parts = out[idx].split("=");
                const left = parts.shift() ?? "";
                const right = parts.join("=");
                const pad = " ".repeat(Math.max(1, col - visualWidth(left, 2)));
                out[idx] = left.replace(/[ \t]+$/g, "") + pad + " = " + right.replace(/^\s+/, "");
            }
        }
        i++;
    }
    return out;
}
/* ============================================================================
 * Wrap de commentaires
 * ========================================================================== */
function wrapCommentLines(lines, max) {
    const out = [];
    for (const l of lines) {
        const m = /^(\s*\/\/\s?)(.*)$/.exec(l);
        if (!m) {
            out.push(l);
            continue;
        }
        const prefix = m[1];
        const body = m[2].trim();
        const wrapped = wrapText(body, max - visualWidth(prefix, 2));
        for (const w of wrapped)
            out.push(prefix + w);
    }
    return out;
}
/* ============================================================================
 * EOL / Ranges
 * ========================================================================== */
function splitLines(text) {
    return text.replace(/\r\n/g, "\n").split(/\n/);
}
function joinWithEOL(lines, mode) {
    const lf = lines.join("\n");
    return mode === "crlf" ? lf.replace(/\n/g, "\r\n") : lf;
}
function fullRange(doc) {
    const lastLine = Math.max(0, doc.lineCount - 1);
    const lastText = doc.getText(vscode_languageserver_1.Range.create(vscode_languageserver_1.Position.create(lastLine, 0), vscode_languageserver_1.Position.create(lastLine, Number.MAX_SAFE_INTEGER)));
    return vscode_languageserver_1.Range.create(vscode_languageserver_1.Position.create(0, 0), vscode_languageserver_1.Position.create(lastLine, lastText.length));
}
/* ============================================================================
 * Analyse chaînes / commentaires et réécriture sécurisée
 * ========================================================================== */
function stripStrings(s) {
    let out = "";
    let inStr = null;
    for (let i = 0; i < s.length; i++) {
        const ch = s[i];
        if (inStr) {
            if (ch === "\\") {
                i++;
                continue;
            }
            if (ch === inStr)
                inStr = null;
            continue;
        }
        if (ch === '"' || ch === "'") {
            inStr = ch;
            continue;
        }
        out += ch;
    }
    return out;
}
function splitCodeAndComment(line) {
    let inStr = null;
    for (let i = 0; i < line.length; i++) {
        const ch = line[i];
        if (inStr) {
            if (ch === "\\") {
                i++;
                continue;
            }
            if (ch === inStr)
                inStr = null;
            continue;
        }
        if (ch === '"' || ch === "'") {
            inStr = ch;
            continue;
        }
        if (ch === "/" && i + 1 < line.length && line[i + 1] === "/") {
            return { code: line.slice(0, i), comment: line.slice(i) };
        }
        // support /* ... */ inline (simplifié)
        if (ch === "/" && i + 1 < line.length && line[i + 1] === "*") {
            const j = line.indexOf("*/", i + 2);
            if (j >= 0) {
                return { code: line.slice(0, i), comment: line.slice(i, j + 2) };
            }
        }
    }
    return { code: line, comment: "" };
}
/** Réécrit les segments hors chaînes */
function rewriteOutsideStrings(line, fn) {
    let out = "";
    let i = 0;
    let inStr = null;
    while (i < line.length) {
        const ch = line[i];
        if (inStr) {
            out += ch;
            if (ch === "\\") {
                if (i + 1 < line.length) {
                    out += line[i + 1];
                    i += 2;
                    continue;
                }
            }
            else if (ch === inStr) {
                inStr = null;
            }
            i++;
            continue;
        }
        if (ch === '"' || ch === "'") {
            inStr = ch;
            out += ch;
            i++;
            continue;
        }
        // accumule chunk jusqu’à prochaine quote
        let j = i;
        while (j < line.length && line[j] !== "'" && line[j] !== "\"")
            j++;
        const chunk = line.slice(i, j);
        out += fn(chunk);
        i = j;
    }
    return out;
}
/** Réécrit les chaînes elles-mêmes via callback (contenu + quote) */
function rewriteStrings(line, fn) {
    let out = "";
    let i = 0;
    while (i < line.length) {
        const ch = line[i];
        if (ch !== "'" && ch !== "\"") {
            out += ch;
            i++;
            continue;
        }
        const quote = ch;
        i++;
        let content = "";
        while (i < line.length) {
            const c = line[i];
            if (c === "\\") {
                content += c;
                if (i + 1 < line.length) {
                    content += line[i + 1];
                    i += 2;
                    continue;
                }
            }
            if (c === quote) {
                i++;
                break;
            }
            content += c;
            i++;
        }
        out += fn(content, quote);
    }
    return out;
}
/* ============================================================================
 * Wrap simple
 * ========================================================================== */
function wrapText(s, width) {
    if (s.length <= width)
        return [s];
    const words = s.split(/\s+/);
    const lines = [];
    let cur = "";
    for (const w of words) {
        if (cur.length === 0) {
            cur = w;
            continue;
        }
        if (cur.length + 1 + w.length <= width)
            cur += " " + w;
        else {
            lines.push(cur);
            cur = w;
        }
    }
    if (cur.length)
        lines.push(cur);
    return lines;
}
/* ============================================================================
 * Divers
 * ========================================================================== */
function visualWidth(s, tabSize) {
    let w = 0;
    for (const ch of s) {
        if (ch === "\t")
            w += tabSize - (w % tabSize);
        else
            w += 1;
    }
    return w;
}
//# sourceMappingURL=formatting.js.map