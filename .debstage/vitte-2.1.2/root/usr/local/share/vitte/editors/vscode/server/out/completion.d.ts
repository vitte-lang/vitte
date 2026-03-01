import type { CompletionItem, Position } from "vscode-languageserver/node";
import type { TextDocument } from "vscode-languageserver-textdocument";
export declare function provideCompletions(doc: TextDocument, position: Position): CompletionItem[];
export declare function resolveCompletion(item: CompletionItem): CompletionItem;
export declare function triggerCharacters(): string[];
