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
const suggestions_1 = require("../../utils/suggestions");
function createItem(label, kind) {
    return new vscode.CompletionItem(label, kind);
}
function baseTuning() {
    return {
        adaptiveRanking: true,
        dynamicBudget: false,
        showExplainLabels: false,
        showMatchHighlights: false,
        rankingVariant: "default",
        churnPenaltyEnabled: true,
        churnPenaltyWeight: 1.0,
        typeBudgetEnabled: false,
        topN: 12,
        minN: 12,
        maxN: 12,
        slowMs: 180,
        intentMode: "code",
        rankingDeadlineMs: 50,
    };
}
async function openDoc(language, content) {
    return vscode.workspace.openTextDocument({ language, content });
}
suite("Suggestions ranking snapshots", () => {
    teardown(() => {
        (0, suggestions_1.resetSuggestionLearningState)();
    });
    test("snapshot: default ranking (vitte, prefix gre)", async () => {
        const doc = await openDoc("vitte", [
            "module demo/app",
            "use math/vector",
            "fn greet_user(name: Text) gives Text {",
            "  let helperScope = 1",
            "  let localCache = 2",
            "  gre",
            "}",
        ].join("\n"));
        const pos = new vscode.Position(5, 5);
        const items = [
            createItem("greet_user", vscode.CompletionItemKind.Function),
            createItem("gret_user", vscode.CompletionItemKind.Function),
            createItem("greatTool", vscode.CompletionItemKind.Method),
            createItem("helperScope", vscode.CompletionItemKind.Variable),
            createItem("vectorLength", vscode.CompletionItemKind.Function),
            createItem("assertEqual", vscode.CompletionItemKind.Function),
            createItem("moduleIndex", vscode.CompletionItemKind.Variable),
            createItem("readmeGuide", vscode.CompletionItemKind.Snippet),
        ];
        const ranked = (0, suggestions_1.rankAndTrimCompletionItems)(items, doc, pos, baseTuning(), []);
        const labels = ranked.map((it) => (typeof it.label === "string" ? it.label : it.label.label));
        assert.deepEqual(labels, [
            "greet_user",
            "gret_user",
            "greatTool",
            "helperScope",
            "moduleIndex",
            "vectorLength",
            "assertEqual",
            "readmeGuide",
        ]);
    });
    test("snapshot: intent modes (test/docs)", async () => {
        const doc = await openDoc("vitte", [
            "module demo/tests",
            "fn suite() gives Bool {",
            "  ",
            "}",
        ].join("\n"));
        const pos = new vscode.Position(2, 2);
        const items = [
            createItem("assertEqual", vscode.CompletionItemKind.Function),
            createItem("mockUser", vscode.CompletionItemKind.Function),
            createItem("fixtureFactory", vscode.CompletionItemKind.Function),
            createItem("readmeGuide", vscode.CompletionItemKind.Snippet),
            createItem("docComment", vscode.CompletionItemKind.Snippet),
            createItem("renameSymbol", vscode.CompletionItemKind.Function),
        ];
        const tuningTest = { ...baseTuning(), intentMode: "test" };
        const testRanked = (0, suggestions_1.rankAndTrimCompletionItems)(items, doc, pos, tuningTest, []);
        const testLabels = testRanked.map((it) => (typeof it.label === "string" ? it.label : it.label.label));
        assert.deepEqual(testLabels, [
            "mockUser",
            "assertEqual",
            "fixtureFactory",
            "renameSymbol",
            "docComment",
            "readmeGuide",
        ]);
        const tuningDocs = { ...baseTuning(), intentMode: "docs" };
        const docsRanked = (0, suggestions_1.rankAndTrimCompletionItems)(items, doc, pos, tuningDocs, []);
        const docsLabels = docsRanked.map((it) => (typeof it.label === "string" ? it.label : it.label.label));
        assert.deepEqual(docsLabels, [
            "mockUser",
            "docComment",
            "readmeGuide",
            "assertEqual",
            "fixtureFactory",
            "renameSymbol",
        ]);
    });
    test("snapshot: ranking variants (default/prefix/scope)", async () => {
        const doc = await openDoc("vitte", [
            "fn compute() gives Text {",
            "  let helperScope = 1",
            "  he",
            "}",
        ].join("\n"));
        const pos = new vscode.Position(2, 4);
        const items = [
            createItem("helperScope", vscode.CompletionItemKind.Variable),
            createItem("helloWorld", vscode.CompletionItemKind.Function),
            createItem("hexDump", vscode.CompletionItemKind.Function),
            createItem("heapStats", vscode.CompletionItemKind.Variable),
            createItem("moduleIndex", vscode.CompletionItemKind.Variable),
        ];
        const labelsDefault = (0, suggestions_1.rankAndTrimCompletionItems)(items, doc, pos, { ...baseTuning(), rankingVariant: "default" }, []).map((it) => (typeof it.label === "string" ? it.label : it.label.label));
        assert.deepEqual(labelsDefault, [
            "helperScope",
            "heapStats",
            "helloWorld",
            "hexDump",
            "moduleIndex",
        ]);
        const labelsPrefix = (0, suggestions_1.rankAndTrimCompletionItems)(items, doc, pos, { ...baseTuning(), rankingVariant: "prefix_heavy" }, []).map((it) => (typeof it.label === "string" ? it.label : it.label.label));
        assert.deepEqual(labelsPrefix, [
            "helperScope",
            "heapStats",
            "helloWorld",
            "hexDump",
            "moduleIndex",
        ]);
        const labelsScope = (0, suggestions_1.rankAndTrimCompletionItems)(items, doc, pos, { ...baseTuning(), rankingVariant: "scope_heavy" }, []).map((it) => (typeof it.label === "string" ? it.label : it.label.label));
        assert.deepEqual(labelsScope, [
            "helperScope",
            "heapStats",
            "helloWorld",
            "hexDump",
            "moduleIndex",
        ]);
    });
});
//# sourceMappingURL=suggestions.ranking.snapshot.test.js.map