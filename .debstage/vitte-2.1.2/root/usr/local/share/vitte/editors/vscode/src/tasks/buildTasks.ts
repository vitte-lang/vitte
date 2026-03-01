import * as vscode from 'vscode';
import * as cp from 'child_process';
import { locateVitteRuntime } from '../debug/runtimeLocator';

/**
 * Build / Run / Test tasks & commands for Vitte
 *
 * Commands:
 *  - vitte.build           → builds the workspace
 *  - vitte.clean           → cleans build artifacts
 *  - vitte.run             → runs the project (or current file)
 *  - vitte.test            → runs all tests discovered by the toolchain
 *  - vitte.testCurrent     → runs tests for the current file if applicable
 *  - vitte.switchProfile   → cycles dev → test → release → bench
 *  - vitte.toggleIncremental → toggles incremental build
 *
 * Task provider:
 *  - type: 'vitte' with command: 'build' | 'clean' | 'run' | 'test'
 */
export function registerBuildTasks(ctx: vscode.ExtensionContext) {
  // Commands
  ctx.subscriptions.push(
    vscode.commands.registerCommand('vitte.build', async () => { await runBuild('build'); }),
    vscode.commands.registerCommand('vitte.clean', async () => { await runBuild('clean'); }),
    vscode.commands.registerCommand('vitte.run', async () => { await runBuild('run'); }),
    vscode.commands.registerCommand('vitte.test', async () => { await runBuild('test'); }),
    vscode.commands.registerCommand('vitte.testCurrent', async () => { await runTestCurrentFile(); }),
    vscode.commands.registerCommand('vitte.switchProfile', async () => { await cycleProfile(); }),
    vscode.commands.registerCommand('vitte.toggleIncremental', async () => { await toggleIncremental(); }),
  );

  const provider = {
    provideTasks: async (_token?: vscode.CancellationToken): Promise<vscode.Task[]> => {
      const defs: { cmd: SubCmd; label: string }[] = [
        { cmd: 'build', label: 'Vitte Build' },
        { cmd: 'run',   label: 'Vitte Run' },
        { cmd: 'test',  label: 'Vitte Test' },
        { cmd: 'clean', label: 'Vitte Clean' },
      ];
      return Promise.all(defs.map(async ({ cmd, label }) => {
        const exec = new vscode.ShellExecution(await buildCommandLine(cmd));
        const definition: VitteTaskDefinition = { type: 'vitte', command: cmd };
        return new vscode.Task(definition, vscode.TaskScope.Workspace, label, 'vitte', exec);
      }));
    },
    resolveTask: (task: vscode.Task) => task,
  } as unknown as vscode.TaskProvider;
  ctx.subscriptions.push(vscode.tasks.registerTaskProvider('vitte', provider));
}

// ---- Types & helpers ----

type SubCmd = 'build' | 'clean' | 'run' | 'test';

interface VitteBuildConfig {
  build?: {
    profile?: string;
    distributed?: boolean;
    incremental?: boolean;
  };
  targets?: (string | { triple?: string })[];
}

interface VitteTaskDefinition extends vscode.TaskDefinition {
  type: 'vitte';
  command: SubCmd;
}

async function readProjectConfig(): Promise<VitteBuildConfig | undefined> {
  try {
    const files = await vscode.workspace.findFiles('vitte.config.json', '**/node_modules/**', 1);
    const first = files[0];
    if (!first) return undefined;
    const doc = await vscode.workspace.openTextDocument(first);
    return JSON.parse(doc.getText()) as VitteBuildConfig;
  } catch {
    return undefined;
  }
}

async function buildBin(): Promise<string> {
  const located = await locateVitteRuntime();
  return located.buildPath ?? 'vitte-build';
}

async function buildArgs(sub: SubCmd, extra?: { currentFile?: string }): Promise<string[]> {
  const cfg = vscode.workspace.getConfiguration('vitte');
  const project = await readProjectConfig();

  const profile = project?.build?.profile ?? cfg.get<string>('build.profile') ?? 'dev';
  const distributed = (project?.build?.distributed ?? cfg.get<boolean>('build.distributed')) ?? false;
  const incremental = (project?.build?.incremental ?? cfg.get<boolean>('build.incremental')) ?? false;
  const targets = collectTargets(project);

  const args: string[] = [sub, '--profile', profile];
  if (distributed) args.push('--distributed');
  if (incremental) args.push('--incremental');
  for (const t of targets) args.push('--target', t);

  if (sub === 'run') {
    // allow running current file with runtime directly if build tool supports pass-through
    if (extra?.currentFile) args.push('--file', extra.currentFile);
  }

  return args;
}

