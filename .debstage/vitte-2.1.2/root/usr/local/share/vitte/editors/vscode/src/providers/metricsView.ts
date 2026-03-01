import * as vscode from "vscode";
import type { LanguageClient } from "vscode-languageclient/node";
import type { ServerMetricEntry } from "../types/metrics";

type MetricTreeNode = ServerMetricEntry | PlaceholderNode;

interface PlaceholderNode {
  placeholder: true;
  message: string;
  command?: vscode.Command;
  icon?: vscode.ThemeIcon;
  context?: string;
  description?: string;
}

class VitteMetricsProvider implements vscode.TreeDataProvider<MetricTreeNode>, vscode.Disposable {
  private readonly _onDidChangeTreeData = new vscode.EventEmitter<MetricTreeNode | void>();
  readonly onDidChangeTreeData = this._onDidChangeTreeData.event;

  private metrics: ServerMetricEntry[] = [];
  private lastError: string | undefined;
  private hasClient = false;
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
        this.hasClient = false;
        this.metrics = [];
        this.lastError = undefined;
      } else {
        this.hasClient = true;
        this.metrics = await client.sendRequest<ServerMetricEntry[]>("vitte/metrics");
        this.lastError = undefined;
      }
    } catch {
      this.metrics = [];
      this.lastError = "Unable to fetch metrics from the server.";
    }
    this._onDidChangeTreeData.fire();
  }

  getTreeItem(element: MetricTreeNode): vscode.TreeItem {
    if (isPlaceholder(element)) {
      const item = new vscode.TreeItem(element.message, vscode.TreeItemCollapsibleState.None);
      item.iconPath = element.icon ?? new vscode.ThemeIcon("info");
      item.contextValue = element.context ?? "placeholder";
      if (element.command) {
        item.command = element.command;
      }
      if (element.description) {
        item.description = element.description;
      }
      return item;
    }
    const avg = element.averageMs.toFixed(2);
    const last = element.lastMs.toFixed(2);
    const max = element.maxMs.toFixed(2);
    const p99 = typeof element.p99Ms === "number" ? element.p99Ms.toFixed(2) : undefined;
    const item = new vscode.TreeItem(`${element.name}`, vscode.TreeItemCollapsibleState.None);
    const errCount = typeof element.errorCount === "number" ? element.errorCount : 0;
    item.description = `${avg} ms avg (${element.count}×)${errCount ? `, ${errCount} errors` : ""}`;
    const when = element.lastAt ? new Date(element.lastAt).toLocaleTimeString() : "—";
    const countInfo = typeof element.lastCount === "number" ? `last n=${element.lastCount}` : "";
    item.tooltip = [
      `Average: ${avg} ms`,
      p99 ? `P99: ${p99} ms` : undefined,
      `Last: ${last} ms`,
      `Max: ${max} ms`,
      `Calls: ${element.count}`,
      errCount ? `Errors: ${errCount}` : undefined,
      element.lastError ? `Last error: ${element.lastError}` : undefined,
      `Last URI: ${element.lastUri || "n/a"}`,
      `Last at: ${when} ${countInfo}`.trim(),
    ].filter(Boolean).join("\n");
    item.contextValue = "metricEntry";
    return item;
  }

  getChildren(): MetricTreeNode[] | Thenable<MetricTreeNode[]> {
    if (this.metrics.length === 0) {
      return this.getEmptyStateNodes();
    }
    return this.getMetricsNodes();
  }

  private getEmptyStateNodes(): MetricTreeNode[] {
    const nodes: MetricTreeNode[] = [];

    if (!this.hasClient) {
      nodes.push({
        placeholder: true,
        message: "Vitte server not running.",
        icon: new vscode.ThemeIcon("debug-stop"),
        command: { command: "vitte.restartServer", title: "Start Vitte server" },
        context: "placeholder.server",
      });
    } else if (this.lastError) {
      nodes.push({
        placeholder: true,
        message: this.lastError,
        icon: new vscode.ThemeIcon("error"),
        command: { command: "vitte.showServerLog", title: "Open server log" },
        context: "placeholder.error",
      });
    } else {
      nodes.push({
        placeholder: true,
        message: "No metrics captured yet.",
        icon: new vscode.ThemeIcon("info"),
        context: "placeholder.empty",
      });
    }

    nodes.push({
      placeholder: true,
      message: "Open a .vitte or .vit file and trigger completions/hover to record metrics.",
      icon: new vscode.ThemeIcon("file-code"),
      context: "placeholder.hint",
    });
    nodes.push({
      placeholder: true,
      message: "Refresh metrics",
      icon: new vscode.ThemeIcon("refresh"),
      command: { command: "vitte.metrics.refresh", title: "Refresh metrics" },
      context: "placeholder.refresh",
    });
    nodes.push({
      placeholder: true,
      message: "Show metrics snapshot in the Output panel",
      icon: new vscode.ThemeIcon("output"),
      command: { command: "vitte.showServerMetrics", title: "Show server metrics" },
      context: "placeholder.snapshot",
    });

    return nodes;
  }

  private getMetricsNodes(): MetricTreeNode[] {
    const nodes: MetricTreeNode[] = [];
    const totalCalls = this.metrics.reduce((sum, entry) => sum + entry.count, 0);
    const weightedAvg = totalCalls > 0
      ? this.metrics.reduce((sum, entry) => sum + entry.averageMs * entry.count, 0) / totalCalls
      : 0;
    const first = this.metrics[0];
    if (!first) return nodes;
    const slowestAvg = this.metrics.reduce((best, entry) => (entry.averageMs > best.averageMs ? entry : best), first);
    const highestMax = this.metrics.reduce((best, entry) => (entry.maxMs > best.maxMs ? entry : best), first);
    const lastSeen = this.metrics.reduce((best, entry) => (entry.lastAt > best.lastAt ? entry : best), first);

    nodes.push({
      placeholder: true,
      message: "Summary",
      icon: new vscode.ThemeIcon("dashboard"),
      context: "summary.header",
    });
    nodes.push({
      placeholder: true,
      message: "Total metrics",
      description: `${this.metrics.length}`,
      icon: new vscode.ThemeIcon("symbol-number"),
      context: "summary.total",
    });
    nodes.push({
      placeholder: true,
      message: "Total calls",
      description: `${totalCalls}`,
      icon: new vscode.ThemeIcon("symbol-event"),
      context: "summary.calls",
    });
    nodes.push({
      placeholder: true,
      message: "Weighted avg",
      description: `${weightedAvg.toFixed(2)} ms`,
      icon: new vscode.ThemeIcon("clock"),
      context: "summary.avg",
    });
    nodes.push({
      placeholder: true,
      message: "Slowest avg",
      description: `${slowestAvg.name} (${slowestAvg.averageMs.toFixed(2)} ms)`,
      icon: new vscode.ThemeIcon("warning"),
      context: "summary.slowest",
    });
    nodes.push({
      placeholder: true,
      message: "Highest max",
      description: `${highestMax.name} (${highestMax.maxMs.toFixed(2)} ms)`,
      icon: new vscode.ThemeIcon("graph"),
      context: "summary.max",
    });
    nodes.push({
      placeholder: true,
      message: "Last activity",
      description: `${lastSeen.name} (${new Date(lastSeen.lastAt).toLocaleTimeString()})`,
      icon: new vscode.ThemeIcon("history"),
      context: "summary.last",
    });
    nodes.push({
      placeholder: true,
      message: "Refresh metrics",
      icon: new vscode.ThemeIcon("refresh"),
      command: { command: "vitte.metrics.refresh", title: "Refresh metrics" },
      context: "summary.refresh",
    });
    nodes.push({
      placeholder: true,
      message: "Reset metrics",
      icon: new vscode.ThemeIcon("trash"),
      command: { command: "vitte.metrics.reset", title: "Reset metrics" },
      context: "summary.reset",
    });

    const sorted = [...this.metrics].sort((a, b) => b.averageMs - a.averageMs);
    return [...nodes, ...sorted];
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
