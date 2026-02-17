import type { Position, Hover, SemanticTokensLegend, SemanticTokens } from "vscode-languageserver/node";
import type { TextDocument } from "vscode-languageserver-textdocument";
export declare function getSemanticTokensLegend(): SemanticTokensLegend;
export declare function provideHover(doc: TextDocument, position: Position): Hover | null;
export declare function buildSemanticTokens(doc: TextDocument): SemanticTokens;
