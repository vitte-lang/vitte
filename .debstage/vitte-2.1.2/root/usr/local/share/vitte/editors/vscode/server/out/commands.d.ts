import type { Connection, InitializeParams } from "vscode-languageserver/node";
export declare const VITTE_COMMANDS: {
    readonly SHOW_SERVER_LOG: "vitte.showServerLog";
    readonly RESTART_SERVER: "vitte.restartServer";
    readonly RUN_ACTION: "vitte.runAction";
    readonly RUN_ACTION_ARGS: "vitte.runActionWithArgs";
    readonly DEBUG_RUN_FILE: "vitte.debug.runFile";
    readonly DEBUG_ATTACH: "vitte.debug.attachServer";
    readonly FORMAT_DOC: "vitte.formatDocument";
    readonly ORGANIZE_IMPORTS: "vitte.organizeImports";
    readonly FIX_ALL: "vitte.fixAll";
    readonly RENAME_SYMBOL: "vitte.renameSymbol";
    readonly APPLY_EDIT_SAMPLE: "vitte.applyEditSample";
    readonly PROGRESS_SAMPLE: "vitte.progressSample";
    readonly SHOW_INFO: "vitte.showInfo";
};
export type CommandId = typeof VITTE_COMMANDS[keyof typeof VITTE_COMMANDS];
export declare function registerCommands(connection: Connection): CommandId[];
/** À utiliser dans la réponse d'initialize: executeCommandProvider */
export declare function buildExecuteCommandProvider(): {
    commands: string[];
};
/** Optionnel: log des capabilities client */
export declare function logClientCaps(params: InitializeParams, connection: Connection): void;
