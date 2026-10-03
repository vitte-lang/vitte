/**
 * runTest.ts — lance les tests d'intégration VS Code via @vscode/test-electron.
 */

import * as path from "node:path";
import { access } from "node:fs/promises";
import { downloadAndUnzipVSCode, runTests } from "@vscode/test-electron";

interface TestRunFailedError extends Error {
  code?: number;
  signal?: string;
}

async function existingExecutable(candidate: string): Promise<string | undefined> {
  try {
    await access(candidate);
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
  const workspacePath = extensionDevelopmentPath;

  delete process.env.ELECTRON_RUN_AS_NODE;
  process.env.VSCODE_TESTING = "1";

  const version = process.env.VSCODE_TEST_VERSION ?? "stable";
  const timeout = Number(process.env.VSCODE_TEST_DOWNLOAD_TIMEOUT_MS ?? 120_000);
  const vscodeExecutablePath = await resolveTestExecutable(version, timeout);

  await runTests({
    extensionDevelopmentPath,
    extensionTestsPath,
    vscodeExecutablePath,
    launchArgs: [workspacePath],
    extensionTestsEnv: {
      VSCODE_TESTING: "1",
    },
  });
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
