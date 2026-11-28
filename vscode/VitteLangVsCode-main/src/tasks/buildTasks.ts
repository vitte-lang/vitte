import { projectCommand, BUILD_PROFILES, detectVitteProject, expandToolPath, findCompilerRepository, hasMakeTarget, resolveProjectRoot } from "../utils/project";
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

type SubCmd = 'check' | 'build' | 'clean' | 'run' | 'test';

interface VitteTaskDefinition extends vscode.TaskDefinition {
  type: 'vitte';
  command: SubCmd;
  /** Optional command-line arguments for the compiler, separate from the source path. */
  args?: string[];
  /** Arguments forwarded to the running Vitte program after `--`. */
  programArgs?: string[];
  /** Run the selected CLI source action instead of a compiler checkout Make target. */
  currentFileOnly?: boolean;
  /** Explicitly build the compiler checkout with its Make target. */
  compilerRepository?: boolean;
  /** Optional Vitte source file. Defaults to the active file in this workspace folder. */
  file?: string;
  /** Optional target triple/platform passed through to the CLI. */
  target?: string;
  /** Optional CLI manifest path; relative paths resolve from the detected project root. */
  manifestPath?: string;
  /** Stable workspace-folder URI used by generated multi-root tasks. */
  projectFolder?: string;
}

