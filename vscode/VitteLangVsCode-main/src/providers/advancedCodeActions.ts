import * as path from "node:path";
import * as vscode from "vscode";
import { compilerSuggestionsFor } from "../utils/compilerDiagnostics";

function diagnosticCodeText(diagnostic: vscode.Diagnostic): string {
  const raw = diagnostic.code;
  if (typeof raw === "string" || typeof raw === "number") return String(raw);
  if (raw && typeof raw === "object" && "value" in raw) {
    const value = (raw as { value?: unknown }).value;
    if (typeof value === "string" || typeof value === "number") return String(value);
  }
  return "";
}

function canonicalCode(diagnostic: vscode.Diagnostic): string | undefined {
  const value = diagnosticCodeText(diagnostic).trim().toUpperCase();
  if (!value) return undefined;
  const code = value.slice(value.lastIndexOf(":") + 1).trim();
  return code || undefined;
}

function explainableDiagnosticCode(diagnostic: vscode.Diagnostic): string | undefined {
  const code = canonicalCode(diagnostic);
  return code && (/^E\d{4}$/.test(code) || /^VITTE-[A-Z]\d{4}$/.test(code)) ? code : undefined;
}

function hasUniqueImportTarget(suggestion: ReturnType<typeof compilerSuggestionsFor>[number]): boolean {
  const kind = `${suggestion.kind ?? ""} ${suggestion.message}`.toLowerCase();
  const replacement = suggestion.replacement ?? "";
  const importOrQualification = /import|qualif/.test(kind)
    || /^\s*(?:use|import|pull)\b/.test(replacement)
    || /[A-Za-z_][A-Za-z0-9_]*\s*::\s*[A-Za-z_][A-Za-z0-9_]*/.test(replacement);
  if (!importOrQualification) return true;
  return Boolean(
    suggestion.candidate?.trim()
      && suggestion.destination?.trim()
      && suggestion.candidateCount === 1
      && suggestion.destinationCount === 1,
  );
}

function compilerSuggestionTitle(message: string): string {
  const firstSentence = message.replace(/\s+/g, " ").trim().split(/(?<=[.!?])\s/, 1)[0] ?? "";
  const concise = firstSentence.length > 68 ? `${firstSentence.slice(0, 65).trimEnd()}…` : firstSentence;
  return `Vitte: ${concise || "Apply compiler-proven fix"}`;
}

function buildCompilerSuggestionQuickFixes(document: vscode.TextDocument, diagnostic: vscode.Diagnostic): vscode.CodeAction[] {
  if (!String(diagnostic.source ?? "").startsWith("vitte") || diagnostic.severity !== vscode.DiagnosticSeverity.Error) return [];
  const code = canonicalCode(diagnostic);
  if (!code) return [];
  const start = document.offsetAt(diagnostic.range.start);
  const end = document.offsetAt(diagnostic.range.end);
  return compilerSuggestionsFor(document.uri.toString(), code, start, end).flatMap((suggestion) => {
    if (suggestion.applicability !== "machine-applicable" || suggestion.replacement === undefined) return [];
    if (!suggestion.message.trim() || /^apply (?:the )?(?:compiler )?(?:suggestion|fix)$/i.test(suggestion.message.trim())) return [];
    if (!hasUniqueImportTarget(suggestion)) return [];
    if (!suggestion.source || path.resolve(suggestion.source) !== path.resolve(document.uri.fsPath)) return [];
    if (suggestion.start < 0 || suggestion.end < suggestion.start || suggestion.end > document.getText().length) return [];

    const action = new vscode.CodeAction(compilerSuggestionTitle(suggestion.message), vscode.CodeActionKind.QuickFix);
    action.isPreferred = false;
    action.diagnostics = [diagnostic];
    action.edit = new vscode.WorkspaceEdit();
    action.edit.replace(
      document.uri,
      new vscode.Range(document.positionAt(suggestion.start), document.positionAt(suggestion.end)),
      suggestion.replacement,
    );
    return [action];
  });
}

function advisoryAction(
  title: string,
  command: string,
  document: vscode.TextDocument,
  diagnostic: vscode.Diagnostic,
): vscode.CodeAction {
  const action = new vscode.CodeAction(`Vitte (advisory): ${title}`, vscode.CodeActionKind.Empty);
  action.diagnostics = [diagnostic];
  action.command = {
    command,
    title,
    arguments: [{ uri: document.uri, diagnostic }],
  };
  return action;
}

export function registerAdvancedCodeActions(context: vscode.ExtensionContext): void {
  const provider: vscode.CodeActionProvider = {
    provideCodeActions(document, _range, ctx) {
      if (!["vitte", "vit"].includes(document.languageId)) return [];
      const actions: vscode.CodeAction[] = [];

      for (const diagnostic of ctx.diagnostics) {
        const code = diagnosticCodeText(diagnostic);
        const doctorCode = code.toUpperCase().startsWith("DOCTOR:")
          ? code.slice("DOCTOR:".length)
          : code.toUpperCase().startsWith("DOCTOR_") ? code.slice("DOCTOR_".length) : "";
        if (doctorCode) {
          const tool = doctorCode.replace(/[_:].*$/, "").toLowerCase();
          const action = new vscode.CodeAction(`Vitte (advisory): rerun Doctor ${tool}`, vscode.CodeActionKind.Empty);
          action.diagnostics = [diagnostic];
          action.command = { command: "vitte.doctor.runTool", title: "Rerun Doctor tool", arguments: [tool] };
          actions.push(action);
        }

        if (String(diagnostic.source ?? "").startsWith("vitte")) {
          actions.push(advisoryAction("Explain this diagnostic", "vitte.diagnostics.explain", document, diagnostic));
        }
        if (explainableDiagnosticCode(diagnostic)) {
          actions.push(advisoryAction("Open diagnostics documentation", "vitte.diagnostics.openDoc", document, diagnostic));
          actions.push(advisoryAction("Copy explain command", "vitte.diagnostics.copyExplainCommand", document, diagnostic));
        }

        actions.push(...buildCompilerSuggestionQuickFixes(document, diagnostic));
      }
      return actions;
    },
  };

  context.subscriptions.push(vscode.languages.registerCodeActionsProvider(
    [{ language: "vitte" }, { language: "vit" }],
    provider,
    { providedCodeActionKinds: [vscode.CodeActionKind.QuickFix, vscode.CodeActionKind.Empty] },
  ));
}
