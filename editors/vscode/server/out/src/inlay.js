"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.provideInlayHints = provideInlayHints;
const node_1 = require("vscode-languageserver/node");
function inferTypeFromExpr(expr) {
    const s = expr.trim();
    if (/^".*"$/.test(s) || /^'.*'$/.test(s))
        return "string";
    if (/^(true|false)\b/.test(s))
        return "bool";
    if (/^\d+(?:\.\d+)?\b/.test(s))
        return s.includes(".") ? "f64" : "i32";
    if (/^\[.*\]$/.test(s))
        return "[any]";
    if (/^\{.*\}$/.test(s))
        return "map";
    if (/^[A-Z][A-Za-z0-9_]*\(/.test(s))
        return "type";
    if (/^[A-Za-z_][A-Za-z0-9_]*\(/.test(s))
        return "unknown";
    return "any";
}
function inferAlias(pathExpr) {
    const clean = pathExpr.replace(/[:.]/g, "/");
    const segs = clean.split("/").filter(Boolean);
    const last = segs.length ? segs[segs.length - 1] : "mod";
    return `${last.toLowerCase()}_pkg`;
}
function splitArgs(raw) {
    const out = [];
    let cur = "";
    let depth = 0;
    for (const ch of raw) {
        if (ch === "(" || ch === "[" || ch === "{")
            depth++;
        if (ch === ")" || ch === "]" || ch === "}")
            depth = Math.max(0, depth - 1);
        if (ch === "," && depth === 0) {
            out.push(cur.trim());
            cur = "";
            continue;
        }
        cur += ch;
    }
    if (cur.trim())
        out.push(cur.trim());
    return out;
}
function parseProcInferredReturn(lines, startLine) {
    const max = Math.min(lines.length - 1, startLine + 40);
    for (let i = startLine + 1; i <= max; i++) {
        const m = /\bgive\s+(.+?)\s*$/.exec(lines[i]);
        if (!m)
            continue;
        return inferTypeFromExpr(m[1]);
    }
    return "void";
}
function provideInlayHints(doc, range, prefs) {
    const cfg = {
        parameterHints: prefs?.parameterHints ?? true,
        typeHints: prefs?.typeHints ?? true,
        returnHints: prefs?.returnHints ?? true,
        aliasHints: prefs?.aliasHints ?? true,
    };
    const text = doc.getText();
    const lines = text.split(/\r?\n/);
    const out = [];
    const startLine = Math.max(0, range.start.line);
    const endLine = Math.min(lines.length - 1, range.end.line);
    const maxHints = 240;
    for (let i = startLine; i <= endLine && out.length < maxHints; i++) {
        const line = lines[i];
        const decl = /^\s*(?:let|const|static)\s+(?:mut\s+)?([A-Za-z_]\w*)\s*=\s*(.+?)\s*$/.exec(line);
        if (decl && cfg.typeHints) {
            const name = decl[1];
            const type = inferTypeFromExpr(decl[2]);
            const ch = line.indexOf(name) + name.length;
            if (ch >= 0) {
                out.push({
                    position: node_1.Position.create(i, ch),
                    label: `: ${type}`,
                    kind: node_1.InlayHintKind.Type,
                    paddingLeft: true,
                });
            }
        }
        const proc = /\b(?:proc|fn)\s+([A-Za-z_]\w*)\s*\(([^)]*)\)\s*(?:->\s*([A-Za-z_][\w\[\], ]*))?/.exec(line);
        if (proc && !proc[3] && cfg.returnHints) {
            const inferred = parseProcInferredReturn(lines, i);
            const closeParen = line.indexOf(")", proc.index);
            if (closeParen >= 0) {
                out.push({
                    position: node_1.Position.create(i, closeParen + 1),
                    label: ` -> ${inferred}`,
                    kind: node_1.InlayHintKind.Type,
                    paddingLeft: true,
                });
            }
        }
        const alias = /\b(?:use|pull|import)\s+([A-Za-z_][\w./:]*)\b(?!\s+as\b)/.exec(line);
        if (alias && cfg.aliasHints) {
            const p = alias[1];
            const c = line.indexOf(p) + p.length;
            if (c >= 0) {
                out.push({
                    position: node_1.Position.create(i, c),
                    label: ` as ${inferAlias(p)}`,
                    kind: node_1.InlayHintKind.Type,
                    paddingLeft: true,
                });
            }
        }
        if (cfg.parameterHints && !/^\s*(proc|fn|entry|match|select|when|if|for|while)\b/.test(line)) {
            const call = /\b([A-Za-z_]\w*)\s*\(([^)]*)\)/g;
            let cm;
            while ((cm = call.exec(line)) && out.length < maxHints) {
                const args = splitArgs(cm[2]).slice(0, 4);
                let offset = cm.index + cm[0].indexOf("(") + 1;
                for (let a = 0; a < args.length; a++) {
                    const arg = args[a];
                    const local = line.indexOf(arg, offset);
                    if (local < 0)
                        continue;
                    out.push({
                        position: node_1.Position.create(i, local),
                        label: `arg${a + 1}:`,
                        kind: node_1.InlayHintKind.Parameter,
                        paddingRight: true,
                    });
                    offset = local + arg.length;
                    if (out.length >= maxHints)
                        break;
                }
            }
        }
    }
    return out;
}
//# sourceMappingURL=inlay.js.map