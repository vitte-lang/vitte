import * as path from "node:path";
import * as os from "node:os";
import * as fs from "node:fs";
import * as vscode from "vscode";

interface OfflineNode {
  label: string;
  description?: string;
  icon?: vscode.ThemeIcon;
  command?: vscode.Command;
}

class OfflineViewProvider implements vscode.TreeDataProvider<OfflineNode> {
  private readonly _onDidChangeTreeData = new vscode.EventEmitter<OfflineNode | void>();
  readonly onDidChangeTreeData = this._onDidChangeTreeData.event;

  constructor(
    private readonly getReason: () => string | undefined,
    private readonly getOutput: () => vscode.OutputChannel,
    private readonly getSince?: () => string,
    private readonly getSummary?: () => string | undefined
  ) {}

  dispose(): void {
    this._onDidChangeTreeData.dispose();
  }

  refresh(): void {
    this._onDidChangeTreeData.fire();
  }

  getTreeItem(element: OfflineNode): vscode.TreeItem {
    const item = new vscode.TreeItem(element.label, vscode.TreeItemCollapsibleState.None);
    if (element.description) item.description = element.description;
    if (element.icon) item.iconPath = element.icon;
    if (element.command) item.command = element.command;
    return item;
  }

  getChildren(): OfflineNode[] {
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
    ].filter(Boolean) as OfflineNode[];
  }

  getOfflineLogPath(): string {
    const folder = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    const base = folder ?? os.tmpdir();
    return path.join(base, ".vitte", "offline.log");
  }

  async openOfflineLog(): Promise<void> {
    const filePath = this.getOfflineLogPath();
    try {
      if (!fs.existsSync(filePath)) {
        await fs.promises.mkdir(path.dirname(filePath), { recursive: true });
        await fs.promises.writeFile(filePath, "", "utf8");
      }
      const uri = vscode.Uri.file(filePath);
      await vscode.commands.executeCommand("vscode.open", uri);
    } catch {
      const output = this.getOutput();
      output.appendLine(`[offline] Unable to open offline log at ${filePath}`);
      output.show(true);
    }
  }
}

export function registerOfflineView(
  context: vscode.ExtensionContext,
  getReason: () => string | undefined,
  getOutput: () => vscode.OutputChannel,
  getSince?: () => string,
  getSummary?: () => string | undefined
): void {
  const provider = new OfflineViewProvider(getReason, getOutput, getSince, getSummary);
  const tree = vscode.window.createTreeView("vitteOffline", { treeDataProvider: provider });
  context.subscriptions.push(tree);
  context.subscriptions.push(provider);
  context.subscriptions.push(vscode.commands.registerCommand("vitte.offline.openLog", () => provider.openOfflineLog()));
  context.subscriptions.push(vscode.commands.registerCommand("vitte.offline.refresh", () => provider.refresh()));
}
