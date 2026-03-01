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
exports.VitteProjectTreeProvider = void 0;
exports.registerProjectTree = registerProjectTree;
const vscode = __importStar(require("vscode"));
const path = __importStar(require("path"));
// Compat: VS Code versions without Uri.joinPath
function joinPathCompat(base, ...parts) {
    const anyUri = vscode.Uri;
    if (typeof anyUri.joinPath === 'function') {
        return anyUri.joinPath(base, ...parts);
    }
    return vscode.Uri.file(path.join(base.fsPath, ...parts));
}
// ---- Utils ----
function readWorkspaceRoot() {
    return vscode.workspace.workspaceFolders?.[0]?.uri;
}
async function findOne(glob) {
    const list = await vscode.workspace.findFiles(glob, '**/node_modules/**', 1);
    return list[0];
}
async function findMany(glob, limit = 50) {
    return vscode.workspace.findFiles(glob, '**/node_modules/**', limit);
}
function mkId(...parts) { return parts.join(':'); }
function mkFile(uri, label) {
    return { id: mkId('file', uri.fsPath), label: label ?? path.basename(uri.fsPath), contextValue: 'vitte:file', kind: 'file', uri, collapsible: vscode.TreeItemCollapsibleState.None };
}
function mkDir(uri, label) {
    return { id: mkId('dir', uri.fsPath), label: label ?? path.basename(uri.fsPath), contextValue: 'vitte:dir', kind: 'dir', uri, collapsible: vscode.TreeItemCollapsibleState.Collapsed };
}
function mkGroup(id, label, icon, children = []) {
    const group = {
        id,
        label,
        contextValue: 'vitte:group',
        kind: 'group',
        collapsible: vscode.TreeItemCollapsibleState.Collapsed,
        children,
    };
    if (icon !== undefined) {
        group.icon = icon;
    }
    return group;
}
function mkTarget(triple) {
    return {
        id: mkId('target', triple),
        label: triple,
        contextValue: 'vitte:target',
        kind: 'target',
        collapsible: vscode.TreeItemCollapsibleState.None,
        triple,
        icon: 'rocket',
    };
}
// ---- Provider ----
class VitteProjectTreeProvider {
    iconAsset(name) {
        // Use the same SVG for light/dark; you can add themed variants later if needed.
        const p = (n) => vscode.Uri.file(path.join(this.ctx.extensionUri.fsPath, 'media', 'icons', `${n}.svg`));
        const u = p(name);
        return { light: u, dark: u };
    }
    constructor(ctx) {
        this.ctx = ctx;
        this._onDidChangeTreeData = new vscode.EventEmitter();
        this.onDidChangeTreeData = this._onDidChangeTreeData.event;
    }
    refresh() { this._onDidChangeTreeData.fire(undefined); }
    getTreeItem(element) {
        const item = new vscode.TreeItem(element.label, element.collapsible);
        item.contextValue = element.contextValue;
        switch (element.kind) {
            case 'file':
                item.resourceUri = element.uri;
                item.command = { command: 'vscode.open', title: 'Open', arguments: [element.uri] };
                item.iconPath = vscode.ThemeIcon.File;
                break;
            case 'dir':
                item.resourceUri = element.uri;
                item.iconPath = vscode.ThemeIcon.Folder;
                break;
            case 'group':
                // Use packaged SVG (media/icons/vitte.svg or other). Fallback to 'vitte'.
                item.iconPath = this.iconAsset(element.icon ?? 'vitte');
                break;
            case 'target':
                item.iconPath = this.iconAsset(element.icon ?? 'vitte');
                break;
        }
        return item;
    }
    async getChildren(element) {
        if (!element)
            return await this.buildRoot();
        if (element.kind === 'group')
            return element.children;
        if (element.kind === 'dir')
            return await this.readDirChildren(element.uri);
        return [];
    }
    // ---- Builders ----
    async buildRoot() {
        const root = readWorkspaceRoot();
        if (!root)
            return [];
        const nodes = [];
        // Config group
        const vitteConfig = (await findOne('vitte.config.json')) ?? (await findOne('.vitte/config.json'));
        const configGroup = mkGroup('group:config', 'Configuration', 'settings');
        if (vitteConfig)
            configGroup.children.push(mkFile(vitteConfig, 'vitte.config.json'));
        const tasksJson = await findOne('.vscode/tasks.json');
        if (tasksJson)
            configGroup.children.push(mkFile(tasksJson));
        const launchJson = await findOne('.vscode/launch.json');
        if (launchJson)
            configGroup.children.push(mkFile(launchJson));
        nodes.push(configGroup);
        // Targets group from config if present
        const targets = await this.readTargetsFromConfig(vitteConfig);
        const targetGroup = mkGroup('group:targets', 'Cibles', 'rocket');
        for (const t of targets)
            targetGroup.children.push(mkTarget(t));
        nodes.push(targetGroup);
        // Sources group
        const srcGroup = mkGroup('group:sources', 'Sources', 'source-control');
        const modulesDirs = await findMany('{src,modules,crates}/**/*.vitte', 200);
        // Group by folder
        const byDir = new Map();
        for (const uri of modulesDirs) {
            const dir = path.dirname(uri.fsPath);
            const arr = byDir.get(dir) ?? [];
            arr.push(uri);
            byDir.set(dir, arr);
        }
        for (const [dir, files] of byDir) {
            const children = files.map(f => mkFile(f));
            const label = path.basename(dir);
            const dirGroup = mkGroup(`group:src:${dir}`, label, 'source-control', children);
            srcGroup.children.push(dirGroup);
        }
        nodes.push(srcGroup);
        // Tests group
        const tests = await findMany('**/*_test.vitte', 200);
        const testGroup = mkGroup('group:tests', `Tests (${tests.length})`, 'beaker');
        for (const t of tests)
            testGroup.children.push(mkFile(t));
        nodes.push(testGroup);
        // Benchmarks group
        const benches = await findMany('**/*_bench.vitte', 200);
        const benchGroup = mkGroup('group:bench', `Benchmarks (${benches.length})`, 'graph');
        for (const b of benches)
            benchGroup.children.push(mkFile(b));
        nodes.push(benchGroup);
        return nodes;
    }
    async readDirChildren(dir) {
        try {
            const entries = await vscode.workspace.fs.readDirectory(dir);
            const nodes = [];
            for (const [name, type] of entries) {
                const uri = joinPathCompat(dir, name);
                if (type === vscode.FileType.File && /\.(vitte|vit)$/i.test(name))
                    nodes.push(mkFile(uri));
                if (type === vscode.FileType.Directory)
                    nodes.push(mkDir(uri));
            }
            return nodes;
        }
        catch {
            return [];
        }
    }
    async readTargetsFromConfig(configUri) {
        if (!configUri)
            return [];
        try {
            const doc = await vscode.workspace.openTextDocument(configUri);
            const json = JSON.parse(doc.getText());
            const rawTargets = json.targets;
            if (Array.isArray(rawTargets)) {
                const list = [];
                for (const entry of rawTargets) {
                    if (typeof entry === 'string') {
                        list.push(entry);
                    }
                    else if (entry && typeof entry === 'object') {
                        const triple = entry.triple;
                        if (typeof triple === 'string')
                            list.push(triple);
                    }
                }
                return list;
            }
        }
        catch {
            // ignore
        }
        return [];
    }
}
exports.VitteProjectTreeProvider = VitteProjectTreeProvider;
// ---- Registration helper ----
function registerProjectTree(ctx) {
    const provider = new VitteProjectTreeProvider(ctx);
    ctx.subscriptions.push(vscode.window.registerTreeDataProvider('vitteExplorer', provider), vscode.commands.registerCommand('vitte.refreshExplorer', () => provider.refresh()), vscode.commands.registerCommand('vitte.revealInExplorer', (uri) => {
        void vscode.commands.executeCommand('revealInExplorer', uri);
    }));
    return provider;
}
//# sourceMappingURL=projectTree.js.map