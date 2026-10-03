#!/usr/bin/env node
import { promises as fs } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { readVitteImportNames, renderVitteImports } from './vitte-imports.mjs';

const extensionRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const repoRoot = path.resolve(extensionRoot, '..', '..');
const outputFile = path.join(extensionRoot, 'server', 'src', 'generated', 'vitteImports.ts');

async function main() {
  const moduleNames = await readVitteImportNames(repoRoot);
  if (moduleNames === undefined) {
    console.warn('[imports] No Vitte source tree found; keeping the checked-in import list.');
    return;
  }

  const body = renderVitteImports(moduleNames);

  await fs.writeFile(outputFile, body, 'utf8');
  console.log(`Generated ${moduleNames.length} import paths -> ${path.relative(repoRoot, outputFile)}`);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
