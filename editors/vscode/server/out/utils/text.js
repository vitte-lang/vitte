"use strict";
/**
 * Vitte LSP Utils — Text helpers (ultra complete)
 * ------------------------------------------------
 * Pure utilities for text/position/range manipulation with LSP (UTF‑16) semantics.
 * No I/O, no side effects, dependency‑free.
 *
 * IMPORTANT: JavaScript strings are UTF‑16. All offsets and lengths here are in
 * UTF‑16 code units, matching LSP expectations.
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.indexLineStarts = indexLineStarts;
exports.clamp = clamp;
exports.clampPosition = clampPosition;
exports.comparePosition = comparePosition;
exports.normalizeRange = normalizeRange;
exports.mergeRanges = mergeRanges;
exports.positionAt = positionAt;
exports.offsetAt = offsetAt;
exports.fullDocumentRange = fullDocumentRange;
exports.sliceByRange = sliceByRange;
exports.replaceRange = replaceRange;
exports.edit = edit;
exports.applyEdits = applyEdits;
exports.wordAtPosition = wordAtPosition;
exports.lineTextAt = lineTextAt;
exports.lineLengthAt = lineLengthAt;
exports.fullReplaceEdit = fullReplaceEdit;
exports.fastSplitLines = fastSplitLines;
exports.countLines = countLines;
exports.isEmptyRange = isEmptyRange;
exports.cursorAt = cursorAt;
exports.expandRangeOnLine = expandRangeOnLine;
exports.rangeContains = rangeContains;
exports.computeMinimalEdits = computeMinimalEdits;
exports.computeSmartLineEdit = computeSmartLineEdit;
exports.makePosition = makePosition;
exports.makeRange = makeRange;
exports.positionEquals = positionEquals;
exports.rangeEquals = rangeEquals;
exports.rangeToOffsets = rangeToOffsets;
exports.offsetsToRange = offsetsToRange;
exports.clampRangeToText = clampRangeToText;
exports.lineRange = lineRange;
exports.isBlankLine = isBlankLine;
exports.indentationLevelOf = indentationLevelOf;
exports.normalizeIndentation = normalizeIndentation;
exports.nextNonWhitespaceColumn = nextNonWhitespaceColumn;
exports.prevNonWhitespaceColumn = prevNonWhitespaceColumn;
exports.wordBoundaries = wordBoundaries;
exports.findMatchingBracket = findMatchingBracket;
exports.expandSelectionToEnclosingBrackets = expandSelectionToEnclosingBrackets;
exports.computeEditsByLine = computeEditsByLine;
exports.applyEditsStrict = applyEditsStrict;
exports.computeMinimalSmartEdits = computeMinimalSmartEdits;
/** Builds an array of line start offsets (UTF‑16 code units). */
function indexLineStarts(text) {
    const starts = [0];
    for (let i = 0; i < text.length; i++) {
        const ch = text.charCodeAt(i);
        if (ch === 13 /*\r*/) {
            if (i + 1 < text.length && text.charCodeAt(i + 1) === 10 /*\n*/)
                i++;
            starts.push(i + 1);
        }
        else if (ch === 10 /*\n*/) {
            starts.push(i + 1);
        }
    }
    return starts;
}
/** Clamp helpers. */
function clamp(v, lo, hi) {
    return Math.min(hi, Math.max(lo, v));
}
function clampPosition(pos, maxLine) {
    return { line: clamp(pos.line, 0, maxLine), character: Math.max(0, pos.character) };
}
/** Compare helpers. */
function comparePosition(a, b) {
    if (a.line < b.line)
        return -1;
    if (a.line > b.line)
        return 1;
    if (a.character < b.character)
        return -1;
    if (a.character > b.character)
        return 1;
    return 0;
}
function normalizeRange(r) {
    return comparePosition(r.start, r.end) <= 0 ? r : { start: r.end, end: r.start };
}
function mergeRanges(ranges) {
    if (ranges.length <= 1)
        return ranges.slice();
    const rs = ranges.map(normalizeRange).sort((a, b) => comparePosition(a.start, b.start));
    const out = [];
    let cur = rs[0];
    for (let i = 1; i < rs.length; i++) {
        const r = rs[i];
        if (comparePosition(r.start, cur.end) <= 0) {
            if (comparePosition(r.end, cur.end) > 0)
                cur.end = r.end;
        }
        else {
            out.push(cur);
            cur = r;
        }
    }
    out.push(cur);
    return out;
}
/** Offset/position mapping (UTF‑16). */
function positionAt(offset, lineStarts) {
    offset = Math.max(0, offset);
    let lo = 0, hi = lineStarts.length - 1;
    while (lo <= hi) {
        const mid = (lo + hi) >>> 1;
        const start = lineStarts[mid];
        if (start === offset)
            return { line: mid, character: 0 };
        if (start < offset)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    const line = Math.max(0, lo - 1);
    return { line, character: offset - lineStarts[line] };
}
function offsetAt(pos, lineStarts) {
    const line = clamp(pos.line, 0, Math.max(0, lineStarts.length - 1));
    return lineStarts[line] + Math.max(0, pos.character);
}
/** Full document range for a given text. */
function fullDocumentRange(text) {
    const starts = indexLineStarts(text);
    const lastLine = Math.max(0, starts.length - 1);
    // Character = length of last line segment (UTF‑16 units)
    const lastChar = text.length - starts[lastLine];
    return { start: { line: 0, character: 0 }, end: { line: lastLine, character: lastChar } };
}
/** Returns text slice by range using a precomputed line index (safer & fast). */
function sliceByRange(text, range, lineStarts = indexLineStarts(text)) {
    const a = offsetAt(range.start, lineStarts);
    const b = offsetAt(range.end, lineStarts);
    return text.slice(Math.max(0, Math.min(a, b)), Math.max(a, b));
}
/** Replaces the content at a given range. */
function replaceRange(text, range, replacement, lineStarts = indexLineStarts(text)) {
    const a = offsetAt(range.start, lineStarts);
    const b = offsetAt(range.end, lineStarts);
    const lo = Math.max(0, Math.min(a, b));
    const hi = Math.max(a, b);
    return text.slice(0, lo) + replacement + text.slice(hi);
}
/** Builds a TextEdit struct. */
function edit(range, newText) {
    return { range: normalizeRange(range), newText };
}
/** Applies a set of non‑overlapping edits. If overlapping, sorts & compacts first. */
function applyEdits(text, edits) {
    if (edits.length === 0)
        return text;
    const starts = indexLineStarts(text);
    // Normalize & sort
    const es = edits.map(e => ({ range: normalizeRange(e.range), newText: e.newText }));
    es.sort((a, b) => {
        const cmp = comparePosition(a.range.start, b.range.start);
        if (cmp !== 0)
            return cmp;
        return comparePosition(a.range.end, b.range.end);
    });
    // Compact overlaps (keep the last edit for overlapping zones)
    const compact = [];
    for (const e of es) {
        if (compact.length === 0) {
            compact.push(e);
            continue;
        }
        const last = compact[compact.length - 1];
        if (comparePosition(e.range.start, last.range.end) <= 0) {
            // Overlap → extend last to cover farther end and override newText if same span.
            if (comparePosition(e.range.end, last.range.end) > 0)
                last.range.end = e.range.end;
            last.newText = e.newText;
        }
        else {
            compact.push(e);
        }
    }
    // Apply from end to start by offsets to keep indices stable
    let result = text;
    for (let i = compact.length - 1; i >= 0; i--) {
        const r = compact[i].range;
        const a = offsetAt(r.start, starts);
        const b = offsetAt(r.end, starts);
        const lo = Math.max(0, Math.min(a, b));
        const hi = Math.max(a, b);
        result = result.slice(0, lo) + compact[i].newText + result.slice(hi);
    }
    return result;
}
function wordAtPosition(text, pos, lineStarts = indexLineStarts(text), re = /[A-Za-z0-9_]+/g) {
    const line = clamp(pos.line, 0, indexLineStarts(text).length - 1);
    const lineStart = lineStarts[line];
    const lineEnd = (line + 1 < lineStarts.length) ? lineStarts[line + 1] : text.length;
    const lineText = text.slice(lineStart, lineEnd);
    // Position column within the line
    const col = Math.max(0, Math.min(pos.character, lineText.length));
    let m;
    re.lastIndex = 0;
    while ((m = re.exec(lineText))) {
        const s = m.index;
        const e = s + m[0].length;
        if (s <= col && e >= col) {
            return {
                word: m[0],
                range: { start: { line, character: s }, end: { line, character: e } }
            };
        }
    }
    return null;
}
/** Returns the text of a given line (without its EOL). */
function lineTextAt(text, line, lineStarts = indexLineStarts(text)) {
    const ln = clamp(line, 0, Math.max(0, lineStarts.length - 1));
    const start = lineStarts[ln];
    const end = ln + 1 < lineStarts.length ? lineStarts[ln + 1] : text.length;
    return text.slice(start, end);
}
/** Returns the line length (UTF‑16 units) without EOL. */
function lineLengthAt(text, line, lineStarts = indexLineStarts(text)) {
    const ln = clamp(line, 0, Math.max(0, lineStarts.length - 1));
    const start = lineStarts[ln];
    const end = ln + 1 < lineStarts.length ? lineStarts[ln + 1] : text.length;
    return end - start;
}
/** Creates a single full‑document replace edit (useful for formatters). */
function fullReplaceEdit(text, newText) {
    return edit(fullDocumentRange(text), newText);
}
/** Splits text into lines quickly (LF/CRLF tolerated). */
function fastSplitLines(text) {
    return text.replace(/\r\n/g, '\n').split('\n');
}
/** Counts lines without allocating per line. */
function countLines(text) {
    if (text.length === 0)
        return 1;
    let lines = 1;
    for (let i = 0; i < text.length; i++) {
        const c = text.charCodeAt(i);
        if (c === 13 /*\r*/) {
            if (i + 1 < text.length && text.charCodeAt(i + 1) === 10 /*\n*/)
                i++;
            lines++;
        }
        else if (c === 10 /*\n*/) {
            lines++;
        }
    }
    return lines;
}
/** Quick predicate: is range empty? */
function isEmptyRange(r) {
    return r.start.line === r.end.line && r.start.character === r.end.character;
}
/** Make a zero‑length range at a given position. */
function cursorAt(pos) {
    return { start: pos, end: pos };
}
/** Expand a range by delta characters on both ends (line‑local). */
function expandRangeOnLine(r, startDelta, endDelta) {
    return {
        start: { line: r.start.line, character: Math.max(0, r.start.character - startDelta) },
        end: { line: r.end.line, character: Math.max(0, r.end.character + endDelta) },
    };
}
/** Check if a position is within a given range (inclusive of start, exclusive of end). */
function rangeContains(range, pos) {
    if (comparePosition(pos, range.start) < 0)
        return false;
    if (comparePosition(pos, range.end) >= 0)
        return false;
    return true;
}
/** Compute a minimal set of TextEdits to replace `oldText` by `newText` (simple fallback). */
function computeMinimalEdits(oldText, newText) {
    if (oldText === newText)
        return [];
    // Simple strategy: full replace. (Can be improved to diff‑based later.)
    return [fullReplaceEdit(oldText, newText)];
}
/** Try to create a line‑aware replace (only if both share same number of lines & prefixes/suffixes). */
function computeSmartLineEdit(oldText, newText) {
    if (oldText === newText)
        return [];
    const oldLines = fastSplitLines(oldText);
    const newLines = fastSplitLines(newText);
    if (oldLines.length !== newLines.length)
        return null;
    // Find first and last differing line
    let startLine = 0;
    while (startLine < oldLines.length && oldLines[startLine] === newLines[startLine])
        startLine++;
    let endLine = oldLines.length - 1;
    while (endLine >= startLine && oldLines[endLine] === newLines[endLine])
        endLine--;
    if (startLine > endLine)
        return []; // identical, should have returned earlier
    const startChar = commonPrefixLen(oldLines[startLine], newLines[startLine]);
    const endCharOld = oldLines[endLine].length - commonSuffixLen(oldLines[endLine], newLines[endLine]);
    const endCharNew = newLines[endLine].length - commonSuffixLen(newLines[endLine], oldLines[endLine]);
    const range = { start: { line: startLine, character: startChar }, end: { line: endLine, character: endCharOld } };
    const replacement = newLines.slice(startLine, endLine + 1);
    replacement[0] = replacement[0].slice(startChar);
    const lastIdx = replacement.length - 1;
    replacement[lastIdx] = replacement[lastIdx].slice(0, endCharNew);
    return [edit(range, replacement.join('\n'))];
}
function commonPrefixLen(a, b) {
    const n = Math.min(a.length, b.length);
    let i = 0;
    while (i < n && a.charCodeAt(i) === b.charCodeAt(i))
        i++;
    return i;
}
function commonSuffixLen(a, b) {
    const n = Math.min(a.length, b.length);
    let i = 0;
    while (i < n && a.charCodeAt(a.length - 1 - i) === b.charCodeAt(b.length - 1 - i))
        i++;
    return i;
}
// -----------------------------------------------------------------------------
// Extended helpers — ranges, scanning, indentation, brackets, diffs (light)
// -----------------------------------------------------------------------------
/** Factory helpers. */
function makePosition(line, character) {
    return { line, character };
}
function makeRange(sline, schar, eline, echar) {
    return { start: { line: sline, character: schar }, end: { line: eline, character: echar } };
}
function positionEquals(a, b) {
    return a.line === b.line && a.character === b.character;
}
function rangeEquals(a, b) {
    return positionEquals(a.start, b.start) && positionEquals(a.end, b.end);
}
/** Convert to/from offsets (UTF‑16). */
function rangeToOffsets(text, r, starts = indexLineStarts(text)) {
    const a = offsetAt(r.start, starts);
    const b = offsetAt(r.end, starts);
    return [Math.max(0, Math.min(a, b)), Math.max(a, b)];
}
function offsetsToRange(text, start, end, starts = indexLineStarts(text)) {
    return { start: positionAt(Math.max(0, start), starts), end: positionAt(Math.max(0, end), starts) };
}
/** Clamp a range to the text bounds. */
function clampRangeToText(text, r, starts = indexLineStarts(text)) {
    const [a, b] = rangeToOffsets(text, r, starts);
    const lo = 0;
    const hi = text.length;
    return offsetsToRange(text, Math.max(lo, a), Math.min(hi, b), starts);
}
/** Line helpers. */
function lineRange(text, line, starts = indexLineStarts(text)) {
    const ln = clamp(line, 0, Math.max(0, starts.length - 1));
    const s = starts[ln];
    const e = ln + 1 < starts.length ? starts[ln + 1] : text.length;
    return offsetsToRange(text, s, e, starts);
}
function isBlankLine(text, line, starts = indexLineStarts(text)) {
    const lr = lineRange(text, line, starts);
    const slice = sliceByRange(text, lr, starts);
    return /^\s*$/.test(slice);
}
function indentationLevelOf(lineText, tabSize = 2) {
    let col = 0;
    for (const ch of lineText) {
        if (ch === ' ')
            col += 1;
        else if (ch === '\t')
            col += tabSize;
        else
            break;
    }
    return col;
}
/** Normalize indentation (leading whitespace only). */
function normalizeIndentation(text, insertSpaces, tabSize = 2) {
    const unit = insertSpaces ? ' '.repeat(Math.max(1, tabSize)) : '\t';
    const lines = fastSplitLines(text);
    for (let i = 0; i < lines.length; i++) {
        const m = /^(\s*)/.exec(lines[i]);
        if (!m)
            continue;
        const lead = m[1];
        if (insertSpaces) {
            // Tabs → spaces
            const tabMatch = /^\t+/.exec(lead);
            const tabs = tabMatch?.[0] ?? '';
            if (tabs)
                lines[i] = tabs.split('').map(() => unit).join('') + lines[i].slice(tabs.length);
        }
        else {
            // Spaces → tabs (groups of tabSize)
            const re = new RegExp(`^(?: {${Math.max(1, tabSize)}})+`);
            const mm = re.exec(lead);
            if (mm) {
                const spaces = mm[0].length;
                const tabs = Math.floor(spaces / Math.max(1, tabSize));
                lines[i] = '\t'.repeat(tabs) + lines[i].slice(spaces);
            }
        }
    }
    return lines.join('\n');
}
/** Whitespace scanning within a line. */
function nextNonWhitespaceColumn(lineText, from = 0) {
    for (let i = Math.max(0, from); i < lineText.length; i++)
        if (lineText[i] !== ' ' && lineText[i] !== '\t')
            return i;
    return lineText.length;
}
function prevNonWhitespaceColumn(lineText, from) {
    for (let i = Math.min(lineText.length - 1, Math.max(0, from)); i >= 0; i--)
        if (lineText[i] !== ' ' && lineText[i] !== '\t')
            return i;
    return 0;
}
/** Word boundaries on a line. */
function wordBoundaries(lineText, col, re = /[A-Za-z0-9_]+/g) {
    const c = Math.max(0, Math.min(col, lineText.length));
    let m;
    re.lastIndex = 0;
    while ((m = re.exec(lineText))) {
        const s = m.index, e = s + m[0].length;
        if (s <= c && e >= c)
            return [s, e];
    }
    return null;
}
/** Simple bracket matching and selection expansion. */
const BRACKETS = new Map([
    ['(', ')'], ['[', ']'], ['{', '}']
]);
function findMatchingBracket(text, pos, starts = indexLineStarts(text)) {
    const [off] = rangeToOffsets(text, cursorAt(pos), starts);
    const open = text[off];
    const close = BRACKETS.get(open);
    if (close) {
        // Scan forward
        let depth = 0;
        for (let i = off; i < text.length; i++) {
            const ch = text[i];
            if (ch === open)
                depth++;
            else if (ch === close) {
                depth--;
                if (depth === 0)
                    return offsetsToRange(text, off, i + 1, starts);
            }
        }
        return null;
    }
    // Maybe we're on a closing bracket; scan backward
    for (const [o, c] of BRACKETS) {
        if (text[off] === c) {
            let depth = 0;
            for (let i = off; i >= 0; i--) {
                const ch = text[i];
                if (ch === c)
                    depth++;
                else if (ch === o) {
                    depth--;
                    if (depth === 0)
                        return offsetsToRange(text, i, off + 1, starts);
                }
            }
            return null;
        }
    }
    return null;
}
function expandSelectionToEnclosingBrackets(text, pos, starts = indexLineStarts(text)) {
    const r = findMatchingBracket(text, pos, starts);
    if (!r)
        return null;
    return makeRange(r.start.line, r.start.character + 1, r.end.line, Math.max(0, r.end.character - 1));
}
/** Lightweight line‑based diff to produce coarse TextEdits. */
function computeEditsByLine(oldText, newText) {
    if (oldText === newText)
        return [];
    const A = fastSplitLines(oldText);
    const B = fastSplitLines(newText);
    const n = A.length, m = B.length;
    // LCS (O(n*m)) is too heavy for big files; do a forward/backward trim and single replace in the middle.
    let start = 0;
    while (start < n && start < m && A[start] === B[start])
        start++;
    let endA = n - 1, endB = m - 1;
    while (endA >= start && endB >= start && A[endA] === B[endB]) {
        endA--;
        endB--;
    }
    const replacement = B.slice(start, endB + 1).join('\n');
    const range = makeRange(start, 0, endA + 1, 0); // end at column 0 of the line after endA
    return [edit(range, replacement)];
}
/** Strict edit application: throws on overlapping ranges. */
function applyEditsStrict(text, edits) {
    if (edits.length === 0)
        return text;
    const starts = indexLineStarts(text);
    const es = edits.map(e => ({ range: normalizeRange(e.range), newText: e.newText }))
        .sort((a, b) => comparePosition(a.range.start, b.range.start));
    for (let i = 1; i < es.length; i++) {
        if (comparePosition(es[i].range.start, es[i - 1].range.end) < 0) {
            throw new Error('Overlapping edits are not allowed in applyEditsStrict');
        }
    }
    let out = text;
    for (let i = es.length - 1; i >= 0; i--) {
        const a = offsetAt(es[i].range.start, starts);
        const b = offsetAt(es[i].range.end, starts);
        const lo = Math.max(0, Math.min(a, b));
        const hi = Math.max(a, b);
        out = out.slice(0, lo) + es[i].newText + out.slice(hi);
    }
    return out;
}
/** Smart minimal edits: prefer line‑block replace, else full replace. */
function computeMinimalSmartEdits(oldText, newText) {
    const byLine = computeEditsByLine(oldText, newText);
    // If edit covers whole doc, we might as well return a single full replace.
    if (byLine.length === 1) {
        const e = byLine[0];
        if (e.range.start.line === 0 && e.range.start.character === 0 && e.range.end.line >= countLines(oldText) - 1) {
            return [fullReplaceEdit(oldText, newText)];
        }
    }
    return byLine;
}
//# sourceMappingURL=text.js.map