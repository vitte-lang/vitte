import { lexicalContextAt, maskNonCode } from "./lexical.js";
import {
  CompletionItemKind,
  InsertTextFormat,
  MarkupKind,
  Range,
} from "vscode-languageserver/node";
import type {
  CompletionItem,
  MarkupContent,
  Position,
  TextEdit,
} from "vscode-languageserver/node";
import type { TextDocument } from "vscode-languageserver-textdocument";
import {
  BOOL_LITERALS,
  KEYWORDS,
  NIL_LITERALS,
  PRIMITIVE_TYPES,
} from "./languageFacts.js";
import { parseCompletionModel, type ProcedureSymbol, type TypeSymbol } from "./completionModel.js";
import { COMPILER_BUILTINS } from "./callableFacts.js";
import {
  importedCompletionModel,
  importItemPosition,
  importPosition,
  listModulePathCandidates,
  readModuleExports,
  type CompletionEnvironment,
  type ImportedCompletionSymbol,
} from "./moduleResolver.js";

export interface CompletionSettings {
  enabledInComments: boolean;
  enabledInStrings: boolean;
  includeSnippets: boolean;
  workspaceSymbols: boolean;
  maxItems: number;
}

const DEFAULT_COMPLETION_SETTINGS: CompletionSettings = {
  enabledInComments: false,
  enabledInStrings: false,
  includeSnippets: true,
  workspaceSymbols: true,
  maxItems: 200,
};
let completionSettings: CompletionSettings = { ...DEFAULT_COMPLETION_SETTINGS };

export function configureCompletions(settings?: Partial<CompletionSettings>): void {
  completionSettings = {
    ...DEFAULT_COMPLETION_SETTINGS,
    ...(settings ?? {}),
    maxItems: clampNumber(settings?.maxItems, 16, 1000, DEFAULT_COMPLETION_SETTINGS.maxItems),
  };
}

interface CompletionSnippet {
  label: string;
  detail: string;
  documentation: string;
  body: string;
}

const SNIPPETS: readonly CompletionSnippet[] = [
  { label: "space", detail: "Déclare un espace", documentation: "Déclare l’espace courant.", body: "space " + ph(1, "std/os/user") + ";" },
  { label: "use", detail: "Importe un module", documentation: "Importe un module ou un symbole avec alias optionnel.", body: "use " + ph(1, "std/io") + " as " + ph(2, "io") + ";" },
  { label: "export", detail: "Exporte tout le module", documentation: "Ré-exporte la surface du module courant.", body: "export *;" },
  { label: "proc", detail: "Déclare une procédure", documentation: "Procédure avec paramètres et type de retour optionnel.", body: "proc " + ph(1, "name") + "(" + ph(2, "params") + ")" + ph(3, " -> " + ph(4, "Type")) + " {\n\t$0\n}" },
  { label: "form", detail: "Déclare un form", documentation: "Structure avec des champs typés.", body: "form " + ph(1, "Name") + " {\n\t" + ph(2, "name") + ": " + ph(3, "Type") + ",\n}" },
  { label: "trait", detail: "Déclare un trait", documentation: "Contrat de capacités.", body: "trait " + ph(1, "Name") + " {\n\t$0\n}" },
  { label: "pick", detail: "Déclare un pick", documentation: "Somme de variantes nommées.", body: "pick " + ph(1, "Name") + " {\n\t" + ph(2, "Variant") + "(" + ph(3, "Type") + "),\n\t$0\n}" },
  { label: "type", detail: "Alias de type", documentation: "Déclare un alias de type.", body: "type " + ph(1, "Alias") + " = " + ph(2, "Existing") + ";" },
  { label: "match", detail: "Match expression", documentation: "Expression de branchement avec motifs.", body: "match " + ph(1, "expr") + " {\n\t" + ph(2, "Pattern") + " => {\n\t\t$0\n\t},\n}" },
  { label: "ifelse", detail: "If / Else", documentation: "Structure conditionnelle complète.", body: "if " + ph(1, "cond") + " {\n\t" + ph(2, "body") + "\n} else {\n\t$0\n}" },
  { label: "for", detail: "Boucle for", documentation: "Boucle for-in sur un itérable.", body: "for " + ph(1, "item") + " in " + ph(2, "iter") + " {\n\t$0\n}" },
  { label: "loop", detail: "Boucle infinie", documentation: "Boucle sans fin, utiliser break pour sortir.", body: "loop {\n\t$0\n}" },
  { label: "let", detail: "Binding local", documentation: "Déclare une variable locale.", body: "let " + ph(1, "name") + ph(2, " : " + ph(3, "Type")) + " = " + ph(4, "value") + ";" },
  { label: "const", detail: "Constante", documentation: "Déclare une constante globale.", body: "const " + ph(1, "NAME") + ": " + ph(2, "Type") + " = " + ph(3, "value") + ";" },
  { label: "return", detail: "Retour de procédure", documentation: "Retourne une valeur depuis une procédure.", body: "return " + ph(1, "value") + ";" },
  { label: "give", detail: "Retour de procédure", documentation: "Retourne une valeur depuis une procédure.", body: "give " + ph(1, "value") + ";" },
  { label: "print", detail: "Affichage console", documentation: "Affiche un message sur la sortie standard.", body: "println(\"" + ph(1, "msg") + "\");" },
  { label: "doc", detail: "Doc comment", documentation: "Insert a documentation comment.", body: "/// " + ph(1, "Summary") + "\n///\n/// " + ph(0, "Details") },
];

