import * as fs from 'node:fs';
import * as path from 'node:path';
import * as os from 'node:os';

export const BUILD_PROFILES = ['default', 'debug', 'release', 'size', 'sanitize', 'freestanding'] as const;
/** Project configuration files loaded by the native CLI (see `vitte --help`). */
export const PROJECT_CONFIG_FILES = ['vitte.toml', path.join('.vitte', 'config')] as const;

export interface DetectedVitteProject {
  root: string;
  configFile: string;
}

export function compilerProfile(profile = 'default'): string {
  if (profile === 'dev' || profile === 'test') return 'debug';
  if (profile === 'bench') return 'release';
  return (BUILD_PROFILES as readonly string[]).includes(profile) ? profile : 'default';
}

/** Locate the compiler checkout even when VS Code opens a nested folder. */
export function findCompilerRepository(start: string): string | undefined {
  let current = path.resolve(start);
  while (true) {
    if (['Makefile', 'grammar/vitte.ebnf', 'src/bootstrap/cli/command_spec.def'].every(p => fs.existsSync(path.join(current, p)))) return current;
    const parent = path.dirname(current);
    if (parent === current) return undefined;
    current = parent;
  }
}

export function hasMakeTarget(repository: string, target: string): boolean {
  try {
    const makefile = fs.readFileSync(path.join(repository, 'Makefile'), 'utf8');
    const escaped = target.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
    return new RegExp(`^\\s*${escaped}\\s*:`, 'm').test(makefile);
  } catch {
    return false;
  }
}

export function workspaceBinaryRoots(roots: readonly string[]): string[] {
  return [...new Set(roots.flatMap(root => {
    const repository = findCompilerRepository(root);
    return [
      ...(repository ? [path.join(repository, 'build', 'bin')] : []),
      path.join(root, 'build', 'bin'), path.join(root, '.vitte', 'bin'),
      path.join(root, 'vitte', 'bin'), path.join(root, '.bin'), path.join(root, 'tools', 'vitte', 'bin'),
    ];
  }))];
}

export function expandToolPath(value: string, workspaceRoot: string, toolchainRoot?: string): string {
  const substituted = value.replace(/\$\{workspaceFolder}/g, workspaceRoot)
    .replace(/\$\{env:([^}]+)}/g, (_match: string, key: string) => process.env[key] ?? '');
  const expanded = substituted.replace(/^~(?=$|[\\/])/, os.homedir());
  return path.isAbsolute(expanded) ? expanded : path.resolve(toolchainRoot ?? workspaceRoot, expanded);
}

/** Find the nearest project config for a file/folder, walking toward the filesystem root. */
export function detectVitteProject(startPath: string | undefined): DetectedVitteProject | undefined {
  if (!startPath) return undefined;
  let current = path.resolve(startPath);
  try {
    if (!fs.statSync(current).isDirectory()) current = path.dirname(current);
  } catch {
    current = path.extname(current) ? path.dirname(current) : current;
  }
  while (true) {
    for (const relative of PROJECT_CONFIG_FILES) {
      const configFile = path.join(current, relative);
      try {
        if (fs.statSync(configFile).isFile()) return { root: current, configFile };
      } catch { /* inspect the next documented config name */ }
    }
    const parent = path.dirname(current);
    if (parent === current) return undefined;
    current = parent;
  }
}

/** Resolve an explicit per-folder root, then the nearest CLI-recognized project, then the workspace folder. */
export function resolveProjectRoot(configuredRoot: string | undefined, workspaceRoot: string | undefined, resourcePath?: string): string | undefined {
  const configured = configuredRoot?.trim();
  if (configured) return expandToolPath(configured, workspaceRoot ?? process.cwd());
  return detectVitteProject(resourcePath ?? workspaceRoot)?.root ?? workspaceRoot;
}

export interface ProjectCommand {
  bin: string;
  args: string[];
  cwd: string;
  outputDir?: string;
  env?: NodeJS.ProcessEnv;
}

