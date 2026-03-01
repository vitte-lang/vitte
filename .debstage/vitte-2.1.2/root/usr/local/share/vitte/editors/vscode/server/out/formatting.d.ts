import { TextEdit } from "vscode-languageserver";
import type { FormattingOptions } from "vscode-languageserver";
import type { TextDocument } from "vscode-languageserver-textdocument";
export interface ExtraFormattingOptions extends FormattingOptions {
    trimTrailingWhitespace?: boolean;
    insertFinalNewline?: boolean;
    trimFinalNewlines?: boolean;
    normalizeEOL?: "lf" | "crlf";
    maxConsecutiveBlankLines?: number;
    ensureSpaceAroundOperators?: boolean;
    spaceAfterComma?: boolean;
    spaceAroundColon?: "none" | "left" | "right" | "both";
    normalizeQuotes?: "preserve" | "double" | "single";
    keepIndentInsideStrings?: boolean;
    alignInlineComments?: boolean;
    alignEquals?: boolean;
    braceStyle?: "attach" | "break";
    newlineBeforeElse?: boolean;
    wrapCommentsAt?: number;
}
export declare function provideFormattingEdits(doc: TextDocument, options?: ExtraFormattingOptions): TextEdit[];
export declare function formatDocument(doc: TextDocument, options?: ExtraFormattingOptions): TextEdit[];
