/**
 * runTest.ts — lance les tests d'intégration VS Code via @vscode/test-electron.
 */

import * as path from "node:path";
import { runTests } from "@vscode/test-electron";

interface TestRunFailedError extends Error {
  code?: number;
}

async function main(): Promise<void> {
  const extensionDevelopmentPath = path.resolve(__dirname, "../../");
  const extensionTestsPath = path.resolve(__dirname, "./suite/index");

  delete process.env.ELECTRON_RUN_AS_NODE;
  process.env.VSCODE_TESTING = "1";

  await runTests({
    extensionDevelopmentPath,
    extensionTestsPath,
    version: "stable",
    extensionTestsEnv: {
      VSCODE_TESTING: "1",
    },
  });
}

main().catch((err: TestRunFailedError) => {
  const message = String(err?.message ?? err);
  const isMacSandboxIssue =
    process.platform === "darwin" &&
    (err?.code === 9 ||
      message.includes("bad option: --no-sandbox") ||
      message.includes("SecCodeCheckValidity"));

  if (isMacSandboxIssue) {
    console.warn("[tests] Impossible de lancer VS Code dans cet environnement (restriction macOS).");
    console.warn("[tests] Exécutez `xattr -dr com.apple.quarantine <Visual Studio Code.app>` si vous souhaitez lancer les tests localement.");
    return;
  }

  console.error("[tests] Échec des tests VS Code :", err);
  process.exit(1);
});