export function projectCommand(options: {
  root: string; compiler: string; action: 'check' | 'build' | 'run' | 'test' | 'clean';
  input?: string | undefined; currentFileOnly?: boolean | undefined;
  profile?: string | undefined; incremental?: boolean | undefined;
  target?: string | undefined; args?: readonly string[] | undefined;
  programArgs?: readonly string[] | undefined; modulePaths?: readonly string[] | undefined;
  env?: Readonly<Record<string, string>> | undefined;
  compilerRepository?: boolean | undefined;
  manifestPath?: string | undefined;
}): ProjectCommand {
  const { root, compiler, action, input } = options;
  const env: NodeJS.ProcessEnv = { ...process.env, ...(options.env ?? {}) };
  if (options.modulePaths?.length) {
    const resolved = options.modulePaths.filter(modulePath => modulePath.trim()).map(modulePath => expandToolPath(modulePath.trim(), root));
    const existing = env.VITTE_MODULE_PATH?.split(path.delimiter).filter(Boolean) ?? [];
    env.VITTE_MODULE_PATH = [...new Set([...resolved, ...existing])].join(path.delimiter);
  }
  const repository = findCompilerRepository(root);
  const useRepositoryMake = Boolean(repository && ((action === 'test' && !options.currentFileOnly) || action === 'clean' || options.compilerRepository === true));
  if (repository && useRepositoryMake) {
    const detectedProject = detectVitteProject(root);
    if (detectedProject) {
      throw new Error(`The selected Vitte project config at ${detectedProject.configFile} would be ignored by compiler-repository Make targets. Set currentFileOnly for an explicit CLI source action or remove the project-root conflict before running Make.`);
    }
    if (options.target?.trim() || options.manifestPath?.trim() || options.args?.length) {
      throw new Error(`The ${action} task is using the compiler repository Make target, which cannot accept the configured CLI target, manifest or compiler arguments. Set currentFileOnly on the task to invoke the selected CLI file explicitly.`);
    }
    const makeTarget = action === 'build' ? 'all' : action;
    if (!hasMakeTarget(repository, makeTarget)) throw new Error(`Make target “${makeTarget}” is unavailable in ${repository}.`);
    const profile = compilerProfile(options.profile);
    const mode = profile === 'release' || profile === 'size' ? 'release' : profile === 'sanitize' ? 'asan' : 'debug';
    return { bin: 'make', args: [makeTarget, `MODE=${mode}`], cwd: repository, env };
  }
  if (action === 'clean') return { bin: '', args: [], cwd: root, outputDir: path.join(root, 'build', 'vscode') };
  if (action === 'test') {
    const scope = options.currentFileOnly ? 'current-file tests are unavailable' : 'vitte test is unavailable for standalone projects';
    throw new Error(`${scope} in compiler 0.1.1. Use Vitte: Show Capabilities for the current support matrix.`);
  }
  if (!input) throw new Error('Aucun fichier Vitte actif ou src/main.vit trouvé.');
  const args = [action === 'build' ? 'compile' : action, input];
  if (options.manifestPath?.trim()) args.push(`--manifest-path=${expandToolPath(options.manifestPath.trim(), root)}`);
  if (options.profile && options.profile !== 'default') args.push(`--profile=${compilerProfile(options.profile)}`);
  if (options.incremental) args.push('--incremental');
  if (options.target?.trim()) args.push(`--target=${options.target.trim()}`);
  if (options.args) args.push(...options.args);
  if (action === 'run' && options.programArgs?.length) args.push('--', ...options.programArgs);
  if (action === 'build') {
    const outputDir = path.join(root, 'build', 'vscode');
    args.push('-o', path.join(outputDir, path.parse(input).name + (process.platform === 'win32' ? '.exe' : '')));
    return { bin: compiler, args, cwd: root, outputDir, env };
  }
  return { bin: compiler, args, cwd: root, env };
}

export function projectSource(name: string): string {
  return `space ${name};\n\nproc main() -> int {\n    return 0;\n}\n`;
}

// The current configuration reader accepts flat keys, without TOML sections.
export const PROJECT_CONFIG = 'profile = "debug"\nbackend = "c17"\n';

export function workspaceConfiguration(root: string, launchSupport: { enabled: boolean; nativeBackend: boolean } = { enabled: false, nativeBackend: false }) {
  const repository = findCompilerRepository(root);
  const detectedProject = detectVitteProject(root);
  const compilerRepositoryWorkflow = Boolean(repository && !detectedProject);
  const task = (label: string, command: 'check' | 'build' | 'run' | 'test', group: 'build' | 'test', options: { currentFileOnly?: boolean; compilerRepository?: boolean } = {}) => ({
    label,
    type: 'vitte',
    command,
    group,
    ...(options.currentFileOnly ? { currentFileOnly: true } : {}),
    ...(options.compilerRepository ? { compilerRepository: true } : {}),
    problemMatcher: ['$vitte'],
  });
  const tasks = [
    task('vitte: check current file', 'check', 'build', { currentFileOnly: true }),
    task('vitte: build active file', 'build', 'build', { currentFileOnly: true }),
    task('vitte: run project', 'run', 'build'),
  ];
  if (compilerRepositoryWorkflow) {
    tasks.push(task('vitte: build compiler repository', 'build', 'build', { compilerRepository: true }));
    if (hasMakeTarget(repository!, 'test')) tasks.push(task('vitte: test compiler repository', 'test', 'test'));
  }
  const configurations: Record<string, unknown>[] = [
    { name: 'Vitte: Attach to DAP server', type: 'vitte', request: 'attach', host: '127.0.0.1', port: 9333 },
  ];
  if (launchSupport.enabled) {
    configurations.unshift({
      name: launchSupport.nativeBackend ? 'Vitte: Launch current file (native, experimental)' : 'Vitte: Launch current file (external DAP)',
      type: 'vitte',
      request: 'launch',
      program: '${file}',
      args: [],
      ...(launchSupport.nativeBackend ? { nativeBackend: true } : { nativeBackend: false }),
    });
    if (launchSupport.nativeBackend) {
      const projectRoot = detectedProject?.root ?? root;
      const relativeProjectRoot = path.relative(root, projectRoot).split(path.sep).filter(Boolean).join('/');
      const executablePath = [
        '${workspaceFolder}',
        relativeProjectRoot,
        'build/vscode',
        '${fileBasenameNoExtension}',
      ].filter(Boolean).join('/');
      configurations.unshift({
        name: 'Vitte: Debug built executable (DWARF)',
        type: 'vitte',
        request: 'launch',
        program: executablePath,
        preLaunchTask: 'vitte: build active file',
        nativeBackend: true,
        args: [],
      });
    }
  }
  return {
    tasks: { version: '2.0.0', tasks },
    launch: { version: '0.2.0', configurations },
  };
}
