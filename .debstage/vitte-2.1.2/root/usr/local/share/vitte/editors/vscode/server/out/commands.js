"use strict";
/* --------------------------------------------------------------------------
 * commands.ts — gestion centralisée des commandes LSP (serveur)
 * Inspiré de C/C++, rust-analyzer, TypeScript
 * -------------------------------------------------------------------------- */
Object.defineProperty(exports, "__esModule", { value: true });
exports.VITTE_COMMANDS = void 0;
exports.registerCommands = registerCommands;
exports.buildExecuteCommandProvider = buildExecuteCommandProvider;
exports.logClientCaps = logClientCaps;
const node_1 = require("vscode-languageserver/node");
/* =========================================
 * Déclarations officielles des commandes
 * ========================================= */
exports.VITTE_COMMANDS = {
    SHOW_SERVER_LOG: "vitte.showServerLog",
    RESTART_SERVER: "vitte.restartServer",
    RUN_ACTION: "vitte.runAction",
    RUN_ACTION_ARGS: "vitte.runActionWithArgs",
    DEBUG_RUN_FILE: "vitte.debug.runFile",
    DEBUG_ATTACH: "vitte.debug.attachServer",
    FORMAT_DOC: "vitte.formatDocument",
    ORGANIZE_IMPORTS: "vitte.organizeImports",
    FIX_ALL: "vitte.fixAll",
    RENAME_SYMBOL: "vitte.renameSymbol",
    APPLY_EDIT_SAMPLE: "vitte.applyEditSample",
    PROGRESS_SAMPLE: "vitte.progressSample",
    SHOW_INFO: "vitte.showInfo"
};
function registerCommands(connection) {
    connection.onExecuteCommand(async (params) => {
        try {
            return await handleCommand(connection, params);
        }
        catch (err) {
            const message = formatError(err);
            connection.console.error(`Erreur commande ${params.command}: ${message}`);
            connection.window.showErrorMessage(`Commande "${params.command}": ${message}`);
            return null;
        }
    });
    return Object.values(exports.VITTE_COMMANDS);
}
/** À utiliser dans la réponse d'initialize: executeCommandProvider */
function buildExecuteCommandProvider() {
    return { commands: Object.values(exports.VITTE_COMMANDS) };
}
/** Optionnel: log des capabilities client */
function logClientCaps(params, connection) {
    const caps = params.capabilities;
    connection.console.info(`Client: ${params.clientInfo?.name ?? "unknown"} v${params.clientInfo?.version ?? "?"}`);
    connection.console.info(`Workspace edits supported: ${!!caps?.workspace}`);
}
/* =========================================
 * Dispatcher central
 * ========================================= */
async function handleCommand(connection, params) {
    const { command, arguments: rawArgs } = params;
    const args = Array.isArray(rawArgs) ? rawArgs : undefined;
    switch (command) {
        case exports.VITTE_COMMANDS.SHOW_SERVER_LOG:
            connection.console.info("📜 Vitte: ouverture du journal LSP (stub).");
            return ok();
        case exports.VITTE_COMMANDS.RESTART_SERVER:
            connection.console.warn("🔄 Vitte: demande de redémarrage du serveur (stub).");
            return ok();
        case exports.VITTE_COMMANDS.RUN_ACTION:
            connection.console.log("⚡ Vitte: exécution d’une action rapide générique.");
            return ok();
        case exports.VITTE_COMMANDS.RUN_ACTION_ARGS:
            connection.console.log(`⚡ Vitte: action avec args: ${safeJson(args)}`);
            return ok();
        case exports.VITTE_COMMANDS.DEBUG_RUN_FILE: {
            const uri = readStringArg(args, 0);
            return debugRunFile(connection, uri);
        }
        case exports.VITTE_COMMANDS.DEBUG_ATTACH: {
            const a = readObjectArg(args, 0) ?? {};
            return debugAttach(connection, a);
        }
        case exports.VITTE_COMMANDS.FORMAT_DOC: {
            const a = readFormatArgs(args, 0);
            return formatDocument(connection, a);
        }
        case exports.VITTE_COMMANDS.ORGANIZE_IMPORTS: {
            const uri = readUri(args, 0);
            return organizeImports(connection, uri);
        }
        case exports.VITTE_COMMANDS.FIX_ALL: {
            const uri = readUri(args, 0);
            return fixAllProblems(connection, uri);
        }
        case exports.VITTE_COMMANDS.RENAME_SYMBOL: {
            const a = readObjectArg(args, 0);
            if (!a?.uri || !a?.newName) {
                throw new Error("renameSymbol: args invalides. Attendu { uri, newName }.");
            }
            return renameSymbol(connection, a.uri, a.newName);
        }
        case exports.VITTE_COMMANDS.APPLY_EDIT_SAMPLE: {
            const a = readObjectArg(args, 0);
            if (!a?.uri)
                throw new Error("applyEditSample: uri manquant.");
            return applyEditSample(connection, a.uri, a.headerText);
        }
        case exports.VITTE_COMMANDS.PROGRESS_SAMPLE:
            return progressSample(connection);
        case exports.VITTE_COMMANDS.SHOW_INFO: {
            const msg = readStringArg(args, 0) ?? "Vitte: message d'information.";
            connection.window.showInformationMessage(msg);
            return ok();
        }
        default:
            connection.console.warn(`Commande non reconnue: ${command}`);
            return null;
    }
}
/* =========================================
 * Implémentations (stubs réalistes)
 * ========================================= */
