#!/usr/bin/env node
import { promises as fs } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { readVitteImportNames, renderVitteImports } from './vitte-imports.mjs';

const extensionRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const repoRoot = path.resolve(extensionRoot, '..', '..');
const outputFile = path.join(extensionRoot, 'server', 'src', 'generated', 'vitteImports.ts');

async function expectedContent() {
  const moduleNames = await readVitteImportNames(repoRoot);
  return moduleNames ? renderVitteImports(moduleNames) : undefined;
}

async function main() {
  const expected = await expectedContent();
  if (expected === undefined) {
    console.warn('[imports-check] SKIP: no Vitte source tree found; import synchronization cannot be verified.');
    return;
  }

  let current = '';
  try {
    current = await fs.readFile(outputFile, 'utf8');
  } catch {
    console.error('[imports-check] Missing generated file:', path.relative(repoRoot, outputFile));
    console.error('[imports-check] Run: npm run generate:imports');
    process.exit(1);
  }

  if (current !== expected) {
    console.error('[imports-check] Generated imports file is out of date.');
    console.error('[imports-check] Expected:', path.relative(repoRoot, outputFile));
    console.error('[imports-check] Fix by running: npm run generate:imports');
    process.exit(1);
  }

  console.log('[imports-check] OK: generated imports are in sync.');
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
