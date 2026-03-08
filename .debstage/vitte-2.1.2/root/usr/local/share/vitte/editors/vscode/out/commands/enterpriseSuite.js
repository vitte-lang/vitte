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
exports.registerEnterpriseSuite = registerEnterpriseSuite;
const fs = __importStar(require("node:fs/promises"));
const path = __importStar(require("node:path"));
const node_child_process_1 = require("node:child_process");
const os = __importStar(require("node:os"));
const vscode = __importStar(require("vscode"));
const diagnostics_1 = require("../utils/diagnostics");
const knownAutoFix = {
    "style.usePath": "imports",
    "format.tabs": "style",
    "format.trailingWhitespace": "style",
    "style.fnName": "naming",
    "style.varName": "naming",
    "style.constName": "naming",
    "style.fieldName": "naming",
};
function workspaceRoot() {
    return vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
}
async function ensureDir(dir) {
    await fs.mkdir(dir, { recursive: true });
}
async function exists(filePath) {
    try {
        await fs.access(filePath);
        return true;
    }
    catch {
        return false;
    }
}
async function runCommand(cmd, args, cwd) {
    await new Promise((resolve, reject) => {
        const child = (0, node_child_process_1.spawn)(cmd, args, { cwd, shell: false });
        let stderr = "";
        child.stderr.on("data", (d) => {
            stderr += String(d);
        });
        child.on("error", reject);
        child.on("close", (code) => {
            if (code === 0)
                resolve();
            else
                reject(new Error(`${cmd} ${args.join(" ")} failed (${code}): ${stderr.trim()}`));
        });
    });
}
async function commandExists(cmd, args) {
    try {
        await runCommand(cmd, args);
        return true;
    }
    catch {
        return false;
    }
}
async function zipDirectory(sourceDir, outFile) {
    if (await commandExists("zip", ["-v"])) {
        await runCommand("zip", ["-q", "-r", outFile, "."], sourceDir);
        return;
    }
    if (process.platform === "win32") {
        const ps = "Compress-Archive";
        const cmd = `Compress-Archive -Path '${sourceDir}\\*' -DestinationPath '${outFile}' -Force`;
        await runCommand("powershell", ["-NoProfile", "-Command", `${ps}; ${cmd}`]);
        return;
    }
    throw new Error("No zip command available on this system.");
}
function getCfg() {
    return vscode.workspace.getConfiguration("vitte");
}
function cfgSnapshot() {
    const cfg = getCfg();
    const keys = [
        "enterprise.strict",
        "diagnosticsBudget.maxLatencyMs",
        "diagnosticsBudget.maxMemoryMb",
        "diagnosticsBudget.dynamicThrottleFactor",
        "semanticBudget.completionP95Ms",
        "semanticBudget.hoverP95Ms",
        "semanticBudget.renameP95Ms",
        "semanticBudget.referencesP95Ms",
        "channel",
        "policy.path",
        "features.inlayHints",
        "features.semanticTokens",
    ];
    const out = {};
    for (const key of keys)
        out[key] = cfg.get(key);
    return out;
}
function collectLastEdits() {
    const active = vscode.window.activeTextEditor?.document.uri.toString();
    return vscode.workspace.textDocuments
        .filter((d) => d.languageId === "vitte" || d.languageId === "vit")
        .map((d) => ({
        uri: d.uri.toString(),
        relativePath: vscode.workspace.asRelativePath(d.uri, false),
        languageId: d.languageId,
        version: d.version,
        lineCount: d.lineCount,
        isDirty: d.isDirty,
        isActive: d.uri.toString() === active,
    }));
}
async function applyStrictDefaults() {
    const cfg = getCfg();
    const target = vscode.workspace.workspaceFolders?.length ? vscode.ConfigurationTarget.Workspace : vscode.ConfigurationTarget.Global;
    const strict = cfg.get("enterprise.strict", false);
    if (!strict)
        return;
    await cfg.update("diagnosticsBudget.maxLatencyMs", 700, target);
    await cfg.update("diagnosticsBudget.maxMemoryMb", 900, target);
    await cfg.update("diagnosticsBudget.dynamicThrottleFactor", 2, target);
    await cfg.update("features.inlayHints", true, target);
    await cfg.update("inlayHints.parameterHints", true, target);
    await cfg.update("inlayHints.typeHints", true, target);
    await cfg.update("inlayHints.returnHints", true, target);
    await cfg.update("inlayHints.aliasHints", true, target);
}
function scoreHealth(input) {
    let s = 100;
    s -= Math.min(60, input.errors * 8);
    s -= Math.min(20, input.warnings * 2);
    s -= Math.min(20, Math.max(0, (input.p95Latency - 250) / 30));
    s -= Math.min(15, Math.max(0, (input.rssMB - 600) / 40));
    if (input.stale)
        s -= 8;
    s -= Math.min(12, input.crashes * 3);
    return Math.max(0, Math.min(100, Math.round(s)));
}
async function getServerMetrics(getClient) {
    const c = getClient();
    if (!c)
        return [];
    try {
        return await c.sendRequest("vitte/metrics");
    }
    catch {
        return [];
    }
}
async function exportWorkspaceState(deps) {
    const root = workspaceRoot();
    if (!root)
        return;
    const metrics = await getServerMetrics(deps.getClient);
    const summary = (0, diagnostics_1.summarizeWorkspaceDiagnostics)();
    const byDir = (0, diagnostics_1.summarizeDiagnosticsByDirectory)();
    const docs = await vscode.workspace.findFiles("book/**/*.md", "**/{node_modules,.git,out,dist,build}/**", 3000);
    const graph = await deps.getClient()?.sendRequest?.("vitte/symbolGraph").catch(() => null);
    const payload = {
        ts: new Date().toISOString(),
        diagnostics: summary,
        diagnosticsByDirectory: byDir,
        metrics,
        docsStats: { count: docs.length },
        symbolGraph: graph ?? null,
        crashCount: deps.getCrashCount(),
    };
    const dir = path.join(root, ".vitte-cache", "diagnostics");
    await ensureDir(dir);
    const file = path.join(dir, "workspace-state.json");
    await fs.writeFile(file, `${JSON.stringify(payload, null, 2)}\n`, "utf8");
    void vscode.window.showInformationMessage(`Workspace state exported: ${file}`);
}
async function exportSarif(deps) {
    const root = workspaceRoot();
    if (!root)
        return;
    const runs = [];
    const results = [];
    for (const [uri, diags] of vscode.languages.getDiagnostics()) {
        for (const d of diags) {
            results.push({
                ruleId: String(d.code ?? d.source ?? "VITTE-DIAG"),
                level: d.severity === vscode.DiagnosticSeverity.Error ? "error" : d.severity === vscode.DiagnosticSeverity.Warning ? "warning" : "note",
                message: { text: d.message },
                locations: [{
                        physicalLocation: {
                            artifactLocation: { uri: vscode.workspace.asRelativePath(uri, false) },
                            region: {
                                startLine: d.range.start.line + 1,
                                startColumn: d.range.start.character + 1,
                            },
                        },
                    }],
            });
        }
    }
    const metrics = await getServerMetrics(deps.getClient);
    runs.push({
        tool: { driver: { name: "vitte-enterprise", version: "1.0.0" } },
        results,
        properties: { metrics },
    });
    const sarif = { $schema: "https://json.schemastore.org/sarif-2.1.0.json", version: "2.1.0", runs };
    const dir = path.join(root, ".vitte-cache", "diagnostics");
    await ensureDir(dir);
    const file = path.join(dir, "vitte-report.sarif");
    await fs.writeFile(file, `${JSON.stringify(sarif, null, 2)}\n`, "utf8");
    void vscode.window.showInformationMessage(`SARIF exported: ${file}`);
}
async function runDoctorPlugin(plugin, output) {
    const root = workspaceRoot();
    if (!root)
        return;
    const map = {
        security: "npm audit --json",
        licenses: "npm ls --json",
        "dead-code": "vitte check --json",
        freshness: "npm outdated --json",
        "docs-coverage": "vitte check --json",
        "api-breakage": "vitte check --json",
    };
    const fallback = "npm audit --json";
    const cmd = map[plugin] ?? fallback;
    output.appendLine(`[doctor-plugin] ${plugin}: ${cmd}`);
    await new Promise((resolve) => {
        const child = (0, node_child_process_1.spawn)(cmd, [], { cwd: root, shell: true });
        child.stdout.on("data", (d) => output.appendLine(`[doctor-plugin:${plugin}] ${String(d).trimEnd()}`));
        child.stderr.on("data", (d) => output.appendLine(`[doctor-plugin:${plugin}:err] ${String(d).trimEnd()}`));
        child.on("close", () => resolve());
        child.on("error", () => resolve());
    });
}
async function perfBaselineUpdate() {
    const root = workspaceRoot();
    if (!root)
        return;
    const src = path.join(root, ".vitte-cache", "diagnostics", "perf-session.json");
    const dst = path.join(root, ".vitte-cache", "diagnostics", "perf-baseline.json");
    const raw = await fs.readFile(src, "utf8");
    await fs.writeFile(dst, raw, "utf8");
    void vscode.window.showInformationMessage(`Perf baseline updated: ${dst}`);
}
async function perfBaselineCheck() {
    const root = workspaceRoot();
    if (!root)
        return;
    const curPath = path.join(root, ".vitte-cache", "diagnostics", "perf-session.json");
    const basePath = path.join(root, ".vitte-cache", "diagnostics", "perf-baseline.json");
    const cur = JSON.parse(await fs.readFile(curPath, "utf8"));
    const base = JSON.parse(await fs.readFile(basePath, "utf8"));
    const drift = [];
    const check = (k, factor) => {
        if (typeof cur[k] !== "number" || typeof base[k] !== "number")
            return;
        if (cur[k] > base[k] * factor)
            drift.push(`${k}: ${cur[k]} > ${base[k]}*${factor}`);
    };
    check("activationMs", 1.25);
    check("rssMB", 1.20);
    check("completionAvgMs", 1.30);
    if (drift.length > 0) {
        void vscode.window.showErrorMessage(`Perf baseline check failed: ${drift.join(" | ")}`);
        return;
    }
    void vscode.window.showInformationMessage("Perf baseline check: OK");
}
async function publishGate(deps) {
    const cfg = getCfg();
    const strict = cfg.get("enterprise.strict", false);
    if (!strict) {
        void vscode.window.showInformationMessage("Enterprise strict is disabled.");
        return;
    }
    const summary = (0, diagnostics_1.summarizeWorkspaceDiagnostics)();
    const metrics = await getServerMetrics(deps.getClient);
    const p95 = metrics.find((m) => m.name === "completion")?.p95Ms ?? 0;
    const rss = Math.round(process.memoryUsage().rss / (1024 * 1024));
    const score = scoreHealth({
        errors: summary.errors,
        warnings: summary.warnings,
        p95Latency: p95,
        rssMB: rss,
        stale: false,
        crashes: deps.getCrashCount(),
    });
    if (summary.errors > 0 || score < 70) {
        throw new Error(`Publish blocked (strict): errors=${summary.errors}, health=${score}`);
    }
    void vscode.window.showInformationMessage(`Publish gate passed (health ${score}).`);
}
async function crashForensicsBundle(deps) {
    const root = workspaceRoot();
    if (!root)
        return;
    const ts = Date.now();
    const iso = new Date(ts).toISOString();
    const bundleRoot = path.join(root, ".vitte-cache", "forensics");
    const staging = await fs.mkdtemp(path.join(os.tmpdir(), "vitte-forensics-"));
    const payloadDir = path.join(staging, "payload");
    await ensureDir(payloadDir);
    const metrics = await getServerMetrics(deps.getClient);
    const summary = (0, diagnostics_1.summarizeWorkspaceDiagnostics)();
    const byDir = (0, diagnostics_1.summarizeDiagnosticsByDirectory)();
    const doctorSummary = {
        errors: summary.errors,
        warnings: summary.warnings,
        hints: summary.hints,
        byPackageCount: byDir.length,
        generatedAt: iso,
    };
    const payload = {
        schemaVersion: 1,
        ts: iso,
        crashCount: deps.getCrashCount(),
        diagnostics: summary,
        diagnosticsByDir: byDir,
        doctorSummary,
        metrics,
        config: cfgSnapshot(),
        lastEdits: collectLastEdits(),
        workspaceFolders: (vscode.workspace.workspaceFolders ?? []).map((f) => f.uri.fsPath),
    };
    await fs.writeFile(path.join(payloadDir, "summary.json"), `${JSON.stringify(payload, null, 2)}\n`, "utf8");
    const diagnosticsDir = path.join(root, ".vitte-cache", "diagnostics");
    if (await exists(diagnosticsDir)) {
        const entries = await fs.readdir(diagnosticsDir);
        for (const name of entries) {
            if (!name.endsWith(".json") && !name.endsWith(".sarif") && !name.endsWith(".log"))
                continue;
            const src = path.join(diagnosticsDir, name);
            const dst = path.join(payloadDir, "diagnostics", name);
            await ensureDir(path.dirname(dst));
            try {
                const raw = await fs.readFile(src);
                await fs.writeFile(dst, raw);
            }
            catch {
                // ignore single-file copy errors
            }
        }
    }
    const relSettings = path.join(root, ".vscode", "settings.json");
    if (await exists(relSettings)) {
        const dst = path.join(payloadDir, "workspace", "settings.json");
        await ensureDir(path.dirname(dst));
        await fs.writeFile(dst, await fs.readFile(relSettings));
    }
    await ensureDir(bundleRoot);
    const zipPath = path.join(bundleRoot, `forensics-${ts}.zip`);
    try {
        await zipDirectory(payloadDir, zipPath);
    }
    finally {
        await fs.rm(staging, { recursive: true, force: true });
    }
    void vscode.window.showInformationMessage(`Crash forensics bundle: ${zipPath}`);
}
function registerAutoRemediation(context) {
    let lastPromptAt = 0;
    context.subscriptions.push(vscode.languages.onDidChangeDiagnostics(async () => {
        const strict = getCfg().get("enterprise.strict", false);
        if (!strict)
            return;
        const now = Date.now();
        if (now - lastPromptAt < 45000)
            return;
        const active = vscode.window.activeTextEditor;
        if (!active)
            return;
        const diags = vscode.languages.getDiagnostics(active.document.uri);
        const bucket = new Set();
        for (const d of diags) {
            const code = String(d.code ?? "");
            const cat = knownAutoFix[code];
            if (cat)
                bucket.add(cat);
        }
        if (bucket.size === 0)
            return;
        lastPromptAt = now;
        const pick = await vscode.window.showInformationMessage(`Auto-remediation available: ${Array.from(bucket).join(", ")}.`, "Preview Fixes", "Later");
        if (pick !== "Preview Fixes")
            return;
        for (const cat of bucket) {
            await vscode.commands.executeCommand("vitte.fixCategoryPreview", cat, active.document.uri);
        }
    }));
}
function registerEnterpriseSuite(context, deps) {
    void applyStrictDefaults();
    registerAutoRemediation(context);
    context.subscriptions.push(vscode.commands.registerCommand("vitte.enterprise.strict.apply", async () => applyStrictDefaults()), vscode.commands.registerCommand("vitte.enterprise.publishGate", async () => {
        try {
            await publishGate(deps);
        }
        catch (err) {
            void vscode.window.showErrorMessage(String(err));
        }
    }), vscode.commands.registerCommand("vitte.perf.baseline.update", async () => perfBaselineUpdate()), vscode.commands.registerCommand("vitte.perf.baseline.check", async () => perfBaselineCheck()), vscode.commands.registerCommand("vitte.exportSarif", async () => exportSarif(deps)), vscode.commands.registerCommand("vitte.exportWorkspaceState", async () => exportWorkspaceState(deps)), vscode.commands.registerCommand("vitte.doctor.plugin", async () => {
        const pick = await vscode.window.showQuickPick(["security", "licenses", "dead-code", "freshness", "docs-coverage", "api-breakage"]);
        if (!pick)
            return;
        await runDoctorPlugin(pick, deps.output);
    }), vscode.commands.registerCommand("vitte.refactor.recipe", async () => {
        const recipe = await vscode.window.showQuickPick(["module split@v1", "api rename@v1", "layer extraction@v1"], { placeHolder: "Select recipe" });
        if (!recipe)
            return;
        const doc = vscode.window.activeTextEditor?.document;
        if (!doc)
            return;
        const lines = doc.getText().split(/\r?\n/).length;
        const funcs = (doc.getText().match(/\b(?:proc|fn|entry)\b/g) ?? []).length;
        const impact = {
            recipe,
            file: vscode.workspace.asRelativePath(doc.uri, false),
            dryRun: true,
            estimatedFiles: recipe.startsWith("module split") ? 2 : recipe.startsWith("layer extraction") ? 3 : 1,
            estimatedLineTouches: Math.min(lines, Math.max(20, Math.floor(lines * 0.35))),
            detectedFunctions: funcs,
        };
        const root = workspaceRoot();
        if (!root)
            return;
        const dir = path.join(root, ".vitte-cache", "refactor");
        await ensureDir(dir);
        const file = path.join(dir, `recipe-${Date.now()}.json`);
        await fs.writeFile(file, `${JSON.stringify(impact, null, 2)}\n`, "utf8");
        void vscode.window.showInformationMessage(`Refactor dry-run report: ${file}`);
    }), vscode.commands.registerCommand("vitte.teamProfile.apply", async () => {
        const profile = await vscode.window.showQuickPick(["backend", "frontend", "platform"], { placeHolder: "Team profile" });
        if (!profile)
            return;
        const cfg = getCfg();
        const target = vscode.workspace.workspaceFolders?.length ? vscode.ConfigurationTarget.Workspace : vscode.ConfigurationTarget.Global;
        if (profile === "backend") {
            await cfg.update("features.inlayHints", true, target);
            await cfg.update("build.profile", "test", target);
        }
        else if (profile === "frontend") {
            await cfg.update("features.semanticTokens", true, target);
            await cfg.update("build.profile", "dev", target);
        }
        else {
            await cfg.update("diagnosticsBudget.maxLatencyMs", 600, target);
            await cfg.update("indexerCacheEnabled", true, target);
        }
        void vscode.window.showInformationMessage(`Team profile applied: ${profile}`);
    }), vscode.commands.registerCommand("vitte.migration.assistant", async () => {
        await vscode.commands.executeCommand("vitte.generateWorkspaceConfig");
        await vscode.commands.executeCommand("vitte.enterprise.strict.apply");
        await vscode.commands.executeCommand("vitte.teamProfile.apply");
    }), vscode.commands.registerCommand("vitte.canary.toggle", async () => {
        const cfg = getCfg();
        const cur = cfg.get("channel", "stable");
        const next = cur === "canary" ? "stable" : "canary";
        const target = vscode.workspace.workspaceFolders?.length ? vscode.ConfigurationTarget.Workspace : vscode.ConfigurationTarget.Global;
        await cfg.update("channel", next, target);
        void vscode.window.showInformationMessage(`Vitte channel: ${next}`);
    }), vscode.commands.registerCommand("vitte.crashForensics.bundle", async () => {
        try {
            await crashForensicsBundle(deps);
        }
        catch (err) {
            void vscode.window.showErrorMessage(`Crash forensics bundle failed: ${String(err)}`);
        }
    }), vscode.languages.registerHoverProvider([{ language: "vitte" }, { language: "vit" }], {
        async provideHover(document, position) {
            const range = document.getWordRangeAtPosition(position);
            if (!range)
                return undefined;
            const word = document.getText(range);
            const snippets = await vscode.workspace.findFiles("snippets/vitte.json", "**/node_modules/**", 1);
            let sample = "";
            if (snippets[0]) {
                try {
                    const raw = await fs.readFile(snippets[0].fsPath, "utf8");
                    if (raw.includes(word))
                        sample = "Snippet sample available in vitte.json";
                }
                catch { /* ignore */ }
            }
            const diags = vscode.languages.getDiagnostics(document.uri).filter((d) => d.range.contains(position));
            const msg = [
                `**${word}**`,
                diags[0] ? `Diagnostic: ${diags[0].message}` : "",
                sample,
                `Docs: run \`Vitte: Search Offline Docs\``,
            ].filter(Boolean).join("\n\n");
            return new vscode.Hover(new vscode.MarkdownString(msg), range);
        },
    }));
}
//# sourceMappingURL=enterpriseSuite.js.map