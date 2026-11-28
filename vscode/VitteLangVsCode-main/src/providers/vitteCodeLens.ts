import * as vscode from "vscode";

/** Registers the command used by source-index-backed LSP reference lenses. */
export function registerVitteCodeLens(context: vscode.ExtensionContext): void {
  context.subscriptions.push(vscode.commands.registerCommand(
    "vitte.codelens.openRefs",
    async (uriValue: string | vscode.Uri, positionValue: vscode.Position | { line: number; character: number }) => {
      const uri = typeof uriValue === "string" ? vscode.Uri.parse(uriValue) : uriValue;
      const position = positionValue instanceof vscode.Position
        ? positionValue
        : new vscode.Position(positionValue.line, positionValue.character);
      const references = await vscode.commands.executeCommand<vscode.Location[]>("vscode.executeReferenceProvider", uri, position);
      const locations = references ?? [];
      await vscode.commands.executeCommand("editor.action.showReferences", uri, position, locations);
    },
  ));
}
