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
exports.registerBenchTasks = registerBenchTasks;
const vscode = __importStar(require("vscode"));
const cp = __importStar(require("child_process"));
const path = __importStar(require("path"));
const fs = __importStar(require("fs"));
const runtimeLocator_1 = require("../debug/runtimeLocator");
/**
 * Bench tasks & commands for Vitte
 *
 * Commands:
 *  - vitte.bench: Run benchmarks for the current workspace (profile/targets from settings or vitte.config.json)
 *  - vitte.benchReport: Run + open the latest HTML/SVG report if produced
 *  - vitte.openBenchDir: Reveal the bench report directory
 *
 * Task provider:
 *  - type: 'vitte' with command: 'bench' to integrate with VS Code Tasks
 */
function registerBenchTasks(ctx) {
    // Commands
    ctx.subscriptions.push(vscode.commands.registerCommand('vitte.bench', async () => {
        await runBench(false);
    }), vscode.commands.registerCommand('vitte.benchReport', async () => {
        await runBench(true);
    }), vscode.commands.registerCommand('vitte.openBenchDir', async () => {
        const dir = await benchReportDir();
        if (dir)
            void vscode.commands.executeCommand('revealFileInOS', vscode.Uri.file(dir));
    }));
    const provider = {
        provideTasks: async (_token) => {
            const cmd = await benchCommandLine();
            const definition = { type: 'vitte', command: 'bench' };
            const task = new vscode.Task(definition, vscode.TaskScope.Workspace, 'Vitte Bench', 'vitte', new vscode.ShellExecution(cmd));
            return [task];
        },
        resolveTask: (task) => task,
    };
    ctx.subscriptions.push(vscode.tasks.registerTaskProvider('vitte', provider));
}
// ---- Helpers ----
async function benchBin() {
    const located = await (0, runtimeLocator_1.locateVitteRuntime)();
    return located.benchPath ?? 'vitte-bench';
}
async function readProjectConfig() {
    try {
        const files = await vscode.workspace.findFiles('vitte.config.json', '**/node_modules/**', 1);
        const uri = files[0];
        if (!uri)
            return undefined;
        const doc = await vscode.workspace.openTextDocument(uri);
        return JSON.parse(doc.getText());
    }
    catch {
        return undefined;
    }
}
async function benchReportDir() {
    const cfg = vscode.workspace.getConfiguration('vitte');
    const project = await readProjectConfig();
    const root = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    if (!root)
        return undefined;
    const reportDir = (project?.bench?.reportDir ?? cfg.get('bench.reportDir')) ?? '.vitte/bench';
    return path.isAbsolute(reportDir) ? reportDir : path.join(root, reportDir);
}
async function benchArgs() {
    const cfg = vscode.workspace.getConfiguration('vitte');
    const project = await readProjectConfig();
    const profile = project?.build?.profile ?? cfg.get('build.profile') ?? 'bench';
    const distributed = (project?.build?.distributed ?? cfg.get('build.distributed')) ?? false;
    const incremental = (project?.build?.incremental ?? cfg.get('build.incremental')) ?? false;
    const reportDir = await benchReportDir();
    const args = ['--profile', profile];
    if (distributed)
        args.push('--distributed');
    if (incremental)
        args.push('--incremental');
    if (reportDir)
        args.push('--out', reportDir);
    // Export formats from config
    const exportKinds = project?.bench?.export;
    if (Array.isArray(exportKinds)) {
        for (const kind of exportKinds) {
            if (typeof kind === 'string') {
                args.push('--export', kind);
            }
        }
    }
    // Regression threshold
    if (typeof project?.bench?.regressionThreshold === 'number') {
        args.push('--threshold', String(project.bench.regressionThreshold));
    }
    return args;
}
async function benchCommandLine() {
    const bin = await benchBin();
    const args = await benchArgs();
    const cmd = [quote(bin), ...args.map(quote)].join(' ');
    return cmd;
}
async function runBench(openAfter) {
    const root = vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
    if (!root) {
        void vscode.window.showErrorMessage('Vitte: aucun workspace ouvert.');
        return;
    }
    const cmd = await benchCommandLine();
    const outChan = vscode.window.createOutputChannel('Vitte Bench');
    await vscode.window.withProgress({
        location: vscode.ProgressLocation.Notification,
        title: 'Vitte: Benchmarks en cours…',
        cancellable: false,
    }, async () => {
        outChan.clear();
        outChan.show(true);
        outChan.appendLine(`[cmd] ${cmd}`);
        await new Promise((resolve) => {
            const proc = cp.spawn(cmd, { cwd: root, shell: true, env: process.env });
            proc.stdout?.on('data', (b) => outChan.append(b.toString()));
            proc.stderr?.on('data', (b) => outChan.append(b.toString()));
            proc.on('error', (e) => {
                outChan.appendLine(`\n[error] ${e.message}`);
                resolve();
            });
            proc.on('close', (code) => {
                outChan.appendLine(`\n[exit] code=${code}`);
                resolve();
            });
        });
    });
    if (openAfter)
        await openLatestReport();
}
async function openLatestReport() {
    try {
        const dir = await benchReportDir();
        if (!dir)
            return;
        const files = await vscode.workspace.findFiles(new vscode.RelativePattern(dir, '*.{html,svg}'));
        if (files.length === 0) {
            void vscode.window.showInformationMessage('Vitte Bench: aucun rapport HTML/SVG trouvé.');
            return;
        }
        // Sort by mtime (best-effort). Guard array access for noUncheckedIndexedAccess.
        const entries = files
            .map((u) => {
            try {
                return { uri: u, mtime: fs.statSync(u.fsPath).mtimeMs };
            }
            catch {
                return { uri: u, mtime: 0 };
            }
        })
            .sort((a, b) => b.mtime - a.mtime);
        const first = entries[0];
        if (!first) {
            void vscode.window.showInformationMessage('Vitte Bench: aucun rapport HTML/SVG trouvé.');
            return;
        }
        void vscode.commands.executeCommand('vscode.open', first.uri);
    }
    catch (e) {
        const message = e instanceof Error ? e.message : String(e);
        void vscode.window.showWarningMessage(`Vitte Bench: échec d'ouverture du rapport (${message})`);
    }
}
function quote(s) {
    if (process.platform === 'win32')
        return `"${s.replace(/"/g, '\\"')}"`;
    return `'${s.replace(/'/g, `\'`)}'`;
}
//# sourceMappingURL=benchTasks.js.map