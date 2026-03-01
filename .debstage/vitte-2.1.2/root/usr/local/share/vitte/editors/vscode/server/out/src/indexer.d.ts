import { TextDocument } from "vscode-languageserver-textdocument";
import type { DocumentSymbol as LspDocumentSymbol, SymbolInformation as LspSymbolInformation } from "vscode-languageserver-types";
export declare enum SK {
    Module = 2,
    Namespace = 3,
    Class = 5,
    Method = 6,
    Property = 7,
    Field = 8,
    Constructor = 9,
    Enum = 10,
    Interface = 11,
    Function = 12,
    Variable = 13,
    Constant = 14,
    Struct = 23,
    TypeParameter = 26
}
export interface IndexedSymbol {
    name: string;
    kind: SK;
    uri: string;
    line: number;
    character: number;
    endLine?: number;
    endCharacter?: number;
    containerName?: string;
}
type Uri = string;
export interface IndexSnapshot {
    version: number;
    entries: {
        uri: string;
        symbols: IndexedSymbol[];
    }[];
}
/** Indexe un document (remplace l’entrée précédente). */
export declare function indexDocument(doc: TextDocument): void;
/** Indexe une chaîne pour un uri donné. */
export declare function indexText(uri: string, text: string): void;
/** Réindexe un document (alias plus explicite). */
export declare function updateDocument(doc: TextDocument): void;
/** Supprime un document de l’index. */
export declare function removeDocument(uri: string): void;
/** Vide l’index. */
export declare function clearIndex(): void;
/** Récupère l’index brut (lecture seule). */
export declare function getIndex(): ReadonlyMap<Uri, IndexedSymbol[]>;
/** Récupère les symboles d’un document. */
export declare function getDocumentIndex(uri: string): IndexedSymbol[];
export declare function exportIndexSnapshot(): IndexSnapshot;
export declare function loadIndexSnapshot(snapshot: IndexSnapshot | null | undefined): number;
/** Recherche globale par fuzzy + préfixe, tri par score et nature. */
export declare function searchWorkspaceSymbols(query: string, limit?: number): IndexedSymbol[];
/** Conversion vers SymbolInformation[] (Workspace Symbols). */
export declare function toWorkspaceSymbols(syms: IndexedSymbol[]): LspSymbolInformation[];
/** Conversion hiérarchique DocumentSymbol[] pour un document. */
export declare function toDocumentSymbols(doc: TextDocument): LspDocumentSymbol[];
/** Renvoie les symboles à une position. */
export declare function symbolsAtPosition(uri: string, pos: {
    line: number;
    character: number;
}): IndexedSymbol[];
/** Définition naïve: symbole portant le même nom, priorisant le même fichier. */
export declare function findDefinition(uri: string, name: string): IndexedSymbol | undefined;
/** Références naïves: symboles du même nom dans l’index. */
export declare function findReferences(_uri: string, name: string, limit?: number): IndexedSymbol[];
export {};
