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
exports.registerDebugConfigurationProvider = registerDebugConfigurationProvider;
const vscode = __importStar(require("vscode"));
const path = __importStar(require("path"));
/** Read and parse JSON file into object. */
async function readJsonFile(uri) {
    try {
        const doc = await vscode.workspace.openTextDocument(uri);
        return JSON.parse(doc.getText());
    }
    catch {
        return undefined;
    }
}
/** Try multiple known config locations, first hit wins then shallow-merge. */
async function readVitteProjectConfig() {
    const results = [];
    // 1) vitte.config.json at workspace root
    const rootCfg = await vscode.workspace.findFiles('vitte.config.json', '**/node_modules/**', 1);
    const rootUri = rootCfg[0];
    if (rootUri) {
        const j = await readJsonFile(rootUri);
        if (j)
            results.push(j);
    }
    // 2) .vitte/config.json fallback
    const hiddenCfg = await vscode.workspace.findFiles('.vitte/config.json', '**/node_modules/**', 1);
    const hiddenUri = hiddenCfg[0];
    if (hiddenUri) {
        const j = await readJsonFile(hiddenUri);
        if (j)
            results.push(j);
    }
    // 3) package.json { vitte: { debug: {...} } }
    const pkg = await vscode.workspace.findFiles('package.json', '**/node_modules/**', 1);
    const pkgUri = pkg[0];
    if (pkgUri) {
        const pjson = await readJsonFile(pkgUri);
        const vitteSection = pjson?.vitte;
        if (vitteSection && typeof vitteSection === 'object') {
            const pick = {};
            const debugSection = vitteSection.debug;
            const toolchainSection = vitteSection.toolchain;
            if (debugSection && typeof debugSection === 'object')
                pick.debug = { ...debugSection };
            if (toolchainSection && typeof toolchainSection === 'object')
                pick.toolchain = { ...toolchainSection };
            results.push(pick);
        }
    }
    // Merge shallowly from first to last, later wins
    const merged = {};
    for (const r of results) {
        if (r.toolchain)
            merged.toolchain = { ...(merged.toolchain ?? {}), ...r.toolchain };
        if (r.debug)
            merged.debug = { ...(merged.debug ?? {}), ...r.debug };
    }
    return merged;
}
function ensureWorkspaceFolder() {
    return vscode.workspace.workspaceFolders?.[0]?.uri.fsPath;
}
/** Expand VS Code variables and env vars in strings. */
function expand(str, fileUri) {
    const wf = ensureWorkspaceFolder() ?? '';
    const active = fileUri ?? vscode.window.activeTextEditor?.document.uri;
    const filePath = active?.fsPath ?? '';
    const fileDir = filePath ? path.dirname(filePath) : '';
    const env = process.env;
    const table = {
        workspaceFolder: wf,
        file: filePath,
        fileDirname: fileDir,
    };
    return str.replace(/\$\{(env:)?([^}]+)}/g, function (_m, envPrefix, key) {
        const k = String(key);
        if (envPrefix)
            return env[k] ?? '';
        return table[k] ?? '';
    });
}
function coerceArgs(v) {
    if (Array.isArray(v))
        return v.filter((x) => typeof x === 'string' && x.length > 0);
    if (typeof v === 'string' && v.length)
        return [v];
    return undefined;
}
function sanitizeEnv(obj) {
    const out = {};
    if (obj && typeof obj === 'object') {
        for (const [k, v] of Object.entries(obj)) {
            if (typeof v === 'string')
                out[k] = v;
        }
    }
    return out;
}
function resolveRuntimePath(cfg, project) {
    // Priority: launch.json > vitte.debug.program (settings) > project.debug.program > toolchain.runtime > default
    const setProgram = cfg.get('debug.program');
    if (setProgram?.trim())
        return setProgram;
    const projectProg = project.debug?.program ?? project.toolchain?.runtime;
    let candidate = projectProg ?? 'vitte-runtime';
    // Prepend toolchain.root when relative
    const toolchainRoot = cfg.get('toolchain.root') ?? project.toolchain?.root ?? cfg.get('toolchainPath');
    if (toolchainRoot && candidate && !path.isAbsolute(candidate)) {
        candidate = path.join(toolchainRoot, candidate);
    }
    return candidate;
}
/**
 * Provides initial debug configurations and resolves them using Vitte settings and project config files.
 */
