"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getDocstringAtLine = getDocstringAtLine;
function isAttributeLine(line) {
    return /^\s*#\s*\[.*\]\s*$/.test(line);
}
function isLineDoc(line) {
    const m = /^\s*(?:\/\/\/|##)\s?(.*)$/.exec(line);
    if (!m)
        return { ok: false, text: "" };
    return { ok: true, text: m[1] ?? "" };
}
function extractBlockDoc(lines, start, end) {
    const out = [];
    for (let i = start; i <= end; i++) {
        const raw = lines[i];
        const line = raw
            .replace(/^\s*\/\*\*?/, "")
            .replace(/\*\/\s*$/, "")
            .replace(/^\s*\*\s?/, "");
        out.push(line);
    }
    return out.join("\n").trim();
}
function getDocstringAtLine(doc, declLine) {
    const lines = doc.getText().split(/\r?\n/);
    let i = Math.min(declLine - 1, lines.length - 1);
    let collected = [];
    let seenDoc = false;
    while (i >= 0) {
        const line = lines[i];
        if (/^\s*$/.test(line)) {
            return seenDoc ? collected.reverse().join("\n").trim() || null : null;
        }
        if (isAttributeLine(line)) {
            i--;
            continue;
        }
        const ld = isLineDoc(line);
        if (ld.ok) {
            seenDoc = true;
            collected.push(ld.text);
            i--;
            continue;
        }
        if (line.includes("*/")) {
            let j = i;
            while (j >= 0 && !lines[j].includes("/**"))
                j--;
            if (j >= 0) {
                const block = extractBlockDoc(lines, j, i);
                return block || null;
            }
        }
        break;
    }
    if (seenDoc) {
        const text = collected.reverse().join("\n").trim();
        return text || null;
    }
    return null;
}
//# sourceMappingURL=docstrings.js.map