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
exports.registerDocsChaptersView = registerDocsChaptersView;
const vscode = __importStar(require("vscode"));
const documentationChapters_1 = require("./documentationChapters");
class DocsChaptersProvider {
    constructor() {
        this._onDidChangeTreeData = new vscode.EventEmitter();
        this.onDidChangeTreeData = this._onDidChangeTreeData.event;
    }
    dispose() {
        this._onDidChangeTreeData.dispose();
    }
    refresh() {
        this._onDidChangeTreeData.fire();
    }
    getTreeItem(element) {
        if (element.kind === "section") {
            const item = new vscode.TreeItem(element.title, vscode.TreeItemCollapsibleState.Expanded);
            item.iconPath = new vscode.ThemeIcon("book");
            return item;
        }
        const item = new vscode.TreeItem(element.title, vscode.TreeItemCollapsibleState.None);
        item.iconPath = new vscode.ThemeIcon("file-text");
        item.command = {
            command: "vitte.docs.openChapter",
            title: "Open Vitte Chapter",
            arguments: [element.url]
        };
        item.tooltip = element.url;
        return item;
    }
    getChildren(element) {
        if (!element) {
            return documentationChapters_1.DOC_SECTIONS.map((section) => ({ kind: "section", title: section.title }));
        }
        if (element.kind === "section") {
            const section = documentationChapters_1.DOC_SECTIONS.find((entry) => entry.title === element.title);
            if (!section)
                return [];
            return section.chapters.map((chapter) => ({
                kind: "chapter",
                title: chapter.title,
                url: (0, documentationChapters_1.makeDocUrl)(chapter.path)
            }));
        }
        return [];
    }
}
function registerDocsChaptersView(context, viewId) {
    const provider = new DocsChaptersProvider();
    const tree = vscode.window.createTreeView(viewId, { treeDataProvider: provider });
    context.subscriptions.push(tree, provider);
    context.subscriptions.push(vscode.commands.registerCommand("vitte.docs.refreshChapters", () => provider.refresh()));
}
//# sourceMappingURL=docsChaptersView.js.map