interface CompletionSite {
  context: "code" | "type" | "attribute" | "member" | "qualified" | "initializer";
  qualifier?: string;
  receiver?: string;
  initializerType?: string;
  initializerStart?: number;
}

export async function provideCompletions(
  doc: TextDocument,
  position: Position,
  environment: CompletionEnvironment = {},
): Promise<CompletionItem[]> {
  const text = doc.getText();
  const offset = doc.offsetAt(position);
  const lexicalContext = lexicalContextAt(text, offset);
  if (lexicalContext === "comment" && !completionSettings.enabledInComments) return [];
  if (lexicalContext === "string" && !completionSettings.enabledInStrings) return [];

  const current = currentToken(doc, position);
  if (text.length > 512 * 1024) return largeFileCompletions(current.token, current.range, isTypePosition(getLinePrefix(doc, position)));
  const pathContext = importPosition(text, offset);
  if (pathContext) {
    const candidates = await listModulePathCandidates(doc, pathContext.parentPath, pathContext.partialName, environment);
    const target = Range.create(doc.positionAt(pathContext.segmentStart), doc.positionAt(pathContext.segmentEnd));
    if (candidates.length === 0) {
      return [unavailableCompletion("Aucun module Vitte trouvé", "Vérifiez le chemin ou ajoutez sa racine dans vitte.modulePaths / VITTE_MODULE_PATH.", target, text.slice(pathContext.segmentStart, pathContext.segmentEnd))];
    }
    return candidates.map((candidate, index) => ({
      label: candidate.name,
      kind: CompletionItemKind.Module,
      detail: candidate.detail,
      documentation: md("Module trouvé sur disque : " + candidate.modulePath + "."),
      sortText: tier(candidate.isDirectory ? 0 : 1, String(index).padStart(3, "0") + "_" + candidate.name),
      filterText: candidate.name,
      textEdit: edit(target, candidate.insertText),
      ...(candidate.isDirectory ? {} : { commitCharacters: SAFE_COMMIT_CHARS }),
      data: { source: "module-path", modulePath: candidate.modulePath, rootKind: candidate.isDirectory ? "directory" : "file" },
    }));
  }

  const importItem = importItemPosition(text, offset);
  if (importItem) {
    const exports = await readModuleExports(importItem.modulePath, doc.uri, environment);
    const target = Range.create(doc.positionAt(importItem.segmentStart), doc.positionAt(importItem.segmentEnd));
    if (exports.symbols.length === 0) {
      return [unavailableCompletion("Module Vitte introuvable ou sans export public", "La complétion lit uniquement les fichiers de module présents dans les racines configurées.", target, importItem.partialName)];
    }
    const items = exports.symbols
      .filter((symbol) => fuzzyScore(symbol.name, importItem.partialName) > 0)
      .map((symbol) => ({
        label: symbol.name,
        kind: completionKind(symbol),
        detail: symbol.kind + " — " + pathBasename(symbol.uri),
        documentation: md(symbol.documentation ?? "Export public de " + symbol.modulePath + "."),
        sortText: tier(0, symbol.name),
        filterText: symbol.name,
        textEdit: edit(target, symbol.name),
        commitCharacters: SAFE_COMMIT_CHARS,
        data: { source: "imported", modulePath: symbol.modulePath, name: symbol.originalName },
      }));
    return items.slice(0, completionSettings.maxItems);
  }

  const linePrefix = getLinePrefix(doc, position);
  const site = completionSite(text, doc, position, linePrefix);
  const model = parseCompletionModel(doc, offset);
  const imported = await importedCompletionModel(doc, environment);
  const items: CompletionItem[] = [];

  if (site.context === "attribute") {
    const declarationPrefix = linePrefix.slice(0, linePrefix.lastIndexOf("#"));
    const attributes = /\bproc\b/.test(declarationPrefix)
      ? [{ name: "cold", description: "Marque cette procédure comme froide pour le backend C17." }]
      : /\b(?:static|global)\b/.test(declarationPrefix)
        ? [{ name: "thread_local", description: "Place cette variable statique dans le stockage local au thread." }]
        : [
            { name: "cold", description: "Attribut de procédure pris en charge par le backend C17." },
            { name: "thread_local", description: "Attribut de variable statique pris en charge par le backend C17." },
          ];
    for (const attribute of attributes) {
      if (fuzzyScore(attribute.name, current.token) <= 0) continue;
      items.push({
        label: attribute.name,
        kind: CompletionItemKind.Keyword,
        detail: "Attribut du backend C17",
        documentation: md(attribute.description),
        sortText: tier(0, attribute.name),
        filterText: attribute.name,
        textEdit: edit(current.range, attribute.name),
        commitCharacters: SAFE_COMMIT_CHARS,
        data: { source: "attribute", name: attribute.name },
      });
    }
    return items;
  }

  if (site.context === "member") {
    const binding = model.bindings.find((candidate) => candidate.name === site.receiver);
    const receiverType = normalizeTypeName(binding?.type) ?? (site.receiver === "self" ? enclosingReceiverType(model.procedures, offset) : undefined);
    const localType = model.types.find((candidate) => candidate.name === receiverType);
    const type = localType ?? imported.symbols.find((candidate) => candidate.name === receiverType)?.type;
    const ownerType = type?.kind === "type" && type.aliasTarget
      ? normalizeTypeName(type.aliasTarget) ?? type.name
      : type?.name ?? receiverType;
    const methods = visibleTraitMethods(model, imported.symbols, receiverType, ownerType, offset);
    if (!type && methods.length === 0) {
      if (binding) return [unavailableCompletion("Type du récepteur non résolu", "Ajoutez une annotation de type ou utilisez une toolchain avec API sémantique éditeur.", current.range, current.token)];
      return [];
    }
    for (const field of type?.fields ?? []) {
      if (fuzzyScore(field.name, current.token) <= 0) continue;
      items.push({
        label: field.name,
        kind: CompletionItemKind.Field,
        detail: field.type,
        documentation: md(field.documentation ?? "Champ " + field.name + ": " + field.type + " de " + (type?.name ?? receiverType ?? "ce type") + "."),
        sortText: tier(0, field.name),
        filterText: field.name,
        textEdit: edit(current.range, field.name),
        commitCharacters: SAFE_COMMIT_CHARS,
        data: { source: "field", owner: type?.name ?? receiverType, name: field.name, type: field.type },
      });
    }
    for (const procedure of methods) {
      if (fuzzyScore(procedure.name, current.token) <= 0) continue;
      items.push(procedureCompletion(procedure, current.range, CompletionItemKind.Method));
    }
    const unsupportedInherentMethod = [...model.procedures, ...imported.symbols.flatMap((symbol) => symbol.procedure ? [symbol.procedure] : [])]
      .some((procedure) => procedure.ownerType === ownerType && !procedure.implementedTrait && fuzzyScore(procedure.name, current.token) > 0);
    if (items.length === 0 && unsupportedInherentMethod) {
      return [unavailableCompletion("Cette méthode n’est pas résolue par le frontend actif", "Les implémentations directes de type ne sont pas validées par Vitte 0.1.1 ; seules les méthodes de traits visibles sont complétées.", current.range, current.token)];
    }
    return dedupe(items.sort((left, right) => (left.sortText ?? "").localeCompare(right.sortText ?? "")), (item) => String(item.label)).slice(0, completionSettings.maxItems);
  }

  if (site.context === "qualified") {
    const localPick = model.types.find((candidate) => candidate.name === site.qualifier && candidate.kind === "pick");
    const importedPick = imported.symbols.find((candidate) => candidate.name === site.qualifier && candidate.type?.kind === "pick")?.type;
    const pick = localPick ?? importedPick;
    if (pick) {
      for (const variant of pick.variants) {
        if (fuzzyScore(variant.name, current.token) <= 0) continue;
        const snippet = variant.payload.length
          ? variant.name + "(" + variant.payload.map((_, index) => ph(index + 1, "value" + (index + 1))).join(", ") + ")"
          : variant.name;
        items.push({
          label: variant.name,
          kind: CompletionItemKind.EnumMember,
          detail: variant.payload.length ? "variante de " + pick.name + " (" + variant.payload.join(", ") + ")" : "variante de " + pick.name,
          documentation: md(variant.documentation ?? "Variante de " + pick.name + "."),
          sortText: tier(0, variant.name),
          filterText: variant.name,
          textEdit: edit(current.range, snippet),
          insertTextFormat: variant.payload.length ? InsertTextFormat.Snippet : InsertTextFormat.PlainText,
          ...(variant.payload.length ? {} : { commitCharacters: SAFE_COMMIT_CHARS }),
          data: { source: "variant", owner: pick.name, name: variant.name, payload: variant.payload },
        });
      }
      return items;
    }
    const localQualifiedType = model.types.find((candidate) => candidate.name === site.qualifier);
    const importedQualifiedType = imported.symbols.find((candidate) => candidate.name === site.qualifier)?.type;
    const qualifiedType = localQualifiedType ?? importedQualifiedType;
    if (qualifiedType) {
      return [unavailableCompletion("Fonctions associées non annoncées", "Aucune résolution des fonctions associées n’est validée pour le frontend actif.", current.range, current.token)];
    }
    const moduleAlias = imported.modules.find((candidate) => candidate.alias === site.qualifier || site.qualifier?.startsWith(candidate.alias + "::"));
    const modulePath = moduleAlias && site.qualifier
      ? moduleAlias.modulePath + site.qualifier.slice(moduleAlias.alias.length)
      : site.qualifier ?? "";
    const exports = await readModuleExports(modulePath, doc.uri, environment);
    if (exports.symbols.length === 0) {
      return [unavailableCompletion("Aucun symbole public disponible", "Le module n’a pas pu être lu dans les racines configurées.", current.range, current.token)];
    }
    for (const symbol of exports.symbols) {
      if (fuzzyScore(symbol.name, current.token) <= 0) continue;
      items.push(importedSymbolCompletion(symbol, current.range));
    }
    return dedupe(items.sort((left, right) => (left.sortText ?? "").localeCompare(right.sortText ?? "")), (item) => String(item.label)).slice(0, completionSettings.maxItems);
  }

  if (site.context === "initializer") {
    const localType = model.types.find((candidate) => candidate.name === site.initializerType);
    const type = localType ?? imported.symbols.find((candidate) => candidate.name === site.initializerType)?.type;
    if (!type || !isFormLike(type)) return [];
    const fields = type.fields;
    const contents = maskNonCode(text.slice((site.initializerStart ?? 0) + 1, offset));
    const used = new Set(Array.from(contents.matchAll(/\b([A-Za-z_]\w*)\s*:/g), (match) => match[1]).filter(isPresent));
    for (const field of fields) {
      if (used.has(field.name) || fuzzyScore(field.name, current.token) <= 0) continue;
      items.push({
        label: field.name,
        kind: CompletionItemKind.Field,
        detail: field.type,
        documentation: md(field.documentation ?? "Valeur du champ " + field.name + ": " + field.type + "."),
        sortText: tier(0, field.name),
        filterText: field.name,
        textEdit: edit(current.range, field.name + ": " + ph(1, escapeSnippet(field.name))),
        insertTextFormat: InsertTextFormat.Snippet,
        data: { source: "constructor-field", owner: type.name, name: field.name, type: field.type },
      });
    }
    return items;
  }

  const typePosition = site.context === "type";
  if (typePosition) {
    for (const name of PRIMITIVE_TYPES) {
      if (fuzzyScore(name, current.token) <= 0) continue;
      items.push({
        label: name,
        kind: CompletionItemKind.TypeParameter,
        detail: "Type reconnu par le frontend",
        documentation: md("Type primitif Vitte " + name + "."),
        sortText: tier(0, name),
        filterText: name,
        textEdit: edit(current.range, name),
        commitCharacters: SAFE_COMMIT_CHARS,
      });
    }
    for (const name of model.genericParameters) {
      if (fuzzyScore(name, current.token) <= 0) continue;
      items.push({
        label: name,
        kind: CompletionItemKind.TypeParameter,
        detail: "Paramètre générique en portée",
        documentation: md("Paramètre générique " + name + "."),
        sortText: tier(0, "generic:" + name),
        filterText: name,
        textEdit: edit(current.range, name),
        commitCharacters: SAFE_COMMIT_CHARS,
      });
    }
    for (const type of model.types) {
      if (fuzzyScore(type.name, current.token) <= 0) continue;
      items.push(typeCompletion(type, current.range, true));
    }
    for (const symbol of imported.symbols) {
      if (!symbol.type || fuzzyScore(symbol.name, current.token) <= 0) continue;
      items.push(importedSymbolCompletion(symbol, current.range));
    }
  } else {
    for (const binding of model.bindings) {
      if (fuzzyScore(binding.name, current.token) <= 0) continue;
      const callableParameters = binding.type ? callableTypeParameterNames(binding.type) : undefined;
      items.push({
        label: binding.name,
        kind: binding.kind,
        detail: binding.detail,
        documentation: md("Binding visible ici : " + binding.name + (binding.type ? ": " + binding.type : "") + "."),
        sortText: tier(0, binding.name),
        filterText: binding.name,
        textEdit: edit(current.range, callableParameters ? callableSnippet(binding.name, callableParameters) : binding.name),
        ...(callableParameters ? { insertTextFormat: InsertTextFormat.Snippet } : {}),
        commitCharacters: SAFE_COMMIT_CHARS,
        data: { source: "binding", name: binding.name, type: binding.type },
      });
    }
    for (const procedure of model.procedures.filter((candidate) => !candidate.ownerType && !candidate.traitOwner)) {
      if (fuzzyScore(procedure.name, current.token) <= 0) continue;
      items.push(procedureCompletion(procedure, current.range, CompletionItemKind.Function));
    }
    for (const symbol of imported.symbols) {
      if (symbol.procedure?.ownerType) continue;
      if (fuzzyScore(symbol.name, current.token) <= 0) continue;
      items.push(importedSymbolCompletion(symbol, current.range));
    }
    for (const builtin of COMPILER_BUILTINS) {
      if (fuzzyScore(builtin.name, current.token) <= 0) continue;
      items.push({
        label: builtin.name,
        kind: CompletionItemKind.Function,
        detail: builtin.parameters.join(", ") + " -> " + builtin.returnType,
        documentation: md(builtin.documentation),
        sortText: tier(1, "builtin:" + builtin.name),
        filterText: builtin.name,
        textEdit: edit(current.range, callableSnippet(builtin.name, builtin.parameters)),
        insertTextFormat: InsertTextFormat.Snippet,
        data: { source: "compiler-builtin", signature: builtin.name + "(" + builtin.parameters.join(", ") + ") -> " + builtin.returnType },
      });
    }
    for (const type of model.types) {
      if (fuzzyScore(type.name, current.token) <= 0) continue;
      items.push(typeCompletion(type, current.range, false));
    }
    for (const symbol of imported.symbols) {
      if (!symbol.type || fuzzyScore(symbol.name, current.token) <= 0) continue;
      items.push(importedSymbolCompletion(symbol, current.range));
    }
    for (const literal of [...BOOL_LITERALS, ...NIL_LITERALS]) {
      if (fuzzyScore(literal, current.token) <= 0) continue;
      items.push({
        label: literal,
        kind: CompletionItemKind.Value,
        detail: "Littéral",
        documentation: md("Littéral pris en charge " + literal + "."),
        sortText: tier(2, "literal:" + literal),
        filterText: literal,
        textEdit: edit(current.range, literal),
        commitCharacters: SAFE_COMMIT_CHARS,
      });
    }
    for (const keyword of KEYWORDS) {
      if (fuzzyScore(keyword, current.token) <= 0) continue;
      items.push({
        label: keyword,
        kind: CompletionItemKind.Keyword,
        detail: "Mot-clé du langage",
        documentation: md("Mot-clé Vitte " + keyword + "."),
        sortText: tier(4, keyword),
        filterText: keyword,
        textEdit: edit(current.range, keyword),
        commitCharacters: SAFE_COMMIT_CHARS,
      });
    }
    if (completionSettings.includeSnippets) {
      for (const snippet of SNIPPETS) {
        if (fuzzyScore(snippet.label, current.token) <= 0) continue;
        items.push({
          label: snippet.label,
          kind: CompletionItemKind.Snippet,
          detail: snippet.detail,
          documentation: md(snippet.documentation),
          sortText: tier(5, snippet.label),
          filterText: snippet.label,
          textEdit: edit(current.range, snippet.body),
          insertTextFormat: InsertTextFormat.Snippet,
          data: { source: "snippet", label: snippet.label },
        });
      }
    }
  }

  return dedupe(items.sort((left, right) => (left.sortText ?? "").localeCompare(right.sortText ?? "")), (item) => String(item.label))
    .slice(0, completionSettings.maxItems);
}

