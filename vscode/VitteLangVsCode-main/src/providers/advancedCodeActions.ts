import * as vscode from "vscode";

type FixCategory = "imports" | "style" | "naming" | "contracts";
const SYNTAX_CODES = new Set(["E0001", "E0002", "E0003", "E0004", "E0005", "E0006", "E0007"]);
const TOP_LEVEL_KEYWORDS = [
  "space", "pull", "use", "share", "const", "let", "make",
  "type", "macro", "form", "trait", "pick", "proc", "entry",
];
const TOP_LEVEL_STATEMENT_KEYWORDS = new Set([
  "if", "loop", "for", "set", "give", "emit", "select",
  "match", "when", "return", "break", "continue", "unsafe", "asm",
]);
const syntaxAutoFixInFlight = new Set<string>();

function editDistance(a: string, b: string): number {
  const m = a.length;
  const n = b.length;
  if (m === 0) return n;
  if (n === 0) return m;
  const dp: number[][] = Array.from({ length: m + 1 }, () => Array<number>(n + 1).fill(0));
  for (let i = 0; i <= m; i += 1) dp[i]![0] = i;
  for (let j = 0; j <= n; j += 1) dp[0]![j] = j;
  for (let i = 1; i <= m; i += 1) {
    for (let j = 1; j <= n; j += 1) {
      const cost = a[i - 1] === b[j - 1] ? 0 : 1;
      dp[i]![j] = Math.min(
        dp[i - 1]![j]! + 1,
        dp[i]![j - 1]! + 1,
        dp[i - 1]![j - 1]! + cost,
      );
    }
  }
  return dp[m]![n]!;
}

function bestTopLevelKeyword(word: string): string | undefined {
  const normalized = word.toLowerCase();
  if (TOP_LEVEL_KEYWORDS.includes(normalized)) return undefined;
  let best: string | undefined;
  let bestScore = Number.POSITIVE_INFINITY;
  for (const kw of TOP_LEVEL_KEYWORDS) {
    const score = editDistance(normalized, kw);
    if (score < bestScore) {
      bestScore = score;
      best = kw;
    }
  }
  return bestScore <= 2 ? best : undefined;
}

function syntaxCodeOf(d: vscode.Diagnostic): string | undefined {
  const raw = d.code;
  let code = "";
  if (typeof raw === "string" || typeof raw === "number") {
    code = String(raw).trim().toUpperCase();
  } else if (raw && typeof raw === "object" && "value" in raw) {
    const value = (raw as { value?: unknown }).value;
    if (typeof value === "string" || typeof value === "number") {
      code = String(value).trim().toUpperCase();
    }
  }
  const prefixed = /^([A-Z][A-Z0-9_-]*):(.*)$/.exec(code);
  const base = (prefixed?.[2] ?? code).trim().toUpperCase();
  return SYNTAX_CODES.has(base) ? base : undefined;
}

function diagnosticCodeText(d: vscode.Diagnostic): string {
  const raw = d.code;
  if (typeof raw === "string" || typeof raw === "number") return String(raw);
  if (raw && typeof raw === "object" && "value" in raw) {
    const value = (raw as { value?: unknown }).value;
    if (typeof value === "string" || typeof value === "number") return String(value);
  }
  return "";
}

function explainableDiagnosticCode(d: vscode.Diagnostic): string | undefined {
  const code = diagnosticCodeText(d).trim().toUpperCase();
  if (!code) return undefined;
  const prefixed = /^([A-Z][A-Z0-9_-]*):(.*)$/.exec(code);
  const base = (prefixed?.[2] ?? code).trim().toUpperCase();
  if (/^E\d{4}$/.test(base) || /^VITTE-[A-Z]\d{4}$/.test(base)) return base;
  return undefined;
}

