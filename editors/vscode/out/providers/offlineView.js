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
exports.registerOfflineView = registerOfflineView;
const path = __importStar(require("node:path"));
const os = __importStar(require("node:os"));
const fs = __importStar(require("node:fs"));
const vscode = __importStar(require("vscode"));
class OfflineViewProvider {
    constructor(getReason, getOutput, getSince, getSummary) {
        this.getReason = getReason;
        this.getOutput = getOutput;
        this.getSince = getSince;
        this.getSummary = getSummary;
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
        const item = new vscode.TreeItem(element.label, vscode.TreeItemCollapsibleState.None);
        if (element.description)
            item.description = element.description;
        if (element.icon)
            item.iconPath = element.icon;
        if (element.command)
            item.command = element.command;
        return item;
    }
    getChildren() {
        const reason = this.getReason() ?? "Offline mode is enabled.";
        const since = this.getSince?.() ?? "unknown";
        const summary = this.getSummary?.();
        return [
            { label: "Server Offline", description: reason, icon: new vscode.ThemeIcon("circle-slash") },
            { label: "Offline Since", description: since, icon: new vscode.ThemeIcon("history") },
            summary ? { label: "Local Diagnostics", description: summary, icon: new vscode.ThemeIcon("warning") } : undefined,
            { label: "Explain Offline", icon: new vscode.ThemeIcon("question"), command: { command: "vitte.offline.explain", title: "Explain Offline" } },
            { label: "Copy Offline Report", icon: new vscode.ThemeIcon("copy"), command: { command: "vitte.offline.copyReport", title: "Copy Offline Report" } },
            { label: "Open Settings", icon: new vscode.ThemeIcon("settings-gear"), command: { command: "workbench.action.openSettings", title: "Open Settings", arguments: ["vitte.server"] } },
            { label: "Open Offline Log", icon: new vscode.ThemeIcon("file-text"), command: { command: "vitte.offline.openLog", title: "Open Offline Log" } },
            { label: "Open Server Log", icon: new vscode.ThemeIcon("output"), command: { command: "vitte.showServerLog", title: "Open Server Log" } },
        ].filter(Boolean);
    }
    getOfflineLogPath() {
        const folder = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
        const base = folder ?? os.tmpdir();
        return path.join(base, ".vitte", "offline.log");
    }
    async openOfflineLog() {
        const filePath = this.getOfflineLogPath();
        try {
            if (!fs.existsSync(filePath)) {
                await fs.promises.mkdir(path.dirname(filePath), { recursive: true });
                await fs.promises.writeFile(filePath, "", "utf8");
            }
            const uri = vscode.Uri.file(filePath);
            await vscode.commands.executeCommand("vscode.open", uri);
        }
        catch {
            const output = this.getOutput();
            output.appendLine(`[offline] Unable to open offline log at ${filePath}`);
            output.show(true);
        }
    }
}
function registerOfflineView(context, getReason, getOutput, getSince, getSummary) {
    const provider = new OfflineViewProvider(getReason, getOutput, getSince, getSummary);
    const tree = vscode.window.createTreeView("vitteOffline", { treeDataProvider: provider });
    context.subscriptions.push(tree);
    context.subscriptions.push(provider);
    context.subscriptions.push(vscode.commands.registerCommand("vitte.offline.openLog", () => provider.openOfflineLog()));
    context.subscriptions.push(vscode.commands.registerCommand("vitte.offline.refresh", () => provider.refresh()));
}
//# sourceMappingURL=offlineView.js.map