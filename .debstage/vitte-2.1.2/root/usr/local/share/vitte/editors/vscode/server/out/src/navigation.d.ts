import type { TextDocument } from "vscode-languageserver-textdocument";
import { Location, Position, Range, SymbolKind } from "vscode-languageserver/node";
import type { CallHierarchyIncomingCall, CallHierarchyItem, CallHierarchyOutgoingCall, DocumentSymbol, TypeHierarchyItem, TypeHierarchySubtypesParams, TypeHierarchySupertypesParams, WorkspaceSymbol } from "vscode-languageserver/node";
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
export declare function renameIdentifierByName(doc: TextDocument, oldName: string, newName: string): {
    range: Range;
    newText: string;
}[];
export declare function tokenAtPosition(doc: TextDocument, pos: Position): string | null;
export declare function prepareCallHierarchy(doc: TextDocument, pos: Position, uri: string): CallHierarchyItem[];
export declare function callHierarchyIncoming(doc: TextDocument, item: CallHierarchyItem): CallHierarchyIncomingCall[];
export declare function callHierarchyOutgoing(doc: TextDocument, item: CallHierarchyItem): CallHierarchyOutgoingCall[];
export declare function prepareTypeHierarchy(doc: TextDocument, pos: Position, uri: string): TypeHierarchyItem[];
export declare function typeHierarchySupertypes(doc: TextDocument, params: TypeHierarchySupertypesParams): TypeHierarchyItem[];
export declare function typeHierarchySubtypes(doc: TextDocument, params: TypeHierarchySubtypesParams): TypeHierarchyItem[];
export declare function workspaceSymbols(query: string, openDocs: {
    uri: string;
    doc: TextDocument;
}[], limit?: number): WorkspaceSymbol[];
export declare function indexDocForTests(doc: TextDocument): DocIndex;
export {};