function buildSyntaxQuickFixes(document: vscode.TextDocument, d: vscode.Diagnostic): vscode.CodeAction[] {
  const actions: vscode.CodeAction[] = [];
  const code = syntaxCodeOf(d);
  if (!code) return actions;
  const lineNo = d.range.start.line;
  if (lineNo < 0 || lineNo >= document.lineCount) return actions;
  const line = document.lineAt(lineNo);
  const text = line.text;
  const trimmed = text.trim();
  const message = d.message ?? "";

  // Safe fix: remove a stray closing brace at top level.
  if (code === "E0007" && trimmed === "}") {
    const a = new vscode.CodeAction("Vitte: Remove stray `}`", vscode.CodeActionKind.QuickFix);
    a.diagnostics = [d];
    a.isPreferred = true;
    a.edit = new vscode.WorkspaceEdit();
    a.edit.replace(document.uri, line.rangeIncludingLineBreak, "");
    actions.push(a);
  }

  const insertExpectedToken = (token: string, title: string) => {
    const a = new vscode.CodeAction(title, vscode.CodeActionKind.QuickFix);
    a.diagnostics = [d];
    a.edit = new vscode.WorkspaceEdit();
    a.edit.insert(document.uri, d.range.end, token);
    actions.push(a);
  };

  if (/expected\s*['"]\)['"]/i.test(message) || /attendu.*\)/i.test(message)) {
    insertExpectedToken(")", "Vitte: Insert missing `)`");
  }
  if (/expected\s*['"]\]['"]/i.test(message) || /attendu.*\]/i.test(message)) {
    insertExpectedToken("]", "Vitte: Insert missing `]`");
  }
  if (/expected\s*['"]\}['"]/i.test(message) || /attendu.*\}/i.test(message)) {
    insertExpectedToken("}", "Vitte: Insert missing `}`");
  }
  if (code === "E0005") {
    const a = new vscode.CodeAction("Vitte: Insert missing `.end`", vscode.CodeActionKind.QuickFix);
    a.diagnostics = [d];
    a.edit = new vscode.WorkspaceEdit();
    a.edit.insert(document.uri, new vscode.Position(lineNo, line.text.length), "\n.end");
    actions.push(a);
  }

  // Safe-ish fix: keyword typo correction for top-level declaration.
  if (code === "E0007") {
    const m = /^\s*([A-Za-z_]\w*)/.exec(text);
    const word = m?.[1];
    if (word) {
      const replacement = bestTopLevelKeyword(word);
      if (replacement) {
        const start = new vscode.Position(lineNo, m?.index ?? 0);
        const end = start.translate(0, word.length);
        const a = new vscode.CodeAction(
          `Vitte: Replace \`${word}\` with \`${replacement}\``,
          vscode.CodeActionKind.QuickFix,
        );
        a.diagnostics = [d];
        a.edit = new vscode.WorkspaceEdit();
        a.edit.replace(document.uri, new vscode.Range(start, end), replacement);
        actions.push(a);
      }

      if (TOP_LEVEL_STATEMENT_KEYWORDS.has(word.toLowerCase())) {
        const a = new vscode.CodeAction("Vitte: Wrap statement in `entry main` block", vscode.CodeActionKind.QuickFix);
        a.diagnostics = [d];
        a.edit = new vscode.WorkspaceEdit();
        const wrapped = [
          "entry main at app/main {",
          `  ${trimmed}`,
          "}",
        ].join("\n");
        a.edit.replace(document.uri, line.range, wrapped);
        actions.push(a);
        if (word.toLowerCase() === "return") {
          const b = new vscode.CodeAction("Vitte: Replace top-level `return` by `give` in `entry main`", vscode.CodeActionKind.QuickFix);
          b.diagnostics = [d];
          b.edit = new vscode.WorkspaceEdit();
          const replaced = trimmed.replace(/^return\b/i, "give");
          const wrappedReturn = [
            "entry main at app/main {",
            `  ${replaced}`,
            "}",
          ].join("\n");
          b.edit.replace(document.uri, line.range, wrappedReturn);
          actions.push(b);
        }
      }
    }
  }

  if (text.includes("\t")) {
    const a = new vscode.CodeAction("Vitte: Convert tabs to spaces (line)", vscode.CodeActionKind.QuickFix);
    a.diagnostics = [d];
    a.edit = new vscode.WorkspaceEdit();
    a.edit.replace(document.uri, line.range, text.replace(/\t/g, "  "));
    actions.push(a);

    const b = new vscode.CodeAction("Vitte: Convert tabs to spaces (file)", vscode.CodeActionKind.QuickFix);
    b.diagnostics = [d];
    b.edit = new vscode.WorkspaceEdit();
    b.edit.replace(
      document.uri,
      new vscode.Range(0, 0, document.lineCount, 0),
      document.getText().replace(/\t/g, "  "),
    );
    actions.push(b);
  }

  if (/[ \t]+$/.test(text)) {
    const a = new vscode.CodeAction("Vitte: Trim trailing spaces (line)", vscode.CodeActionKind.QuickFix);
    a.diagnostics = [d];
    a.edit = new vscode.WorkspaceEdit();
    a.edit.replace(document.uri, line.range, text.replace(/[ \t]+$/g, ""));
    actions.push(a);

    const b = new vscode.CodeAction("Vitte: Trim trailing spaces (file)", vscode.CodeActionKind.QuickFix);
    b.diagnostics = [d];
    b.edit = new vscode.WorkspaceEdit();
    b.edit.replace(
      document.uri,
      new vscode.Range(0, 0, document.lineCount, 0),
      document.getText().replace(/[ \t]+$/gm, ""),
    );
    actions.push(b);
  }

  // Broad safe cleanup inspired by common language-server quick fixes.
  const normalize = new vscode.CodeAction(
    "Vitte: Normalize indentation and trailing spaces",
    vscode.CodeActionKind.QuickFix,
  );
  normalize.diagnostics = [d];
  normalize.edit = new vscode.WorkspaceEdit();
  normalize.edit.replace(
    document.uri,
    new vscode.Range(0, 0, document.lineCount, 0),
    transformStyle(document.getText()),
  );
  actions.push(normalize);

  return actions;
}

function isVitteLikeDocument(doc: vscode.TextDocument): boolean {
  return doc.languageId === "vitte" || doc.languageId === "vit";
}

async function applySafeSyntaxFixes(document: vscode.TextDocument): Promise<boolean> {
  if (!isVitteLikeDocument(document)) return false;
  const cfg = vscode.workspace.getConfiguration("vitte");
  const normalizeIndentation = cfg.get<boolean>("syntax.autoFixIndentation", true);
  const transformed = computeSafeSyntaxText(document, normalizeIndentation);
  if (!transformed) return false;
  if (transformed.after === transformed.before) return false;

  const edit = new vscode.WorkspaceEdit();
  edit.replace(document.uri, new vscode.Range(0, 0, document.lineCount, 0), transformed.after);
  const ok = await vscode.workspace.applyEdit(edit);
  if (!ok) return false;
  await document.save();
  return true;
}

function computeSafeSyntaxText(document: vscode.TextDocument, normalizeIndentation: boolean): { before: string; after: string } | undefined {
  if (!isVitteLikeDocument(document)) return undefined;
  const diagnostics = vscode.languages.getDiagnostics(document.uri);
  const syntaxDiagnostics = diagnostics.filter((d) => syntaxCodeOf(d) !== undefined);
  if (syntaxDiagnostics.length === 0) return undefined;

  const before = document.getText().replace(/\r\n/g, "\n");
  const hadFinalNewline = before.endsWith("\n");
  const lines = before.split("\n");
  const removedLine = new Set<number>();

  for (const d of syntaxDiagnostics) {
    const code = syntaxCodeOf(d);
    if (code !== "E0007") continue;
    const lineNo = d.range.start.line;
    if (lineNo < 0 || lineNo >= lines.length) continue;
    const text = lines[lineNo] ?? "";
    const trimmed = text.trim();
    if (trimmed === "}") {
      removedLine.add(lineNo);
      continue;
    }
    const m = /^(\s*)([A-Za-z_]\w*)/.exec(text);
    const word = m?.[2];
    if (!word) continue;
    const replacement = bestTopLevelKeyword(word);
    if (!replacement || replacement === word.toLowerCase()) continue;
    const prefix = m?.[1] ?? "";
    lines[lineNo] = `${prefix}${replacement}${text.slice((m?.[0] ?? "").length)}`;
  }

  let after = lines.filter((_, idx) => !removedLine.has(idx)).join("\n");
  if (hadFinalNewline && !after.endsWith("\n")) after += "\n";
  if (normalizeIndentation) after = transformStyle(after);
  return { before, after };
}

async function previewAndApplySafeSyntaxFixes(doc: vscode.TextDocument): Promise<void> {
  if (!isVitteLikeDocument(doc)) return;
  const cfg = vscode.workspace.getConfiguration("vitte");
  const normalizeIndentation = cfg.get<boolean>("syntax.autoFixIndentation", true);
  const transformed = computeSafeSyntaxText(doc, normalizeIndentation);
  if (!transformed || transformed.after === transformed.before) {
    void vscode.window.showInformationMessage("Vitte syntax: no safe fixes available.");
    return;
  }
  const tmp = await vscode.workspace.openTextDocument({ content: transformed.after, language: doc.languageId });
  await vscode.commands.executeCommand(
    "vscode.diff",
    doc.uri,
    tmp.uri,
    `Vitte syntax preview: ${vscode.workspace.asRelativePath(doc.uri, false)}`
  );
  const pick = await vscode.window.showInformationMessage("Apply Vitte syntax safe fixes?", "Apply", "Cancel");
  if (pick !== "Apply") return;
  const edit = new vscode.WorkspaceEdit();
  edit.replace(doc.uri, new vscode.Range(0, 0, doc.lineCount, 0), transformed.after);
  await vscode.workspace.applyEdit(edit);
  await doc.save();
}

function explainTitle(message: string): string {
  const didYouMean = /did you mean\s+[`'"]?([^`'".,;]+)[`'"]?/i.exec(message);
  if (!didYouMean?.[1]) return "Vitte: Explain this diagnostic";
  return `Vitte: Explain diagnostic (did you mean \`${didYouMean[1]}\`?)`;
}

function toSnakeCase(name: string): string {
  return name
    .replace(/([a-z0-9])([A-Z])/g, "$1_$2")
    .replace(/[-\s]+/g, "_")
    .replace(/__+/g, "_")
    .toLowerCase();
}

function transformImports(text: string): string {
  const lines = text.replace(/\r\n/g, "\n").split("\n");
  const importRows: string[] = [];
  const kept: string[] = [];
  for (const line of lines) {
    if (/^\s*(import|use|pull)\b/.test(line)) importRows.push(line.trim());
    else kept.push(line);
  }
  if (importRows.length === 0) return text;
  const sorted = Array.from(new Set(importRows)).sort((a, b) => a.localeCompare(b));
  const newText = [...sorted, "", ...kept].join("\n").replace(/\n{3,}/g, "\n\n");
  return text.endsWith("\n") && !newText.endsWith("\n") ? `${newText}\n` : newText;
}

function transformStyle(text: string): string {
  return text
    .replace(/\t/g, "  ")
    .split(/\r?\n/)
    .map((l) => l.replace(/[ \t]+$/g, ""))
    .join("\n");
}

function transformNaming(text: string): string {
  const renames = new Map<string, string>();
  const declRx = /\b(?:proc|fn|let|const|static)\s+(?:mut\s+)?([A-Za-z_]\w*)/g;
  let m: RegExpExecArray | null;
  while ((m = declRx.exec(text))) {
    const oldName = m[1];
    if (!oldName) continue;
    const snake = toSnakeCase(oldName);
    if (oldName !== snake && /^[A-Za-z_]\w*$/.test(snake)) {
      renames.set(oldName, snake);
    }
    if (m[0].length === 0) declRx.lastIndex++;
  }
  let out = text;
  for (const [oldName, newName] of renames) {
    out = out.replace(new RegExp(`(?<![A-Za-z0-9_])${oldName}(?![A-Za-z0-9_])`, "g"), newName);
  }
  return out;
}

function transformContracts(text: string): string {
  if (/^\s*<<<\s+ROLE-CONTRACT\b/m.test(text)) return text;
  const pkg = /^\s*(?:space|module)\s+([A-Za-z_][\w./:-]*)/m.exec(text)?.[1] ?? "my/package";
  const block = [
    "",
    "<<< ROLE-CONTRACT",
    `package: ${pkg}`,
    "role: Responsibility",
    "input_contract: Explicit normalized inputs",
    "output_contract: Stable explicit outputs",
    "boundary: No business policy decisions",
    ">>>",
    "",
  ].join("\n");
  return text.endsWith("\n") ? `${text}${block}` : `${text}\n${block}`;
}

function applyCategory(category: FixCategory, text: string): string {
  switch (category) {
    case "imports": return transformImports(text);
    case "style": return transformStyle(text);
    case "naming": return transformNaming(text);
    case "contracts": return transformContracts(text);
  }
}

async function previewAndApplyCategory(category: FixCategory, doc: vscode.TextDocument): Promise<void> {
  const before = doc.getText();
  const after = applyCategory(category, before);
  if (after === before) {
    void vscode.window.showInformationMessage(`Vitte ${category}: no changes.`);
    return;
  }
  const tmp = await vscode.workspace.openTextDocument({ content: after, language: doc.languageId });
  await vscode.commands.executeCommand(
    "vscode.diff",
    doc.uri,
    tmp.uri,
    `Vitte ${category} preview: ${vscode.workspace.asRelativePath(doc.uri, false)}`
  );
  const pick = await vscode.window.showInformationMessage(`Apply Vitte ${category} fixes?`, "Apply", "Cancel");
  if (pick !== "Apply") return;
  const edit = new vscode.WorkspaceEdit();
  const full = new vscode.Range(0, 0, doc.lineCount, 0);
  edit.replace(doc.uri, full, after);
  await vscode.workspace.applyEdit(edit);
  await doc.save();
}

function categoryTitle(category: FixCategory): string {
  switch (category) {
    case "imports": return "imports";
    case "style": return "style";
    case "naming": return "naming";
    case "contracts": return "contracts";
  }
}

export function registerAdvancedCodeActions(context: vscode.ExtensionContext): void {
  const provider: vscode.CodeActionProvider = {
    provideCodeActions(document, _range, ctx) {
      if (!["vitte", "vit"].includes(document.languageId)) return [];
      const actions: vscode.CodeAction[] = [];
      const addCategory = (category: FixCategory, kind: vscode.CodeActionKind) => {
        const action = new vscode.CodeAction(
          `Vitte: Fix all ${categoryTitle(category)} (preview diff)`,
          kind
        );
        action.command = {
          command: "vitte.fixCategoryPreview",
          title: "Vitte fix category preview",
          arguments: [category, document.uri],
        };
        action.isPreferred = category === "imports";
        actions.push(action);
      };

      addCategory("imports", vscode.CodeActionKind.SourceFixAll.append("vitte.imports"));
      addCategory("style", vscode.CodeActionKind.SourceFixAll.append("vitte.style"));
      addCategory("naming", vscode.CodeActionKind.SourceFixAll.append("vitte.naming"));
      addCategory("contracts", vscode.CodeActionKind.SourceFixAll.append("vitte.contracts"));

      for (const d of ctx.diagnostics) {
        const code = diagnosticCodeText(d);
        const doctorCode = code.toUpperCase().startsWith("DOCTOR:")
          ? code.slice("DOCTOR:".length)
          : (code.toUpperCase().startsWith("DOCTOR_") ? code.slice("DOCTOR_".length) : "");
        if (!doctorCode) continue;
        const tool = doctorCode.replace(/[_:].*$/, "").toLowerCase();
        const action = new vscode.CodeAction(`Vitte Doctor: rerun ${tool}`, vscode.CodeActionKind.QuickFix);
        action.diagnostics = [d];
        action.command = { command: "vitte.doctor.runTool", title: "Run Doctor tool", arguments: [tool] };
        actions.push(action);
      }
      for (const d of ctx.diagnostics) {
        if (String(d.source ?? "").startsWith("vitte-") || syntaxCodeOf(d) !== undefined) {
          const explain = new vscode.CodeAction(explainTitle(d.message ?? ""), vscode.CodeActionKind.QuickFix);
          explain.diagnostics = [d];
          explain.command = {
            command: "vitte.diagnostics.explain",
            title: "Explain this diagnostic",
            arguments: [{ uri: document.uri, diagnostic: d }],
          };
          actions.push(explain);
        }
        const code = explainableDiagnosticCode(d);
        if (code) {
          const openDoc = new vscode.CodeAction("Vitte: Open diagnostics doc", vscode.CodeActionKind.QuickFix);
          openDoc.diagnostics = [d];
          openDoc.command = {
            command: "vitte.diagnostics.openDoc",
            title: "Open diagnostics doc",
            arguments: [{ uri: document.uri, diagnostic: d }],
          };
          actions.push(openDoc);

          const copyExplain = new vscode.CodeAction("Vitte: Copy explain command", vscode.CodeActionKind.QuickFix);
          copyExplain.diagnostics = [d];
          copyExplain.command = {
            command: "vitte.diagnostics.copyExplainCommand",
            title: "Copy explain command",
            arguments: [{ uri: document.uri, diagnostic: d }],
          };
          actions.push(copyExplain);
        }
        actions.push(...buildSyntaxQuickFixes(document, d));
      }
      if (ctx.diagnostics.some((d) => syntaxCodeOf(d) !== undefined)) {
        const action = new vscode.CodeAction(
          "Vitte: Apply safe syntax fixes (document)",
          vscode.CodeActionKind.QuickFix,
        );
        action.command = {
          command: "vitte.syntax.autoFixDocument",
          title: "Apply safe syntax fixes",
          arguments: [document.uri],
        };
        actions.push(action);

        const fixAllSyntax = new vscode.CodeAction(
          "Vitte: Fix all syntax (safe, preview diff)",
          vscode.CodeActionKind.SourceFixAll.append("vitte.syntax"),
        );
        fixAllSyntax.command = {
          command: "vitte.syntax.fixAllPreview",
          title: "Fix all syntax (safe, preview diff)",
          arguments: [document.uri],
        };
        actions.push(fixAllSyntax);
      }
      return actions;
    },
  };

  context.subscriptions.push(
    vscode.languages.registerCodeActionsProvider(
      [{ language: "vitte" }, { language: "vit" }],
      provider,
      {
        providedCodeActionKinds: [
          vscode.CodeActionKind.QuickFix,
          vscode.CodeActionKind.SourceFixAll.append("vitte.imports"),
          vscode.CodeActionKind.SourceFixAll.append("vitte.style"),
          vscode.CodeActionKind.SourceFixAll.append("vitte.naming"),
          vscode.CodeActionKind.SourceFixAll.append("vitte.contracts"),
          vscode.CodeActionKind.SourceFixAll.append("vitte.syntax"),
        ],
      }
    ),
    vscode.commands.registerCommand("vitte.fixCategoryPreview", async (category: FixCategory, uri?: vscode.Uri) => {
      if (!uri) return;
      const doc = await vscode.workspace.openTextDocument(uri);
      await previewAndApplyCategory(category, doc);
    }),
    vscode.commands.registerCommand("vitte.syntax.autoFixDocument", async (uri?: vscode.Uri) => {
      if (!uri) return;
      const doc = await vscode.workspace.openTextDocument(uri);
      const changed = await applySafeSyntaxFixes(doc);
      if (!changed) {
        void vscode.window.showInformationMessage("Vitte syntax: no safe fixes available.");
      }
    }),
    vscode.commands.registerCommand("vitte.syntax.fixAllPreview", async (uri?: vscode.Uri) => {
      if (!uri) return;
      const doc = await vscode.workspace.openTextDocument(uri);
      await previewAndApplySafeSyntaxFixes(doc);
    }),
    vscode.workspace.onDidSaveTextDocument(async (doc) => {
      if (!isVitteLikeDocument(doc)) return;
      const cfg = vscode.workspace.getConfiguration("vitte");
      const enabled = cfg.get<boolean>("syntax.autoFixOnSave", false);
      if (!enabled) return;
      const key = doc.uri.toString();
      if (syntaxAutoFixInFlight.has(key)) return;
      syntaxAutoFixInFlight.add(key);
      try {
        await applySafeSyntaxFixes(doc);
      } finally {
        syntaxAutoFixInFlight.delete(key);
      }
    }),
  );
}
