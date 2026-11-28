/**
 * runTest.ts — lance les tests d'intégration VS Code via @vscode/test-electron.
 */

import * as path from "node:path";
import * as os from "node:os";
import * as fs from "node:fs/promises";
import { downloadAndUnzipVSCode, runTests } from "@vscode/test-electron";

interface TestRunFailedError extends Error {
  code?: number;
  signal?: string;
}

async function existingExecutable(candidate: string): Promise<string | undefined> {
  try {
    await fs.access(candidate);
    return candidate;
  } catch {
    return undefined;
  }
}

/**
 * Recent macOS VS Code archives use `Code` as the bundle executable while
 * older @vscode/test-electron releases still derive `Visual Studio Code`.
 * Resolve the downloaded executable once and pass it explicitly to runTests.
 */
async function resolveTestExecutable(version: string, timeout: number): Promise<string> {
  const configured = process.env.VSCODE_EXECUTABLE_PATH;
  if (configured) {
    const resolved = path.resolve(configured);
    const executable = await existingExecutable(resolved);
    if (!executable) throw new Error(`VSCODE_EXECUTABLE_PATH does not exist: ${resolved}`);
    return executable;
  }

  const downloaded = await downloadAndUnzipVSCode({ version, timeout });
  const direct = await existingExecutable(downloaded);
  if (direct) return direct;

  if (process.platform === "darwin") {
    const macCode = path.join(path.dirname(downloaded), "Code");
    const fallback = await existingExecutable(macCode);
    if (fallback) return fallback;
  }

  throw new Error(`Downloaded VS Code executable not found: ${downloaded}`);
}

async function main(): Promise<void> {
  const extensionDevelopmentPath = path.resolve(__dirname, "../../");
  const extensionTestsPath = path.resolve(__dirname, "./suite/index");
  const testDataRoot = path.join(extensionDevelopmentPath, ".vscode-test");
  await fs.mkdir(testDataRoot, { recursive: true });
  const hostProjectPath = await fs.mkdtemp(path.join(os.tmpdir(), "vitte editor é project "));
  const hostSourcePath = path.join(hostProjectPath, "src", "main.vit");
  const hostSettingsPath = path.join(hostProjectPath, ".vscode", "settings.json");
  const workspaceFile = path.join(testDataRoot, `host-workspace-${process.pid}.code-workspace`);
  const testProfileRoot = path.join(os.tmpdir(), `vt-${process.pid}`);
  const testUserData = path.join(testProfileRoot, "u");
  const testExtensions = path.join(testProfileRoot, "e");
  await fs.mkdir(path.dirname(hostSourcePath), { recursive: true });
  await fs.mkdir(path.dirname(hostSettingsPath), { recursive: true });
  await fs.writeFile(path.join(hostProjectPath, "vitte.toml"), '[project]\nname = "editor-host-test"\n', "utf8");
  await fs.writeFile(hostSourcePath, "proc main() -> int {\n    give 0;\n}\n", "utf8");
  await fs.writeFile(hostSettingsPath, JSON.stringify({ "vitte.compiler.path": process.execPath }, null, 2) + "\n", "utf8");
  await fs.writeFile(workspaceFile, JSON.stringify({
    folders: [
      { path: hostProjectPath },
      { path: extensionDevelopmentPath },
    ],
  }, null, 2) + "\n", "utf8");

  delete process.env.ELECTRON_RUN_AS_NODE;
  process.env.VSCODE_TESTING = "1";

  const version = process.env.VSCODE_TEST_VERSION ?? "stable";
  const timeout = Number(process.env.VSCODE_TEST_DOWNLOAD_TIMEOUT_MS ?? 120_000);
  const vscodeExecutablePath = await resolveTestExecutable(version, timeout);

  try {
    await runTests({
      extensionDevelopmentPath,
      extensionTestsPath,
      vscodeExecutablePath,
      launchArgs: [workspaceFile, `--user-data-dir=${testUserData}`, `--extensions-dir=${testExtensions}`, "--new-window"],
      extensionTestsEnv: {
        VSCODE_TESTING: "1",
        VITTE_HOST_TEST_FOLDER: hostProjectPath,
        VITTE_HOST_TEST_COMPILER: process.execPath,
        ...(process.env.VITTE_TEST_GREP ? { VITTE_TEST_GREP: process.env.VITTE_TEST_GREP } : {}),
      },
    });
  } finally {
    await fs.rm(hostProjectPath, { recursive: true, force: true });
    await fs.rm(workspaceFile, { force: true });
    await fs.rm(testUserData, { recursive: true, force: true });
    await fs.rm(testExtensions, { recursive: true, force: true });
    await fs.rm(testProfileRoot, { recursive: true, force: true });
  }
}

main().catch((err: TestRunFailedError) => {
  const message = String(err?.message ?? err);
  const signal = typeof err?.signal === "string" ? err.signal : "";
  const isMacSandboxIssue =
    process.platform === "darwin" &&
    (err?.code === 9 ||
      signal === "SIGABRT" ||
      message.includes("signal SIGABRT") ||
      message.includes("bad option: --no-sandbox") ||
      message.includes("SecCodeCheckValidity"));

  if (isMacSandboxIssue) {
    console.warn("[tests] Impossible de lancer VS Code dans cet environnement (restriction/runtime macOS).");
    console.warn("[tests] Exécutez `xattr -dr com.apple.quarantine <Visual Studio Code.app>` si vous souhaitez lancer les tests localement.");
    return;
  }

  console.error("[tests] Échec des tests VS Code :", err);
  process.exit(1);
});
