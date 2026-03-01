import type { TextDocument } from "vscode-languageserver-textdocument";

function isAttributeLine(line: string): boolean {
  return /^\s*#\s*\[.*\]\s*$/.test(line);
}

function isLineDoc(line: string): { ok: boolean; text: string } {
  const m = /^\s*(?:\/\/\/|##)\s?(.*)$/.exec(line);
  if (!m) return { ok: false, text: "" };
  return { ok: true, text: m[1] ?? "" };
}

function extractBlockDoc(lines: string[], start: number, end: number): string {
  const out: string[] = [];
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

export function getDocstringAtLine(doc: TextDocument, declLine: number): string | null {
  const lines = doc.getText().split(/\r?\n/);
  let i = Math.min(declLine - 1, lines.length - 1);
  const collected: string[] = [];
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
      while (j >= 0 && !lines[j].includes("/**")) j--;
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
