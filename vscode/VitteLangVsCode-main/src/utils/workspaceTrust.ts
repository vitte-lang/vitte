import * as vscode from "vscode";

/** Prevent project-local commands and binaries from running in Restricted Mode. */
export async function requireTrustedWorkspace(action: string): Promise<boolean> {
  if (vscode.workspace.isTrusted) return true;

  const selection = await vscode.window.showWarningMessage(
    `Vitte paused ${action} because this workspace is not trusted. Trust the workspace before running project tools.`,
    "Manage Workspace Trust",
  );
  if (selection === "Manage Workspace Trust") {
    await vscode.commands.executeCommand("workbench.action.manageTrust");
  }
  return false;
}
