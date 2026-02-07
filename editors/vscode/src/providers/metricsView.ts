import * as vscode from "vscode";
import type { LanguageClient } from "vscode-languageclient/node";
import type { ServerMetricEntry } from "../types/metrics";

type MetricTreeNode = ServerMetricEntry | PlaceholderNode;

interface PlaceholderNode {
  placeholder: true;
  message: string;
}

class VitteMetricsProvider implements vscode.TreeDataProvider<MetricTreeNode>, vscode.Disposable {
  private readonly _onDidChangeTreeData = new vscode.EventEmitter<MetricTreeNode | void>();
  readonly onDidChangeTreeData = this._onDidChangeTreeData.event;

  private metrics: ServerMetricEntry[] = [];
  private interval: NodeJS.Timeout | undefined;

  constructor(private readonly getClient: () => LanguageClient | undefined) {}

  dispose(): void {
    if (this.interval) {
      clearInterval(this.interval);
      this.interval = undefined;
    }
  }

  start(): void {
    void this.refresh();
    this.interval = setInterval(() => { void this.refresh(); }, 5000);
  }

  async refresh(): Promise<void> {
    try {
      const client = this.getClient();
      if (!client) {
        this.metrics = [];
      } else {
        this.metrics = await client.sendRequest<ServerMetricEntry[]>("vitte/metrics");
      }
    } catch {
      this.metrics = [];
    }
    this._onDidChangeTreeData.fire();
  }

  getTreeItem(element: MetricTreeNode): vscode.TreeItem {
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

  getChildren(): MetricTreeNode[] | Thenable<MetricTreeNode[]> {
    if (this.metrics.length === 0) {
      return [{ placeholder: true, message: "No metrics captured yet." }];
    }
    return this.metrics;
  }
}

function isPlaceholder(node: MetricTreeNode): node is PlaceholderNode {
  return (node as PlaceholderNode).placeholder === true;
}

export function registerMetricsView(context: vscode.ExtensionContext, getClient: () => LanguageClient | undefined): void {
  const provider = new VitteMetricsProvider(getClient);
  const treeView = vscode.window.createTreeView("vitteMetrics", { treeDataProvider: provider });
  context.subscriptions.push(treeView);
  context.subscriptions.push(provider);
  context.subscriptions.push(vscode.commands.registerCommand("vitte.metrics.refresh", () => provider.refresh()));
  provider.start();
}
