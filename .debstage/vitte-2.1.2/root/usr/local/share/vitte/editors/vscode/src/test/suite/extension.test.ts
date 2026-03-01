import * as assert from "node:assert/strict";
import * as fs from "node:fs";
import * as path from "node:path";
import * as vscode from "vscode";

const EXTENSION_ID = "VitteStudio.vitte-studio";

interface ExtensionTestApi {
  getStatusText(): string;
  getStatusTooltip(): string;
  getClientState(): unknown;
  runAction(action: string): Promise<void>;
  resolveServerModuleForTest(ctx: Pick<vscode.ExtensionContext, "asAbsolutePath">): string;
}

async function waitUntil(condition: () => boolean | Promise<boolean>, timeout = 5000, step = 50): Promise<void> {
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
  let extension: vscode.Extension<unknown> | undefined;
  let api: ExtensionTestApi | undefined;

  suiteSetup(async () => {
    extension = vscode.extensions.getExtension(EXTENSION_ID);
    assert.ok(extension, "Extension introuvable dans le registre VS Code");
    await extension.activate();
    assert.ok(extension.isActive, "Extension non active après activation");
    api = extension.exports as ExtensionTestApi;
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
    assert.ok(
      tooltip.includes("opérationnel") || tooltip.includes("running"),
      "Le statut du client n’indique pas qu’il est démarré"
    );
    assert.ok(
      tooltip.includes("diagnostics"),
      "Le statut ne mentionne pas la synthèse des diagnostics"
    );
  });

  test("La commande runAction déclenche l’action sélectionnée", async () => {
    const testApi = api;
    assert.ok(testApi, "API de test non disponible");

    const disposables: vscode.Disposable[] = [];
    let formatCalled = false;

    disposables.push(
      vscode.commands.registerCommand("editor.action.formatDocument", () => {
        formatCalled = true;
      })
    );

    const windowAny = vscode.window as unknown as { showQuickPick: (...args: unknown[]) => Thenable<unknown> };
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
    } finally {
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

    const fakeRoot = path.join(
      ext.extensionPath,
      ".test-missing-server",
      Date.now().toString(36)
    );
    assert.ok(!fs.existsSync(fakeRoot), "Le répertoire factice ne devrait pas exister");
    const fakeContext = {
      asAbsolutePath: (relPath: string) => path.join(fakeRoot, relPath),
    } as Pick<vscode.ExtensionContext, "asAbsolutePath">;

    assert.throws(
      () => testApi.resolveServerModuleForTest(fakeContext),
      /Module serveur Vitte introuvable/
    );
  });

  test("Quick Actions exécutent la séquence d’onboarding", async () => {
    const runOrder: string[] = [];
    const tracked = new Set(["vitte.openDocs", "vitte.detectToolchain", "vitte.build"]);
    const commandsAny = vscode.commands as unknown as {
      executeCommand: (command: string, ...rest: unknown[]) => Thenable<unknown>;
    };
    const originalExecuteCommand = commandsAny.executeCommand.bind(vscode.commands);

    commandsAny.executeCommand = (command: string, ...rest: unknown[]) => {
      if (tracked.has(command)) {
        runOrder.push(command);
        return Promise.resolve(undefined);
      }
      return originalExecuteCommand(command, ...rest);
    };

    const windowAny = vscode.window as unknown as {
      createQuickPick: (options?: vscode.QuickPickOptions) => vscode.QuickPick<vscode.QuickPickItem>;
    };
    const originalCreateQuickPick = windowAny.createQuickPick;
    let createdQuickPick: QuickPickStub<vscode.QuickPickItem> | undefined;

    windowAny.createQuickPick = () => {
      createdQuickPick = new QuickPickStub<vscode.QuickPickItem>();
      return createdQuickPick;
    };

    try {
      const commandPromise = vscode.commands.executeCommand("vitte.quickActions");
      const quickPick = await waitFor(() => createdQuickPick, 1000);
      const items = await quickPick.waitForItems(
        (list) => list.some((item) => (item as QuickActionItemMetadata).actionId === "onboarding.setup"),
      );
      const target = items.find((item) => (item as QuickActionItemMetadata).actionId === "onboarding.setup");
      assert.ok(target, "Action onboarding.setup introuvable");
      quickPick.selectedItems = [target];
      quickPick.fireAccept();
      await commandPromise;

      assert.deepEqual(
        runOrder,
        ["vitte.openDocs", "vitte.detectToolchain", "vitte.build"],
        "La séquence Run setup n’a pas déclenché tous les sous-commandes"
      );
    } finally {
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

    const runOrder: string[] = [];
    const tracked = new Set<string>(["vitte.build", "vitte.test", "vitte.bench"]);
    const commandsAny = vscode.commands as unknown as {
      executeCommand: (command: string, ...rest: unknown[]) => Thenable<unknown>;
    };
    const originalExecuteCommand = commandsAny.executeCommand.bind(vscode.commands);
    commandsAny.executeCommand = (command: string, ...rest: unknown[]) => {
      if (tracked.has(command)) {
        runOrder.push(command);
        return Promise.resolve(undefined);
      }
      return originalExecuteCommand(command, ...rest);
    };

    const windowAny = vscode.window as unknown as {
      createQuickPick: (options?: vscode.QuickPickOptions) => vscode.QuickPick<vscode.QuickPickItem>;
    };
    const originalCreateQuickPick = windowAny.createQuickPick;
    let createdQuickPick: QuickPickStub<vscode.QuickPickItem> | undefined;
    windowAny.createQuickPick = () => {
      createdQuickPick = new QuickPickStub<vscode.QuickPickItem>();
      return createdQuickPick;
    };

    try {
      const commandPromise = vscode.commands.executeCommand("vitte.quickActions");
      const quickPick = await waitFor(() => createdQuickPick, 1000);
      const items = await quickPick.waitForItems(
        (list) => list.some((item) => (item as QuickActionItemMetadata).actionId === "custom.buildTestBench"),
        2000
      );
      const target = items.find((item) => (item as QuickActionItemMetadata).actionId === "custom.buildTestBench");
      assert.ok(target, "Séquence personnalisée introuvable");
      quickPick.selectedItems = [target];
      quickPick.fireAccept();
      await commandPromise;

      assert.deepEqual(
        runOrder,
        ["vitte.build", "vitte.test", "vitte.bench"],
        "La séquence personnalisée n’a pas exécuté toutes les commandes"
      );
    } finally {
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

    const tracked = new Set<string>(["vitte.bench", "vitte.diagnostics.refresh"]);
    const runOrder: string[] = [];

    const commandsAny = vscode.commands as unknown as {
      executeCommand: (command: string, ...rest: unknown[]) => Thenable<unknown>;
    };
    const originalExecuteCommand = commandsAny.executeCommand.bind(vscode.commands);
    commandsAny.executeCommand = (command: string, ...rest: unknown[]) => {
      if (tracked.has(command)) {
        runOrder.push(command);
        return Promise.resolve(undefined);
      }
      return originalExecuteCommand(command, ...rest);
    };

    const windowAny = vscode.window as unknown as {
      createQuickPick: (options?: vscode.QuickPickOptions) => vscode.QuickPick<vscode.QuickPickItem>;
    };
    const originalCreateQuickPick = windowAny.createQuickPick;
    let createdQuickPick: QuickPickStub<vscode.QuickPickItem> | undefined;
    windowAny.createQuickPick = () => {
      createdQuickPick = new QuickPickStub<vscode.QuickPickItem>();
      return createdQuickPick;
    };

    async function runQuickAction(actionId: string): Promise<void> {
      const commandPromise = vscode.commands.executeCommand("vitte.quickActions");
      const quickPick = await waitFor(() => createdQuickPick, 1000);
      const items = await quickPick.waitForItems(
        (list) => list.some((item) => (item as QuickActionItemMetadata).actionId === actionId),
        2000
      );
      const target = items.find((item) => (item as QuickActionItemMetadata).actionId === actionId);
      assert.ok(target, `Action ${actionId} introuvable`);
      quickPick.selectedItems = [target];
      quickPick.fireAccept();
      await commandPromise;
      createdQuickPick = undefined;
    }

    try {
      await runQuickAction("vitte.bench");
      await runQuickAction("vitte.diagnostics.refresh");
      assert.deepEqual(
        runOrder,
        ["vitte.bench", "vitte.diagnostics.refresh"],
        "Les actions bench/diagnostics n'ont pas été exécutées dans l'ordre attendu"
      );
    } finally {
      commandsAny.executeCommand = originalExecuteCommand;
      windowAny.createQuickPick = originalCreateQuickPick;
      if (fs.existsSync(benchConfigPath)) {
        fs.unlinkSync(benchConfigPath);
      }
    }
  });

  test("Quick Actions toggles profile/incremental", async () => {
    const cfg = vscode.workspace.getConfiguration("vitte");
    const originalProfile = cfg.get<string>("build.profile");
    const originalIncremental = cfg.get<boolean>("build.incremental");
    await cfg.update("build.profile", "dev", vscode.ConfigurationTarget.Global);
    await cfg.update("build.incremental", false, vscode.ConfigurationTarget.Global);

    const commandsAny = vscode.commands as unknown as {
      executeCommand: (command: string, ...rest: unknown[]) => Thenable<unknown>;
    };
    const originalExecuteCommand = commandsAny.executeCommand.bind(vscode.commands);
    const tracked: string[] = [];
    commandsAny.executeCommand = (command: string, ...rest: unknown[]) => {
      if (command === "vitte.switchProfile" || command === "vitte.toggleIncremental") {
        tracked.push(command);
        return Promise.resolve(undefined);
      }
      return originalExecuteCommand(command, ...rest);
    };

    const windowAny = vscode.window as unknown as {
      createQuickPick: (options?: vscode.QuickPickOptions) => vscode.QuickPick<vscode.QuickPickItem>;
    };
    const originalCreateQuickPick = windowAny.createQuickPick;
    let activeQuickPick: QuickPickStub<vscode.QuickPickItem> | undefined;
    windowAny.createQuickPick = () => {
      activeQuickPick = new QuickPickStub<vscode.QuickPickItem>();
      return activeQuickPick;
    };

    try {
      const commandPromise = vscode.commands.executeCommand("vitte.quickActions");
      const quickPick = await waitFor(() => activeQuickPick, 1000);
      const items = await quickPick.waitForItems();
      const profileItem = items.find((item) => (item as QuickActionItemMetadata).label?.includes("Profile"));
      const incrementalItem = items.find((item) => (item as QuickActionItemMetadata).label?.includes("Incremental"));
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

      assert.deepEqual(
        tracked,
        ["vitte.switchProfile", "vitte.toggleIncremental"],
        "Toggles were not executed in order"
      );
    } finally {
      windowAny.createQuickPick = originalCreateQuickPick;
      commandsAny.executeCommand = originalExecuteCommand;
      await cfg.update("build.profile", originalProfile, vscode.ConfigurationTarget.Global);
      await cfg.update("build.incremental", originalIncremental, vscode.ConfigurationTarget.Global);
    }
  });

  test("Quick Actions trigger Next Diagnostic", async () => {
    const commandsAny = vscode.commands as unknown as {
      executeCommand: (command: string, ...rest: unknown[]) => Thenable<unknown>;
    };
    const originalExecuteCommand = commandsAny.executeCommand.bind(vscode.commands);
    let markerCalled = false;
    commandsAny.executeCommand = (command: string, ...rest: unknown[]) => {
      if (command === "editor.action.marker.next") {
        markerCalled = true;
        return Promise.resolve(undefined);
      }
      return originalExecuteCommand(command, ...rest);
    };

    const windowAny = vscode.window as unknown as {
      createQuickPick: (options?: vscode.QuickPickOptions) => vscode.QuickPick<vscode.QuickPickItem>;
    };
    const originalCreateQuickPick = windowAny.createQuickPick;
    let activeQuickPick: QuickPickStub<vscode.QuickPickItem> | undefined;
    windowAny.createQuickPick = () => {
      activeQuickPick = new QuickPickStub<vscode.QuickPickItem>();
      return activeQuickPick;
    };

    try {
      const commandPromise = vscode.commands.executeCommand("vitte.quickActions");
      const quickPick = await waitFor(() => activeQuickPick, 1000);
      const items = await quickPick.waitForItems();
      const diagItem = items.find((item) => (item as QuickActionItemMetadata).label?.includes("Next issue"));
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
    } finally {
      commandsAny.executeCommand = originalExecuteCommand;
      windowAny.createQuickPick = originalCreateQuickPick;
    }
  });
});

interface QuickActionItemMetadata extends vscode.QuickPickItem {
  readonly actionId?: string;
}

async function waitFor<T>(
  resolver: () => T | undefined,
  timeout = 1000,
  step = 10
): Promise<T> {
  const start = Date.now();
  while (Date.now() - start < timeout) {
    const value = resolver();
    if (value !== undefined) return value;
    await new Promise((resolve) => setTimeout(resolve, step));
  }
  throw new Error("Timed out waiting for value");
}

class QuickPickStub<T extends vscode.QuickPickItem> implements vscode.QuickPick<T> {
  private readonly acceptEmitter = new vscode.EventEmitter<void>();
  private readonly hideEmitter = new vscode.EventEmitter<void>();
  private readonly selectionEmitter = new vscode.EventEmitter<readonly T[]>();
  private readonly valueEmitter = new vscode.EventEmitter<string>();
  private readonly activeEmitter = new vscode.EventEmitter<readonly T[]>();
  private readonly buttonEmitter = new vscode.EventEmitter<vscode.QuickInputButton>();
  private readonly itemButtonEmitter = new vscode.EventEmitter<vscode.QuickPickItemButtonEvent<T>>();
  private readonly itemsEmitter = new vscode.EventEmitter<readonly T[]>();

  private _items: readonly T[] = [];
  private _selectedItems: readonly T[] = [];
  private _buttons: readonly vscode.QuickInputButton[] = [];
  private _activeItems: readonly T[] = [];

  public title: string | undefined;
  public step: number | undefined;
  public totalSteps: number | undefined;
  public enabled = true;
  public busy = false;
  public ignoreFocusOut = true;
  public canSelectMany = false;
  public matchOnDescription = true;
  public matchOnDetail = false;
  public keepScrollPosition = false;
  public placeholder: string | undefined;
  public value = "";
  public sortByLabel = true;

  public get items(): readonly T[] {
    return this._items;
  }
  public set items(value: readonly T[]) {
    this._items = value;
    this.itemsEmitter.fire(value);
  }

  public get buttons(): readonly vscode.QuickInputButton[] {
    return this._buttons;
  }
  public set buttons(value: readonly vscode.QuickInputButton[]) {
    this._buttons = value;
  }

  public get selectedItems(): readonly T[] {
    return this._selectedItems;
  }
  public set selectedItems(value: readonly T[]) {
    this._selectedItems = value;
    this.selectionEmitter.fire(value);
  }

  public get activeItems(): readonly T[] {
    return this._activeItems;
  }
  public set activeItems(value: readonly T[]) {
    this._activeItems = value;
    this.activeEmitter.fire(value);
  }

  public onDidAccept = this.acceptEmitter.event;
  public onDidHide = this.hideEmitter.event;
  public onDidChangeSelection = this.selectionEmitter.event;
  public onDidChangeValue = this.valueEmitter.event;
  public onDidChangeActive = this.activeEmitter.event;
  public onDidTriggerButton = this.buttonEmitter.event;
  public onDidTriggerItemButton = this.itemButtonEmitter.event;

  show(): void {
    /* no-op */
  }

  hide(): void {
    this.hideEmitter.fire();
  }

  dispose(): void {
    this.acceptEmitter.dispose();
    this.hideEmitter.dispose();
    this.selectionEmitter.dispose();
    this.valueEmitter.dispose();
    this.activeEmitter.dispose();
    this.buttonEmitter.dispose();
    this.itemButtonEmitter.dispose();
    this.itemsEmitter.dispose();
  }

  fireAccept(): void {
    this.acceptEmitter.fire();
  }

  waitForItems(predicate?: (items: readonly T[]) => boolean, timeout = 2000): Promise<readonly T[]> {
    const check = predicate ?? ((list: readonly T[]) => list.length > 0);
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
