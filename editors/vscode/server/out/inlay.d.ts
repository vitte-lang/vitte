import { InlayHint, Range } from "vscode-languageserver/node";
import type { TextDocument } from "vscode-languageserver-textdocument";
export interface InlayHintPrefs {
    parameterHints: boolean;
    typeHints: boolean;
    returnHints: boolean;
    aliasHints: boolean;
}
export declare function provideInlayHints(doc: TextDocument, range: Range, prefs?: Partial<InlayHintPrefs>): InlayHint[];
