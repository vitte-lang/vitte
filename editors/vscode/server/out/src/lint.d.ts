import type { Diagnostic } from "vscode-languageserver/node";
export interface LintOptions {
    maxLineLength?: number;
    allowTabs?: boolean;
    allowTrailingWhitespace?: boolean;
}
export declare function lintText(text: string, uri?: string, opts?: LintOptions): Diagnostic[];
export declare function lintToPublishable(text: string, uri: string, options?: LintOptions): Diagnostic[];
