import type { TextDocument } from "vscode-languageserver-textdocument";
import { Location, Position, Range, SymbolKind } from "vscode-languageserver/node";
import type { DocumentSymbol, WorkspaceSymbol } from "vscode-languageserver/node";
interface FlatSymbol {
    name: string;
    kind: SymbolKind;
    range: Range;
    selectionRange: Range;
    containerName?: string;
}
interface DocIndex {
    flat: FlatSymbol[];
    outline: DocumentSymbol[];
    byName: Map<string, FlatSymbol[]>;
}
export declare function documentSymbols(doc: TextDocument): DocumentSymbol[];
export declare function symbolOutline(doc: TextDocument): DocumentSymbol[];
export declare function definitionAtPosition(doc: TextDocument, pos: Position, uri: string): Location[];
export declare function referencesAtPosition(doc: TextDocument, pos: Position, uri: string): Location[];
export declare function prepareRename(doc: TextDocument, pos: Position): {
    range: Range;
    placeholder: string;
} | null;
export declare function renameSymbol(doc: TextDocument, pos: Position, newName: string): {
    range: Range;
    newText: string;
}[];
export declare function workspaceSymbols(query: string, openDocs: {
    uri: string;
    doc: TextDocument;
}[], limit?: number): WorkspaceSymbol[];
export declare function indexDocForTests(doc: TextDocument): DocIndex;
export {};
