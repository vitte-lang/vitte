
/**
 * Vitte Language Server — Formatting Helpers
 * ------------------------------------------
 * Pure deterministic text formatter with no LSP side effects.
 * Provides whitespace normalization, indentation policy enforcement,
 * EOL normalization, and final newline management.
 */

export type EolKind = 'lf' | 'crlf' | 'auto';

export interface FormattingSettings {
  tabSize: number;
  insertSpaces: boolean;
  trimTrailingWhitespace: boolean;
  insertFinalNewline: boolean;
  eol: EolKind;
  detectMixedIndentation?: boolean;
}

export const defaultFormatting: FormattingSettings = {
  tabSize: 2,
  insertSpaces: true,
  trimTrailingWhitespace: true,
  insertFinalNewline: true,
  eol: 'lf',
  detectMixedIndentation: true,
};

/**
 * Detects indentation style in a given text sample.
 * Returns true if spaces are dominant, false if tabs are dominant.
 */
export function detectIndentationStyle(text: string): boolean {
  const lines = text.split(/\r?\n/);
  let spaceCount = 0;
  let tabCount = 0;
  for (const line of lines) {
    if (/^\s*$/.test(line)) continue;
    const m = line.match(/^(\s+)/);
    if (m) {
      if (m[1].includes('\t')) tabCount++;
      else if (m[1].includes(' ')) spaceCount++;
    }
  }
  return spaceCount >= tabCount;
}

/**
 * Normalize EOLs in text to desired format.
 */
export function normalizeEol(text: string, eol: EolKind): string {
  if (eol === 'auto') return text;
  if (eol === 'crlf') {
    return text.replace(/\r\n/g, '\n').replace(/\n/g, '\r\n');
  }
  // default 'lf'
  return text.replace(/\r\n/g, '\n');
}

/**
 * Apply indentation and trailing space policy.
 */
export function applyWhitespacePolicy(lines: string[], opts: FormattingSettings): string[] {
  const tabUnit = opts.insertSpaces ? ' '.repeat(Math.max(1, opts.tabSize)) : '\t';
  return lines.map((line) => {
    if (opts.insertSpaces) {
      // Replace leading tabs by configured number of spaces
      const matchTabs = line.match(/^\t+/)?.[0] ?? '';
      if (matchTabs.length > 0) {
        line = matchTabs.split('').map(() => tabUnit).join('') + line.slice(matchTabs.length);
      }
    } else {
      // Replace groups of spaces by tabs
      const re = new RegExp(`^(?: {${opts.tabSize}})+`);
      const m = line.match(re);
      if (m) {
        const spaces = m[0].length;
        const tabs = Math.floor(spaces / opts.tabSize);
        line = '\t'.repeat(tabs) + line.slice(spaces);
      }
    }

    // Remove trailing spaces/tabs if configured
    if (opts.trimTrailingWhitespace) {
      line = line.replace(/[ \t]+$/g, '');
    }
    return line;
  });
}

/**
 * Determines appropriate EOL character sequence for a text sample.
 */
export function detectEol(text: string): EolKind {
  const crlfCount = (text.match(/\r\n/g) || []).length;
  const lfCount = (text.match(/(?<!\r)\n/g) || []).length;
  if (crlfCount === 0 && lfCount === 0) return 'lf';
  if (crlfCount > lfCount) return 'crlf';
  return 'lf';
}

/**
 * Performs deterministic format of provided text according to settings.
 * @param text - Original text
 * @param opts - Formatting settings
 * @param isFragment - Whether text is a fragment or full document
 * @returns Formatted text
 */
export function formatText(text: string, opts: FormattingSettings, isFragment = false): string {
  if (typeof text !== 'string') return '';

  // Optionally detect indentation
  let cfg = { ...opts };
  if (cfg.detectMixedIndentation && text.match(/\t/) && text.match(/^ +/m)) {
    cfg.insertSpaces = detectIndentationStyle(text);
  }

  // Normalize input line endings to LF for consistent handling
  let work = text.replace(/\r\n/g, '\n');
  const lines = work.split('\n');

  // Apply whitespace and indentation normalization
  const processed = applyWhitespacePolicy(lines, cfg);
  work = processed.join('\n');

  // Normalize EOLs to target style
  work = normalizeEol(work, cfg.eol);

  // Ensure trailing newline for full documents
  if (!isFragment && cfg.insertFinalNewline) {
    const ending = cfg.eol === 'crlf' ? '\r\n' : '\n';
    if (!work.endsWith(ending)) work += ending;
  }

  return work;
}

/**
 * Compare two formatted texts for equality ignoring EOL differences.
 */
export function isSameFormat(a: string, b: string): boolean {
  return a.replace(/\r\n/g, '\n') === b.replace(/\r\n/g, '\n');
}

/**
 * Utility to quickly preview what would change by formatting.
 */
export function diffPreview(before: string, after: string, contextLines = 2): string {
  const beforeLines = before.split(/\r?\n/);
  const afterLines = after.split(/\r?\n/);
  const diffs: string[] = [];
  const max = Math.max(beforeLines.length, afterLines.length);
  for (let i = 0; i < max; i++) {
    const a = beforeLines[i] ?? '';
    const b = afterLines[i] ?? '';
    if (a !== b) {
      const start = Math.max(0, i - contextLines);
      const end = Math.min(max, i + contextLines + 1);
      diffs.push(`--- difference around line ${i + 1} ---`);
      for (let j = start; j < end; j++) {
        const beforeLine = beforeLines[j] ?? '';
        const afterLine = afterLines[j] ?? '';
        if (beforeLine !== afterLine) {
          diffs.push(`- ${beforeLine}`);
          diffs.push(`+ ${afterLine}`);
        } else {
          diffs.push(`  ${beforeLine}`);
        }
      }
    }
  }
  return diffs.join('\n');
}
