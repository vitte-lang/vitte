import * as assert from "node:assert/strict";
import * as fs from "node:fs";
import * as path from "node:path";
import * as vscode from "vscode";

const EXTENSION_ID = "VitteStudio.vitte-studio";

interface ExtensionTestApi {
  apiVersion: "1.0.0";
  extensionVersion: string;
  capabilities: {
    status: true;
    restart: true;
    runAction: true;
    serverResolutionTestHook: true;
  };
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

async function getRegisteredCommands(): Promise<Set<string>> {
  return new Set(await vscode.commands.getCommands(true));
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

  test("API publique versionnée: contrat stable exposé", () => {
    const testApi = api;
    assert.ok(testApi, "API de test non disponible");
    assert.equal(testApi.apiVersion, "1.0.0", "Version de contrat API inattendue");
    assert.equal(typeof testApi.extensionVersion, "string");
    assert.ok(testApi.extensionVersion.length > 0, "La version de l’extension doit être exposée");
    assert.equal(testApi.capabilities.status, true);
    assert.equal(testApi.capabilities.restart, true);
    assert.equal(testApi.capabilities.runAction, true);
    assert.equal(testApi.capabilities.serverResolutionTestHook, true);
  });

  test("Les commandes principales sont déclarées", async () => {
    const commands = await getRegisteredCommands();
    const expected = [
      "vitte.showServerLog",
      "vitte.restartServer",
      "vitte.runAction",
      "vitte.quickActions",
      "vitte.diagnostics.refreshHelpCache",
      "vitte.diagnostics.exportSnapshot",
      "vitte.diagnostics.exportExplainBundle",
      "vitte.diagnostics.openDoc",
      "vitte.diagnostics.goToFirstErrorInFile",
      "vitte.topSyntaxErrors.setCodeFilter",
      "vitte.topSyntaxErrors.clearCodeFilter",
    ];

    for (const cmd of expected) {
      assert.ok(commands.has(cmd), `Commande ${cmd} non enregistrée`);
    }
  });

  test("Smoke E2E: commandes critiques exécutables sans crash", async () => {
    const critical = [
      "vitte.moduleGraph.refresh",
      "vitte.commandCenter.refresh",
      "vitte.packageProblems.refresh",
      "vitte.offline.refresh",
      "vitte.offline.copyReport",
      "vitte.diagnostics.refresh",
      "vitte.topSyntaxErrors.refresh",
      "vitte.debug.runFile",
      "vitte.debug.attachServer",
    ];

    const available = await getRegisteredCommands();
    const runnable = critical.filter((cmd) => available.has(cmd));
    await vscode.commands.executeCommand("workbench.action.closeAllEditors");
    for (const cmd of runnable) {
      await assert.doesNotReject(
        async () => vscode.commands.executeCommand(cmd),
        `La commande ${cmd} ne doit pas faire planter l'extension`,
      );
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
    const tracked = new Set(["vitte.detectToolchain", "vitte.build"]);
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
        ["vitte.detectToolchain", "vitte.build"],
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

  test("Go to first error in file moves cursor to earliest error", async () => {
    const document = await vscode.workspace.openTextDocument({
      content: "line0\nline1\nline2\n",
      language: "vitte",
    });
    const editor = await vscode.window.showTextDocument(document, { preview: false });
    editor.selection = new vscode.Selection(new vscode.Position(2, 0), new vscode.Position(2, 0));

    const testCollection = vscode.languages.createDiagnosticCollection("vitte-test-first-error");
    const warning = new vscode.Diagnostic(
      new vscode.Range(0, 0, 0, 5),
      "warning",
      vscode.DiagnosticSeverity.Warning,
    );
    const firstError = new vscode.Diagnostic(
      new vscode.Range(1, 2, 1, 6),
      "first error",
      vscode.DiagnosticSeverity.Error,
    );
    const laterError = new vscode.Diagnostic(
      new vscode.Range(2, 1, 2, 5),
      "later error",
      vscode.DiagnosticSeverity.Error,
    );
    testCollection.set(document.uri, [laterError, warning, firstError]);

    try {
      await waitUntil(() => {
        const list = vscode.languages.getDiagnostics(document.uri);
        return list.some((d) => d.message === "first error");
      }, 2000, 20);

      await vscode.commands.executeCommand("vitte.diagnostics.goToFirstErrorInFile");
      const pos = editor.selection.active;
      assert.equal(pos.line, 1, "Cursor should jump to earliest error line");
      assert.equal(pos.character, 2, "Cursor should jump to earliest error column");
    } finally {
      testCollection.delete(document.uri);
      testCollection.dispose();
      await vscode.commands.executeCommand("workbench.action.closeActiveEditor");
    }
  });

  test("Diagnostics snapshot export writes timestamped JSON", async () => {
    const workspace = vscode.workspace.workspaceFolders?.[0];
    if (!workspace) return;
    const dir = path.join(workspace.uri.fsPath, ".vitte-cache", "diagnostics");
    const listSnapshotFiles = (): string[] => {
      if (!fs.existsSync(dir)) return [];
      return fs.readdirSync(dir).filter((name) => /^diagnostics-snapshot-\d+\.json$/.test(name));
    };
    const before = new Set(listSnapshotFiles());
    const created: string[] = [];

    try {
      await vscode.commands.executeCommand("vitte.diagnostics.exportSnapshot");
      await waitUntil(() => {
        const now = listSnapshotFiles().filter((name) => !before.has(name));
        if (now.length === 0) return false;
        created.splice(0, created.length, ...now);
        return true;
      }, 4000, 50);

      assert.ok(created.length > 0, "Aucun snapshot diagnostics exporté");
      const latest = created
        .map((name) => ({ name, mtime: fs.statSync(path.join(dir, name)).mtimeMs }))
        .sort((a, b) => b.mtime - a.mtime)[0]?.name;
      assert.ok(latest, "Snapshot diagnostics introuvable après export");
      if (!latest) return;
      const filePath = path.join(dir, latest);
      const raw = fs.readFileSync(filePath, "utf8");
      const payload = JSON.parse(raw) as {
        ts?: unknown;
        workspace?: unknown;
        summary?: { errors?: unknown; warnings?: unknown; info?: unknown; hints?: unknown };
        perFile?: unknown;
        perDirectory?: unknown;
      };
      assert.equal(typeof payload.ts, "string", "Le snapshot doit contenir un timestamp ISO");
      assert.equal(payload.workspace, workspace.uri.fsPath, "Le workspace exporté est incorrect");
      assert.equal(typeof payload.summary?.errors, "number");
      assert.equal(typeof payload.summary?.warnings, "number");
      assert.equal(typeof payload.summary?.info, "number");
      assert.equal(typeof payload.summary?.hints, "number");
      assert.ok(Array.isArray(payload.perFile), "`perFile` doit être un tableau");
      assert.ok(Array.isArray(payload.perDirectory), "`perDirectory` doit être un tableau");
    } finally {
      for (const name of created) {
        const fp = path.join(dir, name);
        if (fs.existsSync(fp)) fs.unlinkSync(fp);
      }
    }
  });

  test("Diagnostics explain bundle export writes timestamped JSON", async () => {
    const workspace = vscode.workspace.workspaceFolders?.[0];
    if (!workspace) return;
    const dir = path.join(workspace.uri.fsPath, ".vitte-cache", "diagnostics");
    const listBundleFiles = (): string[] => {
      if (!fs.existsSync(dir)) return [];
      return fs.readdirSync(dir).filter((name) => /^diagnostics-explain-bundle-\d+\.json$/.test(name));
    };
    const before = new Set(listBundleFiles());
    const created: string[] = [];

    try {
      await vscode.commands.executeCommand("vitte.diagnostics.exportExplainBundle");
      await waitUntil(() => {
        const now = listBundleFiles().filter((name) => !before.has(name));
        if (now.length === 0) return false;
        created.splice(0, created.length, ...now);
        return true;
      }, 4000, 50);

      assert.ok(created.length > 0, "Aucun diagnostics explain bundle exporté");
      const latest = created
        .map((name) => ({ name, mtime: fs.statSync(path.join(dir, name)).mtimeMs }))
        .sort((a, b) => b.mtime - a.mtime)[0]?.name;
      assert.ok(latest, "Diagnostics explain bundle introuvable après export");
      if (!latest) return;
      const filePath = path.join(dir, latest);
      const raw = fs.readFileSync(filePath, "utf8");
      const payload = JSON.parse(raw) as {
        ts?: unknown;
        workspace?: unknown;
        schema?: unknown;
        summary?: { errors?: unknown; warnings?: unknown; info?: unknown; hints?: unknown };
        diagnosticHelp?: { requests?: unknown; explainResolved?: unknown; localFallbackResolved?: unknown };
        perFile?: unknown;
        perDirectory?: unknown;
      };
      assert.equal(typeof payload.ts, "string", "Le bundle doit contenir un timestamp ISO");
      assert.equal(payload.workspace, workspace.uri.fsPath, "Le workspace exporté est incorrect");
      assert.equal(payload.schema, "diagnostics_explain_bundle@1");
      assert.equal(typeof payload.summary?.errors, "number");
      assert.equal(typeof payload.summary?.warnings, "number");
      assert.equal(typeof payload.summary?.info, "number");
      assert.equal(typeof payload.summary?.hints, "number");
      assert.equal(typeof payload.diagnosticHelp?.requests, "number");
      assert.equal(typeof payload.diagnosticHelp?.explainResolved, "number");
      assert.equal(typeof payload.diagnosticHelp?.localFallbackResolved, "number");
      assert.ok(Array.isArray(payload.perFile), "`perFile` doit être un tableau");
      assert.ok(Array.isArray(payload.perDirectory), "`perDirectory` doit être un tableau");
    } finally {
      for (const name of created) {
        const fp = path.join(dir, name);
        if (fs.existsSync(fp)) fs.unlinkSync(fp);
      }
    }
  });

  test("E2E: E0001 diagnostic includes non-empty help", async () => {
    const rendered = await vscode.commands.executeCommand<string>(
      "vitte.test.renderDiagnosticMessage",
      "E0001",
      "expected identifier",
    );
    assert.equal(typeof rendered, "string");
    assert.match(
      rendered ?? "",
      /help:\s*\S+/i,
      `Le diagnostic E0001 n'inclut pas de help non vide. Message: ${rendered ?? ""}`,
    );
  });

  test("E2E: missing vitte binary falls back to local help", async () => {
    const rendered = await vscode.commands.executeCommand<string>(
      "vitte.test.renderDiagnosticMessage",
      "E0001",
      "expected identifier",
      { bin: "__missing_vitte_binary_for_test__" },
    );
    assert.equal(typeof rendered, "string");
    assert.match(
      rendered ?? "",
      /help:\s*expected identifier;/i,
      `Fallback local help absent quand le binaire vitte est introuvable. Message: ${rendered ?? ""}`,
    );
  });

  test("Observability: tracks explain usage vs local fallback", async () => {
    const before = await vscode.commands.executeCommand<{
      requests?: number;
      explainResolved?: number;
      localFallbackResolved?: number;
      localOnlyResolved?: number;
      unresolved?: number;
      explainUsageRate?: number;
      localFallbackRate?: number;
      localOnlyRate?: number;
    }>("vitte.test.getDiagnosticHelpObservability");
    assert.ok(before, "Snapshot observability initial manquant");

    await vscode.commands.executeCommand<string>("vitte.test.renderDiagnosticMessage", "E0001", "expected identifier");
    await vscode.commands.executeCommand<string>(
      "vitte.test.renderDiagnosticMessage",
      "E0001",
      "expected identifier",
      { bin: "__missing_vitte_binary_for_test__" },
    );

    const after = await vscode.commands.executeCommand<{
      requests?: number;
      explainResolved?: number;
      localFallbackResolved?: number;
      localOnlyResolved?: number;
      unresolved?: number;
      explainUsageRate?: number;
      localFallbackRate?: number;
      localOnlyRate?: number;
    }>("vitte.test.getDiagnosticHelpObservability");
    assert.ok(after, "Snapshot observability final manquant");

    const requestDelta = (after.requests ?? 0) - (before.requests ?? 0);
    assert.ok(requestDelta >= 2, `requests delta attendu >= 2, reçu ${requestDelta}`);
    assert.equal(typeof after.explainUsageRate, "number");
    assert.equal(typeof after.localFallbackRate, "number");
    assert.equal(typeof after.localOnlyRate, "number");
  });

  test("Code actions expose syntax fixAll, explain diagnostic, open diagnostics doc, and copy explain command", async () => {
    const document = await vscode.workspace.openTextDocument({
      content: "}\n",
      language: "vitte",
    });
    await vscode.window.showTextDocument(document, { preview: false });
    const line0 = document.lineAt(0);
    const range = new vscode.Range(0, 0, 0, Math.max(1, line0.text.length));

    const testCollection = vscode.languages.createDiagnosticCollection("vitte-test-actions");
    const diagnostic = new vscode.Diagnostic(
      range,
      "Unexpected token at top-level",
      vscode.DiagnosticSeverity.Error,
    );
    diagnostic.code = "PARSE:E0007";
    diagnostic.source = "vitte";
    testCollection.set(document.uri, [diagnostic]);

    try {
      await waitUntil(() => {
        const list = vscode.languages.getDiagnostics(document.uri);
        return list.some((d) => {
          const code = d.code;
          const codeText = typeof code === "string" || typeof code === "number" ? String(code) : "";
          return codeText === "PARSE:E0007" && d.source === "vitte";
        });
      }, 3000, 30);

      let explainAction: vscode.CodeAction | undefined;
      await waitUntil(async () => {
        try {
          const actions =
            (await vscode.commands.executeCommand<(vscode.CodeAction | vscode.Command)[]>(
              "vscode.executeCodeActionProvider",
              document.uri,
              range,
              vscode.CodeActionKind.QuickFix.value,
            )) ?? [];
          const isExplainCodeAction = (a: vscode.CodeAction | vscode.Command): a is vscode.CodeAction =>
            a instanceof vscode.CodeAction
            && a.command?.command === "vitte.diagnostics.explain"
            && (a.title ?? "").toLowerCase().includes("explain");
          explainAction = actions.find(
            isExplainCodeAction,
          );
          return Boolean(explainAction);
        } catch {
          return false;
        }
      }, 4000, 60);
      assert.ok(explainAction, "La code action Explain this diagnostic est absente");

      let openDiagnosticsDocAction: vscode.CodeAction | undefined;
      await waitUntil(async () => {
        try {
          const actions =
            (await vscode.commands.executeCommand<(vscode.CodeAction | vscode.Command)[]>(
              "vscode.executeCodeActionProvider",
              document.uri,
              range,
              vscode.CodeActionKind.QuickFix.value,
            )) ?? [];
          const isOpenDiagnosticsDocCodeAction = (a: vscode.CodeAction | vscode.Command): a is vscode.CodeAction =>
            a instanceof vscode.CodeAction
            && a.command?.command === "vitte.diagnostics.openDoc"
            && (a.title ?? "").toLowerCase().includes("open diagnostics doc");
          openDiagnosticsDocAction = actions.find(
            isOpenDiagnosticsDocCodeAction,
          );
          return Boolean(openDiagnosticsDocAction);
        } catch {
          return false;
        }
      }, 4000, 60);
      assert.ok(openDiagnosticsDocAction, "La code action Open diagnostics doc est absente");

      let copyExplainCommandAction: vscode.CodeAction | undefined;
      await waitUntil(async () => {
        try {
          const actions =
            (await vscode.commands.executeCommand<(vscode.CodeAction | vscode.Command)[]>(
              "vscode.executeCodeActionProvider",
              document.uri,
              range,
              vscode.CodeActionKind.QuickFix.value,
            )) ?? [];
          const isCopyExplainCommandCodeAction = (a: vscode.CodeAction | vscode.Command): a is vscode.CodeAction =>
            a instanceof vscode.CodeAction
            && a.command?.command === "vitte.diagnostics.copyExplainCommand"
            && (a.title ?? "").toLowerCase().includes("copy explain command");
          copyExplainCommandAction = actions.find(
            isCopyExplainCommandCodeAction,
          );
          return Boolean(copyExplainCommandAction);
        } catch {
          return false;
        }
      }, 4000, 60);
      assert.ok(copyExplainCommandAction, "La code action Copy explain command est absente");

      let syntaxFixAllAction: vscode.CodeAction | undefined;
      await waitUntil(async () => {
        try {
          const actions =
            (await vscode.commands.executeCommand<(vscode.CodeAction | vscode.Command)[]>(
              "vscode.executeCodeActionProvider",
              document.uri,
              range,
              vscode.CodeActionKind.SourceFixAll.value,
            )) ?? [];
          const isSyntaxFixAllCodeAction = (a: vscode.CodeAction | vscode.Command): a is vscode.CodeAction =>
            a instanceof vscode.CodeAction
            && a.command?.command === "vitte.syntax.fixAllPreview"
            && (a.kind?.value ?? "").includes("source.fixAll.vitte.syntax");
          syntaxFixAllAction = actions.find(
            isSyntaxFixAllCodeAction,
          );
          return Boolean(syntaxFixAllAction);
        } catch {
          return false;
        }
      }, 4000, 60);
      assert.ok(syntaxFixAllAction, "La code action source.fixAll.vitte.syntax est absente");
    } finally {
      testCollection.delete(document.uri);
      testCollection.dispose();
      await vscode.commands.executeCommand("workbench.action.closeActiveEditor");
    }
  });

  test("Diagnostics commands ignore malformed arguments", async () => {
    const available = await getRegisteredCommands();
    const maybeRun = async (command: string, ...args: unknown[]) => {
      if (!available.has(command)) return;
      await vscode.commands.executeCommand(command, ...args);
    };
    await assert.doesNotReject(async () => {
      await maybeRun("vitte.diagnostics.open", undefined);
      await maybeRun("vitte.diagnostics.copy", undefined);
      await maybeRun("vitte.diagnostics.explain", undefined);
      await maybeRun("vitte.diagnostics.copyExplainCommand", undefined);
      await maybeRun("vitte.diagnostics.openDoc", undefined);
      await maybeRun("vitte.diagnostics.exportExplainBundle");
      await maybeRun("vitte.diagnostics.refreshHelpCache");
      await maybeRun("vitte.diagnostics.open", { uri: "bad" });
      await maybeRun("vitte.diagnostics.copy", { diagnostic: 42 });
      await maybeRun("vitte.diagnostics.explain", { uri: "bad", diagnostic: null });
      await maybeRun("vitte.diagnostics.copyExplainCommand", { uri: "bad", diagnostic: null });
      await maybeRun("vitte.diagnostics.openDoc", { uri: "bad", diagnostic: null });
    });
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