/** Build, check, run and compiler-repository test commands plus VS Code tasks. */
export function registerBuildTasks(ctx: vscode.ExtensionContext) {
  buildOutputChannel = vscode.window.createOutputChannel('Vitte Build');
  ctx.subscriptions.push(buildOutputChannel);
  ctx.subscriptions.push(
    vscode.commands.registerCommand('vitte.check', async (args?: unknown) => { await runBuild('check', ctx, args); }),
    vscode.commands.registerCommand('vitte.build', async (args?: unknown) => { await runBuild('build', ctx, args); }),
    vscode.commands.registerCommand('vitte.clean', async () => { await runBuild('clean', ctx); }),
    vscode.commands.registerCommand('vitte.run', async (args?: unknown) => { await runBuild('run', ctx, args); }),
    vscode.commands.registerCommand('vitte.test', async (args?: unknown) => { await runBuild('test', ctx, args); }),
    vscode.commands.registerCommand('vitte.testCurrent', async () => { await runTestCurrentFile(); }),
    vscode.commands.registerCommand('vitte.runTests', async (args?: unknown) => { await runBuild('test', ctx, args); }),
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

  const provider: vscode.TaskProvider = {
    provideTasks: (): vscode.Task[] => {
      if (!vscode.workspace.isTrusted) return [];
      const folders = vscode.workspace.workspaceFolders ?? [];
      const tasks: vscode.Task[] = [];
      for (const folder of folders) {
        const config = vscode.workspace.getConfiguration('vitte', folder.uri);
        const projectRoot = resolveProjectRoot(config.get<string>('project.root'), folder.uri.fsPath);
        const repository = projectRoot ? findCompilerRepository(projectRoot) : undefined;
        const detectedProject = projectRoot ? detectVitteProject(projectRoot) : undefined;
        const repositoryWorkflow = Boolean(repository && !detectedProject);
        const definitions: { command: SubCmd; currentFileOnly?: boolean; compilerRepository?: boolean; label?: string }[] = [
          { command: 'check', currentFileOnly: true },
          { command: 'build', currentFileOnly: true },
          { command: 'run' },
        ];
        if (repositoryWorkflow && repository && hasMakeTarget(repository, 'all')) {
          definitions.push({ command: 'build', compilerRepository: true, label: 'Build Compiler Repository' });
        }
        if (repositoryWorkflow && repository && hasMakeTarget(repository, 'test')) definitions.push({ command: 'test', label: 'Test Compiler Repository' });
        for (const taskOptions of definitions) {
          const { command } = taskOptions;
          const definition: VitteTaskDefinition = { type: 'vitte', command, projectFolder: folder.uri.toString(), ...(taskOptions.currentFileOnly ? { currentFileOnly: true } : {}), ...(taskOptions.compilerRepository ? { compilerRepository: true } : {}) };
          const suffix = folders.length > 1 ? ` — ${folder.name}` : '';
          const label = taskOptions.label ?? `Vitte ${command[0]!.toUpperCase()}${command.slice(1)}`;
          const task = new vscode.Task(definition, folder, `${label}${suffix}`, 'vitte');
          task.problemMatchers = ['$vitte'];
          task.group = command === 'test' ? vscode.TaskGroup.Test : vscode.TaskGroup.Build;
          tasks.push(task);
        }
      }
      return tasks;
    },
    resolveTask: async (task: vscode.Task) => {
      if (!(await requireTrustedWorkspace('running a Vitte task'))) return undefined;
      const definition = task.definition as Partial<VitteTaskDefinition>;
      if (!isTaskCmd(definition.command)) return undefined;
      const folder = taskFolder(task, definition.projectFolder);
      if (!folder) return undefined;
      try {
        const plan = await buildCommand(definition.command, folder, definition);
        if (plan.outputDir) await fs.promises.mkdir(plan.outputDir, { recursive: true });
        const processOptions = plan.env ? { env: processExecutionEnv(plan.env) } : {};
        task.execution = new vscode.ProcessExecution(plan.bin, plan.args, { cwd: plan.cwd, ...processOptions });
        task.problemMatchers = ['$vitte'];
        return task;
      } catch (error) {
        const message = redactLogText(error);
        buildOutputChannel?.appendLine(`[task:${folder.name}] ${message}`);
        void vscode.window.showErrorMessage(`Vitte: ${message}`, 'Select Compiler', 'Open Output').then(async (choice) => {
          if (choice === 'Select Compiler') await vscode.commands.executeCommand('vitte.selectRuntime', folder.uri.toString());
          else if (choice === 'Open Output') buildOutputChannel?.show(true);
        });
        return undefined;
      }
    },
  };
  ctx.subscriptions.push(vscode.tasks.registerTaskProvider('vitte', provider));
}

function isTaskCmd(value: unknown): value is Exclude<SubCmd, 'clean'> {
  return value === 'check' || value === 'build' || value === 'run' || value === 'test';
}

function taskFolder(task: vscode.Task, projectFolder?: string): vscode.WorkspaceFolder | undefined {
  const scope = task.scope;
  if (scope && typeof scope === 'object' && 'uri' in scope) return scope;
  if (projectFolder) {
    const uri = vscode.Uri.parse(projectFolder);
    return vscode.workspace.getWorkspaceFolder(uri) ?? (vscode.workspace.workspaceFolders ?? []).find(folder => folder.uri.toString() === projectFolder);
  }
  const folders = vscode.workspace.workspaceFolders ?? [];
  return folders.length === 1 ? folders[0] : undefined;
}

async function selectWorkspaceFolder(): Promise<vscode.WorkspaceFolder | undefined> {
  const activeUri = vscode.window.activeTextEditor?.document.uri;
  const activeFolder = activeUri ? vscode.workspace.getWorkspaceFolder(activeUri) : undefined;
  if (activeFolder) return activeFolder;
  const folders = vscode.workspace.workspaceFolders ?? [];
  if (folders.length === 1) return folders[0];
  if (folders.length === 0) return undefined;
  const picked = await vscode.window.showQuickPick(folders.map(folder => ({ label: folder.name, description: folder.uri.fsPath, folder })), {
    placeHolder: 'Select the Vitte workspace folder for this command',
  });
  return picked?.folder;
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

async function buildBin(folder: vscode.WorkspaceFolder): Promise<string> {
  const located = await locateVitteRuntime(undefined, folder.uri);
  const compiler = located.cliPath ?? located.runtimePath;
  if (!located.found || !compiler) {
    throw new Error(`Vitte compiler not found for workspace folder “${folder.name}”. Set vitte.compiler.path or vitte.toolchain.root for this folder, or install Vitte on PATH.`);
  }
  const binary = located.buildPath ?? compiler;
  const version = binary === located.cliPath ? located.versions?.cli : located.versions?.build;
  if (!isSupportedVitteCompilerVersion(version)) {
    const detectedVersion = version ?? 'unknown';
    throw new Error(`Vitte ${detectedVersion} in “${folder.name}” is not verified by this extension; version ${VITTE_COMPILER_EVIDENCE_VERSION} is required for compiler-backed tasks. Run Vitte: Select Compiler or update this folder’s vitte.compiler.path.`);
  }
  return binary;
}

async function defaultInputFile(root: string, folder: vscode.WorkspaceFolder, override?: string): Promise<string | undefined> {
  const active = vscode.window.activeTextEditor?.document;
  const activeFolder = active ? vscode.workspace.getWorkspaceFolder(active.uri) : undefined;
  if (override?.trim()) {
    const expanded = override.replace(/\$\{file\}/g, active?.uri.fsPath ?? '');
    return expandToolPath(expanded, root);
  }
  if (active && activeFolder?.uri.toString() === folder.uri.toString() && /\.(vit|vitl|vitte)$/i.test(active.uri.fsPath)) return active.uri.fsPath;

  const repository = findCompilerRepository(root);
  const candidates = [
    path.join(root, 'src', 'main.vit'),
    path.join(root, 'main.vit'),
    path.join(repository ?? root, 'examples', 'hello.vit'),
  ];
  for (const candidate of candidates) {
    try {
      await vscode.workspace.fs.stat(vscode.Uri.file(candidate));
      return candidate;
    } catch { /* try the next conventional entry point */ }
  }
  return undefined;
}

async function buildCommand(sub: SubCmd, folder: vscode.WorkspaceFolder, definition?: Partial<VitteTaskDefinition>): Promise<ProjectCommand> {
  const cfg = vscode.workspace.getConfiguration('vitte', folder.uri);
  const activeDocument = vscode.window.activeTextEditor?.document;
  const activeFolder = activeDocument ? vscode.workspace.getWorkspaceFolder(activeDocument.uri) : undefined;
  const resourcePath = activeFolder?.uri.toString() === folder.uri.toString() ? activeDocument?.uri.fsPath : undefined;
  const selectedProjectRoot = resolveProjectRoot(cfg.get<string>('project.root'), folder.uri.fsPath, resourcePath) ?? folder.uri.fsPath;
  let projectRoot = selectedProjectRoot;
  const repository = findCompilerRepository(projectRoot);
  if (definition?.compilerRepository) {
    if (!repository) throw new Error(`No Vitte compiler repository is available for workspace folder “${folder.name}”.`);
    const selectedConfig = detectVitteProject(resourcePath ?? selectedProjectRoot);
    if (selectedConfig && path.resolve(selectedConfig.root) !== path.resolve(repository)) {
      throw new Error(`The active Vitte project config at ${selectedConfig.configFile} conflicts with the requested compiler-repository Make build. Run the active source with Vitte Build instead.`);
    }
    projectRoot = repository;
  }
  const currentFileOnly = definition?.currentFileOnly === true;
  const needsCli = (sub === 'check' || sub === 'build' || sub === 'run') && !definition?.compilerRepository && (!repository || sub === 'run' || ((sub === 'check' || sub === 'build') && currentFileOnly));
  const compiler = needsCli ? await buildBin(folder) : '';
  const input = await defaultInputFile(projectRoot, folder, definition?.file);
  const taskArgs = Array.isArray(definition?.args) ? definition.args.filter((value): value is string => typeof value === 'string') : [];
  const programArgs = Array.isArray(definition?.programArgs) ? definition.programArgs.filter((value): value is string => typeof value === 'string') : [];
  const settingArgs = cfg.get<unknown>('build.args', []);
  const configuredArgs = Array.isArray(settingArgs) ? settingArgs.filter((value): value is string => typeof value === 'string') : [];
  const args = [...configuredArgs, ...taskArgs];
  const settingEnv = cfg.get<unknown>('build.env', {});
  const env = settingEnv && typeof settingEnv === 'object' && !Array.isArray(settingEnv)
    ? Object.fromEntries(Object.entries(settingEnv as Record<string, unknown>)
      .filter((entry): entry is [string, string] => typeof entry[1] === 'string')
      .map(([key, value]) => [key, expandToolPath(value, projectRoot)]))
    : {};
  const target = typeof definition?.target === 'string' && definition.target.trim()
    ? definition.target.trim()
    : cfg.get<string>('build.target', '').trim();
  return projectCommand({
    root: projectRoot,
    compiler,
    action: sub,
    input,
    profile: cfg.get<string>('build.profile', 'default'),
    incremental: cfg.get<boolean>('build.incremental', false),
    target,
    args,
    programArgs,
    env,
    modulePaths: cfg.get<string[]>('modulePaths', []),
    compilerRepository: definition?.compilerRepository === true,
    manifestPath: typeof definition?.manifestPath === 'string' && definition.manifestPath.trim()
      ? definition.manifestPath
      : cfg.get<string>('project.manifestPath', ''),
    currentFileOnly,
  });
}

function formatCommandForDisplay(bin: string, args: string[]): string {
  return [bin, ...args].map((s) => JSON.stringify(s)).join(' ');
}

function processExecutionEnv(env: NodeJS.ProcessEnv): Record<string, string> {
  return Object.fromEntries(Object.entries(env).filter((entry): entry is [string, string] => typeof entry[1] === 'string'));
}

function commandArgs(value: unknown): string[] {
  if (Array.isArray(value)) return value.filter((item): item is string => typeof item === 'string');
  if (value && typeof value === 'object' && Array.isArray((value as { args?: unknown }).args)) {
    return (value as { args: unknown[] }).args.filter((item): item is string => typeof item === 'string');
  }
  return [];
}

async function runBuild(sub: SubCmd, ctx: vscode.ExtensionContext, selectedArgs?: unknown) {
  const folder = await selectWorkspaceFolder();
  if (!folder) {
    void vscode.window.showErrorMessage('Vitte: no workspace folder is selected.');
    return;
  }
  if (!(await requireTrustedWorkspace(`running ${sub}`))) return;

  let plan: ProjectCommand;
  try {
    const cliArgs = commandArgs(selectedArgs);
    plan = await buildCommand(sub, folder, { ...(cliArgs.length ? { args: cliArgs } : {}), ...(['check', 'build'].includes(sub) ? { currentFileOnly: true } : {}) });
  } catch (error) {
    const message = redactLogText(error);
    if (/not found|not verified/i.test(message)) {
      const selection = await vscode.window.showErrorMessage(`Vitte: ${message}`, 'Select Compiler', 'Open Settings');
      if (selection === 'Select Compiler') await vscode.commands.executeCommand('vitte.selectRuntime', folder.uri.toString());
      else if (selection === 'Open Settings') await vscode.commands.executeCommand('workbench.action.openSettings', '@ext:VitteStudio.vitte-studio vitte.compiler.path');
    } else {
      const selection = await vscode.window.showErrorMessage(`Vitte: ${message}`, 'Show Capabilities');
      if (selection === 'Show Capabilities') await vscode.commands.executeCommand('vitte.showCapabilities');
    }
    return;
  }
  if (sub === 'clean' && !plan.bin) {
    if (plan.outputDir) await fs.promises.rm(plan.outputDir, { recursive: true, force: true });
    void vscode.window.showInformationMessage(`Vitte: cleaned ${plan.outputDir ?? folder.uri.fsPath}`);
    return;
  }
  if (plan.outputDir) await fs.promises.mkdir(plan.outputDir, { recursive: true });
  const chan = buildOutputChannel ?? vscode.window.createOutputChannel('Vitte Build');
  if (!buildOutputChannel) {
    buildOutputChannel = chan;
    ctx.subscriptions.push(chan);
  }
  await vscode.window.withProgress({ location: vscode.ProgressLocation.Notification, title: `Vitte: ${sub} — ${folder.name}…`, cancellable: false }, async () => {
    chan.clear();
    chan.show(true);
    chan.appendLine(`[folder] ${redactLogText(folder.uri.fsPath)}`);
    chan.appendLine(`[project] ${redactLogText(plan.cwd)}`);
    chan.appendLine(`[cmd] ${redactLogText(formatCommandForDisplay(plan.bin, plan.args))}`);
    const result = await new Promise<{ code: number | null; error?: string }>((resolve) => {
      const proc = spawnManaged(plan.bin, plan.args, { cwd: plan.cwd, shell: false, env: plan.env ?? process.env });
      let settled = false;
      const finish = (value: { code: number | null; error?: string }) => {
        if (settled) return;
        settled = true;
        resolve(value);
      };
      proc.stdout?.on('data', (b: Buffer) => chan.append(redactLogText(b.toString())));
      proc.stderr?.on('data', (b: Buffer) => chan.append(redactLogText(b.toString())));
      proc.on('error', (error) => {
        chan.appendLine(`\n[error] ${redactLogText(error.message)}`);
        finish({ code: null, error: error.message });
      });
      proc.on('close', (code) => {
        chan.appendLine(`\n[exit] code=${code}`);
        finish({ code });
      });
    });
    if (result.code === 0) {
      void vscode.window.showInformationMessage(`Vitte ${sub} completed for ${folder.name}.`);
    } else {
      const status = result.code === null ? 'could not start' : `exited with code ${result.code}`;
      const selection = await vscode.window.showErrorMessage(`Vitte ${sub} ${status} for ${folder.name}. See Vitte Build output for details.`, 'Open Output');
      if (selection === 'Open Output') chan.show(true);
    }
  });
}

async function runTestCurrentFile() {
  const choice = await vscode.window.showWarningMessage(
    'Vitte CLI 0.1.1 has no test discovery metadata or structured test-result contract, so the Test Explorer cannot list or debug Vitte tests. A compiler checkout with a Make test target can still run that repository-wide target from Tasks.',
    'Show Capabilities',
  );
  if (choice === 'Show Capabilities') await vscode.commands.executeCommand('vitte.showCapabilities');
}

async function cycleProfile() {
  const folder = await selectWorkspaceFolder();
  if (!folder) return;
  const cfg = vscode.workspace.getConfiguration('vitte', folder.uri);
  const current = (cfg.get<string>('build.profile') ?? 'default').toLowerCase();
  const order = [...BUILD_PROFILES];
  const idx = order.findIndex(profile => profile === current);
  const next = order[(idx + 1) % order.length];
  await cfg.update('build.profile', next, vscode.ConfigurationTarget.WorkspaceFolder);
  void vscode.window.showInformationMessage(`Vitte (${folder.name}): profile → ${next}`);
}

async function toggleIncremental() {
  const folder = await selectWorkspaceFolder();
  if (!folder) return;
  const cfg = vscode.workspace.getConfiguration('vitte', folder.uri);
  const cur = Boolean(cfg.get<boolean>('build.incremental'));
  await cfg.update('build.incremental', !cur, vscode.ConfigurationTarget.WorkspaceFolder);
  void vscode.window.showInformationMessage(`Vitte (${folder.name}): incremental ${!cur ? 'ON' : 'OFF'}`);
}
