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
exports.ModuleExplorerProvider = void 0;
exports.registerModuleExplorerView = registerModuleExplorerView;
const path = __importStar(require("node:path"));
const vscode = __importStar(require("vscode"));
const diagnostics_1 = require("./utils/diagnostics");
function createBucket() {
    return { children: [], uris: new Set() };
}
class ModuleNode extends vscode.TreeItem {
    constructor(entry) {
        super(entry.label, vscode.TreeItemCollapsibleState.Collapsed);
        this.entry = entry;
        this.contextValue = "vitteModuleNode";
        if (entry.description !== undefined) {
            this.description = entry.description;
        }
        this.tooltip = entry.tooltip;
        this.iconPath = iconForSummary(entry.summary);
    }
    get children() {
        return this.entry.children;
    }
}
class SymbolNode extends vscode.TreeItem {
    constructor(symbol) {
        super(symbol.name, vscode.TreeItemCollapsibleState.None);
        this.symbol = symbol;
        this.description = symbolDetail(symbol);
        this.tooltip = symbolTooltip(symbol);
        this.iconPath = iconForSymbol(symbol.kind);
        this.command = {
            command: "vitte.modules.openSymbol",
            title: "Ouvrir le symbole",
            arguments: [symbol]
        };
        this.contextValue = "vitteModuleSymbol";
    }
}
class ModuleExplorerProvider {
    constructor() {
        this.emitter = new vscode.EventEmitter();
        this.onDidChangeTreeData = this.emitter.event;
        this.nodes = [];
    }
    setTree(tree) {
        this.tree = tree;
    }
    getTreeItem(element) {
        return element;
    }
    getChildren(element) {
        if (!element)
            return this.nodes;
        if (element instanceof ModuleNode)
            return element.children;
        return [];
    }
    dispose() {
        this.emitter.dispose();
        if (this.pendingRefresh)
            clearTimeout(this.pendingRefresh);
        this.pendingRefresh = undefined;
    }
    refreshSoon() {
        if (this.pendingRefresh)
            clearTimeout(this.pendingRefresh);
        this.pendingRefresh = setTimeout(() => {
            this.pendingRefresh = undefined;
            void this.refresh();
        }, 250);
    }
    async refresh() {
        try {
            this.pendingRefresh = undefined;
            const entries = await buildModuleEntries();
            this.nodes = entries.map(entry => new ModuleNode(entry));
            this.emitter.fire(undefined);
            if (this.tree) {
                this.tree.message = this.nodes.length === 0 ? "Aucun module Vitte détecté" : "";
            }
        }
        catch (err) {
            const message = err instanceof Error ? err.message : String(err);
            if (this.tree) {
                this.tree.message = "Impossible d’analyser les modules";
            }
            void vscode.window.showWarningMessage(`Vitte : échec de l’analyse des modules — ${message}`);
        }
    }
}
exports.ModuleExplorerProvider = ModuleExplorerProvider;
function registerModuleExplorerView(context) {
    const provider = new ModuleExplorerProvider();
    const tree = vscode.window.createTreeView("vitteModules", {
        treeDataProvider: provider,
        showCollapseAll: true
    });
    provider.setTree(tree);
    context.subscriptions.push(tree, provider);
    const signals = [
        vscode.workspace.onDidChangeWorkspaceFolders(() => provider.refreshSoon()),
        vscode.workspace.onDidSaveTextDocument((doc) => {
            if (isVitteDocument(doc))
                provider.refreshSoon();
        }),
        vscode.workspace.onDidOpenTextDocument((doc) => {
            if (isVitteDocument(doc))
                provider.refreshSoon();
        }),
        vscode.workspace.onDidCloseTextDocument((doc) => {
            if (isVitteDocument(doc))
                provider.refreshSoon();
        }),
        vscode.languages.onDidChangeDiagnostics(() => provider.refreshSoon()),
        vscode.commands.registerCommand("vitte.modules.refresh", async () => {
            await provider.refresh();
        }),
        vscode.commands.registerCommand("vitte.modules.openSymbol", async (symbol) => {
            if (!symbol?.location)
                return;
            const doc = await vscode.workspace.openTextDocument(symbol.location.uri);
            const editor = await vscode.window.showTextDocument(doc, { preview: true });
            editor.revealRange(symbol.location.range, vscode.TextEditorRevealType.InCenter);
            editor.selection = new vscode.Selection(symbol.location.range.start, symbol.location.range.start);
        })
    ];
    for (const disposable of signals)
        context.subscriptions.push(disposable);
    // Initial population.
    void provider.refresh();
    return provider;
}
async function buildModuleEntries() {
    const symbols = await vscode.commands.executeCommand("vscode.executeWorkspaceSymbolProvider", "");
    const list = (symbols ?? []).filter((sym) => isAcceptableSymbol(sym));
    const byModule = new Map();
    for (const sym of list) {
        const moduleKey = moduleNameForSymbol(sym);
        let bucket = byModule.get(moduleKey);
        if (!bucket) {
            bucket = createBucket();
            byModule.set(moduleKey, bucket);
        }
        if (!bucket.symbol && isModuleSymbol(sym)) {
            bucket.symbol = sym;
        }
        if (!isModuleSymbol(sym)) {
            bucket.children.push(new SymbolNode(sym));
        }
        bucket.uris.add(sym.location.uri.toString());
    }
    const entries = [];
    for (const [name, data] of byModule) {
        const summary = (0, diagnostics_1.summarizeDiagnosticsForUris)(data.uris);
        const fileCount = data.uris.size;
        let description;
        if (fileCount > 1) {
            description = `${fileCount} fichiers`;
        }
        else {
            const [singleUri] = Array.from(data.uris);
            if (singleUri)
                description = uriToBasename(singleUri);
        }
        const tooltipParts = [name];
        if (description)
            tooltipParts.push(description);
        tooltipParts.push((0, diagnostics_1.formatDiagnosticsSummary)(summary));
        const entry = {
            id: name,
            label: name,
            tooltip: tooltipParts.join("\n"),
            summary,
            children: [...data.children].sort(sortSymbolNodes),
            ...(description ? { description } : {})
        };
        entries.push(entry);
    }
    entries.sort((a, b) => {
        const severityDiff = severityWeight(a.summary) - severityWeight(b.summary);
        if (severityDiff !== 0)
            return severityDiff;
        return a.label.localeCompare(b.label);
    });
    return entries;
}
function sortSymbolNodes(a, b) {
    const kindDiff = a.symbol.kind - b.symbol.kind;
    if (kindDiff !== 0)
        return kindDiff;
    return a.symbol.name.localeCompare(b.symbol.name);
}
function severityWeight(summary) {
    const level = (0, diagnostics_1.diagnosticsLevel)(summary);
    switch (level) {
        case "error": return 0;
        case "warning": return 1;
        default: return 2;
    }
}
function isAcceptableSymbol(sym) {
    if (!sym?.location?.uri)
        return false;
    if (sym.location.uri.scheme !== "file")
        return false;
    const ext = path.extname(sym.location.uri.fsPath).toLowerCase();
    return diagnostics_1.VITTE_FILE_EXTS.has(ext);
}
function moduleNameForSymbol(sym) {
    if (isModuleSymbol(sym))
        return sym.name;
    if (sym.containerName)
        return sym.containerName;
    return path.basename(sym.location.uri.fsPath);
}
function isModuleSymbol(sym) {
    return sym.kind === vscode.SymbolKind.Module || sym.kind === vscode.SymbolKind.Namespace;
}
function iconForSummary(summary) {
    const level = (0, diagnostics_1.diagnosticsLevel)(summary);
    if (level === "error")
        return themeIcon("error");
    if (level === "warning")
        return themeIcon("warning");
    return themeIcon("pass-filled");
}
function iconForSymbol(kind) {
    switch (kind) {
        case vscode.SymbolKind.Function:
        case vscode.SymbolKind.Method:
            return themeIcon("symbol-method");
        case vscode.SymbolKind.Struct:
        case vscode.SymbolKind.Class:
            return themeIcon("symbol-structure");
        case vscode.SymbolKind.Enum:
            return themeIcon("symbol-enum");
        case vscode.SymbolKind.Interface:
        case vscode.SymbolKind.TypeParameter:
            return themeIcon("symbol-interface");
        case vscode.SymbolKind.Variable:
        case vscode.SymbolKind.Constant:
            return themeIcon("symbol-variable");
        default:
            return themeIcon("symbol-misc");
    }
}
function symbolDetail(sym) {
    return path.basename(sym.location.uri.fsPath);
}
function symbolTooltip(sym) {
    const range = sym.location.range;
    const position = `L${range.start.line + 1}:C${range.start.character + 1}`;
    return `${sym.name} (${vscode.SymbolKind[sym.kind]})\n${sym.location.uri.fsPath}:${position}`;
}
function uriToBasename(uri) {
    try {
        return path.basename(vscode.Uri.parse(uri).fsPath);
    }
    catch {
        return uri;
    }
}
function themeIcon(id) {
    return { id };
}
function isVitteDocument(document) {
    if (document.uri.scheme !== "file")
        return false;
    return diagnostics_1.VITTE_FILE_EXTS.has(path.extname(document.uri.fsPath).toLowerCase());
}
//# sourceMappingURL=moduleExplorerView.js.map