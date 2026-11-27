import { projectCommand, BUILD_PROFILES, findCompilerRepository } from "../utils/project";
import type { ProjectCommand } from "../utils/project";
import * as vscode from 'vscode';
import * as cp from 'child_process';
import * as path from 'path';
import * as fs from 'fs';
import { locateVitteRuntime } from '../debug/runtimeLocator';

/**
 * Build / Run / Test tasks & commands for Vitte
 *
 * Commands:
 *  - vitte.build             → builds the workspace
 *  - vitte.clean             → cleans build artifacts
 *  - vitte.run               → runs the project (or current file)
 *  - vitte.test              → runs the compiler repository's Make test target
 *  - vitte.testCurrent       → explains that current-file tests are unavailable
 *  - vitte.runTests          → alias for full test run
 *  - vitte.refreshTests      → refresh VS Code Testing view
 *  - vitte.switchProfile     → cycles default → debug → release → size → sanitize → freestanding
 *  - vitte.toggleIncremental → toggles incremental build
 *
 * Task provider:
 *  - type: 'vitte' with command: 'build' | 'clean' | 'run' | 'test'
 */
export function registerBuildTasks(ctx: vscode.ExtensionContext) {
  ctx.subscriptions.push(
    vscode.commands.registerCommand('vitte.build', async () => { await runBuild('build'); }),
    vscode.commands.registerCommand('vitte.clean', async () => { await runBuild('clean'); }),
    vscode.commands.registerCommand('vitte.run', async () => { await runBuild('run'); }),
    vscode.commands.registerCommand('vitte.test', async () => { await runBuild('test'); }),
    vscode.commands.registerCommand('vitte.testCurrent', async () => { await runTestCurrentFile(); }),
    vscode.commands.registerCommand('vitte.runTests', async () => { await runBuild('test'); }),
    vscode.commands.registerCommand('vitte.refreshTests', async () => {
      try {
        await vscode.commands.executeCommand('testing.refreshTests');
      } catch {
        void vscode.window.showInformationMessage('Vitte: impossible de rafraichir la vue Tests automatiquement.');
      }
    }),
    vscode.commands.registerCommand('vitte.switchProfile', async () => { await cycleProfile(); }),
    vscode.commands.registerCommand('vitte.toggleIncremental', async () => { await toggleIncremental(); }),
  );

  const provider = {
    provideTasks: async (_token?: vscode.CancellationToken): Promise<vscode.Task[]> => {
      const defs: { cmd: SubCmd; label: string }[] = [
        { cmd: 'build', label: 'Vitte Build' },
        { cmd: 'run', label: 'Vitte Run' },
      ];
      const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
      if (workspaceRoot && findCompilerRepository(workspaceRoot)) defs.push({ cmd: 'test', label: 'Vitte Test (compiler repository)' });
      const tasks = await Promise.all(defs.map(async ({ cmd, label }) => {
        try {
        const plan = await buildCommand(cmd);
        if (plan.outputDir) await fs.promises.mkdir(plan.outputDir, { recursive: true });
        const exec = new vscode.ProcessExecution(plan.bin, plan.args, { cwd: plan.cwd });
        const definition: VitteTaskDefinition = { type: 'vitte', command: cmd };
        return new vscode.Task(definition, vscode.TaskScope.Workspace, label, 'vitte', exec);
        } catch { return undefined; }
      }));
      return tasks.filter((task): task is vscode.Task => !!task);
    },
    resolveTask: async (task: vscode.Task) => {
      const command: unknown = task.definition.command;
      if (command !== 'build' && command !== 'run' && command !== 'test') return undefined;
      const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
      if (command === 'test' && (!workspaceRoot || !findCompilerRepository(workspaceRoot))) return undefined;
      try {
        const plan = await buildCommand(command);
        if (plan.outputDir) await fs.promises.mkdir(plan.outputDir, { recursive: true });
        task.execution = new vscode.ProcessExecution(plan.bin, plan.args, { cwd: plan.cwd });
        return task;
      } catch { return undefined; }
    },
  } as unknown as vscode.TaskProvider;
  ctx.subscriptions.push(vscode.tasks.registerTaskProvider('vitte', provider));
}

