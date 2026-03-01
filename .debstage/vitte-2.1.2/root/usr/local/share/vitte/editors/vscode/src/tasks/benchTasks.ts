import * as vscode from 'vscode';
import * as cp from 'child_process';
import * as path from 'path';
import * as fs from 'fs';
import { locateVitteRuntime } from '../debug/runtimeLocator';

/**
 * Bench tasks & commands for Vitte
 *
 * Commands:
 *  - vitte.bench: Run benchmarks for the current workspace (profile/targets from settings or vitte.config.json)
 *  - vitte.benchReport: Run + open the latest HTML/SVG report if produced
 *  - vitte.openBenchDir: Reveal the bench report directory
 *
 * Task provider:
 *  - type: 'vitte' with command: 'bench' to integrate with VS Code Tasks
 */
export function registerBenchTasks(ctx: vscode.ExtensionContext) {
  // Commands
  ctx.subscriptions.push(
    vscode.commands.registerCommand('vitte.bench', async () => {
      await runBench(false);
    }),
    vscode.commands.registerCommand('vitte.benchReport', async () => {
      await runBench(true);
    }),
    vscode.commands.registerCommand('vitte.openBenchDir', async () => {
      const dir = await benchReportDir();
      if (dir) void vscode.commands.executeCommand('revealFileInOS', vscode.Uri.file(dir));
    }),
  );

  const provider = {
    provideTasks: async (_token?: vscode.CancellationToken): Promise<vscode.Task[]> => {
      const cmd = await benchCommandLine();
      const definition: VitteBenchTaskDefinition = { type: 'vitte', command: 'bench' };
      const task = new vscode.Task(
        definition,
        vscode.TaskScope.Workspace,
        'Vitte Bench',
        'vitte',
        new vscode.ShellExecution(cmd)
      );
      return [task];
    },
    resolveTask: (task: vscode.Task) => task,
  } as unknown as vscode.TaskProvider;
  ctx.subscriptions.push(vscode.tasks.registerTaskProvider('vitte', provider));
}

interface VitteConfig {
  build?: {
    profile?: string;
    distributed?: boolean;
    incremental?: boolean;
  };
  bench?: {
    reportDir?: string;
    export?: unknown;
    regressionThreshold?: number;
  };
}

interface VitteBenchTaskDefinition extends vscode.TaskDefinition {
  type: 'vitte';
  command: 'bench';
}

// ---- Helpers ----
async function benchBin(): Promise<string> {
  const located = await locateVitteRuntime();
  return located.benchPath ?? 'vitte-bench';
}

async function readProjectConfig(): Promise<VitteConfig | undefined> {
  try {
    const files = await vscode.workspace.findFiles('vitte.config.json', '**/node_modules/**', 1);
    const uri = files[0];
    if (!uri) return undefined;
    const doc = await vscode.workspace.openTextDocument(uri);
    return JSON.parse(doc.getText()) as VitteConfig;
  } catch {
    return undefined;
  }
}

async function benchReportDir(): Promise<string | undefined> {
  const cfg = vscode.workspace.getConfiguration('vitte');
  const project = await readProjectConfig();
  const root = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
  if (!root) return undefined;
  const reportDir = (project?.bench?.reportDir ?? cfg.get<string>('bench.reportDir')) ?? '.vitte/bench';
  return path.isAbsolute(reportDir) ? reportDir : path.join(root, reportDir);
}

async function benchArgs(): Promise<string[]> {
  const cfg = vscode.workspace.getConfiguration('vitte');
  const project = await readProjectConfig();

  const profile = project?.build?.profile ?? cfg.get<string>('build.profile') ?? 'bench';
  const distributed = (project?.build?.distributed ?? cfg.get<boolean>('build.distributed')) ?? false;
  const incremental = (project?.build?.incremental ?? cfg.get<boolean>('build.incremental')) ?? false;
  const reportDir = await benchReportDir();

  const args: string[] = ['--profile', profile];
  if (distributed) args.push('--distributed');
  if (incremental) args.push('--incremental');
  if (reportDir) args.push('--out', reportDir);

  // Export formats from config
  const exportKinds = project?.bench?.export;
  if (Array.isArray(exportKinds)) {
    for (const kind of exportKinds) {
      if (typeof kind === 'string') {
        args.push('--export', kind);
      }
    }
  }

  // Regression threshold
  if (typeof project?.bench?.regressionThreshold === 'number') {
    args.push('--threshold', String(project.bench.regressionThreshold));
  }

  return args;
}

async function benchCommandLine(): Promise<string> {
  const bin = await benchBin();
  const args = await benchArgs();
  const cmd = [quote(bin), ...args.map(quote)].join(' ');
  return cmd;
}

async function runBench(openAfter: boolean) {
  const root = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
  if (!root) {
    void vscode.window.showErrorMessage('Vitte: aucun workspace ouvert.');
    return;
  }

  const cmd = await benchCommandLine();
  const outChan = vscode.window.createOutputChannel('Vitte Bench');

  await vscode.window.withProgress({
    location: vscode.ProgressLocation.Notification,
    title: 'Vitte: Benchmarks en cours…',
    cancellable: false,
  }, async () => {
    outChan.clear();
    outChan.show(true);
    outChan.appendLine(`[cmd] ${cmd}`);

    await new Promise<void>((resolve) => {
      const proc = cp.spawn(cmd, { cwd: root, shell: true, env: process.env });
      proc.stdout?.on('data', (b: Buffer) => outChan.append(b.toString()));
      proc.stderr?.on('data', (b: Buffer) => outChan.append(b.toString()));
      proc.on('error', (e) => {
        outChan.appendLine(`\n[error] ${e.message}`);
        resolve();
      });
      proc.on('close', (code) => {
        outChan.appendLine(`\n[exit] code=${code}`);
        resolve();
      });
    });
  });

  if (openAfter) await openLatestReport();
}

async function openLatestReport() {
  try {
    const dir = await benchReportDir();
    if (!dir) return;
    const files = await vscode.workspace.findFiles(new vscode.RelativePattern(dir, '*.{html,svg}'));
    if (files.length === 0) {
      void vscode.window.showInformationMessage('Vitte Bench: aucun rapport HTML/SVG trouvé.');
      return;
    }
    // Sort by mtime (best-effort). Guard array access for noUncheckedIndexedAccess.
    const entries = files
      .map((u) => {
        try {
          return { uri: u, mtime: fs.statSync(u.fsPath).mtimeMs };
        } catch {
          return { uri: u, mtime: 0 };
        }
      })
      .sort((a, b) => b.mtime - a.mtime);

    const first = entries[0];
    if (!first) {
      void vscode.window.showInformationMessage('Vitte Bench: aucun rapport HTML/SVG trouvé.');
      return;
    }
    void vscode.commands.executeCommand('vscode.open', first.uri);
  } catch (e) {
    const message = e instanceof Error ? e.message : String(e);
    void vscode.window.showWarningMessage(`Vitte Bench: échec d'ouverture du rapport (${message})`);
  }
}

function quote(s: string): string {
  if (process.platform === 'win32') return `"${s.replace(/"/g, '\\"')}"`;
  return `'${s.replace(/'/g, `\'`)}'`;
}
