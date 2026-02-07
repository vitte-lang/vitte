import * as assert from "node:assert/strict";
import * as vscode from "vscode";
import { __test } from "../../commands/quickActions";
import type { QuickActionDefinition, QuickActionsContext, QuickPickWithMeta } from "../../commands/quickActions";

const originalGetConfiguration = vscode.workspace.getConfiguration;

function stubConfiguration(values: Record<string, unknown>): void {
  (vscode.workspace as unknown as { getConfiguration: typeof vscode.workspace.getConfiguration }).getConfiguration =
    ((section: string) => {
      if (section !== "vitte") {
        return originalGetConfiguration.call(vscode.workspace, section);
      }
      return {
        get: <T>(key: string) => values[key] as T,
      } as vscode.WorkspaceConfiguration;
    }) as typeof vscode.workspace.getConfiguration;
}

function restoreConfiguration(): void {
  (vscode.workspace as unknown as { getConfiguration: typeof vscode.workspace.getConfiguration }).getConfiguration =
    originalGetConfiguration;
}

suite("Quick Actions (unit)", () => {
  teardown(() => restoreConfiguration());

  test("readConfiguredSequences filters invalid entries and clones args", () => {
    const sequences = [
      { label: "   ", steps: [{ command: "vitte.clean" }] },
      { label: "Missing command", steps: [{ command: "" }] },
      {
        label: "Valid flow",
        steps: [
          { command: "first.cmd", args: ["a", { x: 1 }] },
          { command: "second.cmd", args: ["b"] },
        ],
      },
    ];
    stubConfiguration({ "quickActions.sequences": sequences });

    const defs = __test.readConfiguredSequences();
    assert.equal(defs.length, 1, "Only the valid sequence should be kept");
    const [def] = defs;
    assert.ok(def, "Definition missing");
    assert.equal(def.label, "Valid flow");
    assert.ok(def.actions, "Actions missing");
    const actions = def.actions ?? [];
    assert.equal(actions.length, 2);
    assert.deepEqual(actions[0]?.args, ["a", { x: 1 }]);

    // Mutating the original array should not leak into stored args
    const originalSteps = sequences[2]?.steps;
    if (Array.isArray(originalSteps)) {
      const [firstStep] = originalSteps as { args?: unknown[] }[];
      if (Array.isArray(firstStep?.args)) {
        firstStep.args.push("extra");
      }
    }
    assert.deepEqual(actions[0]?.args, ["a", { x: 1 }]);
  });

  test("readQuickActionOverrides trims invalid fields and clones args", () => {
    const overrides = {
      build: { label: "   ", command: "   " },
      run: { command: " custom.run ", args: ["foo", { bar: 1 }] },
    };
    stubConfiguration({ "quickActions.defaults": overrides });

    const sanitized = __test.readQuickActionOverrides();
    assert.ok(!sanitized.build?.label, "Empty labels should be ignored");
    assert.ok(!sanitized.build?.command, "Blank commands should be ignored");
    assert.equal(sanitized.run?.command, "custom.run");
    assert.deepEqual(sanitized.run?.args, ["foo", { bar: 1 }]);

    // Ensure args are cloned
    overrides.run.args?.push("mutated");
    assert.deepEqual(sanitized.run?.args, ["foo", { bar: 1 }]);
  });

  test("buildItems orders pinned and recent items deterministically", async () => {
    const ctx: QuickActionsContext = {
      hasBenchConfig: false,
      activeIsTestFile: false,
      workspaceBuilt: false,
      diagnosticsStale: true,
      telemetryEnabled: false,
      usageStats: {},
      buildProfile: "dev",
      buildIncremental: false,
    };
    const defs: QuickActionDefinition[] = [
      { id: "alpha", label: "Alpha", command: "cmd.alpha" },
      { id: "beta", label: "Beta", command: "cmd.beta" },
    ];
    const recentEntries = [
      { command: "cmd.alpha", actionId: "alpha", timestamp: 10 },
      { command: "cmd.beta", actionId: "beta", timestamp: 20 },
    ];

    const items = await __test.buildItems(defs, ctx, recentEntries, "beta");
    assert.deepEqual(
      items.map((item) => item.actionId),
      ["beta", "alpha"],
      "Pinned action should be first, followed by recents"
    );
    assert.ok(items[0]?.label.startsWith("★ "), "Pinned item label should be prefixed");
    assert.ok(items[1]?.label.startsWith("↺ "), "Recent item label should be prefixed");
    assert.ok(items[0]?.__isPinned, "Pinned flag not set");
    assert.ok(items[1]?.__isRecent, "Recent flag not set");
  });

  test("updateQuickPickItems keeps the selected action after reordering", () => {
    const quickPick = {
      items: [] as readonly QuickPickWithMeta[],
      selectedItems: [] as readonly QuickPickWithMeta[],
    } as unknown as vscode.QuickPick<QuickPickWithMeta>;

    const items: QuickPickWithMeta[] = [
      {
        label: "First",
        command: "cmd.one",
        actionId: "one",
        steps: [],
        baseCommand: "cmd.one",
        __isPinned: false,
        __isRecent: false,
        __isSuggested: false,
      },
      {
        label: "Second",
        command: "cmd.two",
        actionId: "two",
        steps: [],
        baseCommand: "cmd.two",
        __isPinned: false,
        __isRecent: false,
        __isSuggested: false,
      },
    ];

    quickPick.selectedItems = [items[1]!];
    __test.updateQuickPickItems(quickPick, [items[1]!, items[0]!]);
    assert.equal(
      quickPick.selectedItems[0]?.actionId,
      "two",
      "Selection should remain on the same action after reorder"
    );
  });
});
