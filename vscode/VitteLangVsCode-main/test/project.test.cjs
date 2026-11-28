const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const { test } = require('node:test');
const { findCompilerRepository, hasMakeTarget, detectVitteProject, workspaceBinaryRoots, expandToolPath, resolveProjectRoot, projectCommand, workspaceConfiguration, projectSource, PROJECT_CONFIG, compilerProfile } = require('../out/utils/project');
const { normalizeCompilerDiagnostics } = require('../out/utils/compilerDiagnostics');
const { redactLogText } = require('../out/utils/logRedaction');
const { spawnManaged, stopManagedProcesses } = require('../out/utils/managedProcesses');
const extensionRoot = path.resolve(__dirname, '..');
const repo = findCompilerRepository(extensionRoot);
const nativeCompiler = repo && path.join(repo, 'build/bin', process.platform === 'win32' ? 'vitte.exe' : 'vitte');

function temporaryProject(t) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'vitte-project-'));
  t.after(() => fs.rmSync(root, { recursive: true, force: true }));
  return root;
}

test('compiler checkout detection works from nested folders without selecting a seed', (t) => {
  const root = temporaryProject(t);
  for (const file of ['Makefile', 'grammar/vitte.ebnf', 'src/bootstrap/cli/command_spec.def']) {
    fs.mkdirSync(path.dirname(path.join(root, file)), { recursive: true });
    fs.writeFileSync(path.join(root, file), '');
  }
  const nested = path.join(root, 'vscode', 'extension');
  assert.equal(findCompilerRepository(nested), root);
  assert.equal(workspaceBinaryRoots([nested])[0], path.join(root, 'build/bin'));
  assert.equal(findCompilerRepository(path.dirname(root)), undefined);
  assert.equal(expandToolPath('${workspaceFolder}/build/bin/vitte', root), path.join(root, 'build/bin/vitte'));
  assert.equal(expandToolPath('build/bin/vitte', root), path.join(root, 'build/bin/vitte'));
  assert.equal(resolveProjectRoot('app', root), path.join(root, 'app'));
  assert.equal(resolveProjectRoot('', root), root);
});

test('project detection follows the nearest native CLI config and keeps workspace folders independent', (t) => {
  const workspace = temporaryProject(t);
  const outerConfig = path.join(workspace, 'vitte.toml');
  fs.writeFileSync(outerConfig, PROJECT_CONFIG);
  const alpha = path.join(workspace, 'alpha');
  const beta = path.join(workspace, 'beta');
  for (const project of [alpha, beta]) fs.mkdirSync(path.join(project, 'src'), { recursive: true });
  fs.mkdirSync(path.join(alpha, '.vitte'), { recursive: true });
  fs.writeFileSync(path.join(alpha, '.vitte', 'config'), 'profile = "release"\n');
  const alphaFile = path.join(alpha, 'src', 'main.vit');
  const betaFile = path.join(beta, 'src', 'main.vit');
  fs.writeFileSync(alphaFile, 'proc main() {}\n');
  fs.writeFileSync(betaFile, 'proc main() {}\n');
  assert.deepEqual(detectVitteProject(alphaFile), { root: alpha, configFile: path.join(alpha, '.vitte', 'config') });
  assert.equal(resolveProjectRoot('', workspace, alphaFile), alpha);
  assert.equal(resolveProjectRoot('', workspace, betaFile), workspace, 'the workspace root config applies when no closer project file exists');
  assert.equal(resolveProjectRoot('beta', workspace, alphaFile), beta, 'an explicit folder root takes precedence over discovered config');
  assert.equal(resolveProjectRoot('${workspaceFolder}/beta', workspace, alphaFile), beta, 'workspace substitutions resolve in the selected folder');

  const jsonOnly = temporaryProject(t);
  fs.mkdirSync(path.join(jsonOnly, '.vitte'), { recursive: true });
  fs.writeFileSync(path.join(jsonOnly, '.vitte', 'config.json'), '{}');
  assert.equal(detectVitteProject(jsonOnly), undefined, 'editor schema JSON is not loaded by the native CLI');
});

