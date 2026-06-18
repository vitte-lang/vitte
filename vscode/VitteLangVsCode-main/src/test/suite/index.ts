/**
 * index.ts — configuration Mocha pour les tests d’intégration VS Code.
 */

import * as path from "node:path";
import Mocha from "mocha";
import { promises as fs } from "node:fs";

async function collectTestFiles(root: string): Promise<string[]> {
  const entries = await fs.readdir(root, { withFileTypes: true });
  const files: string[] = [];
  for (const entry of entries) {
    const fullPath = path.resolve(root, entry.name);
    if (entry.isDirectory()) {
      const nested = await collectTestFiles(fullPath);
      files.push(...nested);
    } else if (entry.isFile() && entry.name.endsWith(".test.js")) {
      files.push(fullPath);
    }
  }
  return files;
}

export async function run(): Promise<void> {
  const mocha = new Mocha({
    ui: "tdd",
    color: true,
    timeout: 60_000,
  });

  const testsRoot = path.resolve(__dirname);
  const files = await collectTestFiles(testsRoot);

  if (files.length === 0) {
    console.warn("[tests] Aucun fichier *.test.js trouvé dans", testsRoot);
  }

  for (const file of files) {
    mocha.addFile(file);
  }

  await new Promise<void>((resolve, reject) => {
    mocha.run((failures: number) => {
      if (failures > 0) {
        reject(new Error(`${failures} test(s) ont échoué.`));
      } else {
        resolve();
      }
    });
  });
}