export function resolveCompletion(item: CompletionItem): CompletionItem {
  const data = item.data && typeof item.data === "object" ? item.data as Record<string, unknown> : undefined;
  if (data?.resolved === true) return item;
  const initialDocumentation = item.documentation && !Array.isArray(item.documentation) && typeof item.documentation === "object" && "value" in item.documentation
    ? String(item.documentation.value)
    : typeof item.documentation === "string" ? item.documentation : "";
  const sections = [initialDocumentation];
  if (typeof data?.signature === "string") sections.push("Signature : " + data.signature);
  if (Array.isArray(data?.fields)) {
    const fields = data.fields.filter((field): field is string => typeof field === "string");
    if (fields.length > 0) sections.push("Champs :\n" + fields.map((field) => "- " + field).join("\n"));
  }
  if (typeof data?.modulePath === "string") sections.push("Module : " + data.modulePath);
  if (sections.some((section) => section.length > 0)) item.documentation = md(sections.filter(Boolean).join("\n\n"));
  if (data) item.data = { ...data, resolved: true };
  return item;
}

export function triggerCharacters(): string[] {
  return [".", ":", "#", "{"];
}

function completionSite(text: string, doc: TextDocument, position: Position, linePrefix: string): CompletionSite {
  const offset = doc.offsetAt(position);
  if (/#\s*!?\s*\[[^\n]*$/.test(linePrefix)) return { context: "attribute" };
  const member = /([A-Za-z_]\w*)\.\s*[A-Za-z_\w]*$/.exec(linePrefix);
  if (member?.[1]) return { context: "member", receiver: member[1] };
  const qualified = /([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)::\s*([A-Za-z_]\w*)?$/.exec(linePrefix);
  if (qualified?.[1]) return { context: "qualified", qualifier: qualified[1] };
  const initializer = initializerPosition(text, offset);
  if (initializer) return { context: "initializer", initializerType: initializer.type, initializerStart: initializer.open };
  if (isTypePosition(linePrefix)) return { context: "type" };
  return { context: "code" };
}

function largeFileCompletions(token: string, range: Range, typePosition: boolean): CompletionItem[] {
  const items: CompletionItem[] = [];
  if (typePosition) {
    for (const name of PRIMITIVE_TYPES) {
      if (fuzzyScore(name, token) <= 0) continue;
      items.push({ label: name, kind: CompletionItemKind.TypeParameter, detail: "Type primitif — complétion légère pour gros fichier", sortText: tier(0, name), filterText: name, textEdit: edit(range, name), commitCharacters: SAFE_COMMIT_CHARS });
    }
  } else {
    for (const builtin of COMPILER_BUILTINS) {
      if (fuzzyScore(builtin.name, token) <= 0) continue;
      items.push({ label: builtin.name, kind: CompletionItemKind.Function, detail: builtin.parameters.join(", ") + " -> " + builtin.returnType + " — complétion légère pour gros fichier", documentation: md(builtin.documentation), sortText: tier(0, builtin.name), filterText: builtin.name, textEdit: edit(range, callableSnippet(builtin.name, builtin.parameters)), insertTextFormat: InsertTextFormat.Snippet });
    }
    for (const keyword of KEYWORDS) {
      if (fuzzyScore(keyword, token) <= 0) continue;
      items.push({ label: keyword, kind: CompletionItemKind.Keyword, detail: "Mot-clé — complétion légère pour gros fichier", sortText: tier(2, keyword), filterText: keyword, textEdit: edit(range, keyword), commitCharacters: SAFE_COMMIT_CHARS });
    }
  }
  return items.slice(0, completionSettings.maxItems);
}

function initializerPosition(text: string, offset: number): { type: string; open: number } | undefined {
  const code = maskNonCode(text);
  const stack: number[] = [];
  for (let index = 0; index < offset; index++) {
    if (code[index] === "{") stack.push(index);
    else if (code[index] === "}") stack.pop();
  }
  const open = stack[stack.length - 1];
  if (open === undefined) return undefined;
  const before = code.slice(Math.max(0, open - 160), open);
  const match = /\b([A-Za-z_]\w*)\s*$/.exec(before);
  return match?.[1] ? { type: match[1], open } : undefined;
}

function isTypePosition(prefix: string): boolean {
  return /(?::|->|\bas\s+|\bis\s+)\s*[A-Za-z_0-9<>&[\]|?:.,/\s]*$/.test(prefix)
    || /\b(?:form|pick|trait|type|opaque)\s+[A-Za-z_]\w*\s*<[^>]*$/.test(prefix)
    || /\bproc\s+[A-Za-z_]\w*\s*\([^)]*:\s*[A-Za-z_\w<>&[\]|?:.,/\s]*$/.test(prefix);
}

function procedureCompletion(procedure: ProcedureSymbol, range: Range, kind: typeof CompletionItemKind.Function | typeof CompletionItemKind.Method, label = procedure.name): CompletionItem {
  const signature = signatureLabel({ ...procedure, name: label });
  const details = procedure.documentation ?? (kind === CompletionItemKind.Method ? "Méthode " : "Procédure ") + signature + ".";
  return {
    label,
    kind,
    detail: signature,
    documentation: md(details),
    sortText: tier(kind === CompletionItemKind.Method ? 0 : 1, procedure.name),
    filterText: label,
    textEdit: edit(range, callableSnippet(label, procedure.parameters.map((parameter) => parameter.name + (parameter.type ? ": " + parameter.type : "")))),
    insertTextFormat: InsertTextFormat.Snippet,
    data: { source: "procedure", signature, name: label },
  };
}

function importedSymbolCompletion(symbol: ImportedCompletionSymbol, range: Range): CompletionItem {
  if (symbol.procedure) return procedureCompletion(symbol.procedure, range, CompletionItemKind.Function, symbol.name);
  if (symbol.kind === "constant") {
    return {
      label: symbol.name,
      kind: CompletionItemKind.Constant,
      detail: symbol.constantType ? "constante publique: " + symbol.constantType : "constante publique",
      documentation: md(symbol.documentation ?? "Constante exportée par " + symbol.modulePath + "."),
      sortText: tier(1, symbol.name),
      filterText: symbol.name,
      textEdit: edit(range, symbol.name),
      commitCharacters: SAFE_COMMIT_CHARS,
      data: { source: "imported-constant", modulePath: symbol.modulePath, type: symbol.constantType },
    };
  }
  return typeCompletion(symbol.type, range, false, symbol.name, symbol.modulePath, symbol.documentation);
}

function typeCompletion(type: TypeSymbol | undefined, range: Range, typePosition: boolean, alias?: string, modulePath?: string, documentation?: string): CompletionItem {
  const name = alias ?? type?.name ?? "";
  const kind = type?.kind === "pick" ? CompletionItemKind.Enum
    : type?.kind === "trait" ? CompletionItemKind.Interface
      : type?.kind === "opaque" || type?.kind === "type" ? CompletionItemKind.TypeParameter
        : CompletionItemKind.Struct;
  let newText = name;
  if (!typePosition && type && isFormLike(type) && type.fields.length > 0) {
    newText = name + " { " + type.fields.map((field, index) => field.name + ": " + ph(index + 1, escapeSnippet(field.name))).join(", ") + " }";
  }
  const generic = type?.generics.length ? "<" + type.generics.join(", ") + ">" : "";
  const signature = (type?.kind ?? "type") + " " + name + generic;
  return {
    label: name,
    kind,
    detail: signature,
    documentation: md(documentation ?? type?.documentation ?? "Type Vitte déclaré" + (modulePath ? " dans " + modulePath : "") + " : " + signature + "."),
    sortText: tier(1, name),
    filterText: name,
    textEdit: edit(range, newText),
    ...(newText !== name ? { insertTextFormat: InsertTextFormat.Snippet } : { commitCharacters: SAFE_COMMIT_CHARS }),
    data: { source: "type", signature, modulePath, name, fields: type?.fields.map((field) => field.name + ": " + field.type) },
  };
}

function completionKind(symbol: ImportedCompletionSymbol): CompletionItemKind {
  if (symbol.procedure) return CompletionItemKind.Function;
  if (symbol.kind === "pick") return CompletionItemKind.Enum;
  if (symbol.kind === "trait") return CompletionItemKind.Interface;
  if (symbol.kind === "constant") return CompletionItemKind.Constant;
  return CompletionItemKind.Struct;
}

function isFormLike(type: TypeSymbol): boolean {
  return type.kind === "form" || (type.kind === "type" && Boolean(type.aliasTarget) && type.fields.length > 0);
}

function visibleTraitMethods(
  model: ReturnType<typeof parseCompletionModel>,
  importedSymbols: readonly ImportedCompletionSymbol[],
  receiverType: string | undefined,
  ownerType: string | undefined,
  offset: number,
): ProcedureSymbol[] {
  const activeProcedure = model.procedures
    .filter((procedure) => procedure.bodyStart !== undefined && procedure.bodyStart <= offset && (procedure.bodyEnd === undefined || offset <= procedure.bodyEnd))
    .sort((left, right) => right.start - left.start)[0];
  const genericBounds = receiverType ? activeProcedure?.genericTraitBounds?.[receiverType] ?? [] : [];
  const traitDefinitions = [
    ...model.types.filter((type) => type.kind === "trait").map((type) => ({ visibleName: type.name, type })),
    ...importedSymbols.flatMap((symbol) => symbol.type?.kind === "trait" ? [{ visibleName: symbol.name, type: symbol.type }] : []),
  ];
  const implementations = [...model.procedures, ...importedSymbols.flatMap((symbol) => symbol.procedure ? [symbol.procedure] : [])];
  const methods = new Map<string, ProcedureSymbol>();
  for (const definition of traitDefinitions) {
    const constrained = genericBounds.includes(definition.visibleName) || genericBounds.includes(definition.type.name);
    const implemented = Boolean(ownerType && implementations.some((procedure) => procedure.ownerType === ownerType && procedure.implementedTrait === definition.type.name));
    if (!constrained && !implemented) continue;
    for (const method of definition.type.methods) methods.set(method.name, method);
  }
  return Array.from(methods.values());
}

function enclosingReceiverType(procedures: ProcedureSymbol[], offset: number): string | undefined {
  return procedures.find((procedure) => procedure.ownerType && procedure.bodyStart !== undefined && procedure.bodyStart <= offset && (procedure.bodyEnd === undefined || offset <= procedure.bodyEnd))?.ownerType;
}

function normalizeTypeName(type: string | undefined): string | undefined {
  if (!type) return undefined;
  const match = /[A-Za-z_]\w*/.exec(type.replace(/^(?:const\s+)?/, ""));
  return match?.[0];
}

function signatureLabel(procedure: ProcedureSymbol): string {
  const generic = procedure.generics.length ? "<" + procedure.generics.join(", ") + ">" : "";
  const parameters = procedure.parameters.map((parameter) => (parameter.mode ? parameter.mode + " " : "") + parameter.name + (parameter.type ? ": " + parameter.type : "")).join(", ");
  return "proc " + procedure.name + generic + "(" + parameters + ")" + (procedure.returnType ? " -> " + procedure.returnType : "");
}

function callableSnippet(name: string, parameters: readonly string[]): string {
  if (parameters.length === 0) return name + "($0)";
  return name + "(" + parameters.map((parameter, index) => {
    const parameterName = parameter.split(":", 1)[0]?.trim() || "arg" + (index + 1);
    return ph(index + 1, escapeSnippet(parameterName));
  }).join(", ") + ")$0";
}

function callableTypeParameterNames(type: string): string[] | undefined {
  const match = /^proc\s*\(([\s\S]*)\)\s*(?:->\s*[\s\S]+)?$/.exec(type.trim());
  if (!match) return undefined;
  const parameters = splitCallableParameters(match[1] ?? "");
  return parameters.map((parameter, index) => {
    const colon = parameter.indexOf(":");
    const name = (colon < 0 ? "" : parameter.slice(0, colon)).trim();
    return name || "arg" + (index + 1);
  });
}

function splitCallableParameters(value: string): string[] {
  const result: string[] = [];
  const stack: string[] = [];
  let angleDepth = 0;
  let start = 0;
  for (let index = 0; index < value.length; index++) {
    const character = value[index];
    if (character === "(" || character === "[" || character === "{") stack.push(character);
    else if (character === ")" || character === "]" || character === "}") stack.pop();
    else if (stack.length === 0 && character === "<") angleDepth++;
    else if (stack.length === 0 && character === ">" && angleDepth > 0) angleDepth--;
    else if (character === "," && stack.length === 0 && angleDepth === 0) {
      result.push(value.slice(start, index).trim());
      start = index + 1;
    }
  }
  if (start < value.length) result.push(value.slice(start).trim());
  return result.filter(Boolean);
}

function currentToken(doc: TextDocument, position: Position): { token: string; range: Range } {
  const line = doc.getText(Range.create({ line: position.line, character: 0 }, { line: position.line, character: 10_000 }));
  const index = Math.min(position.character, line.length);
  const left = /[A-Za-z_]\w*$/.exec(line.slice(0, index));
  const right = /^\w*/.exec(line.slice(index));
  const start = left ? index - left[0].length : index;
  const end = right ? index + right[0].length : index;
  return { token: line.slice(start, end), range: Range.create({ line: position.line, character: start }, { line: position.line, character: end }) };
}

function getLinePrefix(doc: TextDocument, position: Position): string {
  return doc.getText(Range.create({ line: position.line, character: 0 }, position));
}

function fuzzyScore(candidate: string, query: string): number {
  if (!query) return 1;
  if (candidate.toLowerCase().startsWith(query.toLowerCase())) return 2;
  let queryIndex = 0;
  const normalized = query.toLowerCase();
  for (const character of candidate.toLowerCase()) if (character === normalized[queryIndex]) queryIndex++;
  return queryIndex === normalized.length ? 1 : 0;
}

function dedupe<T>(values: T[], keyFor: (value: T) => string): T[] {
  const seen = new Set<string>();
  return values.filter((value) => {
    const key = keyFor(value);
    if (seen.has(key)) return false;
    seen.add(key);
    return true;
  });
}

function pathBasename(uri: string): string {
  try { return decodeURIComponent(uri.split("/").pop() ?? uri); }
  catch { return uri; }
}

function tier(priority: number, value = ""): string { return String(priority).padStart(2, "0") + "_" + value; }
function md(value: string): MarkupContent { return { kind: MarkupKind.Markdown, value }; }
function edit(range: Range, newText: string): TextEdit { return { range, newText }; }
function ph(index: number, placeholder: string): string { return "$" + "{" + index + ":" + placeholder + "}"; }
function escapeSnippet(value: string): string { return value.replace(/[\\$}]/g, "\\$&"); }
function isPresent<T>(value: T | undefined): value is T { return value !== undefined; }
function unavailableCompletion(label: string, detail: string, range: Range, preserveText: string): CompletionItem {
  return {
    label,
    kind: CompletionItemKind.Text,
    detail,
    documentation: md("Cette entrée explique pourquoi aucune suggestion sémantique fiable n’est disponible ici."),
    filterText: preserveText,
    sortText: tier(99, label),
    preselect: false,
    textEdit: edit(range, preserveText),
    data: { source: "unavailable" },
  };
}
function clampNumber(value: number | undefined, min: number, max: number, fallback: number): number {
  if (typeof value !== "number" || !Number.isFinite(value)) return fallback;
  return Math.min(max, Math.max(min, Math.floor(value)));
}
const SAFE_COMMIT_CHARS = [" ", "\t", "=", ",", ";", "."];
