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
exports.registerOfflineDocsIndex = registerOfflineDocsIndex;
const path = __importStar(require("node:path"));
const vscode = __importStar(require("vscode"));
const fs = __importStar(require("node:fs/promises"));
class OfflineDocNode extends vscode.TreeItem {
    constructor(uri, label) {
        super(label, vscode.TreeItemCollapsibleState.None);
        this.uri = uri;
        this.contextValue = "vitteOfflineDoc";
        this.description = vscode.workspace.asRelativePath(uri, false);
        this.command = {
            command: "vitte.docs.openLocal",
            title: "Open local doc",
            arguments: [uri],
        };
        this.iconPath = new vscode.ThemeIcon("book");
    }
}
class OfflineDocsProvider {
    constructor() {
        this.emitter = new vscode.EventEmitter();
        this.onDidChangeTreeData = this.emitter.event;
        this.nodes = [];
        this.inverted = new Map();
        this.docsByPath = new Map();
        this.snippets = new Map();
    }
    async refresh() {
        const files = await vscode.workspace.findFiles("book/**/*.md", "**/{node_modules,.git,out,dist,build}/**", 2000);
        const sorted = files.sort((a, b) => a.fsPath.localeCompare(b.fsPath));
        this.nodes = sorted.map((uri) => new OfflineDocNode(uri, path.basename(uri.fsPath)));
        this.inverted.clear();
        this.docsByPath.clear();
        this.snippets.clear();
        for (const uri of sorted) {
            const rel = vscode.workspace.asRelativePath(uri, false);
            this.docsByPath.set(rel, uri);
            let content = "";
            try {
                content = await fs.readFile(uri.fsPath, "utf8");
            }
            catch {
                continue;
            }
            const plain = content.replace(/[`*_>#-]/g, " ").replace(/\s+/g, " ").trim();
            const words = plain.toLowerCase().split(/\W+/).filter((w) => w.length >= 3);
            const unique = new Set(words);
            for (const w of unique) {
                const set = this.inverted.get(w) ?? new Set();
                set.add(rel);
                this.inverted.set(w, set);
            }
            const headings = content.split(/\r?\n/).filter((l) => /^#+\s+/.test(l)).slice(0, 6).join(" | ");
            this.snippets.set(rel, headings || plain.slice(0, 300));
        }
        this.emitter.fire(undefined);
    }
    getTreeItem(element) {
        return element;
    }
    getChildren() {
        return this.nodes;
    }
    search(query) {
        const terms = query.toLowerCase().split(/\W+/).filter((t) => t.length >= 2);
        if (terms.length === 0)
            return [];
        const scores = new Map();
        for (const t of terms) {
            const hits = this.inverted.get(t);
            if (!hits)
                continue;
            for (const rel of hits) {
                scores.set(rel, (scores.get(rel) ?? 0) + 1);
            }
        }
        return Array.from(scores.entries())
            .map(([rel, score]) => ({ rel, score, uri: this.docsByPath.get(rel) }))
            .filter((x) => !!x.uri)
            .sort((a, b) => b.score - a.score || a.rel.localeCompare(b.rel));
    }
    docSnippet(rel) {
        return this.snippets.get(rel);
    }
    dispose() {
        this.emitter.dispose();
    }
}
function registerOfflineDocsIndex(context) {
    const provider = new OfflineDocsProvider();
    const tree = vscode.window.createTreeView("vitteOfflineDocs", {
        treeDataProvider: provider,
        showCollapseAll: false,
    });
    context.subscriptions.push(provider, tree, vscode.commands.registerCommand("vitte.docs.refreshLocalIndex", async () => provider.refresh()), vscode.commands.registerCommand("vitte.docs.openLocal", async (uri) => {
        if (!uri)
            return;
        await vscode.commands.executeCommand("vscode.open", uri);
    }), vscode.commands.registerCommand("vitte.docs.searchLocal", async () => {
        const query = await vscode.window.showInputBox({ prompt: "Search local docs (full-text)" });
        if (!query)
            return;
        const picks = provider.search(query).slice(0, 60).map((r) => ({
            label: path.basename(r.uri.fsPath),
            description: `${r.rel} (score ${r.score})`,
            detail: provider.docSnippet(r.rel) ?? "",
            uri: r.uri,
        }));
        const pick = await vscode.window.showQuickPick(picks, {
            placeHolder: "Search local docs",
            matchOnDescription: true,
            matchOnDetail: true,
        });
        if (!pick)
            return;
        await vscode.commands.executeCommand("vscode.open", pick.uri);
    }), vscode.languages.registerHoverProvider([{ language: "vitte" }, { language: "vit" }], {
        provideHover(document, position) {
            const range = document.getWordRangeAtPosition(position);
            if (!range)
                return undefined;
            const word = document.getText(range).toLowerCase();
            if (word.length < 3)
                return undefined;
            const hits = provider.search(word);
            if (hits.length === 0)
                return undefined;
            const top = hits.slice(0, 3);
            const lines = top.map((h) => {
                const snip = provider.docSnippet(h.rel) ?? "";
                return `- **${h.rel}**: ${snip.slice(0, 120)}`;
            });
            return new vscode.Hover(new vscode.MarkdownString(`Local docs for \`${word}\`:\n${lines.join("\n")}`), range);
        },
    }));
    void provider.refresh();
}
//# sourceMappingURL=offlineDocsIndex.js.map