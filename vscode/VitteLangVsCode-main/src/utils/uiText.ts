import * as vscode from "vscode";

/** Translate extension-owned UI strings covered by the current en/fr UI surface. */
export function uiText(english: string, french: string): string {
  const language = vscode.workspace.getConfiguration("vitte").get<string>("lang", "en").trim().toLowerCase();
  return language.startsWith("fr") ? french : english;
}
