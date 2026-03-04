"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.registerAdvancedCodeActions = registerAdvancedCodeActions;
const vscode = __importStar(require("vscode"));
function toSnakeCase(name) {
    return name
        .replace(/([a-z0-9])([A-Z])/g, "$1_$2")
        .replace(/[-\s]+/g, "_")
        .replace(/__+/g, "_")
        .toLowerCase();
}
function transformImports(text) {
    const lines = text.replace(/\r\n/g, "\n").split("\n");
    const importRows = [];
    const kept = [];
    for (const line of lines) {
        if (/^\s*(import|use|pull)\b/.test(line))
            importRows.push(line.trim());
        else
            kept.push(line);
    }
    if (importRows.length === 0)
        return text;
    const sorted = Array.from(new Set(importRows)).sort((a, b) => a.localeCompare(b));
    const newText = [...sorted, "", ...kept].join("\n").replace(/\n{3,}/g, "\n\n");
    return text.endsWith("\n") && !newText.endsWith("\n") ? `${newText}\n` : newText;
}
function transformStyle(text) {
    return text
        .replace(/\t/g, "  ")
        .split(/\r?\n/)
        .map((l) => l.replace(/[ \t]+$/g, ""))
        .join("\n");
}
function transformNaming(text) {
    const renames = new Map();
    const declRx = /\b(?:proc|fn|let|const|static)\s+(?:mut\s+)?([A-Za-z_]\w*)/g;
    let m;
    while ((m = declRx.exec(text))) {
        const oldName = m[1];
        if (!oldName)
            continue;
        const snake = toSnakeCase(oldName);
        if (oldName !== snake && /^[A-Za-z_]\w*$/.test(snake)) {
            renames.set(oldName, snake);
        }
        if (m[0].length === 0)
            declRx.lastIndex++;
    }
    let out = text;
    for (const [oldName, newName] of renames) {
        out = out.replace(new RegExp(`(?<![A-Za-z0-9_])${oldName}(?![A-Za-z0-9_])`, "g"), newName);
    }
    return out;
}
function transformContracts(text) {
    if (/^\s*<<<\s+ROLE-CONTRACT\b/m.test(text))
        return text;
    const pkg = /^\s*(?:space|module)\s+([A-Za-z_][\w./:-]*)/m.exec(text)?.[1] ?? "my/package";
    const block = [
        "",
        "<<< ROLE-CONTRACT",
        `package: ${pkg}`,
        "role: Responsibility",
        "input_contract: Explicit normalized inputs",
        "output_contract: Stable explicit outputs",
        "boundary: No business policy decisions",
        ">>>",
        "",
    ].join("\n");
    return text.endsWith("\n") ? `${text}${block}` : `${text}\n${block}`;
}
function applyCategory(category, text) {
    switch (category) {
        case "imports": return transformImports(text);
        case "style": return transformStyle(text);
        case "naming": return transformNaming(text);
        case "contracts": return transformContracts(text);
    }
}
async function previewAndApplyCategory(category, doc) {
    const before = doc.getText();
    const after = applyCategory(category, before);
    if (after === before) {
        void vscode.window.showInformationMessage(`Vitte ${category}: no changes.`);
        return;
    }
    const tmp = await vscode.workspace.openTextDocument({ content: after, language: doc.languageId });
    await vscode.commands.executeCommand("vscode.diff", doc.uri, tmp.uri, `Vitte ${category} preview: ${vscode.workspace.asRelativePath(doc.uri, false)}`);
    const pick = await vscode.window.showInformationMessage(`Apply Vitte ${category} fixes?`, "Apply", "Cancel");
    if (pick !== "Apply")
        return;
    const edit = new vscode.WorkspaceEdit();
    const full = new vscode.Range(0, 0, doc.lineCount, 0);
    edit.replace(doc.uri, full, after);
    await vscode.workspace.applyEdit(edit);
    await doc.save();
}
function categoryTitle(category) {
    switch (category) {
        case "imports": return "imports";
        case "style": return "style";
        case "naming": return "naming";
        case "contracts": return "contracts";
    }
}
function registerAdvancedCodeActions(context) {
    const provider = {
        provideCodeActions(document, _range, ctx) {
            if (!["vitte", "vit"].includes(document.languageId))
                return [];
            const actions = [];
            const addCategory = (category, kind) => {
                const action = new vscode.CodeAction(`Vitte: Fix all ${categoryTitle(category)} (preview diff)`, kind);
                action.command = {
                    command: "vitte.fixCategoryPreview",
                    title: "Vitte fix category preview",
                    arguments: [category, document.uri],
                };
                action.isPreferred = category === "imports";
                actions.push(action);
            };
            addCategory("imports", vscode.CodeActionKind.SourceFixAll.append("vitte.imports"));
            addCategory("style", vscode.CodeActionKind.SourceFixAll.append("vitte.style"));
            addCategory("naming", vscode.CodeActionKind.SourceFixAll.append("vitte.naming"));
            addCategory("contracts", vscode.CodeActionKind.SourceFixAll.append("vitte.contracts"));
            for (const d of ctx.diagnostics) {
                const code = String(d.code ?? "");
                if (!code.startsWith("DOCTOR_"))
                    continue;
                const tool = code.replace(/^DOCTOR_/, "").toLowerCase();
                const action = new vscode.CodeAction(`Vitte Doctor: rerun ${tool}`, vscode.CodeActionKind.QuickFix);
                action.diagnostics = [d];
                action.command = { command: "vitte.doctor.runTool", title: "Run Doctor tool", arguments: [tool] };
                actions.push(action);
            }
            return actions;
        },
    };
    context.subscriptions.push(vscode.languages.registerCodeActionsProvider([{ language: "vitte" }, { language: "vit" }], provider, {
        providedCodeActionKinds: [
            vscode.CodeActionKind.QuickFix,
            vscode.CodeActionKind.SourceFixAll.append("vitte.imports"),
            vscode.CodeActionKind.SourceFixAll.append("vitte.style"),
            vscode.CodeActionKind.SourceFixAll.append("vitte.naming"),
            vscode.CodeActionKind.SourceFixAll.append("vitte.contracts"),
        ],
    }), vscode.commands.registerCommand("vitte.fixCategoryPreview", async (category, uri) => {
        if (!uri)
            return;
        const doc = await vscode.workspace.openTextDocument(uri);
        await previewAndApplyCategory(category, doc);
    }));
}
//# sourceMappingURL=advancedCodeActions.js.map