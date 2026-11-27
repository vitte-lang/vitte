#!/usr/bin/env node
import { promises as fs } from "node:fs";
import { execFileSync } from "node:child_process";
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

function trackedVsixArtifacts() {
  let gitRoot;
  try {
    gitRoot = execFileSync("git", ["rev-parse", "--show-toplevel"], {
      cwd: root,
      encoding: "utf8",
      stdio: ["ignore", "pipe", "ignore"],
    }).trim();
  } catch {
    return [];
  }

  const packageRoot = path.relative(gitRoot, root).split(path.sep).join("/");
  const trackedFiles = execFileSync("git", ["ls-files", "--", "*.vsix"], {
    cwd: gitRoot,
    encoding: "utf8",
    stdio: ["ignore", "pipe", "ignore"],
  }).split(/\r?\n/).filter(Boolean);

  return trackedFiles.filter((file) =>
    file.endsWith(".vsix") && (!packageRoot || file.startsWith(`${packageRoot}/`)));
}

function ignoreMatcher(pattern) {
  const escaped = pattern
    .replace(/[.+^${}()|[\]\\]/g, "\\$&")
    .replace(/\*\*/g, ".*")
    .replace(/\*/g, "[^/]*");
  return new RegExp(`^${escaped}$`);
}

function isIgnored(rules, relativePath) {
  const normalized = stripRelativePrefix(relativePath).replace(/\/$/, "");
  return rules.some((rule) => {
    if (rule.startsWith("!")) return false;
    const pattern = stripRelativePrefix(rule).replace(/\/$/, "");
    if (ignoreMatcher(pattern).test(normalized)) return true;
    return pattern.endsWith("/**") && normalized.startsWith(pattern.slice(0, -3) + "/");
  });
}

function contributionPaths(pkg) {
  const contributes = pkg.contributes ?? {};
  const paths = [];

  if (typeof pkg.icon === "string") paths.push(pkg.icon);

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
  const matrix = JSON.parse(await fs.readFile(path.join(root, "server/src/capability-matrix.json"), "utf8"));
  const ignorePath = path.join(root, ".vscodeignore");
  const ignoreText = await fs.readFile(ignorePath, "utf8").catch(() => "");
  const ignoreRules = ignoreText.split(/\r?\n/).map((line) => line.trim()).filter((line) => line && !line.startsWith("#"));
  const errors = [];

  if (Array.isArray(pkg.files)) {
    errors.push('Use .vscodeignore as the sole package filter; VSCE rejects a simultaneous package.json "files" field.');
  }
  if (ignoreRules.length === 0) {
    errors.push(".vscodeignore must define the files shipped in the extension package.");
  }
  if (pkg.main !== "./out/extension.js") errors.push('Manifest main must target the compiled extension entry point "./out/extension.js".');
  if (pkg.engines?.vscode !== "^1.105.0") errors.push('Manifest engine range must match the validated VS Code baseline "^1.105.0".');
  if (pkg.version !== matrix.versions?.extension) errors.push("Extension package version must match the capability matrix.");
  const extensionSeries = /^([0-9]+\.[0-9]+)\./.exec(pkg.version ?? "")?.[1];
  const serverSeries = /^([0-9]+\.[0-9]+)\./.exec(matrix.versions?.languageServer ?? "")?.[1];
  if (!extensionSeries || extensionSeries !== serverSeries) errors.push("Packaged language-server version must be compatible with the extension major/minor series.");

  const languageRows = asArray(pkg.contributes?.languages);
  const languageExtensions = new Map(languageRows.map((language) => [language.id, asArray(language.extensions)]));
  for (const [id, extensions] of [["vit", [".vit", ".vitl"]], ["vitte", [".vitte"]]]) {
    const actual = languageExtensions.get(id) ?? [];
    for (const extension of extensions) {
      if (!actual.includes(extension)) errors.push(`Language "${id}" is missing file extension "${extension}".`);
    }
  }
  const activationEvents = asArray(pkg.activationEvents);
  for (const event of ["onLanguage:vit", "onLanguage:vitte", "workspaceContains:vitte.toml", "workspaceContains:vitte.config.json", "workspaceContains:.vitte/config.json"]) {
    if (!activationEvents.includes(event)) errors.push(`Activation event is missing: ${event}`);
  }
  if (activationEvents.some((event) => event === "*" || event === "onStartupFinished")) {
    errors.push("The extension must not start language services through broad startup activation events.");
  }
  const grammarLanguages = new Set(asArray(pkg.contributes?.grammars).map((grammar) => grammar.language));
  const snippetLanguages = new Set(asArray(pkg.contributes?.snippets).map((snippet) => snippet.language));
  for (const id of ["vit", "vitte"]) {
    if (!grammarLanguages.has(id)) errors.push(`No grammar contribution exists for language "${id}".`);
    if (!snippetLanguages.has(id)) errors.push(`No snippet contribution exists for language "${id}".`);
  }

  const properties = pkg.contributes?.configuration?.properties ?? {};
  const allowedScopes = new Set(["application", "machine", "window", "resource", "machine-overridable", "language-overridable"]);
  for (const [key, setting] of Object.entries(properties)) {
    if (!setting || typeof setting !== "object") {
      errors.push(`Configuration setting "${key}" must have a schema object.`);
      continue;
    }
    if (typeof setting.type !== "string") errors.push(`Configuration setting "${key}" must declare a JSON schema type.`);
    if (!allowedScopes.has(setting.scope)) errors.push(`Configuration setting "${key}" has an invalid or missing scope.`);
    if (typeof setting.description !== "string" || setting.description.trim().length === 0) errors.push(`Configuration setting "${key}" must be documented.`);
    if (Object.hasOwn(setting, "default") && typeof setting.type === "string") {
      const expectedType = setting.type;
      const valid = expectedType === "array" ? Array.isArray(setting.default)
        : expectedType === "object" ? setting.default !== null && typeof setting.default === "object" && !Array.isArray(setting.default)
          : typeof setting.default === expectedType;
      if (!valid) errors.push(`Configuration setting "${key}" default does not match its declared type "${expectedType}".`);
    }
  }

  const requiredArtifacts = [
    "out/extension.js",
    "out/server.js",
    "server/out/src/server.js",
    "server/out/src/capability-matrix.json",
    "assets/logo.png",
    "node_modules/vscode-languageclient/node.js",
    "node_modules/vscode-languageserver/node.js",
    "node_modules/vscode-languageserver-textdocument/lib/umd/main.js",
    "syntaxes/vitte.tmLanguage.json",
    "snippets/vitte.json",
  ];
  for (const relativePath of requiredArtifacts) {
    if (!(await exists(relativePath))) {
      errors.push(`Missing required runtime asset: ${relativePath}`);
    }
    if (isIgnored(ignoreRules, relativePath)) {
      errors.push(`Required runtime asset is excluded by .vscodeignore: ${relativePath}`);
    }
  }

  for (const relativePath of contributionPaths(pkg)) {
    if (!(await exists(relativePath))) {
      errors.push(`Missing contributed file: ${relativePath}`);
      continue;
    }
    if (isIgnored(ignoreRules, relativePath)) {
      errors.push(`Contributed file is excluded by .vscodeignore: ${relativePath}`);
    }
  }

  const artifacts = trackedVsixArtifacts();
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
