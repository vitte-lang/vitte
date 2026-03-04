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
exports.registerPackageProblemsView = registerPackageProblemsView;
const vscode = __importStar(require("vscode"));
const diagnostics_1 = require("../utils/diagnostics");
class PackageSummaryNode extends vscode.TreeItem {
    constructor(data) {
        super(data.dir || "(root)", vscode.TreeItemCollapsibleState.None);
        this.data = data;
        this.contextValue = "vittePackageProblems";
        this.description = `${data.errors}e ${data.warnings}w (${data.files} files)`;
        this.tooltip = [
            `Package: ${data.dir || "(root)"}`,
            `Errors: ${data.errors}`,
            `Warnings: ${data.warnings}`,
            `Info: ${data.info}`,
            `Hints: ${data.hints}`,
            `Files: ${data.files}`,
        ].join("\n");
        this.iconPath = data.errors > 0
            ? new vscode.ThemeIcon("error")
            : data.warnings > 0
                ? new vscode.ThemeIcon("warning")
                : new vscode.ThemeIcon("pass-filled");
        this.command = {
            command: "vitte.packageProblems.open",
            title: "Open package problems",
            arguments: [data.dir],
        };
    }
}
class PackageProblemsProvider {
    constructor() {
        this.emitter = new vscode.EventEmitter();
        this.onDidChangeTreeData = this.emitter.event;
        this.nodes = [];
    }
    refreshSoon() {
        if (this.refreshTimer)
            clearTimeout(this.refreshTimer);
        this.refreshTimer = setTimeout(() => {
            this.refreshTimer = undefined;
            this.refresh();
        }, 220);
    }
    refresh() {
        const rows = (0, diagnostics_1.summarizeDiagnosticsByDirectory)().map((d) => new PackageSummaryNode(d));
        this.nodes = rows;
        this.emitter.fire(undefined);
    }
    getTreeItem(element) {
        return element;
    }
    getChildren() {
        return this.nodes;
    }
    dispose() {
        if (this.refreshTimer)
            clearTimeout(this.refreshTimer);
        this.emitter.dispose();
    }
}
function inDir(relPath, dir) {
    if (!dir)
        return true;
    return relPath === dir || relPath.startsWith(`${dir}/`);
}
function registerPackageProblemsView(context) {
    const provider = new PackageProblemsProvider();
    const tree = vscode.window.createTreeView("vittePackageProblems", {
        treeDataProvider: provider,
        showCollapseAll: false,
    });
    const refresh = () => provider.refreshSoon();
    provider.refresh();
    context.subscriptions.push(provider, tree, vscode.languages.onDidChangeDiagnostics(refresh), vscode.workspace.onDidSaveTextDocument(refresh), vscode.workspace.onDidOpenTextDocument(refresh), vscode.workspace.onDidCloseTextDocument(refresh), vscode.commands.registerCommand("vitte.packageProblems.refresh", () => provider.refresh()), vscode.commands.registerCommand("vitte.packageProblems.open", async (dir) => {
        const all = vscode.languages.getDiagnostics();
        let candidate;
        for (const [uri, list] of all) {
            const rel = vscode.workspace.asRelativePath(uri, false).replace(/\\/g, "/");
            if (!inDir(rel.includes("/") ? rel.slice(0, rel.lastIndexOf("/")) : "", dir ?? ""))
                continue;
            for (const d of list) {
                const sev = d.severity ?? vscode.DiagnosticSeverity.Hint;
                if (!candidate || sev < candidate.sev) {
                    candidate = { uri, range: d.range, sev };
                }
            }
        }
        if (!candidate)
            return;
        const doc = await vscode.workspace.openTextDocument(candidate.uri);
        const editor = await vscode.window.showTextDocument(doc, { preview: true });
        editor.revealRange(candidate.range, vscode.TextEditorRevealType.InCenter);
        editor.selection = new vscode.Selection(candidate.range.start, candidate.range.start);
    }));
}
//# sourceMappingURL=packageProblemsView.js.map