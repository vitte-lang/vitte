import { spawnSync } from 'node:child_process';
import { promises as fs } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const testRoot = path.join(root, 'test');

async function collectTests(directory) {
  const entries = await fs.readdir(directory, { withFileTypes: true });
  const files = [];

  for (const entry of entries) {
    const entryPath = path.join(directory, entry.name);
    if (entry.isDirectory()) {
      files.push(...await collectTests(entryPath));
    } else if (entry.isFile() && entry.name.endsWith('.test.cjs')) {
      files.push(entryPath);
    }
  }

  return files;
}

const testFiles = (await collectTests(testRoot)).sort();
if (testFiles.length === 0) throw new Error(`No unit test files found under ${testRoot}`);

const result = spawnSync(process.execPath, ['--test', ...testFiles], {
  cwd: root,
  shell: false,
  stdio: 'inherit',
  windowsHide: true,
});

if (result.error) throw result.error;
process.exitCode = result.status ?? 1;