function debugRunFile(connection, uri) {
    if (!uri) {
        connection.console.error("🐞 debug.runFile: aucun fichier fourni.");
        return fail();
    }
    connection.console.log(`🐞 Lancement debug sur fichier: ${uri}`);
    return ok();
}
function debugAttach(connection, a) {
    const host = a.host ?? "127.0.0.1";
    const port = a.port ?? 6009;
    const timeout = a.timeoutMs ?? 10000;
    connection.console.log(`🐞 Attach au serveur debug ${host}:${port} (timeout ${timeout}ms).`);
    return ok();
}
/** Formatage: exemple d’édition — insère un header au début du fichier ou de la plage */
async function formatDocument(connection, arg) {
    if (!arg)
        return ok();
    const { uri, range } = normalizeFormatArgs(arg);
    const where = range ? range.start : node_1.Position.create(0, 0);
    const edit = {
        changes: {
            [uri]: [node_1.TextEdit.insert(where, "// formatted by Vitte LSP\n")]
        }
    };
    await connection.workspace.applyEdit(edit);
    return ok();
}
/** Organize imports: squelette — à remplacer par analyse réelle */
async function organizeImports(connection, uri) {
    if (!uri)
        return ok();
    connection.console.log(`📦 Organize imports sur ${uri}`);
    const edit = {
        changes: {
            [uri]: [
                node_1.TextEdit.replace(node_1.Range.create(node_1.Position.create(0, 0), node_1.Position.create(0, Number.MAX_SAFE_INTEGER)), "import std.modules.core\n")
            ]
        }
    };
    await connection.workspace.applyEdit(edit);
    return ok();
}
/** Fix all: correctif générique d’exemple */
async function fixAllProblems(connection, uri) {
    if (!uri)
        return ok();
    connection.console.log(`🛠 Fix all sur ${uri}`);
    const edit = {
        changes: { [uri]: [node_1.TextEdit.insert(node_1.Position.create(0, 0), "// fix-all applied\n")] }
    };
    await connection.workspace.applyEdit(edit);
    return ok();
}
/** Rename: démo — remplacer par une indexation et documentChanges réels */
async function renameSymbol(connection, uri, newName) {
    connection.console.log(`✏️ Rename dans ${uri} → ${newName}`);
    const edit = {
        changes: { [uri]: [node_1.TextEdit.insert(node_1.Position.create(0, 0), `// rename to: ${newName}\n`)] }
    };
    await connection.workspace.applyEdit(edit);
    return ok();
}
/** Exemple d’édition paramétrable via settings (headerText) */
async function applyEditSample(connection, uri, headerText) {
    const text = headerText ?? "// edited by Vitte LSP\n";
    const edit = {
        changes: { [uri]: [node_1.TextEdit.insert(node_1.Position.create(0, 0), text)] }
    };
    await connection.workspace.applyEdit(edit);
    return ok();
}
/** Démo de progression serveur → client */
async function progressSample(connection) {
    const progress = await connection.window.createWorkDoneProgress();
    progress.begin("Vitte: préparation", 0, "initialisation…", true);
    await delay(300);
    progress.report(25, "analyse…");
    await delay(300);
    progress.report(60, "application des changements…");
    await delay(300);
    progress.report(90, "presque terminé…");
    await delay(200);
    progress.done();
    connection.window.showInformationMessage("Vitte: tâche terminée.");
    return ok();
}
/* =========================================
 * Utilitaires génériques
 * ========================================= */
function readStringArg(args, index) {
    const v = args?.[index];
    return typeof v === "string" ? v : undefined;
}
function readObjectArg(args, index) {
    const v = args?.[index];
    return isRecord(v) ? v : undefined;
}
function readUri(args, index) {
    const uri = readStringArg(args, index);
    if (!uri)
        return undefined;
    if (uri.startsWith("file://") || uri.includes("://"))
        return uri;
    return "file://" + uri;
}
function readFormatArgs(args, index) {
    const v = args?.[index];
    if (typeof v === "string")
        return v;
    if (isRecord(v) && typeof v.uri === "string") {
        const candidateRange = v.range;
        const range = isRangeLike(candidateRange) ? candidateRange : undefined;
        return { uri: v.uri, range };
    }
    return undefined;
}
function normalizeFormatArgs(arg) {
    if (typeof arg === "string")
        return { uri: arg };
    const uri = arg.uri;
    const r = arg.range
        ? node_1.Range.create(node_1.Position.create(arg.range.start.line, arg.range.start.character), node_1.Position.create(arg.range.end.line, arg.range.end.character))
        : undefined;
    return { uri, range: r };
}
function safeJson(v) {
    try {
        return JSON.stringify(v);
    }
    catch {
        return String(v);
    }
}
function formatError(err) {
    if (err instanceof Error && typeof err.message === "string") {
        return err.message;
    }
    return String(err);
}
function delay(ms) {
    return new Promise(r => setTimeout(r, ms));
}
function ok() {
    return { ok: true };
}
function fail() {
    return { ok: false };
}
function isRecord(value) {
    return typeof value === "object" && value !== null;
}
function isPosLike(value) {
    return (isRecord(value) &&
        typeof value.line === "number" &&
        typeof value.character === "number");
}
function isRangeLike(value) {
    return (isRecord(value) &&
        isPosLike(value.start) &&
        isPosLike(value.end));
}
//# sourceMappingURL=commands.js.map