class VitteDebugConfigurationProvider {
    provideDebugConfigurations(_folder) {
        const workspaceFolder = ensureWorkspaceFolder() ?? '${workspaceFolder}';
        return [
            {
                name: 'Vitte: Launch current file',
                type: 'vitte',
                request: 'launch',
                program: 'vitte-runtime',
                args: ['run', '${file}'],
                cwd: workspaceFolder
            },
            {
                name: 'Vitte: Launch project entry',
                type: 'vitte',
                request: 'launch',
                program: 'vitte-runtime',
                args: ['run'],
                cwd: workspaceFolder
            },
            {
                name: 'Vitte: Launch with args…',
                type: 'vitte',
                request: 'launch',
                program: 'vitte-runtime',
                args: [],
                cwd: workspaceFolder
            }
        ];
    }
    async resolveDebugConfiguration(folder, config) {
        const settings = vscode.workspace.getConfiguration('vitte');
        const project = await readVitteProjectConfig();
        const workspaceFolder = folder?.uri.fsPath ?? ensureWorkspaceFolder() ?? process.cwd();
        const activeUri = vscode.window.activeTextEditor?.document.uri;
        const configRecord = config;
        const configEnvValue = configRecord.env;
        const configEnvObject = (typeof configEnvValue === 'object' && configEnvValue) ? configEnvValue : undefined;
        const mergedEnvRaw = {
            ...(project.debug?.env ?? {}),
            ...(configEnvObject ?? {}),
        };
        const mergedEnv = sanitizeEnv(mergedEnvRaw);
        const base = {
            type: 'vitte',
            request: 'launch',
            name: typeof configRecord.name === 'string' && configRecord.name.length > 0 ? configRecord.name : 'Vitte: Launch',
            program: resolveRuntimePath(settings, project),
            args: coerceArgs(configRecord.args) ?? coerceArgs(project.debug?.args) ?? ['run'],
            cwd: (() => {
                const cfgCwd = configRecord.cwd;
                if (typeof cfgCwd === 'string' && cfgCwd.length > 0)
                    return cfgCwd;
                if (typeof project.debug?.cwd === 'string' && project.debug.cwd.length > 0)
                    return project.debug.cwd;
                return workspaceFolder;
            })(),
            env: mergedEnv,
        };
        // Optional flags
        const configTraceRaw = configRecord.trace;
        if (typeof project.debug?.trace === 'boolean' && typeof configTraceRaw !== 'boolean') {
            base.trace = project.debug.trace;
        }
        const configSourceMapsRaw = configRecord.sourceMaps;
        if (typeof project.debug?.sourceMaps === 'boolean' && typeof configSourceMapsRaw !== 'boolean') {
            base.sourceMaps = project.debug.sourceMaps;
        }
        // VS Code variable expansion
        const programValue = typeof base.program === 'string' ? base.program : String(base.program ?? '');
        base.program = expand(programValue, activeUri);
        const cwdValue = typeof base.cwd === 'string' ? base.cwd : String(base.cwd ?? workspaceFolder);
        base.cwd = expand(cwdValue, activeUri);
        const argsValue = Array.isArray(base.args) ? base.args : [];
        base.args = argsValue.map((s) => expand(String(s), activeUri));
        if (base.env && typeof base.env === 'object') {
            const env = {};
            for (const [k, v] of Object.entries(base.env)) {
                if (typeof v === 'string')
                    env[k] = expand(v, activeUri);
            }
            base.env = env;
        }
        // Sanity
        if (!base.program || typeof base.program !== 'string') {
            void vscode.window.showErrorMessage('Vitte: aucun binaire runtime configuré. Configurez "vitte.debug.program" ou vitte.config.json > debug.program.');
            return undefined;
        }
        return base;
    }
}
function registerDebugConfigurationProvider(ctx) {
    const provider = new VitteDebugConfigurationProvider();
    ctx.subscriptions.push(vscode.debug.registerDebugConfigurationProvider('vitte', provider));
}
//# sourceMappingURL=configurationProvider.js.map