test('extension logs redact common tokens, bearer credentials and URL passwords', () => {
  const redacted = redactLogText('token=abc123 Authorization: Bearer top-secret https://user:pass@example.test/path');
  assert.doesNotMatch(redacted, /abc123|top-secret|user:pass/);
  assert.match(redacted, /token=\[redacted\]/);
  assert.match(redacted, /Authorization: \[redacted\]/);
  assert.match(redacted, /https:\/\/\[redacted\]@example\.test/);
});

test('managed child processes stop cleanly during extension shutdown', async () => {
  const child = spawnManaged(process.execPath, ['-e', 'setInterval(() => {}, 1000)'], { stdio: 'ignore' });
  await new Promise((resolve, reject) => {
    child.once('spawn', resolve);
    child.once('error', reject);
  });
  await stopManagedProcesses(500);
  assert.ok(child.exitCode !== null || child.signalCode !== null, 'child process must be stopped');
});

test('compiler repository Make workflows are explicit; Check and Build use the active CLI file', { skip: !repo }, () => {
  for (const [action, target, compilerRepository] of [['build', 'all', true], ['test', 'test', false], ['clean', 'clean', false]]) {
    const command = projectCommand({ root: extensionRoot, compiler: nativeCompiler, action, ...(compilerRepository ? { compilerRepository: true } : {}) });
    assert.equal(command.bin, 'make');
    assert.equal(command.args[0], target);
    assert.equal(command.cwd, repo);
    assert.equal(hasMakeTarget(repo, target), true);
  }
  const file = path.join(repo, 'examples/hello.vit');
  const currentCheck = projectCommand({ root: repo, compiler: nativeCompiler, action: 'check', currentFileOnly: true, input: file });
  assert.equal(currentCheck.bin, nativeCompiler);
  assert.equal(currentCheck.cwd, repo, 'the CLI discovers project config from its working directory');
  const currentBuild = projectCommand({ root: repo, compiler: nativeCompiler, action: 'build', currentFileOnly: true, input: file });
  assert.equal(currentBuild.bin, nativeCompiler);
  assert.throws(
    () => projectCommand({ root: repo, compiler: nativeCompiler, action: 'test', currentFileOnly: true, input: '/tmp/main.vit' }),
    /current-file tests are unavailable/,
  );
});

test('application commands use canonical CLI options and isolate editor output', (t) => {
  const root = temporaryProject(t);
  const input = path.join(root, 'src/main.vit');
  const command = projectCommand({ root, compiler: 'vitte', action: 'build', input, profile: 'dev', incremental: true });
  assert.deepEqual(command.args.slice(0, 4), ['compile', input, '--profile=debug', '--incremental']);
  assert.equal(command.outputDir, path.join(root, 'build/vscode'));
  assert.equal(projectCommand({ root, compiler: 'vitte', action: 'clean' }).outputDir, path.join(root, 'build/vscode'));
  const check = projectCommand({ root, compiler: 'vitte', action: 'check', input, target: 'x86_64-unknown-linux-gnu', args: ['--quiet'], modulePaths: ['modules'] });
  assert.deepEqual(check.args, ['check', input, '--target=x86_64-unknown-linux-gnu', '--quiet']);
  assert.ok(check.env.VITTE_MODULE_PATH.split(path.delimiter).includes(path.join(root, 'modules')));
  const withManifest = projectCommand({ root, compiler: 'vitte', action: 'check', input, manifestPath: 'config/custom.toml' });
  assert.ok(withManifest.args.includes(`--manifest-path=${path.join(root, 'config/custom.toml')}`));
  const run = projectCommand({ root, compiler: 'vitte', action: 'run', input, programArgs: ['hello world', '--flag'] });
  assert.deepEqual(run.args.slice(-3), ['--', 'hello world', '--flag']);
  assert.equal(compilerProfile('bench'), 'release');
  assert.throws(() => projectCommand({ root, compiler: 'vitte', action: 'build' }), /fichier Vitte/);
  assert.throws(() => projectCommand({ root, compiler: 'vitte', action: 'test', input }), /vitte test is unavailable/);
});

