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
const vscode = __importStar(require("vscode"));
const quickActions_1 = require("../../commands/quickActions");
const originalGetConfiguration = vscode.workspace.getConfiguration;
function stubConfiguration(values) {
    vscode.workspace.getConfiguration =
        ((section) => {
            if (section !== "vitte") {
                return originalGetConfiguration.call(vscode.workspace, section);
            }
            return {
                get: (key) => values[key],
            };
        });
}
function restoreConfiguration() {
    vscode.workspace.getConfiguration =
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
        const defs = quickActions_1.__test.readConfiguredSequences();
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
            const [firstStep] = originalSteps;
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
        const sanitized = quickActions_1.__test.readQuickActionOverrides();
        assert.ok(!sanitized.build?.label, "Empty labels should be ignored");
        assert.ok(!sanitized.build?.command, "Blank commands should be ignored");
        assert.equal(sanitized.run?.command, "custom.run");
        assert.deepEqual(sanitized.run?.args, ["foo", { bar: 1 }]);
        // Ensure args are cloned
        overrides.run.args?.push("mutated");
        assert.deepEqual(sanitized.run?.args, ["foo", { bar: 1 }]);
    });
    test("buildItems orders pinned and recent items deterministically", async () => {
        const ctx = {
            hasBenchConfig: false,
            activeIsTestFile: false,
            workspaceBuilt: false,
            diagnosticsStale: true,
            telemetryEnabled: false,
            usageStats: {},
            buildProfile: "dev",
            buildIncremental: false,
        };
        const defs = [
            { id: "alpha", label: "Alpha", command: "cmd.alpha" },
            { id: "beta", label: "Beta", command: "cmd.beta" },
        ];
        const recentEntries = [
            { command: "cmd.alpha", actionId: "alpha", timestamp: 10 },
            { command: "cmd.beta", actionId: "beta", timestamp: 20 },
        ];
        const items = await quickActions_1.__test.buildItems(defs, ctx, recentEntries, "beta");
        assert.deepEqual(items.map((item) => item.actionId), ["beta", "alpha"], "Pinned action should be first, followed by recents");
        assert.ok(items[0]?.label.startsWith("★ "), "Pinned item label should be prefixed");
        assert.ok(items[1]?.label.startsWith("↺ "), "Recent item label should be prefixed");
        assert.ok(items[0]?.__isPinned, "Pinned flag not set");
        assert.ok(items[1]?.__isRecent, "Recent flag not set");
    });
    test("updateQuickPickItems keeps the selected action after reordering", () => {
        const quickPick = {
            items: [],
            selectedItems: [],
        };
        const items = [
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
        quickPick.selectedItems = [items[1]];
        quickActions_1.__test.updateQuickPickItems(quickPick, [items[1], items[0]]);
        assert.equal(quickPick.selectedItems[0]?.actionId, "two", "Selection should remain on the same action after reorder");
    });
});
//# sourceMappingURL=quickActions.unit.test.js.map