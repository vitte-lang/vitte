#!/usr/bin/env node
import { createRequire } from "node:module";
import { promises as fs } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const require = createRequire(import.meta.url);
const yauzl = require("yauzl");
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const vsixPath = path.resolve(process.argv[2] ?? path.join(root, "vitte-studio.vsix"));

const requiredEntries = [
  "extension/README.md",
  "extension/CHANGELOG.md",
  "extension/LICENSE.txt",
  "extension/assets/logo.png",
  "extension/language-configuration.json",
  "extension/syntaxes/vitte.tmLanguage.json",
  "extension/snippets/vitte.json",
  "extension/schemas/vitte-config.schema.json",
  "extension/out/extension.js",
  "extension/out/server.js",
  "extension/out/debug/tcl.js",
  "extension/server/out/src/server.js",
  "extension/server/out/src/capability-matrix.json",
];

function readRequiredFiles(archivePath, names) {
  return new Promise((resolve, reject) => {
    yauzl.open(archivePath, { lazyEntries: true }, (openError, zip) => {
      if (openError || !zip) {
        reject(openError ?? new Error(`Unable to open VSIX: ${archivePath}`));
        return;
      }

      const wanted = new Set(names);
      const found = new Map();
      zip.once("error", reject);
      zip.on("entry", (entry) => {
        if (!wanted.has(entry.fileName)) {
          zip.readEntry();
          return;
        }
        zip.openReadStream(entry, (streamError, stream) => {
          if (streamError || !stream) {
            reject(streamError ?? new Error(`Unable to read ${entry.fileName}`));
            zip.close();
            return;
          }
          const chunks = [];
          stream.on("data", (chunk) => chunks.push(chunk));
          stream.once("error", reject);
          stream.once("end", () => {
            found.set(entry.fileName, Buffer.concat(chunks));
            zip.readEntry();
          });
        });
      });
      zip.once("end", () => resolve(found));
      zip.readEntry();
    });
  });
}

async function main() {
  await fs.access(vsixPath);
  const sourceManifest = JSON.parse(await fs.readFile(path.join(root, "package.json"), "utf8"));
  const productionDependencies = Object.keys(sourceManifest.dependencies ?? {});
  const files = await readRequiredFiles(vsixPath, [
    ...requiredEntries,
    "extension/package.json",
    "extension.vsixmanifest",
    ...productionDependencies.map((dependency) => `extension/node_modules/${dependency}/package.json`),
  ]);
  const missing = [...requiredEntries, "extension/package.json", "extension.vsixmanifest"]
    .filter((name) => !files.has(name));
  if (missing.length > 0) throw new Error(`VSIX is missing packaged files: ${missing.join(", ")}`);

  const manifest = JSON.parse(files.get("extension/package.json").toString("utf8"));
  if (manifest.name !== sourceManifest.name || manifest.publisher !== sourceManifest.publisher) {
    throw new Error("Packaged extension identity does not match package.json.");
  }
  if (manifest.version !== sourceManifest.version) {
    throw new Error(`Packaged version ${manifest.version} does not match source version ${sourceManifest.version}.`);
  }
  if (!manifest.license || !files.get("extension/LICENSE.txt").length) {
    throw new Error("VSIX must include the declared extension license.");
  }
  if (productionDependencies.length === 0 || Object.keys(manifest.dependencies ?? {}).length === 0) {
    throw new Error("VSIX manifest does not declare its production dependencies.");
  }
  for (const dependency of productionDependencies) {
    const entry = `extension/node_modules/${dependency}/package.json`;
    if (!files.has(entry)) {
      throw new Error(`Production dependency is not packaged: ${dependency}`);
    }
  }

  const vsixManifest = files.get("extension.vsixmanifest").toString("utf8");
  if (!vsixManifest.includes(`Version="${manifest.version}"`)) {
    throw new Error("VSIX metadata version does not match the packaged extension manifest.");
  }

  console.log(`[vsix-verify] ${manifest.publisher}.${manifest.name}@${manifest.version}`);
  console.log(`[vsix-verify] ${Object.keys(manifest.dependencies).length} production dependencies, license, changelog, docs, icon, server and editor resources present`);
}

main().catch((error) => {
  console.error(`[vsix-verify] ${error instanceof Error ? error.message : String(error)}`);
  process.exitCode = 1;
});
