#!/usr/bin/env node
import { promises as fs } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const packagePath = path.join(root, "package.json");

function asArray(value) {
  return Array.isArray(value) ? value : [];
}

function stripRelativePrefix(value) {
  return value.replace(/^\.\//, "");
}

async function exists(relativePath) {
  try {
    await fs.access(path.join(root, relativePath));
    return true;
  } catch {
    return false;
  }
}

async function collectFiles(dir, out = []) {
  const entries = await fs.readdir(dir, { withFileTypes: true });
  for (const entry of entries) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      if (entry.name === "node_modules" || entry.name === "out" || entry.name === ".vscode-test") continue;
      await collectFiles(full, out);
      continue;
    }
    out.push(path.relative(root, full));
  }
  return out;
}

function filesAllow(files, relativePath) {
  const normalized = stripRelativePrefix(relativePath);
  return files.some((entry) => {
    const pattern = stripRelativePrefix(entry);
    if (pattern.endsWith("/**")) return normalized.startsWith(pattern.slice(0, -2));
    return normalized === pattern;
  });
}

function contributionPaths(pkg) {
  const contributes = pkg.contributes ?? {};
  const paths = [];

  for (const language of asArray(contributes.languages)) {
    if (typeof language.configuration === "string") paths.push(language.configuration);
  }
  for (const grammar of asArray(contributes.grammars)) {
    if (typeof grammar.path === "string") paths.push(grammar.path);
  }
  for (const snippet of asArray(contributes.snippets)) {
    if (typeof snippet.path === "string") paths.push(snippet.path);
  }
  for (const validation of asArray(contributes.jsonValidation)) {
    if (typeof validation.url === "string" && validation.url.startsWith(".")) paths.push(validation.url);
  }
  for (const walkthrough of asArray(contributes.walkthroughs)) {
    for (const step of asArray(walkthrough.steps)) {
      const image = step?.media?.image;
      if (typeof image === "string") paths.push(image);
    }
  }

  return [...new Set(paths.map(stripRelativePrefix))].sort();
}

async function main() {
  const pkg = JSON.parse(await fs.readFile(packagePath, "utf8"));
  const packageFiles = asArray(pkg.files);
  const errors = [];

  if (!filesAllow(packageFiles, "snippets/vitte.json")) {
    errors.push('package.json "files" must include snippets/** because snippets are contributed.');
  }

  for (const relativePath of contributionPaths(pkg)) {
    if (!(await exists(relativePath))) {
      errors.push(`Missing contributed file: ${relativePath}`);
      continue;
    }
    if (!filesAllow(packageFiles, relativePath)) {
      errors.push(`Contributed file is not included in package files: ${relativePath}`);
    }
  }

  const artifacts = (await collectFiles(root)).filter((file) => file.endsWith(".vsix"));
  for (const artifact of artifacts) {
    errors.push(`Generated VSIX artifact should not be committed: ${artifact}`);
  }

  if (await exists("vitte.tmLanguage.json")) {
    errors.push("Root vitte.tmLanguage.json duplicates syntaxes/vitte.tmLanguage.json; keep the packaged grammar as the single source.");
  }

  if (errors.length > 0) {
    console.error("[manifest-check] Failed:");
    for (const error of errors) console.error(`- ${error}`);
    process.exit(1);
  }

  console.log("[manifest-check] OK: package manifest contributions are publishable.");
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
