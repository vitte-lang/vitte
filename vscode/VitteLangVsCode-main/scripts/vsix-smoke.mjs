#!/usr/bin/env node
import { createRequire } from "node:module";
import { promises as fs } from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { downloadAndUnzipVSCode, resolveCliPathFromVSCodeExecutablePath } from "@vscode/test-electron";
import { resolveSmokeExecutable } from "./vscode-smoke-executable.mjs";

const require = createRequire(import.meta.url);
const { createVSIX } = require("@vscode/vsce");
const { sync: spawnSync } = require("cross-spawn");
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const packageManifest = JSON.parse(await fs.readFile(path.join(root, "package.json"), "utf8"));
delete process.env.ELECTRON_RUN_AS_NODE;
const sourceVsix = path.resolve(process.env.VITTE_VSIX_PATH ?? path.join(root, "vitte-studio.vsix"));
const tempRoot = await fs.mkdtemp(path.join(os.tmpdir(), "vitte smoke é-"));
const tempRootResolved = path.resolve(tempRoot);
const tempPrefix = `${path.resolve(os.tmpdir())}${path.sep}`;
if (!tempRootResolved.startsWith(tempPrefix) || !path.basename(tempRootResolved).startsWith("vitte smoke é-")) {
  throw new Error("Refusing to use a VSIX smoke directory outside the dedicated temporary root.");
}

const userDataDir = path.join(tempRoot, "clean user data");
const extensionsDir = path.join(tempRoot, "extensions with spaces");
const workspaceDir = path.join(tempRoot, "workspace é project");
const hostWorkspaceDir = path.join(tempRoot, "host project é");
const workspaceFile = path.join(tempRoot, "installed-extension-tests.code-workspace");
const smokeVsix = path.join(tempRoot, "vitte-studio-update.vsix");
const sourceSentinel = path.join(workspaceDir, "keep-user-data.txt");
const userSettings = path.join(userDataDir, "User", "settings.json");
const otherExtensionDir = path.join(extensionsDir, "smoke.keep-me-1.0.0");
const otherExtensionSentinel = path.join(otherExtensionDir, "keep-user-extension.txt");

function runCode(executable, args, operation) {
  const cli = resolveCliPathFromVSCodeExecutablePath(executable);
  const result = spawnSync(cli, args, {
    cwd: tempRoot,
    encoding: "utf8",
    shell: false,
    timeout: 120_000,
    windowsHide: true,
  });
  if (result.error || result.status !== 0) {
    const detail = [result.error?.message, result.stderr, result.stdout].filter(Boolean).join("\n");
    throw new Error(`VS Code ${operation} failed${result.status === null ? "" : ` (${result.status})`}: ${detail}`);
  }
}

