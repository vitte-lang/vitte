import { projectCommand, BUILD_PROFILES, findCompilerRepository, resolveProjectRoot } from "../utils/project";
import type { ProjectCommand } from "../utils/project";
import * as vscode from 'vscode';
import * as path from 'path';
import * as fs from 'fs';
import { isSupportedVitteCompilerVersion, locateVitteRuntime, VITTE_COMPILER_EVIDENCE_VERSION } from '../debug/runtimeLocator';
import { spawnManaged } from '../utils/managedProcesses';
import { requireTrustedWorkspace } from '../utils/workspaceTrust';
import { redactLogText } from '../utils/logRedaction';

const activeTaskExecutions = new Set<vscode.TaskExecution>();
let buildOutputChannel: vscode.OutputChannel | undefined;

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
  buildOutputChannel = vscode.window.createOutputChannel('Vitte Build');
  ctx.subscriptions.push(buildOutputChannel);
  ctx.subscriptions.push(
    vscode.commands.registerCommand('vitte.build', async () => { await runBuild('build', ctx); }),
    vscode.commands.registerCommand('vitte.clean', async () => { await runBuild('clean', ctx); }),
    vscode.commands.registerCommand('vitte.run', async () => { await runBuild('run', ctx); }),
    vscode.commands.registerCommand('vitte.test', async () => { await runBuild('test', ctx); }),
    vscode.commands.registerCommand('vitte.testCurrent', async () => { await runTestCurrentFile(); }),
    vscode.commands.registerCommand('vitte.runTests', async () => { await runBuild('test', ctx); }),
    vscode.commands.registerCommand('vitte.refreshTests', async () => {
      try {
        await vscode.commands.executeCommand('testing.refreshTests');
      } catch {
        void vscode.window.showInformationMessage('Vitte: impossible de rafraichir la vue Tests automatiquement.');
      }
    }),
    vscode.commands.registerCommand('vitte.switchProfile', async () => { await cycleProfile(); }),
    vscode.commands.registerCommand('vitte.toggleIncremental', async () => { await toggleIncremental(); }),
    vscode.tasks.onDidStartTask((event) => {
      if ((event.execution.task.definition as Partial<VitteTaskDefinition>).type === 'vitte') activeTaskExecutions.add(event.execution);
    }),
    vscode.tasks.onDidEndTask((event) => activeTaskExecutions.delete(event.execution)),
  );

  const provider = {
    provideTasks: async (_token?: vscode.CancellationToken): Promise<vscode.Task[]> => {
      if (!vscode.workspace.isTrusted) return [];
      const defs: { cmd: SubCmd; label: string }[] = [
        { cmd: 'build', label: 'Vitte Build' },
        { cmd: 'run', label: 'Vitte Run' },
      ];
      const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
      const configuredRoot = resolveProjectRoot(vscode.workspace.getConfiguration('vitte').get<string>('project.root'), workspaceRoot);
      if (configuredRoot && findCompilerRepository(configuredRoot)) defs.push({ cmd: 'test', label: 'Vitte Test (compiler repository)' });
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
      if (!(await requireTrustedWorkspace('running a Vitte task'))) return undefined;
      const command: unknown = task.definition.command;
      if (command !== 'build' && command !== 'run' && command !== 'test') return undefined;
      const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
      const configuredRoot = resolveProjectRoot(vscode.workspace.getConfiguration('vitte').get<string>('project.root'), workspaceRoot);
      if (command === 'test' && (!configuredRoot || !findCompilerRepository(configuredRoot))) return undefined;
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

export async function stopVitteTasks(): Promise<void> {
  const pending = [...activeTaskExecutions];
  for (const execution of pending) {
    try { execution.terminate(); } catch { /* task may already have ended */ }
  }
  const deadline = Date.now() + 1500;
  while (activeTaskExecutions.size > 0 && Date.now() < deadline) {
    await new Promise<void>((resolve) => setTimeout(resolve, 25));
  }
  activeTaskExecutions.clear();
}

type SubCmd = 'build' | 'clean' | 'run' | 'test';

interface VitteTaskDefinition extends vscode.TaskDefinition {
  type: 'vitte';
  command: SubCmd;
}

async function buildBin(): Promise<string> {
  const located = await locateVitteRuntime();
  const compiler = located.cliPath ?? located.runtimePath;
  if (!located.found || !compiler) {
    throw new Error('Vitte compiler not found. Set vitte.compiler.path, vitte.toolchain.root, or install Vitte on PATH.');
  }
  const binary = located.buildPath ?? compiler;
  const version = binary === located.cliPath ? located.versions?.cli : located.versions?.build;
  if (!isSupportedVitteCompilerVersion(version)) {
    const detectedVersion = version ?? 'unknown';
    throw new Error(`Vitte ${detectedVersion} is not verified by this extension; version ${VITTE_COMPILER_EVIDENCE_VERSION} is required for compiler-backed tasks. Run Vitte: Select Compiler or update vitte.compiler.path.`);
  }
  return binary;
}

async function defaultInputFile(root: string): Promise<string | undefined> {
  const current = vscode.window.activeTextEditor?.document.uri.fsPath;
  if (current && /\.(vit|vitl|vitte)$/i.test(current)) return current;

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
  const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath ?? process.cwd();
  const cfg = vscode.workspace.getConfiguration('vitte');
  const root = resolveProjectRoot(cfg.get<string>('project.root'), workspaceRoot) ?? workspaceRoot;
  const inCompilerRepository = Boolean(findCompilerRepository(root));
  const needsCli = !inCompilerRepository && (sub === 'build' || sub === 'run');
  const plan = projectCommand({
    root, compiler: needsCli ? await buildBin() : '', action: sub,
    input: await defaultInputFile(root),
    profile: cfg.get<string>('build.profile', 'default'), incremental: cfg.get<boolean>('build.incremental', false),
  });
  if (plan.bin !== 'make') {
    const extraArgs = cfg.get<unknown>('build.args', []);
    if (Array.isArray(extraArgs)) plan.args.push(...extraArgs.filter((value): value is string => typeof value === 'string'));
  }
  return plan;
}

function formatCommandForDisplay(bin: string, args: string[]): string {
  return [bin, ...args].map((s) => JSON.stringify(s)).join(' ');
}

async function runBuild(sub: SubCmd, ctx: vscode.ExtensionContext) {
  const workspaceRoot = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
  if (!workspaceRoot) {
    void vscode.window.showErrorMessage('Vitte: aucun workspace ouvert.');
    return;
  }
  if (!(await requireTrustedWorkspace(`running ${sub}`))) return;

  let plan: ProjectCommand;
  try { plan = await buildCommand(sub); }
  catch (error) {
    const message = redactLogText(error);
    if (/not found|not verified/i.test(message)) {
      const selection = await vscode.window.showErrorMessage(`Vitte: ${message}`, 'Select Compiler', 'Open Settings');
      if (selection === 'Select Compiler') await vscode.commands.executeCommand('vitte.selectRuntime');
      else if (selection === 'Open Settings') await vscode.commands.executeCommand('workbench.action.openSettings', '@ext:VitteStudio.vitte-studio vitte.compiler.path');
    } else {
      const selection = await vscode.window.showErrorMessage(`Vitte: ${message}`, 'Show Capabilities');
      if (selection === 'Show Capabilities') await vscode.commands.executeCommand('vitte.showCapabilities');
    }
    return;
  }
  if (sub === 'clean' && !plan.bin) {
    if (plan.outputDir) await fs.promises.rm(plan.outputDir, { recursive: true, force: true });
    return;
  }
  if (plan.outputDir) await fs.promises.mkdir(plan.outputDir, { recursive: true });
  const { bin, args } = plan;

  const chan = buildOutputChannel ?? vscode.window.createOutputChannel('Vitte Build');
  if (!buildOutputChannel) {
    buildOutputChannel = chan;
    ctx.subscriptions.push(chan);
  }
  await vscode.window.withProgress({
    location: vscode.ProgressLocation.Notification,
    title: `Vitte: ${sub}…`,
    cancellable: false,
  }, async () => {
    chan.clear();
    chan.show(true);
    chan.appendLine(`[cmd] ${redactLogText(formatCommandForDisplay(bin, args))}`);
    await new Promise<void>((resolve) => {
      const proc = spawnManaged(bin, args, { cwd: plan.cwd, shell: false, env: process.env });
      proc.stdout?.on('data', (b: Buffer) => chan.append(redactLogText(b.toString())));
      proc.stderr?.on('data', (b: Buffer) => chan.append(redactLogText(b.toString())));
      proc.on('error', (e) => { chan.appendLine(`\n[error] ${redactLogText(e.message)}`); resolve(); });
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
