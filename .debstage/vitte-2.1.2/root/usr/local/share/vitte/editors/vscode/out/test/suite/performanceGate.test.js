"use strict";
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
const assert = __importStar(require("node:assert/strict"));
const fs = __importStar(require("node:fs"));
const path = __importStar(require("node:path"));
const vscode = __importStar(require("vscode"));
suite("Performance Gate", () => {
    test("benchExtensionCi stays within configured thresholds", async function () {
        this.timeout(30000);
        const workspace = vscode.workspace.workspaceFolders?.[0];
        if (!workspace)
            return;
        await vscode.commands.executeCommand("vitte.benchExtensionCi");
        const reportPath = path.join(workspace.uri.fsPath, ".vitte-cache", "diagnostics", "extension-bench.json");
        assert.ok(fs.existsSync(reportPath), `Bench report missing: ${reportPath}`);
        const raw = fs.readFileSync(reportPath, "utf8");
        const report = JSON.parse(raw);
        const maxActivation = Number(process.env.VITTE_PERF_MAX_ACTIVATION_MS ?? 4000);
        const maxRss = Number(process.env.VITTE_PERF_MAX_RSS_MB ?? 1400);
        const maxCompletionAvg = Number(process.env.VITTE_PERF_MAX_COMPLETION_AVG_MS ?? 900);
        const maxCompletionMax = Number(process.env.VITTE_PERF_MAX_COMPLETION_MAX_MS ?? 1800);
        if (typeof report.activationMs === "number") {
            assert.ok(report.activationMs <= maxActivation, `activationMs ${report.activationMs} > ${maxActivation}`);
        }
        if (typeof report.rssMB === "number") {
            assert.ok(report.rssMB <= maxRss, `rssMB ${report.rssMB} > ${maxRss}`);
        }
        if (typeof report.completionAvgMs === "number") {
            assert.ok(report.completionAvgMs <= maxCompletionAvg, `completionAvgMs ${report.completionAvgMs} > ${maxCompletionAvg}`);
        }
        if (typeof report.completionMaxMs === "number") {
            assert.ok(report.completionMaxMs <= maxCompletionMax, `completionMaxMs ${report.completionMaxMs} > ${maxCompletionMax}`);
        }
    });
});
//# sourceMappingURL=performanceGate.test.js.map