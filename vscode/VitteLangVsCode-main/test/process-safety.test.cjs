const assert = require('node:assert/strict');
const { once } = require('node:events');
const fs = require('node:fs/promises');
const os = require('node:os');
const path = require('node:path');
const test = require('node:test');
const { spawnManaged, stopManagedProcesses } = require('../out/utils/managedProcesses.js');
const { quoteTclWord } = require('../out/debug/tcl.js');

test('Tcl words escape command substitutions and preserve path characters', () => {
  const input = '$HOME [open /tmp/file] "quote" \\folder\nnext\r';
  assert.equal(
    quoteTclWord(input),
    '"\\$HOME \\[open /tmp/file\\] \\"quote\\" \\\\folder\\nnext\\r"',
  );
});

test('generated import ordering and line endings are platform independent', async () => {
  const { normalizeLineEndings, renderVitteImports, sortVitteImportNames } = await import('../scripts/vitte-imports.mjs');
  const names = sortVitteImportNames(['core::zeta', 'Core::alpha', 'core::alpha']);
  const generated = renderVitteImports(names);

  assert.match(generated, /  "Core::alpha",\n  "core::alpha",\n  "core::zeta",/);
  assert.equal(normalizeLineEndings(generated.replace(/\n/g, '\r\n')), generated);
});

test('VS Code macOS smoke resolves current Code executable layouts', async () => {
  const { resolveSmokeExecutable } = await import('../scripts/vscode-smoke-executable.mjs');
  const root = await fs.mkdtemp(path.join(os.tmpdir(), 'vitte-vscode-layout-'));
  try {
    const codePath = path.join(root, 'Code');
    await fs.writeFile(codePath, '');
    assert.equal(await resolveSmokeExecutable(path.join(root, 'Electron'), 'darwin'), codePath);
    await assert.rejects(resolveSmokeExecutable(path.join(root, 'Electron'), 'linux'), { code: 'ENOENT' });
  } finally {
    await fs.rm(root, { recursive: true, force: true });
  }
});

test('managed processes preserve argv and environment and stop cleanly', async () => {
  const argument = 'a path with spaces; $HOME & é "quoted"';
  const child = spawnManaged(
    process.execPath,
    [
      '-e',
      'process.stdout.write(JSON.stringify({args:process.argv.slice(1),value:process.env.VITTE_TEST_VALUE})); setInterval(() => {}, 1000);',
      '--',
      argument,
    ],
    {
      shell: false,
      env: { ...process.env, VITTE_TEST_VALUE: 'environment value with spaces' },
      stdio: ['ignore', 'pipe', 'pipe'],
    },
  );

  let stdout = '';
  child.stdout.setEncoding('utf8');
  child.stdout.on('data', (chunk) => { stdout += chunk; });
  await new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error('managed child did not report argv')), 5000);
    child.stdout.once('data', () => {
      clearTimeout(timer);
      resolve();
    });
    child.once('error', reject);
  });

  assert.deepEqual(JSON.parse(stdout), {
    args: [argument],
    value: 'environment value with spaces',
  });

  const exited = once(child, 'exit');
  let exitTimer;
  await stopManagedProcesses(1000);
  const exitResult = await Promise.race([
    exited,
    new Promise((_, reject) => {
      exitTimer = setTimeout(() => reject(new Error('managed child was not terminated')), 5000);
    }),
  ]).finally(() => clearTimeout(exitTimer));
  const [code, signal] = exitResult;
  assert.ok(code !== null || signal !== null, 'managed child must terminate');
});
