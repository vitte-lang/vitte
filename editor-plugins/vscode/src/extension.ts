import * as vscode from 'vscode';
import { spawn } from 'child_process';
import { LanguageClient, LanguageClientOptions, ServerOptions, TransportKind } from 'vscode-languageclient/node';

let client: LanguageClient | undefined;

export function activate(context: vscode.ExtensionContext) {
  // Formatter
  context.subscriptions.push(vscode.commands.registerCommand('vitte.format', async () => {
    const ed = vscode.window.activeTextEditor;
    if (!ed || ed.document.languageId !== 'vitte') { return; }
    const formatted = await simpleFormat(ed.document.getText());
    await ed.edit(b => {
      const full = new vscode.Range(
        ed.document.positionAt(0),
        ed.document.positionAt(ed.document.getText().length)
      );
      b.replace(full, formatted);
    });
  }));

  // Start LSP
  context.subscriptions.push(vscode.commands.registerCommand('vitte.startLsp', async () => {
    if (client) { vscode.window.showInformationMessage('Vitte LSP déjà lancé.'); return; }
    try {
      const serverCommand = 'vitte-cli';
      const serverArgs = ['lsp'];
      const serverOptions: ServerOptions = {
        command: serverCommand, args: serverArgs, transport: TransportKind.stdio
      };
      const clientOptions: LanguageClientOptions = {
        documentSelector: [{ scheme: 'file', language: 'vitte' }],
        synchronize: { fileEvents: vscode.workspace.createFileSystemWatcher('**/*.vitte') }
      };
      client = new LanguageClient('vitteLsp', 'Vitte LSP', serverOptions, clientOptions);
      await client.start();
      vscode.window.showInformationMessage('Vitte LSP lancé.');
    } catch (e:any) {
      vscode.window.showErrorMessage('Impossible de démarrer le serveur LSP (vitte-cli lsp).');
    }
  }));

  // Auto activate on Vitte file open
  if (vscode.window.activeTextEditor?.document.languageId === 'vitte') {
    vscode.commands.executeCommand('vitte.startLsp');
  }
}

export function deactivate() {
  return client?.stop();
}

// Naive formatter (placeholder) — à remplacer par vitte-fmt si dispo
async function simpleFormat(src: string): Promise<string> {
  return src.replace(/[ \t]+$/gm, '').replace(/\n{3,}/g, '\n\n');
}
