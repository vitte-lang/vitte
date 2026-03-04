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
exports.registerVitteCodeLens = registerVitteCodeLens;
const vscode = __importStar(require("vscode"));
function isVitteDoc(doc) {
    return doc.languageId === "vitte" || doc.languageId === "vit";
}
function wordAt(doc, pos) {
    return doc.getText(doc.getWordRangeAtPosition(pos));
}
function registerVitteCodeLens(context) {
    const provider = {
        provideCodeLenses(document) {
            if (!isVitteDoc(document))
                return [];
            const lenses = [];
            const text = document.getText();
            const lines = text.split(/\r?\n/);
            for (let i = 0; i < lines.length; i++) {
                const line = lines[i] ?? "";
                const decl = /^\s*(?:proc|fn|entry)\s+([A-Za-z_]\w*)/.exec(line);
                if (decl) {
                    const name = decl[1] ?? "symbol";
                    const idx = decl[1] ? line.indexOf(decl[1]) : 0;
                    const start = new vscode.Position(i, 0);
                    const range = new vscode.Range(start, start);
                    lenses.push(new vscode.CodeLens(range, {
                        command: "vitte.codelens.runProc",
                        title: `Run ${name}`,
                        arguments: [document.uri, name],
                    }), new vscode.CodeLens(range, {
                        command: "vitte.codelens.runTest",
                        title: `Run tests`,
                        arguments: [document.uri, name],
                    }), new vscode.CodeLens(range, {
                        command: "vitte.codelens.openRefs",
                        title: "Open refs",
                        arguments: [document.uri, new vscode.Position(i, Math.max(0, idx))],
                    }));
                }
                const codeRx = /\bVITTE-[A-Z0-9][A-Z0-9_-]*\b/g;
                let m;
                while ((m = codeRx.exec(line))) {
                    const code = m[0];
                    const pos = new vscode.Position(i, m.index);
                    const range = new vscode.Range(pos, pos);
                    lenses.push(new vscode.CodeLens(range, {
                        command: "vitte.codelens.explainCode",
                        title: `Explain ${code}`,
                        arguments: [code],
                    }));
                    if (m[0].length === 0)
                        codeRx.lastIndex++;
                }
            }
            return lenses;
        },
    };
    context.subscriptions.push(vscode.languages.registerCodeLensProvider([{ language: "vitte" }, { language: "vit" }], provider), vscode.commands.registerCommand("vitte.codelens.runProc", async (uri) => {
        const editor = vscode.window.visibleTextEditors.find((e) => e.document.uri.toString() === uri?.toString());
        if (editor)
            await vscode.window.showTextDocument(editor.document, { preview: false, preserveFocus: true });
        await vscode.commands.executeCommand("vitte.debug.runFile");
    }), vscode.commands.registerCommand("vitte.codelens.runTest", async (uri) => {
        const editor = vscode.window.visibleTextEditors.find((e) => e.document.uri.toString() === uri?.toString());
        if (editor)
            await vscode.window.showTextDocument(editor.document, { preview: false, preserveFocus: true });
        await vscode.commands.executeCommand("vitte.testCurrent");
    }), vscode.commands.registerCommand("vitte.codelens.openRefs", async (uri, pos) => {
        const refs = await vscode.commands.executeCommand("vscode.executeReferenceProvider", uri, pos);
        const locations = refs ?? [];
        const curDoc = await vscode.workspace.openTextDocument(uri);
        const word = wordAt(curDoc, pos) ?? "symbol";
        await vscode.commands.executeCommand("editor.action.showReferences", uri, pos, locations);
        if (locations.length === 0) {
            void vscode.window.showInformationMessage(`No references for '${word}'.`);
        }
    }), vscode.commands.registerCommand("vitte.codelens.explainCode", async (code) => {
        if (!code)
            return;
        const url = `https://vitte.netlify.app/pages/errors/?q=${encodeURIComponent(code)}`;
        const pick = await vscode.window.showInformationMessage(`${code}: open docs explanation?`, "Open Docs", "Copy Code");
        if (pick === "Open Docs") {
            await vscode.env.openExternal(vscode.Uri.parse(url));
        }
        else if (pick === "Copy Code") {
            await vscode.env.clipboard.writeText(code);
        }
    }));
}
//# sourceMappingURL=vitteCodeLens.js.map