function collectTargets(project?: VitteBuildConfig): string[] {
  const t = project?.targets;
  const out: string[] = [];
  if (Array.isArray(t)) {
    for (const x of t) {
      if (typeof x === 'string') out.push(x);
      else if (x && typeof x === 'object') {
        const triple = (x as { triple?: unknown }).triple;
        if (typeof triple === 'string') out.push(triple);
      }
    }
  }
  return out;
}

async function buildCommandLine(sub: SubCmd, extra?: { currentFile?: string }): Promise<string> {
  const bin = await buildBin();
  const args = await buildArgs(sub, extra);
  return [quote(bin), ...args.map(quote)].join(' ');
}

async function runBuild(sub: SubCmd) {
  const root = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
  if (!root) {
    void vscode.window.showErrorMessage('Vitte: aucun workspace ouvert.');
    return;
  }

  // run current file for run/test if applicable
  let cmdLine: string;
  if (sub === 'run') {
    const current = vscode.window.activeTextEditor?.document.uri.fsPath;
    const isVitte = !!(current && /\.(vitte|vit)$/i.test(current));
    const extra = isVitte && current ? { currentFile: current } : undefined;
    cmdLine = await buildCommandLine('run', extra);
  } else if (sub === 'test') {
    cmdLine = await buildCommandLine('test');
  } else if (sub === 'clean') {
    cmdLine = await buildCommandLine('clean');
  } else {
    cmdLine = await buildCommandLine('build');
  }

  const chan = vscode.window.createOutputChannel('Vitte Build');
  await vscode.window.withProgress({
    location: vscode.ProgressLocation.Notification,
    title: `Vitte: ${sub}…`,
    cancellable: false,
  }, async () => {
    chan.clear();
    chan.show(true);
    chan.appendLine(`[cmd] ${cmdLine}`);
    await new Promise<void>((resolve) => {
      const proc = cp.spawn(cmdLine, { cwd: root, shell: true, env: process.env });
      proc.stdout?.on('data', (b: Buffer) => chan.append(b.toString()));
      proc.stderr?.on('data', (b: Buffer) => chan.append(b.toString()));
      proc.on('error', (e) => { chan.appendLine(`\n[error] ${e.message}`); resolve(); });
      proc.on('close', (code) => { chan.appendLine(`\n[exit] code=${code}`); resolve(); });
    });
  });
}

async function runTestCurrentFile() {
  const file = vscode.window.activeTextEditor?.document.uri.fsPath;
  if (!file) { void vscode.window.showInformationMessage('Aucun fichier actif.'); return; }
  const isTest = /(_test\.vitte|\.(vitte|vit))$/i.test(file); // permissive
  if (!isTest) { void vscode.window.showInformationMessage('Le fichier actif ne semble pas être un test.'); return; }
  await runBuild('test');
}

async function cycleProfile() {
  const cfg = vscode.workspace.getConfiguration('vitte');
  const current = (cfg.get<string>('build.profile') ?? 'dev').toLowerCase();
  const order = ['dev', 'test', 'release', 'bench'];
  const idx = order.indexOf(current);
  const next = order[(idx + 1) % order.length];
  await cfg.update('build.profile', next, vscode.ConfigurationTarget.Workspace);
  void vscode.window.showInformationMessage(`Vitte: profil → ${next}`);
}

async function toggleIncremental() {
  const cfg = vscode.workspace.getConfiguration('vitte');
  const cur = Boolean(cfg.get<boolean>('build.incremental'));
  await cfg.update('build.incremental', !cur, vscode.ConfigurationTarget.Workspace);
  void vscode.window.showInformationMessage(`Vitte: incremental → ${!cur ? 'ON' : 'OFF'}`);
}

function quote(s: string): string {
  if (process.platform === 'win32') return `"${s.replace(/"/g, '\\"')}"`;
  return `'${s.replace(/'/g, `\'`)}'`;
}
