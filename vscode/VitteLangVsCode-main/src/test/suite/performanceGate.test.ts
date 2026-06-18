import * as assert from "node:assert/strict";
import * as fs from "node:fs";
import * as path from "node:path";
import * as vscode from "vscode";

suite("Performance Gate", () => {
  test("benchExtensionCi stays within configured thresholds", async function () {
    this.timeout(30000);
    const workspace = vscode.workspace.workspaceFolders?.[0];
    if (!workspace) return;

    await vscode.commands.executeCommand("vitte.benchExtensionCi");
    const reportPath = path.join(workspace.uri.fsPath, ".vitte-cache", "diagnostics", "extension-bench.json");
    assert.ok(fs.existsSync(reportPath), `Bench report missing: ${reportPath}`);

    const raw = fs.readFileSync(reportPath, "utf8");
    const report = JSON.parse(raw) as {
      activationMs?: number;
      rssMB?: number;
      completionAvgMs?: number;
      completionMaxMs?: number;
    };

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