async function main() {
  await fs.access(sourceVsix);
  await fs.mkdir(workspaceDir, { recursive: true });
  await fs.mkdir(path.join(hostWorkspaceDir, "src"), { recursive: true });
  await fs.mkdir(path.join(hostWorkspaceDir, ".vscode"), { recursive: true });
  await fs.mkdir(path.dirname(userSettings), { recursive: true });
  await fs.mkdir(otherExtensionDir, { recursive: true });
  await fs.writeFile(sourceSentinel, "keep-this-project-file\n", "utf8");
  await fs.writeFile(path.join(hostWorkspaceDir, "vitte.toml"), '[project]\nname = "editor-host-test"\n', "utf8");
  await fs.writeFile(path.join(hostWorkspaceDir, "src", "main.vit"), "proc main() -> int {\n    give 0;\n}\n", "utf8");
  await fs.writeFile(path.join(hostWorkspaceDir, ".vscode", "settings.json"), JSON.stringify({ "vitte.compiler.path": process.execPath }, null, 2) + "\n", "utf8");
  await fs.writeFile(workspaceFile, JSON.stringify({
    folders: [
      { path: path.relative(tempRoot, workspaceDir) },
      { path: path.relative(tempRoot, hostWorkspaceDir) },
    ],
  }, null, 2) + "\n", "utf8");
  await fs.writeFile(userSettings, JSON.stringify({ "smoke.preserve": true }, null, 2) + "\n", "utf8");
  await fs.writeFile(path.join(otherExtensionDir, "package.json"), JSON.stringify({
    name: "keep-me",
    publisher: "smoke",
    version: "1.0.0",
    engines: { vscode: "^1.105.0" },
    main: "./extension.js",
  }, null, 2) + "\n", "utf8");
  await fs.writeFile(path.join(otherExtensionDir, "extension.js"), "exports.activate = () => undefined;\n", "utf8");
  await fs.writeFile(otherExtensionSentinel, "keep-this-other-extension\n", "utf8");

  const version = process.env.VSCODE_TEST_VERSION ?? "stable";
  const executablePath = process.env.VSCODE_EXECUTABLE_PATH
    ? path.resolve(process.env.VSCODE_EXECUTABLE_PATH)
    : await downloadAndUnzipVSCode({
      version,
      timeout: Number(process.env.VSCODE_TEST_DOWNLOAD_TIMEOUT_MS ?? 120_000),
    });
  const executable = await resolveSmokeExecutable(executablePath);

  const profileArgs = ["--user-data-dir", userDataDir, "--extensions-dir", extensionsDir];
  runCode(executable, ["--install-extension", sourceVsix, "--force", ...profileArgs], "VSIX install");
  await assertPersistentData();
  const updateVersion = `${packageManifest.version.split(".")[0]}.${packageManifest.version.split(".")[1]}.${Number(packageManifest.version.split(".")[2]) + 1}`;
  await createVSIX({
    cwd: root,
    packagePath: smokeVsix,
    version: updateVersion,
    updatePackageJson: false,
  });
  runCode(executable, ["--install-extension", smokeVsix, "--force", ...profileArgs], "VSIX update");
  await assertPersistentData();

  // Install the release artifact over the higher test version to exercise rollback.
  runCode(executable, ["--install-extension", sourceVsix, "--force", ...profileArgs], "VSIX rollback");
  await assertPersistentData();

  const launchArgs = [
    workspaceFile,
    "--no-sandbox",
    "--disable-gpu-sandbox",
    "--disable-updates",
    "--skip-welcome",
    "--skip-release-notes",
    "--disable-workspace-trust",
    `--extensionTestsPath=${path.join(root, "test/vsix-smoke/index.js")}`,
    `--extensionDevelopmentPath=${path.join(root, "test/vsix-smoke-harness")}`,
    `--user-data-dir=${userDataDir}`,
    `--extensions-dir=${extensionsDir}`,
  ];
  const testRun = spawnSync(executable, launchArgs, {
    cwd: root,
    env: {
      ...process.env,
      VSCODE_TESTING: "1",
      VITTE_SMOKE_EXTENSIONS_DIR: extensionsDir,
      VITTE_SMOKE_SOURCE_ROOT: root,
      VITTE_SMOKE_VERSION: packageManifest.version,
      VITTE_HOST_TEST_FOLDER: hostWorkspaceDir,
      VITTE_HOST_TEST_COMPILER: process.execPath,
    },
    encoding: "utf8",
    maxBuffer: 16 * 1024 * 1024,
    shell: false,
    timeout: 240_000,
    windowsHide: true,
  });
  if (testRun.error || testRun.status !== 0) {
    const diagnostics = [testRun.error?.message, testRun.stdout, testRun.stderr]
      .filter(Boolean)
      .join("\n")
      .slice(-8000);
    throw new Error(`Packaged VSIX activation smoke failed${testRun.status === null ? "" : ` (${testRun.status})`}:\n${diagnostics}`);
  }

  runCode(executable, ["--uninstall-extension", "VitteStudio.vitte-studio", ...profileArgs], "VSIX uninstall");
  await assertPersistentData();
  console.log(`[vsix-smoke] Installed, updated, rolled back, activated, and uninstalled Vitte ${packageManifest.version} with an isolated ${version} profile.`);
}

async function assertPersistentData() {
  const value = await fs.readFile(sourceSentinel, "utf8");
  if (value !== "keep-this-project-file\n") {
    throw new Error("Extension install/update/rollback/uninstall changed project data.");
  }
  const settings = JSON.parse(await fs.readFile(userSettings, "utf8"));
  if (settings["smoke.preserve"] !== true) {
    throw new Error("Extension install/update/rollback/uninstall changed the VS Code user profile.");
  }
  const otherExtension = await fs.readFile(otherExtensionSentinel, "utf8");
  if (otherExtension !== "keep-this-other-extension\n") {
    throw new Error("Extension install/update/rollback/uninstall changed another installed extension.");
  }
}

try {
  await main();
} finally {
  await fs.rm(tempRoot, { recursive: true, force: true });
}
