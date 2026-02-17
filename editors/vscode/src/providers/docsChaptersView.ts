import * as vscode from "vscode";
import { DOC_SECTIONS, makeDocUrl } from "./documentationChapters";

type ChapterNode = SectionNode | ItemNode;

interface SectionNode {
  kind: "section";
  title: string;
}

interface ItemNode {
  kind: "chapter";
  title: string;
  url: string;
}

class DocsChaptersProvider implements vscode.TreeDataProvider<ChapterNode> {
  private readonly _onDidChangeTreeData = new vscode.EventEmitter<ChapterNode | void>();
  readonly onDidChangeTreeData = this._onDidChangeTreeData.event;

  dispose(): void {
    this._onDidChangeTreeData.dispose();
  }

  refresh(): void {
    this._onDidChangeTreeData.fire();
  }

  getTreeItem(element: ChapterNode): vscode.TreeItem {
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

  getChildren(element?: ChapterNode): ChapterNode[] {
    if (!element) {
      return DOC_SECTIONS.map((section) => ({ kind: "section", title: section.title }));
    }
    if (element.kind === "section") {
      const section = DOC_SECTIONS.find((entry) => entry.title === element.title);
      if (!section) return [];
      return section.chapters.map((chapter) => ({
        kind: "chapter",
        title: chapter.title,
        url: makeDocUrl(chapter.path)
      }));
    }
    return [];
  }
}

export function registerDocsChaptersView(context: vscode.ExtensionContext, viewId: string): void {
  const provider = new DocsChaptersProvider();
  const tree = vscode.window.createTreeView(viewId, { treeDataProvider: provider });
  context.subscriptions.push(tree, provider);
  context.subscriptions.push(
    vscode.commands.registerCommand("vitte.docs.refreshChapters", () => provider.refresh())
  );
}
