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
exports.registerMetricsView = registerMetricsView;
const vscode = __importStar(require("vscode"));
class VitteMetricsProvider {
    constructor(getClient) {
        this.getClient = getClient;
        this._onDidChangeTreeData = new vscode.EventEmitter();
        this.onDidChangeTreeData = this._onDidChangeTreeData.event;
        this.metrics = [];
    }
    dispose() {
        if (this.interval) {
            clearInterval(this.interval);
            this.interval = undefined;
        }
    }
    start() {
        void this.refresh();
        this.interval = setInterval(() => { void this.refresh(); }, 5000);
    }
    async refresh() {
        try {
            const client = this.getClient();
            if (!client) {
                this.metrics = [];
            }
            else {
                this.metrics = await client.sendRequest("vitte/metrics");
            }
        }
        catch {
            this.metrics = [];
        }
        this._onDidChangeTreeData.fire();
    }
    getTreeItem(element) {
        if (isPlaceholder(element)) {
            const item = new vscode.TreeItem(element.message, vscode.TreeItemCollapsibleState.None);
            item.iconPath = new vscode.ThemeIcon("info");
            item.contextValue = "placeholder";
            return item;
        }
        const avg = element.averageMs.toFixed(2);
        const last = element.lastMs.toFixed(2);
        const max = element.maxMs.toFixed(2);
        const item = new vscode.TreeItem(`${element.name}`, vscode.TreeItemCollapsibleState.None);
        item.description = `${avg} ms avg (${element.count}×)`;
        const when = element.lastAt ? new Date(element.lastAt).toLocaleTimeString() : "—";
        const countInfo = typeof element.lastCount === "number" ? `last n=${element.lastCount}` : "";
        item.tooltip = [
            `Average: ${avg} ms`,
            `Last: ${last} ms`,
            `Max: ${max} ms`,
            `Calls: ${element.count}`,
            `Last URI: ${element.lastUri || "n/a"}`,
            `Last at: ${when} ${countInfo}`.trim(),
        ].join("\n");
        item.contextValue = "metricEntry";
        return item;
    }
    getChildren() {
        if (this.metrics.length === 0) {
            return [{ placeholder: true, message: "No metrics captured yet." }];
        }
        return this.metrics;
    }
}
function isPlaceholder(node) {
    return node.placeholder === true;
}
function registerMetricsView(context, getClient) {
    const provider = new VitteMetricsProvider(getClient);
    const treeView = vscode.window.createTreeView("vitteMetrics", { treeDataProvider: provider });
    context.subscriptions.push(treeView);
    context.subscriptions.push(provider);
    context.subscriptions.push(vscode.commands.registerCommand("vitte.metrics.refresh", () => provider.refresh()));
    provider.start();
}
//# sourceMappingURL=metricsView.js.map