test('workspace scaffolding uses scoped Vitte tasks and verified launch templates', (t) => {
  const root = temporaryProject(t);
  const config = workspaceConfiguration(root, { enabled: true, nativeBackend: true });
  assert.ok(config.tasks.tasks.every(task => task.type === 'vitte'));
  assert.deepEqual(config.tasks.tasks.map(task => task.command), ['check', 'build', 'run']);
  assert.equal(config.tasks.tasks[0].currentFileOnly, true);
  assert.equal(config.tasks.tasks[1].currentFileOnly, true);
  assert.ok(config.tasks.tasks.every(task => task.problemMatcher.includes('$vitte')));
  const launch = config.launch.configurations.find(item => item.request === 'launch');
  assert.equal(launch.program, '${file}');
  assert.equal(launch.nativeBackend, true);
  assert.equal(config.launch.configurations.some(item => item.request === 'attach'), true);
  assert.ok(!PROJECT_CONFIG.includes('[build]'));
  assert.ok(projectSource('demo').includes('proc main() -> int'));
  assert.ok(projectSource('demo').includes('return 0;'));
});

test('generated repository tasks defer to folder-scoped compiler and Make capabilities', { skip: !repo }, () => {
  const config = workspaceConfiguration(repo);
  assert.ok(config.tasks.tasks.every(task => task.type === 'vitte'));
  assert.deepEqual(config.tasks.tasks.map(task => task.command), ['check', 'build', 'run', 'build', ...(hasMakeTarget(repo, 'test') ? ['test'] : [])]);
  const cliBuild = config.tasks.tasks.find(task => task.command === 'build' && task.currentFileOnly);
  const repositoryBuild = config.tasks.tasks.find(task => task.command === 'build' && task.compilerRepository);
  assert.ok(cliBuild);
  assert.ok(repositoryBuild);
  assert.ok(config.launch.configurations.every(item => item.request === 'attach'), 'no unverified native launch config is written by default');
});

test('contributed Vitte task matcher captures compiler severity, code and source location', () => {
  const manifest = JSON.parse(fs.readFileSync(path.join(extensionRoot, 'package.json'), 'utf8'));
  const matcher = manifest.contributes.problemMatchers.find(item => item.name === '$vitte');
  assert.ok(matcher);
  const header = new RegExp(matcher.pattern[0].regexp).exec('error[E0401]: unknown symbol `missing`');
  const location = new RegExp(matcher.pattern[1].regexp).exec('  --> src/main.vit:2:29');
  assert.equal(header[1], 'error');
  assert.equal(header[2], 'E0401');
  assert.equal(header[3], 'unknown symbol `missing`');
  assert.equal(location[1], 'src/main.vit');
  assert.equal(Number(location[2]), 2);
  assert.equal(Number(location[3]), 29);
});

test('native JSON diagnostics translate UTF-8 bytes to UTF-16 and exclude imported files', () => {
  const text = '// café 😀\nreturn missing;';
  const start = text.indexOf('missing');
  const position = { source: 'main.vit', start_byte: Buffer.byteLength(text.slice(0, start)), end_byte: Buffer.byteLength(text.slice(0, start + 7)) };
  const payload = { schema: 'vitte-diagnostics', version: 1, diagnostics: [
    { code: 'E0401', severity: 'error', message: 'unknown symbol', position },
    { code: 'E0401', position: { ...position, source: 'imported.vit' } },
  ] };
  const result = normalizeCompilerDiagnostics(payload, text, '/tmp/main.vit', '/tmp');
  assert.equal(result.length, 1);
  assert.equal(result[0].start, start);
  assert.equal(text.slice(result[0].start, result[0].end), 'missing');
  assert.equal(result[0].code, 'E0401');
  assert.equal(normalizeCompilerDiagnostics({ diagnostics: [{ start: 1, end: 3 }] }, text, '/tmp/main.vit', '/tmp')[0].end, 3);
});

