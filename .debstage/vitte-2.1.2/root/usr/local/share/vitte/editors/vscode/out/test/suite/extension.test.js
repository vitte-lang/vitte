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
const EXTENSION_ID = "VitteStudio.vitte-studio";
async function waitUntil(condition, timeout = 5000, step = 50) {
    const start = Date.now();
    while (Date.now() - start <= timeout) {
        if (await condition()) {
            return;
        }
        await new Promise((resolve) => setTimeout(resolve, step));
    }
    throw new Error("Timed out waiting for condition");
}
suite("Vitte extension", () => {
    let extension;
    let api;
    suiteSetup(async () => {
        extension = vscode.extensions.getExtension(EXTENSION_ID);
        assert.ok(extension, "Extension introuvable dans le registre VS Code");
        await extension.activate();
        assert.ok(extension.isActive, "Extension non active après activation");
        api = extension.exports;
    });
    test("Extension s’active sans erreur", () => {
        assert.ok(extension, "Extension non initialisée");
        assert.ok(extension.isActive, "Extension non active après activation");
        assert.ok(api, "API de test non exposée par l’extension");
    });
    test("Les commandes principales sont déclarées", async () => {
        const commands = await vscode.commands.getCommands(true);
        const expected = [
            "vitte.showServerLog",
            "vitte.restartServer",
            "vitte.runAction",
            "vitte.quickActions",
            "vitte.diagnostics.refresh",
        ];
        for (const cmd of expected) {
            assert.ok(commands.includes(cmd), `Commande ${cmd} non enregistrée`);
        }
    });
    test("La commande restart redémarre le client", async () => {
        const testApi = api;
        assert.ok(testApi, "API de test non disponible");
        await vscode.commands.executeCommand("vitte.restartServer");
        await waitUntil(() => testApi.getStatusText().startsWith("$(check)"), 8000);
        const tooltip = testApi.getStatusTooltip().toLowerCase();
        assert.ok(tooltip.includes("opérationnel") || tooltip.includes("running"), "Le statut du client n’indique pas qu’il est démarré");
        assert.ok(tooltip.includes("diagnostics"), "Le statut ne mentionne pas la synthèse des diagnostics");
    });
    test("La commande runAction déclenche l’action sélectionnée", async () => {
        const testApi = api;
        assert.ok(testApi, "API de test non disponible");
        const disposables = [];
        let formatCalled = false;
        disposables.push(vscode.commands.registerCommand("editor.action.formatDocument", () => {
            formatCalled = true;
        }));
        const windowAny = vscode.window;
        const originalQuickPick = windowAny.showQuickPick;
        windowAny.showQuickPick = () => Promise.resolve({
            label: "Format document",
            description: "editor.action.formatDocument",
            action: "format",
        });
        const document = await vscode.workspace.openTextDocument({ content: "test", language: "vitte" });
        await vscode.window.showTextDocument(document, { preview: false });
        try {
            await vscode.commands.executeCommand("vitte.runAction");
            await waitUntil(() => formatCalled, 1000, 20);
        }
        finally {
            windowAny.showQuickPick = originalQuickPick;
            for (const disposable of disposables) {
                disposable.dispose();
            }
            await vscode.commands.executeCommand("workbench.action.closeActiveEditor");
        }
        assert.ok(formatCalled, "La commande de formatage n’a pas été appelée");
    });
    test("Une résolution sans serveur signale une erreur explicite", () => {
        const ext = extension;
        assert.ok(ext, "Extension non initialisée");
        const testApi = api;
        assert.ok(testApi, "API de test non disponible");
        const fakeRoot = path.join(ext.extensionPath, ".test-missing-server", Date.now().toString(36));
        assert.ok(!fs.existsSync(fakeRoot), "Le répertoire factice ne devrait pas exister");
        const fakeContext = {
            asAbsolutePath: (relPath) => path.join(fakeRoot, relPath),
        };
        assert.throws(() => testApi.resolveServerModuleForTest(fakeContext), /Module serveur Vitte introuvable/);
    });
    test("Quick Actions exécutent la séquence d’onboarding", async () => {
        const runOrder = [];
        const tracked = new Set(["vitte.openDocs", "vitte.detectToolchain", "vitte.build"]);
        const commandsAny = vscode.commands;
        const originalExecuteCommand = commandsAny.executeCommand.bind(vscode.commands);
        commandsAny.executeCommand = (command, ...rest) => {
            if (tracked.has(command)) {
                runOrder.push(command);
                return Promise.resolve(undefined);
            }
            return originalExecuteCommand(command, ...rest);
        };
        const windowAny = vscode.window;
        const originalCreateQuickPick = windowAny.createQuickPick;
        let createdQuickPick;
        windowAny.createQuickPick = () => {
            createdQuickPick = new QuickPickStub();
            return createdQuickPick;
        };
        try {
            const commandPromise = vscode.commands.executeCommand("vitte.quickActions");
            const quickPick = await waitFor(() => createdQuickPick, 1000);
            const items = await quickPick.waitForItems((list) => list.some((item) => item.actionId === "onboarding.setup"));
            const target = items.find((item) => item.actionId === "onboarding.setup");
            assert.ok(target, "Action onboarding.setup introuvable");
            quickPick.selectedItems = [target];
            quickPick.fireAccept();
            await commandPromise;
            assert.deepEqual(runOrder, ["vitte.openDocs", "vitte.detectToolchain", "vitte.build"], "La séquence Run setup n’a pas déclenché tous les sous-commandes");
        }
        finally {
            windowAny.createQuickPick = originalCreateQuickPick;
            commandsAny.executeCommand = originalExecuteCommand;
        }
    });
    test("Quick Actions respectent les séquences configurées", async () => {
        const cfg = vscode.workspace.getConfiguration("vitte");
        const customSequence = [
            {
                id: "custom.buildTestBench",
                label: "Build + Test + Bench",
                steps: [
                    { command: "vitte.build" },
                    { command: "vitte.test" },
                    { command: "vitte.bench" }
                ]
            }
        ];
        await cfg.update("quickActions.sequences", customSequence, vscode.ConfigurationTarget.Global);
        const runOrder = [];
        const tracked = new Set(["vitte.build", "vitte.test", "vitte.bench"]);
        const commandsAny = vscode.commands;
        const originalExecuteCommand = commandsAny.executeCommand.bind(vscode.commands);
        commandsAny.executeCommand = (command, ...rest) => {
            if (tracked.has(command)) {
                runOrder.push(command);
                return Promise.resolve(undefined);
            }
            return originalExecuteCommand(command, ...rest);
        };
        const windowAny = vscode.window;
        const originalCreateQuickPick = windowAny.createQuickPick;
        let createdQuickPick;
        windowAny.createQuickPick = () => {
            createdQuickPick = new QuickPickStub();
            return createdQuickPick;
        };
        try {
            const commandPromise = vscode.commands.executeCommand("vitte.quickActions");
            const quickPick = await waitFor(() => createdQuickPick, 1000);
            const items = await quickPick.waitForItems((list) => list.some((item) => item.actionId === "custom.buildTestBench"), 2000);
            const target = items.find((item) => item.actionId === "custom.buildTestBench");
            assert.ok(target, "Séquence personnalisée introuvable");
            quickPick.selectedItems = [target];
            quickPick.fireAccept();
            await commandPromise;
            assert.deepEqual(runOrder, ["vitte.build", "vitte.test", "vitte.bench"], "La séquence personnalisée n’a pas exécuté toutes les commandes");
        }
        finally {
            windowAny.createQuickPick = originalCreateQuickPick;
            commandsAny.executeCommand = originalExecuteCommand;
            await cfg.update("quickActions.sequences", undefined, vscode.ConfigurationTarget.Global);
        }
    });
    test("Quick Actions déclenchent bench et refresh diagnostics", async () => {
        const workspace = vscode.workspace.workspaceFolders?.[0];
        if (!workspace) {
            // Tests peuvent tourner sans workspace (ex: CI headless) — on passe dans ce cas.
            return;
        }
        const benchConfigPath = path.join(workspace.uri.fsPath, "vitte.config.json");
        fs.writeFileSync(benchConfigPath, "{}");
        const tracked = new Set(["vitte.bench", "vitte.diagnostics.refresh"]);
        const runOrder = [];
        const commandsAny = vscode.commands;
        const originalExecuteCommand = commandsAny.executeCommand.bind(vscode.commands);
        commandsAny.executeCommand = (command, ...rest) => {
            if (tracked.has(command)) {
                runOrder.push(command);
                return Promise.resolve(undefined);
            }
            return originalExecuteCommand(command, ...rest);
        };
        const windowAny = vscode.window;
        const originalCreateQuickPick = windowAny.createQuickPick;
        let createdQuickPick;
        windowAny.createQuickPick = () => {
            createdQuickPick = new QuickPickStub();
            return createdQuickPick;
        };
        async function runQuickAction(actionId) {
            const commandPromise = vscode.commands.executeCommand("vitte.quickActions");
            const quickPick = await waitFor(() => createdQuickPick, 1000);
            const items = await quickPick.waitForItems((list) => list.some((item) => item.actionId === actionId), 2000);
            const target = items.find((item) => item.actionId === actionId);
            assert.ok(target, `Action ${actionId} introuvable`);
            quickPick.selectedItems = [target];
            quickPick.fireAccept();
            await commandPromise;
            createdQuickPick = undefined;
        }
        try {
            await runQuickAction("vitte.bench");
            await runQuickAction("vitte.diagnostics.refresh");
            assert.deepEqual(runOrder, ["vitte.bench", "vitte.diagnostics.refresh"], "Les actions bench/diagnostics n'ont pas été exécutées dans l'ordre attendu");
        }
        finally {
            commandsAny.executeCommand = originalExecuteCommand;
            windowAny.createQuickPick = originalCreateQuickPick;
            if (fs.existsSync(benchConfigPath)) {
                fs.unlinkSync(benchConfigPath);
            }
        }
    });
    test("Quick Actions toggles profile/incremental", async () => {
        const cfg = vscode.workspace.getConfiguration("vitte");
        const originalProfile = cfg.get("build.profile");
        const originalIncremental = cfg.get("build.incremental");
        await cfg.update("build.profile", "dev", vscode.ConfigurationTarget.Global);
        await cfg.update("build.incremental", false, vscode.ConfigurationTarget.Global);
        const commandsAny = vscode.commands;
        const originalExecuteCommand = commandsAny.executeCommand.bind(vscode.commands);
        const tracked = [];
        commandsAny.executeCommand = (command, ...rest) => {
            if (command === "vitte.switchProfile" || command === "vitte.toggleIncremental") {
                tracked.push(command);
                return Promise.resolve(undefined);
            }
            return originalExecuteCommand(command, ...rest);
        };
        const windowAny = vscode.window;
        const originalCreateQuickPick = windowAny.createQuickPick;
        let activeQuickPick;
        windowAny.createQuickPick = () => {
            activeQuickPick = new QuickPickStub();
            return activeQuickPick;
        };
        try {
            const commandPromise = vscode.commands.executeCommand("vitte.quickActions");
            const quickPick = await waitFor(() => activeQuickPick, 1000);
            const items = await quickPick.waitForItems();
            const profileItem = items.find((item) => item.label?.includes("Profile"));
            const incrementalItem = items.find((item) => item.label?.includes("Incremental"));
            assert.ok(profileItem, "Profile toggle not found");
            assert.ok(incrementalItem, "Incremental toggle not found");
            quickPick.selectedItems = [profileItem];
            quickPick.fireAccept();
            await commandPromise;
            const firstStub = activeQuickPick;
            if (firstStub) {
                firstStub.dispose();
                activeQuickPick = undefined;
            }
            const secondRun = vscode.commands.executeCommand("vitte.quickActions");
            const quickPick2 = await waitFor(() => activeQuickPick, 1000);
            quickPick2.selectedItems = [incrementalItem];
            quickPick2.fireAccept();
            await secondRun;
            const secondStub = activeQuickPick;
            if (secondStub) {
                secondStub.dispose();
                activeQuickPick = undefined;
            }
            assert.deepEqual(tracked, ["vitte.switchProfile", "vitte.toggleIncremental"], "Toggles were not executed in order");
        }
        finally {
            windowAny.createQuickPick = originalCreateQuickPick;
            commandsAny.executeCommand = originalExecuteCommand;
            await cfg.update("build.profile", originalProfile, vscode.ConfigurationTarget.Global);
            await cfg.update("build.incremental", originalIncremental, vscode.ConfigurationTarget.Global);
        }
    });
    test("Quick Actions trigger Next Diagnostic", async () => {
        const commandsAny = vscode.commands;
        const originalExecuteCommand = commandsAny.executeCommand.bind(vscode.commands);
        let markerCalled = false;
        commandsAny.executeCommand = (command, ...rest) => {
            if (command === "editor.action.marker.next") {
                markerCalled = true;
                return Promise.resolve(undefined);
            }
            return originalExecuteCommand(command, ...rest);
        };
        const windowAny = vscode.window;
        const originalCreateQuickPick = windowAny.createQuickPick;
        let activeQuickPick;
        windowAny.createQuickPick = () => {
            activeQuickPick = new QuickPickStub();
            return activeQuickPick;
        };
        try {
            const commandPromise = vscode.commands.executeCommand("vitte.quickActions");
            const quickPick = await waitFor(() => activeQuickPick, 1000);
            const items = await quickPick.waitForItems();
            const diagItem = items.find((item) => item.label?.includes("Next issue"));
            assert.ok(diagItem, "Next issue action not found");
            quickPick.selectedItems = [diagItem];
            quickPick.fireAccept();
            await commandPromise;
            const diagStub = activeQuickPick;
            if (diagStub) {
                diagStub.dispose();
                activeQuickPick = undefined;
            }
            assert.ok(markerCalled, "Diagnostics navigation command was not executed");
        }
        finally {
            commandsAny.executeCommand = originalExecuteCommand;
            windowAny.createQuickPick = originalCreateQuickPick;
        }
    });
});
async function waitFor(resolver, timeout = 1000, step = 10) {
    const start = Date.now();
    while (Date.now() - start < timeout) {
        const value = resolver();
        if (value !== undefined)
            return value;
        await new Promise((resolve) => setTimeout(resolve, step));
    }
    throw new Error("Timed out waiting for value");
}
class QuickPickStub {
    constructor() {
        this.acceptEmitter = new vscode.EventEmitter();
        this.hideEmitter = new vscode.EventEmitter();
        this.selectionEmitter = new vscode.EventEmitter();
        this.valueEmitter = new vscode.EventEmitter();
        this.activeEmitter = new vscode.EventEmitter();
        this.buttonEmitter = new vscode.EventEmitter();
        this.itemButtonEmitter = new vscode.EventEmitter();
        this.itemsEmitter = new vscode.EventEmitter();
        this._items = [];
        this._selectedItems = [];
        this._buttons = [];
        this._activeItems = [];
        this.enabled = true;
        this.busy = false;
        this.ignoreFocusOut = true;
        this.canSelectMany = false;
        this.matchOnDescription = true;
        this.matchOnDetail = false;
        this.keepScrollPosition = false;
        this.value = "";
        this.sortByLabel = true;
        this.onDidAccept = this.acceptEmitter.event;
        this.onDidHide = this.hideEmitter.event;
        this.onDidChangeSelection = this.selectionEmitter.event;
        this.onDidChangeValue = this.valueEmitter.event;
        this.onDidChangeActive = this.activeEmitter.event;
        this.onDidTriggerButton = this.buttonEmitter.event;
        this.onDidTriggerItemButton = this.itemButtonEmitter.event;
    }
    get items() {
        return this._items;
    }
    set items(value) {
        this._items = value;
        this.itemsEmitter.fire(value);
    }
    get buttons() {
        return this._buttons;
    }
    set buttons(value) {
        this._buttons = value;
    }
    get selectedItems() {
        return this._selectedItems;
    }
    set selectedItems(value) {
        this._selectedItems = value;
        this.selectionEmitter.fire(value);
    }
    get activeItems() {
        return this._activeItems;
    }
    set activeItems(value) {
        this._activeItems = value;
        this.activeEmitter.fire(value);
    }
    show() {
        /* no-op */
    }
    hide() {
        this.hideEmitter.fire();
    }
    dispose() {
        this.acceptEmitter.dispose();
        this.hideEmitter.dispose();
        this.selectionEmitter.dispose();
        this.valueEmitter.dispose();
        this.activeEmitter.dispose();
        this.buttonEmitter.dispose();
        this.itemButtonEmitter.dispose();
        this.itemsEmitter.dispose();
    }
    fireAccept() {
        this.acceptEmitter.fire();
    }
    waitForItems(predicate, timeout = 2000) {
        const check = predicate ?? ((list) => list.length > 0);
        if (check(this._items)) {
            return Promise.resolve(this._items);
        }
        return new Promise((resolve, reject) => {
            const disposable = this.itemsEmitter.event((value) => {
                if (check(value)) {
                    disposable.dispose();
                    clearTimeout(timer);
                    resolve(value);
                }
            });
            const timer = setTimeout(() => {
                disposable.dispose();
                reject(new Error("QuickPick items not populated in time"));
            }, timeout);
        });
    }
}
//# sourceMappingURL=extension.test.js.map