type SubCmd = 'build' | 'clean' | 'run' | 'test';

interface VitteTaskDefinition extends vscode.TaskDefinition {
  type: 'vitte';
  command: SubCmd;
}

async function buildBin(): Promise<string> {
  const located = await locateVitteRuntime();
  return located.buildPath ?? located.cliPath ?? located.runtimePath ?? 'vitte';
}

async function defaultInputFile(): Promise<string | undefined> {
  const current = vscode.window.activeTextEditor?.document.uri.fsPath;
  if (current && /\.(vit|vitl|vitte)$/i.test(current)) return current;

  const root = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
  if (!root) return undefined;

  const candidates = [
    path.join(root, 'src', 'main.vit'),
    path.join(root, 'main.vit'),
    path.join(findCompilerRepository(root) ?? root, 'examples', 'hello.vit'),
  ];
  for (const candidate of candidates) {
    try {
      await vscode.workspace.fs.stat(vscode.Uri.file(candidate));
      return candidate;
    } catch {
      // try next candidate
    }
  }
  return undefined;
}

async function buildCommand(sub: SubCmd): Promise<ProjectCommand> {
  const root = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ?? process.cwd();
  const cfg = vscode.workspace.getConfiguration('vitte');
  return projectCommand({
    root, compiler: await buildBin(), action: sub,
    input: await defaultInputFile(),
    profile: cfg.get<string>('build.profile', 'default'), incremental: cfg.get<boolean>('build.incremental', false),
  });
}

function formatCommandForDisplay(bin: string, args: string[]): string {
  return [bin, ...args].map((s) => JSON.stringify(s)).join(' ');
}

async function runBuild(sub: SubCmd) {
  const root = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
  if (!root) {
    void vscode.window.showErrorMessage('Vitte: aucun workspace ouvert.');
    return;
  }

  let plan: ProjectCommand;
  try { plan = await buildCommand(sub); }
  catch (error) { void vscode.window.showErrorMessage(`Vitte: ${String(error)}`); return; }
  if (sub === 'clean' && !plan.bin) {
    if (plan.outputDir) await fs.promises.rm(plan.outputDir, { recursive: true, force: true });
    return;
  }
  if (plan.outputDir) await fs.promises.mkdir(plan.outputDir, { recursive: true });
  const { bin, args } = plan;

  const chan = vscode.window.createOutputChannel('Vitte Build');
  await vscode.window.withProgress({
    location: vscode.ProgressLocation.Notification,
    title: `Vitte: ${sub}…`,
    cancellable: false,
  }, async () => {
    chan.clear();
    chan.show(true);
    chan.appendLine(`[cmd] ${formatCommandForDisplay(bin, args)}`);
    await new Promise<void>((resolve) => {
      const proc = cp.spawn(bin, args, { cwd: plan.cwd, shell: false, env: process.env });
      proc.stdout?.on('data', (b: Buffer) => chan.append(b.toString()));
      proc.stderr?.on('data', (b: Buffer) => chan.append(b.toString()));
      proc.on('error', (e) => { chan.appendLine(`\n[error] ${e.message}`); resolve(); });
      proc.on('close', (code) => { chan.appendLine(`\n[exit] code=${code}`); resolve(); });
    });
  });
}

async function runTestCurrentFile() {
  const choice = await vscode.window.showWarningMessage(
    'Current-file tests are unavailable in the Vitte 0.1.1 CLI. No test command was run.',
    'Show Capabilities',
  );
  if (choice === 'Show Capabilities') await vscode.commands.executeCommand('vitte.showCapabilities');
}

async function cycleProfile() {
  const cfg = vscode.workspace.getConfiguration('vitte');
  const current = (cfg.get<string>('build.profile') ?? 'default').toLowerCase();
  const order = [...BUILD_PROFILES];
  const idx = order.findIndex(profile => profile === current);
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
