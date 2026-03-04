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
exports.registerProjectAssistant = registerProjectAssistant;
const path = __importStar(require("node:path"));
const fs = __importStar(require("node:fs/promises"));
const node_child_process_1 = require("node:child_process");
const vscode = __importStar(require("vscode"));
function rootDir() {
    return vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
}
function workspaceRoots() {
    return (vscode.workspace.workspaceFolders ?? []).map((f) => f.uri.fsPath);
}
async function ensureDir(p) {
    await fs.mkdir(p, { recursive: true });
}
async function writeIfMissing(p, content) {
    try {
        await fs.access(p);
    }
    catch {
        await ensureDir(path.dirname(p));
        await fs.writeFile(p, content, "utf8");
    }
}
async function generateWorkspaceVscodeFiles(base) {
    const tasksPath = path.join(base, ".vscode", "tasks.json");
    const launchPath = path.join(base, ".vscode", "launch.json");
    const cfg = vscode.workspace.getConfiguration("vitte");
    const runtimeHint = String(cfg.get("runtime.path", "") || cfg.get("debug.program", "") || "").trim();
    const tasks = {
        version: "2.0.0",
        tasks: [
            { label: "vitte: check (dev)", type: "shell", command: "vitte check --profile dev", group: "build", problemMatcher: [] },
            { label: "vitte: check (ci)", type: "shell", command: "vitte check --profile test", group: "build", problemMatcher: [] },
            { label: "vitte: lint", type: "shell", command: "vitte lint --json", problemMatcher: [] },
            { label: "vitte: format", type: "shell", command: "vitte fmt .", problemMatcher: [] },
            { label: "vitte: test (ci)", type: "shell", command: "vitte test --profile test --json", group: "test", problemMatcher: [] },
            { label: "vitte: perf", type: "shell", command: "vitte bench --profile bench --json", problemMatcher: [] },
            { label: "vitte: doctor", type: "shell", command: "vitte check --json && vitte lint --json && vitte test --json && vitte bench --json", problemMatcher: [] },
        ],
    };
    const launch = {
        version: "0.2.0",
        configurations: [
            {
                name: "Vitte: Launch project entry (dev)",
                type: "vitte",
                request: "launch",
                program: runtimeHint,
                cwd: "${workspaceFolder}",
                args: ["run"],
            },
            {
                name: "Vitte: Launch current file (debug)",
                type: "vitte",
                request: "launch",
                program: runtimeHint,
                cwd: "${workspaceFolder}",
                args: ["run", "${file}"],
                trace: true,
            },
            {
                name: "Vitte: Launch project entry (perf)",
                type: "vitte",
                request: "launch",
                program: runtimeHint,
                cwd: "${workspaceFolder}",
                args: ["run", "--profile", "bench"],
            },
        ],
    };
    await writeIfMissing(tasksPath, `${JSON.stringify(tasks, null, 2)}\n`);
    await writeIfMissing(launchPath, `${JSON.stringify(launch, null, 2)}\n`);
}
async function createNewProject() {
    const ws = rootDir();
    if (!ws) {
        void vscode.window.showErrorMessage("Open a workspace folder first.");
        return;
    }
    const name = await vscode.window.showInputBox({ prompt: "New project name", value: "my_vitte_app" });
    if (!name)
        return;
    const dir = path.join(ws, name);
    await ensureDir(path.join(dir, "src"));
    await writeIfMissing(path.join(dir, "src", "main.vit"), `space ${name}\n\nentry main at ${name} {\n  give 0\n}\n`);
    await writeIfMissing(path.join(dir, "vitte.config.json"), `${JSON.stringify({ name, version: "0.1.0" }, null, 2)}\n`);
    await generateWorkspaceVscodeFiles(dir);
    await vscode.commands.executeCommand("vscode.openFolder", vscode.Uri.file(dir), true);
}
async function createNewModule() {
    const ws = rootDir();
    if (!ws)
        return;
    const mod = await vscode.window.showInputBox({ prompt: "Module path (example: app/network/http)" });
    if (!mod)
        return;
    const rel = mod.replace(/\\/g, "/").replace(/^\/+/, "");
    const file = path.join(ws, "src", `${rel}.vit`);
    await ensureDir(path.dirname(file));
    await fs.writeFile(file, `space ${rel}\n\nshare run\n\nproc run() -> i32 {\n  give 0\n}\n`, "utf8");
    await generateWorkspaceVscodeFiles(ws);
    const doc = await vscode.workspace.openTextDocument(vscode.Uri.file(file));
    await vscode.window.showTextDocument(doc, { preview: false });
}
async function createNewPackage() {
    const ws = rootDir();
    if (!ws)
        return;
    const name = await vscode.window.showInputBox({ prompt: "Package name", value: "core_utils" });
    if (!name)
        return;
    const dir = path.join(ws, "packages", name);
    await ensureDir(dir);
    const modFile = path.join(dir, "mod.vit");
    const pkgPath = `vitte/${name}`;
    await fs.writeFile(modFile, `<<<\nmod.vit\npackage ${pkgPath}\n>>>\n\nspace ${pkgPath}\n\nshare ready\n\nproc ready() -> bool {\n  give true\n}\n\n<<< ROLE-CONTRACT\npackage: ${pkgPath}\nrole: Package responsibility\ninput_contract: Explicit normalized inputs\noutput_contract: Stable explicit outputs\nboundary: No business policy decisions\n>>>\n`, "utf8");
    await generateWorkspaceVscodeFiles(ws);
    const doc = await vscode.workspace.openTextDocument(vscode.Uri.file(modFile));
    await vscode.window.showTextDocument(doc, { preview: false });
}
const doctorCollection = vscode.languages.createDiagnosticCollection("vitte-doctor");
function chooseToolCommand(tool) {
    return { cmd: "vitte", args: [tool, "--json"] };
}
function severityFromAny(value) {
    const s = String(value ?? "").toLowerCase();
    if (s.includes("error") || s === "err")
        return vscode.DiagnosticSeverity.Error;
    if (s.includes("warn"))
        return vscode.DiagnosticSeverity.Warning;
    if (s.includes("info"))
        return vscode.DiagnosticSeverity.Information;
    return vscode.DiagnosticSeverity.Hint;
}
function parseLineInt(value, fallback) {
    const n = Number(value);
    if (!Number.isFinite(n))
        return fallback;
    return Math.max(0, Math.floor(n));
}
function extractJsonValues(text) {
    const out = [];
    const trimmed = text.trim();
    if (!trimmed)
        return out;
    try {
        out.push(JSON.parse(trimmed));
    }
    catch {
        // line-based fallback
    }
    for (const line of text.split(/\r?\n/)) {
        const t = line.trim();
        if (!t || (!t.startsWith("{") && !t.startsWith("[")))
            continue;
        try {
            out.push(JSON.parse(t));
        }
        catch {
            // ignore invalid JSON line
        }
    }
    return out;
}
function normalizeRecords(value) {
    if (Array.isArray(value))
        return value.filter((v) => !!v && typeof v === "object");
    if (value && typeof value === "object") {
        const obj = value;
        if (Array.isArray(obj.items))
            return normalizeRecords(obj.items);
        if (Array.isArray(obj.diagnostics))
            return normalizeRecords(obj.diagnostics);
        return [obj];
    }
    return [];
}
function fileUriFromRecord(ws, rec) {
    const pathLike = String(rec.uri
        ?? rec.file
        ?? rec.path
        ?? (typeof rec.location === "object" && rec.location ? rec.location.file : "")
        ?? "");
    if (pathLike.startsWith("file://"))
        return vscode.Uri.parse(pathLike);
    if (pathLike) {
        const fsPath = path.isAbsolute(pathLike) ? pathLike : path.join(ws, pathLike);
        return vscode.Uri.file(fsPath);
    }
    return vscode.Uri.file(path.join(ws, "vitte.config.json"));
}
function diagnosticsFromToolResult(ws, res) {
    const map = new Map();
    const payloads = extractJsonValues(res.stdout);
    const records = payloads.flatMap(normalizeRecords);
    if (records.length === 0) {
        if (res.code !== 0 || res.stderr.trim()) {
            const uri = vscode.Uri.file(path.join(ws, "vitte.config.json"));
            const d = new vscode.Diagnostic(new vscode.Range(0, 0, 0, 1), `${res.tool} failed (${res.code}): ${res.stderr.trim() || "no structured output"}`, vscode.DiagnosticSeverity.Error);
            d.code = `DOCTOR_${res.tool.toUpperCase()}`;
            d.source = "vitte-doctor";
            map.set(uri.toString(), [d]);
        }
        return map;
    }
    for (const rec of records) {
        const uri = fileUriFromRecord(ws, rec);
        const line = parseLineInt(rec.line ?? (typeof rec.location === "object" && rec.location ? rec.location.line : 1), 1) - 1;
        const col = parseLineInt(rec.column ?? rec.col ?? (typeof rec.location === "object" && rec.location ? rec.location.column : 1), 1) - 1;
        const message = String(rec.message ?? rec.msg ?? rec.error ?? rec.reason ?? `${res.tool} issue`);
        const code = String(rec.code ?? `DOCTOR_${res.tool.toUpperCase()}`);
        const severity = severityFromAny(rec.severity ?? rec.level ?? (res.code === 0 ? "warning" : "error"));
        const diag = new vscode.Diagnostic(new vscode.Range(Math.max(0, line), Math.max(0, col), Math.max(0, line), Math.max(1, col + 1)), message, severity);
        diag.code = code.startsWith("DOCTOR_") ? code : `DOCTOR_${res.tool.toUpperCase()}_${code}`;
        diag.source = "vitte-doctor";
        const arr = map.get(uri.toString()) ?? [];
        arr.push(diag);
        map.set(uri.toString(), arr);
    }
    return map;
}
async function runTool(ws, tool) {
    const { cmd, args } = chooseToolCommand(tool);
    const t0 = Date.now();
    return await new Promise((resolve) => {
        const child = (0, node_child_process_1.spawn)(cmd, args, { cwd: ws, shell: false });
        let stdout = "";
        let stderr = "";
        child.stdout.on("data", (d) => { stdout += String(d); });
        child.stderr.on("data", (d) => { stderr += String(d); });
        child.on("error", (err) => {
            resolve({ tool, cmd, args, code: 127, stdout, stderr: `${stderr}\n${String(err)}`, durationMs: Date.now() - t0 });
        });
        child.on("close", (code) => {
            resolve({ tool, cmd, args, code: code ?? 1, stdout, stderr, durationMs: Date.now() - t0 });
        });
    });
}
async function runDoctor(output, selected) {
    const roots = workspaceRoots();
    if (roots.length === 0)
        return;
    const tools = selected ? [selected] : ["check", "lint", "test", "bench"];
    const finalMap = new Map();
    output.appendLine(`[doctor] run ${tools.join(", ")} on ${roots.length} root(s)`);
    for (const ws of roots) {
        for (const tool of tools) {
            const res = await runTool(ws, tool);
            const relRoot = vscode.workspace.asRelativePath(vscode.Uri.file(ws), false) || ws;
            output.appendLine(`[doctor:${tool}] root=${relRoot} cmd=${res.cmd} ${res.args.join(" ")} code=${res.code} ${res.durationMs}ms`);
            if (res.stderr.trim())
                output.appendLine(`[doctor:${tool}:stderr] ${res.stderr.trim()}`);
            const dm = diagnosticsFromToolResult(ws, res);
            for (const [key, list] of dm) {
                const prev = finalMap.get(key) ?? [];
                finalMap.set(key, [...prev, ...list]);
            }
        }
    }
    const entries = [];
    for (const [uriText, diags] of finalMap) {
        entries.push([vscode.Uri.parse(uriText), diags]);
    }
    if (!selected)
        doctorCollection.clear();
    if (entries.length > 0)
        doctorCollection.set(entries);
    const total = entries.reduce((acc, [, d]) => acc + d.length, 0);
    void vscode.window.showInformationMessage(`Vitte Doctor: ${total} issue(s) published to Problems.`);
}
async function runExtensionBench(clientGetter, output, activationMs) {
    const editor = vscode.window.activeTextEditor;
    const mem = process.memoryUsage();
    const sample = {
        ts: new Date().toISOString(),
        activationMs: activationMs ?? null,
        rssMB: Number((mem.rss / (1024 * 1024)).toFixed(2)),
        heapUsedMB: Number((mem.heapUsed / (1024 * 1024)).toFixed(2)),
    };
    const client = clientGetter();
    if (client) {
        const p0 = Date.now();
        try {
            await client.sendRequest("vitte/ping");
            sample.pingMs = Date.now() - p0;
        }
        catch (err) {
            sample.pingError = String(err);
        }
    }
    if (editor) {
        const pos = editor.selection.active;
        const latencies = [];
        for (let i = 0; i < 5; i++) {
            const t = Date.now();
            try {
                await vscode.commands.executeCommand("vscode.executeCompletionItemProvider", editor.document.uri, pos, ".");
                latencies.push(Date.now() - t);
            }
            catch {
                // ignore
            }
        }
        if (latencies.length > 0) {
            sample.completionAvgMs = Number((latencies.reduce((a, b) => a + b, 0) / latencies.length).toFixed(2));
            sample.completionMaxMs = Math.max(...latencies);
        }
    }
    output.appendLine(`[bench-ci] ${JSON.stringify(sample)}`);
    const ws = rootDir();
    if (!ws)
        return;
    const outFile = path.join(ws, ".vitte-cache", "diagnostics", "extension-bench.json");
    await ensureDir(path.dirname(outFile));
    await fs.writeFile(outFile, `${JSON.stringify(sample, null, 2)}\n`, "utf8");
    void vscode.window.showInformationMessage(`Vitte bench saved: ${outFile}`);
}
function registerProjectAssistant(context, output, getClient, getActivationMs) {
    context.subscriptions.push(doctorCollection, vscode.commands.registerCommand("vitte.projectAssistant", async () => {
        const pick = await vscode.window.showQuickPick([
            { label: "New Project", command: "vitte.newProject" },
            { label: "New Module", command: "vitte.newModule" },
            { label: "New Package", command: "vitte.newPackage" },
            { label: "Generate .vscode/tasks+launch", command: "vitte.generateWorkspaceConfig" },
            { label: "Run Doctor", command: "vitte.doctor" },
        ], { placeHolder: "Vitte Project Assistant" });
        if (!pick)
            return;
        await vscode.commands.executeCommand(pick.command);
    }), vscode.commands.registerCommand("vitte.newProject", createNewProject), vscode.commands.registerCommand("vitte.newModule", createNewModule), vscode.commands.registerCommand("vitte.newPackage", createNewPackage), vscode.commands.registerCommand("vitte.generateWorkspaceConfig", async () => {
        const roots = workspaceRoots();
        if (roots.length === 0)
            return;
        for (const ws of roots)
            await generateWorkspaceVscodeFiles(ws);
        void vscode.window.showInformationMessage(`Generated .vscode/tasks.json and launch.json for ${roots.length} root(s).`);
    }), vscode.commands.registerCommand("vitte.doctor", async () => runDoctor(output)), vscode.commands.registerCommand("vitte.doctor.runTool", async (tool) => {
        const t = String(tool ?? "").toLowerCase();
        if (t === "check" || t === "lint" || t === "test" || t === "bench") {
            await runDoctor(output, t);
        }
    }), vscode.commands.registerCommand("vitte.benchExtensionCi", async () => {
        await runExtensionBench(getClient, output, getActivationMs());
    }));
}
//# sourceMappingURL=projectAssistant.js.map