test('new project config and source pass the actual Vitte compiler', { skip: !nativeCompiler || !fs.existsSync(nativeCompiler) }, (t) => {
  const root = temporaryProject(t);
  const source = path.join(root, 'main.vit');
  fs.writeFileSync(source, projectSource('editor_project'));
  fs.writeFileSync(path.join(root, 'vitte.toml'), PROJECT_CONFIG);
  fs.mkdirSync(path.join(root, 'generated'));
  const checkPlan = projectCommand({ root, compiler: nativeCompiler, action: 'check', currentFileOnly: true, input: source, manifestPath: 'vitte.toml', modulePaths: ['generated'] });
  const checked = spawnSync(checkPlan.bin, [...checkPlan.args, '--diagnostic=json'], { cwd: checkPlan.cwd, env: checkPlan.env, encoding: 'utf8', timeout: 10000 });
  assert.equal(checked.status, 0, checked.stdout + checked.stderr);
  const compilePlan = projectCommand({ root, compiler: nativeCompiler, action: 'build', input: source });
  fs.mkdirSync(compilePlan.outputDir, { recursive: true });
  const compiled = spawnSync(compilePlan.bin, compilePlan.args, { cwd: compilePlan.cwd, env: compilePlan.env, encoding: 'utf8', timeout: 10000 });
  assert.equal(compiled.status, 0, compiled.stdout + compiled.stderr);
  const output = path.join(root, 'build', 'vscode', `${path.parse(source).name}${process.platform === 'win32' ? '.exe' : ''}`);
  const executed = spawnSync(output, [], { cwd: root, encoding: 'utf8', timeout: 10000 });
  assert.equal(executed.status, 0, executed.error?.message ?? executed.stderr ?? 'compiled program returned a non-zero status');
});

test('packaged snippets expand to syntax accepted by the active Vitte parser', { skip: !nativeCompiler || !fs.existsSync(nativeCompiler) }, (t) => {
  const snippets = JSON.parse(fs.readFileSync(path.join(extensionRoot, 'snippets/vitte.json'), 'utf8'));
  const temporary = temporaryProject(t);
  const statementPrefixes = new Set(['asm', 'assert', 'break', 'continue', 'defer', 'elif', 'for', 'if', 'ife', 'let', 'letm', 'loop', 'match', 'return', 'set', 'unsafe', 'while']);
  const expand = (body) => body.replace(/\$\{\d+:([^{}]*)\}|\$\{\d+\}|\$(\d+)/g, (_match, defaultValue) => defaultValue ?? '');

  for (const snippet of Object.values(snippets)) {
    let source = expand(snippet.body.join('\n'));
    if (statementPrefixes.has(snippet.prefix)) {
      if (snippet.prefix === 'break' || snippet.prefix === 'continue') {
        source = `proc __snippet() { loop {\n${source}\n} }`;
      } else if (snippet.prefix === 'elif') {
        source = `proc __snippet() { if false {}\n${source}\n}`;
      } else {
        source = `proc __snippet() {\n${source}\n}`;
      }
    } else if (['attr', 'blockcomment', 'doc'].includes(snippet.prefix)) {
      source += '\nproc __snippet() {}';
    }
    if (!['module', 'space'].includes(snippet.prefix)) source = `space __snippet;\n${source}`;
    const file = path.join(temporary, `${snippet.prefix}.vit`);
    fs.writeFileSync(file, `${source}\n`, 'utf8');
    const parsed = spawnSync(nativeCompiler, ['parse', '--no-color', file], {
      cwd: temporary,
      encoding: 'utf8',
      timeout: 10000,
    });
    assert.equal(parsed.status, 0, `${snippet.description} (${snippet.prefix}) is not accepted by the active parser:\n${parsed.stdout}${parsed.stderr}`);
  }
});

test('real compiler diagnostic envelopes identify the correct Unicode source span', { skip: !nativeCompiler || !fs.existsSync(nativeCompiler) }, (t) => {
  const root = temporaryProject(t);
  const file = path.join(root, 'main.vit');
  const text = '// français 😀\nspace editor;\nproc main() -> int { return missing_value; }\n';
  fs.writeFileSync(file, text);
  const run = spawnSync(nativeCompiler, ['check', '--diagnostic=lsp', '--no-color', file], { cwd: root, encoding: 'utf8', timeout: 10000 });
  assert.equal(run.status, 1, run.stdout + run.stderr);
  const payload = JSON.parse(run.stdout || run.stderr);
  assert.ok(Array.isArray(payload), JSON.stringify(payload));
  const diagnostics = normalizeCompilerDiagnostics(payload, text, file, root);
  const unknown = diagnostics.find(diagnostic => diagnostic.code === 'E0401');
  assert.ok(unknown, JSON.stringify(payload));
  assert.equal(text.slice(unknown.start, unknown.end), 'missing_value');
});
