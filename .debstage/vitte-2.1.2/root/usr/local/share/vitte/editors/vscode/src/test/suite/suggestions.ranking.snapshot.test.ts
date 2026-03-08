import * as assert from "node:assert/strict";
import * as vscode from "vscode";
import {
  rankAndTrimCompletionItems,
  resetSuggestionLearningState,
  type SuggestionTuning,
} from "../../utils/suggestions";

function createItem(label: string, kind: vscode.CompletionItemKind): vscode.CompletionItem {
  return new vscode.CompletionItem(label, kind);
}

function baseTuning(): SuggestionTuning {
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

async function openDoc(language: string, content: string): Promise<vscode.TextDocument> {
  return vscode.workspace.openTextDocument({ language, content });
}

suite("Suggestions ranking snapshots", () => {
  teardown(() => {
    resetSuggestionLearningState();
  });

  test("snapshot: default ranking (vitte, prefix gre)", async () => {
    const doc = await openDoc(
      "vitte",
      [
        "module demo/app",
        "use math/vector",
        "fn greet_user(name: Text) gives Text {",
        "  let helperScope = 1",
        "  let localCache = 2",
        "  gre",
        "}",
      ].join("\n"),
    );
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

    const ranked = rankAndTrimCompletionItems(items, doc, pos, baseTuning(), []);
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
    const doc = await openDoc(
      "vitte",
      [
        "module demo/tests",
        "fn suite() gives Bool {",
        "  ",
        "}",
      ].join("\n"),
    );
    const pos = new vscode.Position(2, 2);
    const items = [
      createItem("assertEqual", vscode.CompletionItemKind.Function),
      createItem("mockUser", vscode.CompletionItemKind.Function),
      createItem("fixtureFactory", vscode.CompletionItemKind.Function),
      createItem("readmeGuide", vscode.CompletionItemKind.Snippet),
      createItem("docComment", vscode.CompletionItemKind.Snippet),
      createItem("renameSymbol", vscode.CompletionItemKind.Function),
    ];

    const tuningTest = { ...baseTuning(), intentMode: "test" as const };
    const testRanked = rankAndTrimCompletionItems(items, doc, pos, tuningTest, []);
    const testLabels = testRanked.map((it) => (typeof it.label === "string" ? it.label : it.label.label));
    assert.deepEqual(testLabels, [
      "mockUser",
      "assertEqual",
      "fixtureFactory",
      "renameSymbol",
      "docComment",
      "readmeGuide",
    ]);

    const tuningDocs = { ...baseTuning(), intentMode: "docs" as const };
    const docsRanked = rankAndTrimCompletionItems(items, doc, pos, tuningDocs, []);
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
    const doc = await openDoc(
      "vitte",
      [
        "fn compute() gives Text {",
        "  let helperScope = 1",
        "  he",
        "}",
      ].join("\n"),
    );
    const pos = new vscode.Position(2, 4);
    const items = [
      createItem("helperScope", vscode.CompletionItemKind.Variable),
      createItem("helloWorld", vscode.CompletionItemKind.Function),
      createItem("hexDump", vscode.CompletionItemKind.Function),
      createItem("heapStats", vscode.CompletionItemKind.Variable),
      createItem("moduleIndex", vscode.CompletionItemKind.Variable),
    ];

    const labelsDefault = rankAndTrimCompletionItems(
      items,
      doc,
      pos,
      { ...baseTuning(), rankingVariant: "default" },
      [],
    ).map((it) => (typeof it.label === "string" ? it.label : it.label.label));
    assert.deepEqual(labelsDefault, [
      "helperScope",
      "heapStats",
      "helloWorld",
      "hexDump",
      "moduleIndex",
    ]);

    const labelsPrefix = rankAndTrimCompletionItems(
      items,
      doc,
      pos,
      { ...baseTuning(), rankingVariant: "prefix_heavy" },
      [],
    ).map((it) => (typeof it.label === "string" ? it.label : it.label.label));
    assert.deepEqual(labelsPrefix, [
      "helperScope",
      "heapStats",
      "helloWorld",
      "hexDump",
      "moduleIndex",
    ]);

    const labelsScope = rankAndTrimCompletionItems(
      items,
      doc,
      pos,
      { ...baseTuning(), rankingVariant: "scope_heavy" },
      [],
    ).map((it) => (typeof it.label === "string" ? it.label : it.label.label));
    assert.deepEqual(labelsScope, [
      "helperScope",
      "heapStats",
      "helloWorld",
      "hexDump",
      "moduleIndex",
    ]);
  });
});
