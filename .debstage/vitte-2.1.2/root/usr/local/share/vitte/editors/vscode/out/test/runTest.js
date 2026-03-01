"use strict";
/**
 * runTest.ts — lance les tests d'intégration VS Code via @vscode/test-electron.
 */
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
const path = __importStar(require("node:path"));
const test_electron_1 = require("@vscode/test-electron");
async function main() {
    const extensionDevelopmentPath = path.resolve(__dirname, "../../");
    const extensionTestsPath = path.resolve(__dirname, "./suite/index");
    delete process.env.ELECTRON_RUN_AS_NODE;
    process.env.VSCODE_TESTING = "1";
    await (0, test_electron_1.runTests)({
        extensionDevelopmentPath,
        extensionTestsPath,
        version: "stable",
        extensionTestsEnv: {
            VSCODE_TESTING: "1",
        },
    });
}
main().catch((err) => {
    const message = String(err?.message ?? err);
    const isMacSandboxIssue = process.platform === "darwin" &&
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
//# sourceMappingURL=runTest.js.map