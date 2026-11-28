const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
const { test, before } = require('node:test');
const { TextDocument } = require('vscode-languageserver-textdocument');
const { CompletionItemKind, SymbolKind } = require('vscode-languageserver/node');
const { Registry } = require('vscode-textmate');
const oniguruma = require('vscode-oniguruma');
const { scanLexical, maskNonCode, lexicalContextAt } = require('../server/out/src/lexical');
const facts = require('../server/out/src/languageFacts');
const { buildSemanticTokens, getSemanticTokensLegend } = require('../server/out/src/semantic');
const {
  buildCodeMask,
  documentSymbols,
  workspaceSymbols,
  definitionAtPosition,
  definitionAtPositionWithImports,
  declarationAtPositionWithImports,
  typeDefinitionAtPositionWithImports,
  implementationAtPositionWithImports,
  referencesAtPositionWithWorkspace,
  resolveRenameTarget,
  renameReferencesForTarget,
  validateRenameName,
  validateRenameLocations,
} = require('../server/out/src/navigation');
const { provideCompletions, configureCompletions } = require('../server/out/src/completion');
const { findUniqueModulePathFileMove } = require('../server/out/src/moduleResolver');
const { indexDocument, getDocumentIndex, clearIndex } = require('../server/out/src/indexer');
const { provideSignatureHelp, provideSignatureHelpWithImports } = require('../server/out/src/signatureHelp');
const { provideHover, provideHoverWithImports } = require('../server/out/src/semantic');
const { provideInlayHints } = require('../server/out/src/inlay');
const { provideFoldingRanges, provideSelectionRanges } = require('../server/out/src/documentFeatures');
const { provideDocumentLinks } = require('../server/out/src/documentLinks');
const { resolveVitteProjectScope, resolveProjectPath } = require('../server/out/src/projectScope');
const { isCapabilityAdvertised } = require('../server/out/src/intellisenseCapabilities');
const root = path.resolve(__dirname, '..');
const doc = (text) => TextDocument.create('file:///vitte-editor-test.vit', 'vit', 1, text);

test('multi-root LSP project scopes select the containing folder and its own CLI config', (t) => {
  const temp = fs.mkdtempSync(path.join(require('node:os').tmpdir(), 'vitte-project-scope-'));
  t.after(() => fs.rmSync(temp, { recursive: true, force: true }));
  const alpha = path.join(temp, 'alpha');
  const nested = path.join(alpha, 'nested');
  const beta = path.join(temp, 'beta');
  fs.mkdirSync(path.join(nested, 'src'), { recursive: true });
  fs.mkdirSync(path.join(beta, '.vitte'), { recursive: true });
  fs.writeFileSync(path.join(alpha, 'vitte.toml'), 'profile = "debug"\n');
  fs.writeFileSync(path.join(beta, '.vitte', 'config'), 'profile = "release"\n');
  const alphaFile = path.join(nested, 'src', 'main.vit');
  const betaFile = path.join(beta, 'main.vit');
  fs.writeFileSync(alphaFile, 'proc main() {}\n');
  fs.writeFileSync(betaFile, 'proc main() {}\n');

  const alphaScope = resolveVitteProjectScope(require('node:url').pathToFileURL(alphaFile).toString(), [alpha, nested, beta]);
  const betaScope = resolveVitteProjectScope(require('node:url').pathToFileURL(betaFile).toString(), [alpha, beta]);
  assert.equal(alphaScope.workspaceRoot, nested, 'the most specific VS Code folder owns the file');
  assert.equal(alphaScope.projectRoot, alpha, 'nearest parent CLI config is detected');
  assert.equal(alphaScope.configFile, path.join(alpha, 'vitte.toml'));
  assert.equal(betaScope.workspaceRoot, beta);
  assert.equal(betaScope.projectRoot, beta);
  assert.equal(betaScope.configFile, path.join(beta, '.vitte', 'config'));
  assert.equal(resolveVitteProjectScope(require('node:url').pathToFileURL(alphaFile).toString(), [alpha, nested], 'build').projectRoot, path.join(nested, 'build'));
  assert.equal(resolveProjectPath('modules', alphaScope.projectRoot), path.join(alpha, 'modules'));
});
let grammar;
before(async () => {
  const wasm = fs.readFileSync(require.resolve('vscode-oniguruma/release/onig.wasm'));
  await oniguruma.loadWASM(wasm.buffer.slice(wasm.byteOffset, wasm.byteOffset + wasm.byteLength));
  const registry = new Registry({
    onigLib: Promise.resolve({ createOnigScanner: (patterns) => new oniguruma.OnigScanner(patterns), createOnigString: (s) => new oniguruma.OnigString(s) }),
    loadGrammar: async () => JSON.parse(fs.readFileSync(path.join(root, 'syntaxes/vitte.tmLanguage.json'), 'utf8')),
  });
  grammar = await registry.loadGrammar('source.vitte');
});
function scopesAt(text, needle) {
  let stack;
  let result;
  for (const line of text.split('\n')) {
    result = grammar.tokenizeLine(line, stack);
    stack = result.ruleStack;
    const index = line.indexOf(needle);
    if (index >= 0) return result.tokens.find(t => t.startIndex <= index && t.endIndex > index).scopes;
  }
  throw new Error(`Missing token ${needle}`);
}
function semanticTokens(text) {
  const document = doc(text);
  const data = buildSemanticTokens(document).data;
  const types = getSemanticTokensLegend().tokenTypes;
  let line = 0, character = 0;
  const tokens = [];
  for (let i = 0; i < data.length; i += 5) {
    line += data[i]; character = data[i] ? data[i + 1] : character + data[i + 1];
    const value = document.getText({ start: { line, character }, end: { line, character: character + data[i + 2] } });
    assert.ok(data[i + 2] > 0);
    assert.ok(!/[\r\n]/.test(value));
    const modifiers = getSemanticTokensLegend().tokenModifiers;
    const modifierBits = data[i + 4];
    tokens.push({ value, type: types[data[i + 3]], modifiers: modifiers.filter((_, index) => modifierBits & (1 << index)) });
  }
  return tokens;
}

test('semantic tokenization terminates for ordinary r identifiers and return', () => {
  execFileSync(process.execPath, ['-e', `
    const { TextDocument } = require('vscode-languageserver-textdocument');
    const { buildSemanticTokens } = require('./server/out/src/semantic');
    buildSemanticTokens(TextDocument.create('file:///test.vit', 'vit', 1, 'proc run() -> i32 { return 0; }'));
  `], { cwd: root, timeout: 5000 });
});

test('all quoted EBNF word terminals remain available in language facts', () => {
  const grammarPath = path.resolve(root, '../../grammar/vitte.ebnf');
  if (!fs.existsSync(grammarPath)) return; // Standalone extension checkout.
  const grammarText = fs.readFileSync(grammarPath, 'utf8').split('(* Lexical forms')[0];
  const vocabulary = new Set([
    ...facts.KEYWORDS,
    ...facts.CONTEXTUAL_KEYWORDS,
    ...facts.BOOL_LITERALS,
    ...facts.NIL_LITERALS,
    ...facts.PRIMITIVE_TYPES,
    ...facts.BUILTIN_FUNCTIONS,
  ]);
  for (const match of grammarText.matchAll(/"([a-z][a-z_]*)"/g)) assert.ok(vocabulary.has(match[1]), match[1]);
  assert.ok(facts.suggestCompletions('nu').includes('null'));
  assert.ok(facts.OPERATORS.includes('!'));
});

test('shared lexical masking handles nesting, attributes, raw/triple strings and UTF-16', () => {
  const source = '#![feature]\r\n/* outer /* inner */ proc hidden() {} */\r\n<<< proc zone() {} >>>\r\nlet text = r##"😀 \\" proc raw() {}"##;\r\nlet multi = """one " proc triple() {}\r\ntwo""";\r\nproc visible() { return 0; }';
  const masked = maskNonCode(source);
  assert.equal(masked.length, source.length);
  for (const hidden of ['outer', 'inner', 'hidden', 'zone', 'raw', 'triple', 'two']) assert.ok(!masked.includes(hidden), hidden);
  assert.ok(masked.includes('#![feature]'));
  assert.ok(masked.includes('proc visible()'));
  assert.deepEqual(buildCodeMask(source), scanLexical(source).mask);
  for (let i = 0; i < source.length; i++) if (source[i] === '\r' || source[i] === '\n') assert.equal(masked[i], source[i]);
});

test('completion is suppressed inside every comment and string form, including EOF', async () => {
  configureCompletions();
  assert.equal(lexicalContextAt('// pro\n', 6), 'comment');
  for (const source of ['// pro', '# pro', '/* outer /* inner */ pro', '<<< pro', 'r##"pro', '"""pro', '"pro', "'pro"]) {
    const document = doc(source);
    assert.notEqual(lexicalContextAt(source, source.length), 'code', source);
    assert.deepEqual(await provideCompletions(document, document.positionAt(source.length)), [], source);
  }
  for (const source of ['/* done */', 'r#"done"#', '"""done"""', '<<< done >>>']) assert.equal(lexicalContextAt(source, source.length), 'code', source);
  const document = doc('ret');
  assert.ok((await provideCompletions(document, document.positionAt(3))).some(item => item.label === 'return'));
});

test('completion follows local types, fields, variants, generics and nested lexical scopes', async () => {
  configureCompletions();
  const source = [
    'form Point { x: i32, y: string }',
    'pick Result<T> { Ok(T), Err(Map<string, T>) }',
    'proc inspect<T>(point: Point, value: T) { point. }',
  ].join('\n');
  const document = doc(source);
  const memberOffset = source.indexOf('point.') + 'point.'.length;
  const members = await provideCompletions(document, document.positionAt(memberOffset));
  assert.deepEqual(members.map(item => item.label).sort(), ['x', 'y']);

  const variantSource = source.replace('point. }', 'Result:: }');
  const variantDocument = doc(variantSource);
  const variants = await provideCompletions(variantDocument, variantDocument.positionAt(variantSource.indexOf('Result::') + 'Result::'.length));
  assert.deepEqual(variants.map(item => item.label).sort(), ['Err', 'Ok']);
  assert.match(variants.find(item => item.label === 'Err').textEdit.newText, /\$\{1:value1\}/);

  const constructorSource = 'form Point { x: i32, y: string }\nproc make() { let point = Poi; }';
  const constructorDocument = doc(constructorSource);
  const constructorOffset = constructorSource.indexOf('Poi') + 3;
  const constructor = (await provideCompletions(constructorDocument, constructorDocument.positionAt(constructorOffset))).find(item => item.label === 'Point');
  assert.ok(constructor);
  assert.match(constructor.textEdit.newText, /Point \{ x: \$\{1:x\}, y: \$\{2:y\} \}/);

  const scopes = 'proc scope() {\n let outer: i32 = 0;\n {\n  let inner: i32 = 1;\n  inn\n }\n out\n}';
  const scopeDocument = doc(scopes);
  const innerItems = await provideCompletions(scopeDocument, scopeDocument.positionAt(scopes.indexOf('inn') + 3));
  assert.ok(innerItems.some(item => item.label === 'inner'));
  const outerItems = await provideCompletions(scopeDocument, scopeDocument.positionAt(scopes.lastIndexOf('out') + 3));
  assert.ok(outerItems.some(item => item.label === 'outer'));
  assert.ok(!outerItems.some(item => item.label === 'inner'));

  const shadowSource = 'proc shadow() { let value: i32 = 0; { let value: string = ""; val } }';
  const shadowDocument = doc(shadowSource);
  const shadowItems = await provideCompletions(shadowDocument, shadowDocument.positionAt(shadowSource.lastIndexOf('val') + 3));
  assert.equal(shadowItems.find(item => item.label === 'value').detail, 'liaison locale: string');

  const implSource = 'form Box<T> { value: T }\nimpl<T> Box<T> { proc get(self: Box<T>) { let value: T; } }';
  const implDocument = doc(implSource);
  const genericOffset = implSource.lastIndexOf('T;') + 1;
  const genericItems = await provideCompletions(implDocument, implDocument.positionAt(genericOffset));
  assert.ok(genericItems.some(item => item.label === 'T' && item.detail === 'Paramètre générique en portée'));

  const genericTraitSource = 'trait Readable { proc read() -> i32; }\nproc inspect<T: Readable>(value: T) { value.re }';
  const genericTraitDocument = doc(genericTraitSource);
  const genericTraitOffset = genericTraitSource.indexOf('value.re') + 'value.re'.length;
  const genericTraitItems = await provideCompletions(genericTraitDocument, genericTraitDocument.positionAt(genericTraitOffset));
  assert.ok(genericTraitItems.some(item => item.label === 'read' && item.kind === CompletionItemKind.Method));
  const globalTraitSource = 'trait Readable { proc read() -> i32; }\nproc other() { re }';
  const globalTraitItems = await provideCompletions(doc(globalTraitSource), doc(globalTraitSource).positionAt(globalTraitSource.indexOf('re }') + 2));
  assert.ok(!globalTraitItems.some(item => item.label === 'read'), 'trait declarations are not offered as global functions');

  const unsupportedImplSource = 'form Box { value: i32 }\nimpl Box { proc get(self: Box) {} }\nproc inspect(box: Box) { box.ge }';
  const unsupportedImplDocument = doc(unsupportedImplSource);
  const unsupportedImplOffset = unsupportedImplSource.indexOf('box.ge') + 'box.ge'.length;
  const unsupportedImplItems = await provideCompletions(unsupportedImplDocument, unsupportedImplDocument.positionAt(unsupportedImplOffset));
  assert.equal(unsupportedImplItems[0].data.source, 'unavailable');
  assert.match(unsupportedImplItems[0].label, /méthode n’est pas résolue/);

  const aliasSource = 'form Point { x: i32 }\ntype Alias = Point;\nproc make() { let p = Ali; }';
  const aliasDocument = doc(aliasSource);
  const aliasItems = await provideCompletions(aliasDocument, aliasDocument.positionAt(aliasSource.indexOf('Ali;') + 3));
  assert.match(aliasItems.find(item => item.label === 'Alias').textEdit.newText, /Alias \{ x: \$\{1:x\} \}/);

  const attributeDocument = doc('#[co');
  const attributeItems = await provideCompletions(attributeDocument, attributeDocument.positionAt(4));
  assert.ok(attributeItems.some(item => item.label === 'cold'));

  const unresolvedMemberSource = 'proc inspect(value: MissingType) { value. }';
  const unresolvedMemberDocument = doc(unresolvedMemberSource);
  const unresolvedMemberItems = await provideCompletions(unresolvedMemberDocument, unresolvedMemberDocument.positionAt(unresolvedMemberSource.indexOf('value.') + 6));
  assert.equal(unresolvedMemberItems[0].data.source, 'unavailable');
  assert.match(unresolvedMemberItems[0].label, /Type du récepteur non résolu/);
});

test('completion resolves real module paths, public exports, aliases and imported members', async (t) => {
  const project = fs.mkdtempSync(path.join(require('node:os').tmpdir(), 'vitte-completion-'));
  t.after(() => fs.rmSync(project, { recursive: true, force: true }));
  fs.writeFileSync(path.join(project, 'pkg.vit'), [
    'export form Point { x: i32, hidden: string }',
    'export proc compute(value: i32) -> i32 { return value; }',
    'export trait Readable { proc read() -> i32; }',
    'impl Readable for Point { proc read() -> i32 { return 1; } }',
    'proc private_proc() {}',
    'export const MAX: i32 = 4;',
    'const SECRET: i32 = 0;',
  ].join('\n'));
  const uri = require('node:url').pathToFileURL(path.join(project, 'main.vit')).toString();
  const environment = { projectRoot: project, workspaceRoots: [project] };

  const pathDocument = TextDocument.create(uri, 'vit', 1, 'use pk');
  const pathItems = await provideCompletions(pathDocument, pathDocument.positionAt(6), environment);
  assert.ok(pathItems.some(item => item.label === 'pkg'));
  const configuredRoot = path.join(project, 'extra-modules');
  fs.mkdirSync(configuredRoot);
  fs.writeFileSync(path.join(configuredRoot, 'configured.vit'), 'export proc configured() {}');
  const configuredSource = 'use conf';
  const configuredDocument = TextDocument.create(uri, 'vit', 1, configuredSource);
  const configuredItems = await provideCompletions(configuredDocument, configuredDocument.positionAt(configuredSource.length), { modulePaths: [configuredRoot] });
  assert.ok(configuredItems.some(item => item.label === 'configured'));
  const missingPath = TextDocument.create(uri, 'vit', 1, 'use absent::');
  const missingPathItems = await provideCompletions(missingPath, missingPath.positionAt(missingPath.getText().length), environment);
  assert.equal(missingPathItems[0].data.source, 'unavailable');
  assert.equal(missingPathItems[0].textEdit.newText, '', 'unavailable-result selection leaves source unchanged');

  const importDocument = TextDocument.create(uri, 'vit', 1, 'use pkg::{ com');
  const importItems = await provideCompletions(importDocument, importDocument.positionAt(importDocument.getText().length), environment);
  assert.ok(importItems.some(item => item.label === 'compute'));
  assert.ok(!importItems.some(item => item.label === 'private_proc'));

  const importedSource = 'use pkg::{compute as run, Point, MAX};\nru';
  const importedDocument = TextDocument.create(uri, 'vit', 1, importedSource);
  const items = await provideCompletions(importedDocument, importedDocument.positionAt(importedSource.length), environment);
  assert.ok(items.some(item => item.label === 'run' && item.textEdit.newText.startsWith('run(')));
  assert.ok(!items.some(item => item.label === 'compute' || item.label === 'private_proc' || item.label === 'SECRET'));

  const constantSource = 'use pkg::{MAX};\nMA';
  const constantDocument = TextDocument.create(uri, 'vit', 1, constantSource);
  const constantItems = await provideCompletions(constantDocument, constantDocument.positionAt(constantSource.length), environment);
  assert.equal(constantItems.find(item => item.label === 'MAX').kind, CompletionItemKind.Constant);
  assert.equal(constantItems.find(item => item.label === 'MAX').detail, 'constante publique: i32');

  const memberSource = 'use pkg::{Point, Readable};\nproc inspect(point: Point) { point. }';
  const memberDocument = TextDocument.create(uri, 'vit', 1, memberSource);
  const memberItems = await provideCompletions(memberDocument, memberDocument.positionAt(memberSource.indexOf('point.') + 6), environment);
  assert.deepEqual(memberItems.map(item => item.label).sort(), ['hidden', 'read', 'x']);

  const staticSource = 'use pkg::{Point, Readable};\nproc make() { Point:: }';
  const staticDocument = TextDocument.create(uri, 'vit', 1, staticSource);
  const staticItems = await provideCompletions(staticDocument, staticDocument.positionAt(staticSource.indexOf('Point::') + 'Point::'.length), environment);
  assert.equal(staticItems[0].data.source, 'unavailable');
  assert.match(staticItems[0].label, /Fonctions associées non annoncées/);

  const methodSource = 'use pkg::{Point, Readable};\nproc inspect(point: Point) { point.read(';
  const methodDocument = TextDocument.create(uri, 'vit', 1, methodSource);
  const methodHelp = await provideSignatureHelpWithImports(methodDocument, methodDocument.positionAt(methodSource.length), environment);
  assert.ok(methodHelp);
  assert.match(methodHelp.signatures[0].label, /read\(\) -> i32/);

  const ctorSource = 'use pkg::{Point};\nproc make() { let point = Point {  }; }';
  const ctorDocument = TextDocument.create(uri, 'vit', 1, ctorSource);
  const ctorItems = await provideCompletions(ctorDocument, ctorDocument.positionAt(ctorSource.indexOf('{  }') + 2), environment);
  assert.ok(ctorItems.some(item => item.label === 'x'));
  assert.ok(ctorItems.some(item => item.label === 'hidden'));
});

test('indexing ignores declarations in comments/strings and preserves locations after emoji', () => {
  clearIndex();
  const source = '/* outer /* inner */ proc hidden() {} */\n<<< proc zone() {} >>>\nlet s = "😀"; proc visible() -> i32 { return 0; }';
  const document = doc(source);
  indexDocument(document);
  const symbols = getDocumentIndex(document.uri);
  assert.ok(!symbols.some(s => ['hidden', 'zone'].includes(s.name)));
  const visible = symbols.find(s => s.name === 'visible');
  assert.ok(visible);
  assert.deepEqual({ line: visible.line, character: visible.character }, document.positionAt(source.indexOf('proc visible')));
  clearIndex();
});

test('signature help counts arguments after strings containing surrogate pairs', () => {
  const source = 'proc call(a: str, b: i32) -> i32 { return b; }\nproc main() { call("😀,", ';
  const document = doc(source);
  const help = provideSignatureHelp(document, document.positionAt(source.length));
  assert.ok(help);
  assert.equal(help.activeParameter, 1);
});

test('signature help tracks nested incomplete calls, generics, aliases and parameter docs', async (t) => {
  const source = [
    '/// Combines two values.',
    '/// Params:',
    '/// left: first operand',
    '/// right: second operand',
    'proc combine<T>(left: T, right: T) -> T { return left; }',
    'proc wrapper() { combine<int>(unknown(1, ',
  ].join('\n');
  const document = doc(source);
  const help = provideSignatureHelp(document, document.positionAt(source.length));
  assert.ok(help);
  assert.match(help.signatures[0].label, /combine<T>\(left: T, right: T\)/);
  assert.equal(help.activeParameter, 0);
  assert.equal(help.signatures[0].parameters[0].documentation.value, 'first operand');

  const project = fs.mkdtempSync(path.join(require('node:os').tmpdir(), 'vitte-signature-'));
  t.after(() => fs.rmSync(project, { recursive: true, force: true }));
  fs.writeFileSync(path.join(project, 'pkg.vit'), 'export proc combine(left: i32, right: string) -> i32 { return left; }');
  const uri = require('node:url').pathToFileURL(path.join(project, 'main.vit')).toString();
  const importedSource = 'use pkg::{combine as join};\nproc main() { join(1, ';
  const importedDocument = TextDocument.create(uri, 'vit', 1, importedSource);
  const importedHelp = await provideSignatureHelpWithImports(importedDocument, importedDocument.positionAt(importedSource.length), { projectRoot: project });
  assert.ok(importedHelp);
  assert.match(importedHelp.signatures[0].label, /join\(left: i32, right: string\)/);
  assert.equal(importedHelp.activeParameter, 1);

  const callableSource = 'proc main() { let callback: proc(value: i32) -> bool = nil; callback(';
  const callableDocument = doc(callableSource);
  const callableHelp = provideSignatureHelp(callableDocument, callableDocument.positionAt(callableSource.length));
  assert.ok(callableHelp);
  assert.match(callableHelp.signatures[0].label, /callback\(value: i32\) -> bool/);
  const callableItems = await provideCompletions(callableDocument, callableDocument.positionAt(callableSource.length));
  assert.match(callableItems.find(item => item.label === 'callback').textEdit.newText, /callback\(\$\{1:value\}\)/);
});

test('form initializer signature help follows named fields and documentation', () => {
  const source = '/// A coordinate.\nform Point {\n /// Horizontal axis.\n x: i32,\n y: i32,\n}\nproc make() { let p = Point { x: 1, y: ';
  const document = doc(source);
  const help = provideSignatureHelp(document, document.positionAt(source.length));
  assert.ok(help);
  assert.equal(help.activeParameter, 1);
  assert.match(help.signatures[0].label, /Point \{ x: i32, y: i32 \}/);
  assert.equal(help.signatures[0].parameters[0].documentation.value, 'Horizontal axis.');
});

test('hover explains builtins, keywords and unresolved user symbols', () => {
  const source = 'proc main() -> i32 { return missing_value; }';
  const document = doc(source);
  const returnHover = provideHover(document, document.positionAt(source.indexOf('return')));
  assert.match(returnHover.contents.value, /Retourne depuis une fonction/);
  const typeHover = provideHover(document, document.positionAt(source.indexOf('i32')));
  assert.match(typeHover.contents.value, /Type primitif/);
  const missingHover = provideHover(document, document.positionAt(source.indexOf('missing_value') + 2));
  assert.match(missingHover.contents.value, /indisponible/i);
  assert.equal(provideHover(doc('// print is only a comment'), { line: 0, character: 5 }), null);
  const builtinHover = provideHover(doc('proc main() { print("x"); }'), { line: 0, character: 16 });
  assert.match(builtinHover.contents.value, /proc print\(value: string\) -> void/);
});

test('source hover resolves declarations, signatures, types, docs, fields, variants and simple inference', () => {
  const source = [
    '/// Coordinates in the plane.',
    'form Point {',
    ' /// Horizontal axis.',
    ' x: i32,',
    '}',
    'pick State { Ready, Failed(string) }',
    '/// Adds a value.',
    '/// left: The input amount.',
    'proc add(left: i32, point: Point) -> i32 { return left; }',
    'proc run(point: Point) { let count = 7; point.x; State::Ready; add(count, point); }',
  ].join('\n');
  const document = doc(source);
  const hover = (needle, offset = 1) => provideHover(document, document.positionAt(source.indexOf(needle) + offset)).contents.value;
  assert.match(hover('add(left', 1), /proc add\(left: i32, point: Point\) -> i32/);
  assert.match(hover('left: i32', 1), /paramètre left: i32/);
  assert.match(hover('return left', 8), /input amount/);
  assert.match(hover('point.x', 6), /x: i32/);
  assert.match(hover('State::Ready', 7), /Ready/);
  assert.match(hover('count = 7', 1), /Type annoté ou déduit.*int/);
  assert.match(hover('Point {', 1), /form Point/);
});

test('navigation resolves local scopes, trait implementations, imports, aliases and workspace references', async (t) => {
  const project = fs.mkdtempSync(path.join(require('node:os').tmpdir(), 'vitte-navigation-'));
  t.after(() => fs.rmSync(project, { recursive: true, force: true }));
  const packageSource = [
    'export form Point { x: i32 }',
    'export type Place = Point;',
    'export trait Readable { proc read() -> i32; }',
    'impl Readable for Point { proc read(receiver: Point) -> i32 { return 1; } }',
    'export proc compute(value: i32) -> i32 { return value; }',
  ].join('\n');
  fs.writeFileSync(path.join(project, 'pkg.vit'), packageSource);
  const environment = { projectRoot: project, workspaceRoots: [project] };
  const source = [
    'use pkg::{Point as P, Place as PlaceAlias, Readable, compute as run};',
    'proc inspect(point: P, place: PlaceAlias) { let value: i32 = 0; { let value: string = ""; value; } value; point.x; point.read(); run(value); }',
    'proc other(point: P) { run(1); }',
  ].join('\n');
  const uri = require('node:url').pathToFileURL(path.join(project, 'main.vit')).toString();
  const document = TextDocument.create(uri, 'vit', 1, source);
  const at = (needle, delta = 0) => document.positionAt(source.indexOf(needle) + delta);
  const innerValueDefinition = definitionAtPosition(document, at('value;', 1));
  assert.deepEqual(innerValueDefinition[0].range.start, document.positionAt(source.indexOf('value: string')));
  const outerValuePosition = document.positionAt(source.lastIndexOf('value;') + 1);
  const outerDefinition = definitionAtPosition(document, outerValuePosition);
  assert.deepEqual(outerDefinition[0].range.start, document.positionAt(source.indexOf('value: i32')));
  const refs = await referencesAtPositionWithWorkspace(document, outerValuePosition, environment, [], true);
  assert.equal(refs.locations.length, 3, 'the inner shadowed binding is excluded');

  const importedHover = await provideHoverWithImports(document, at('run(value', 1), environment);
  assert.match(importedHover.contents.value, /proc run\(value: i32\) -> i32/);
  assert.match(importedHover.contents.value, /Alias de `compute`/);
  const importedDefinition = await definitionAtPositionWithImports(document, at('run(value', 1), environment);
  assert.equal(importedDefinition.locations[0].uri, require('node:url').pathToFileURL(path.join(project, 'pkg.vit')).toString());
  const importedDeclaration = await declarationAtPositionWithImports(document, at('run(value', 1), environment);
  assert.equal(importedDeclaration.locations[0].uri, importedDefinition.locations[0].uri);
  const typeDefinition = await typeDefinitionAtPositionWithImports(document, at('point.x', 1), environment);
  assert.equal(typeDefinition.locations[0].uri, require('node:url').pathToFileURL(path.join(project, 'pkg.vit')).toString());
  const aliasTypeDefinition = await typeDefinitionAtPositionWithImports(document, at('PlaceAlias)', 2), environment);
  assert.equal(aliasTypeDefinition.locations[0].range.start.character, packageSource.indexOf('Point {'));
  const fieldDefinition = await definitionAtPositionWithImports(document, at('point.x', 6), environment);
  assert.equal(fieldDefinition.locations[0].range.start.line, 0);
  assert.equal(fieldDefinition.locations[0].range.start.character, packageSource.indexOf('x: i32'));

  const traitPosition = document.positionAt(source.indexOf('Readable') + 2);
  const implementations = await implementationAtPositionWithImports(document, traitPosition, environment, []);
  assert.ok(implementations.locations.some(location => location.uri.endsWith('/pkg.vit')));
  const methodImplementations = await implementationAtPositionWithImports(document, at('point.read', 8), environment, []);
  assert.ok(methodImplementations.locations.some(location => location.uri.endsWith('/pkg.vit')));
  const workspaceRefs = await referencesAtPositionWithWorkspace(document, at('run(value', 1), environment, [
    { uri: require('node:url').pathToFileURL(path.join(project, 'other.vit')).toString(), doc: TextDocument.create(require('node:url').pathToFileURL(path.join(project, 'other.vit')).toString(), 'vit', 1, 'use pkg::{compute as run}; proc main() { run(2); }') },
  ], true);
  assert.ok(workspaceRefs.locations.some(location => location.uri.endsWith('/other.vit')));

  const outline = documentSymbols(doc('space editor;\nform Point { x: i32 }\npick State { Ready, Failed(string) }\ntrait Readable { proc read() -> i32; }\nimpl Readable for Point { proc read(receiver: Point) {} }\nproc run(point: Point) { let value = 1; }'));
  const flatten = items => items.flatMap(item => [item, ...flatten(item.children ?? [])]);
  const outlineItems = flatten(outline);
  assert.ok(outlineItems.some(item => item.name === 'x' && item.kind === SymbolKind.Field));
  assert.ok(outlineItems.some(item => item.name === 'Ready' && item.kind === SymbolKind.EnumMember));
  assert.ok(outlineItems.some(item => item.name === 'impl Readable for Point'));
  assert.ok(outlineItems.some(item => item.name === 'editor' && item.kind === SymbolKind.Module));
  assert.ok(outlineItems.some(item => item.name === 'value'));
  const grouped = workspaceSymbols('read', [{ uri, doc: outline.length ? TextDocument.create(uri, 'vit', 1, 'trait Readable { proc read() {} }\nproc caller() {}') : document }], 20);
  assert.ok(grouped.some(item => item.name === 'read' && item.containerName === 'Readable'));
  const unresolved = await definitionAtPositionWithImports(doc('proc main() { mystery(); }'), { line: 0, character: 17 }, {});
  assert.equal(unresolved.locations.length, 0);
  assert.match(unresolved.unavailableReason, /n’est pas résolue/i);
  const overloaded = doc('proc choose(value: i32) {}\nproc choose(value: string) {}\nproc main() { choose(1); }');
  const ambiguous = await definitionAtPositionWithImports(overloaded, overloaded.positionAt(overloaded.getText().indexOf('choose(1)') + 1), {});
  assert.equal(ambiguous.locations.length, 0);
  assert.match(ambiguous.unavailableReason, /n’est pas résolue/i);
});

test('rename resolves symbol identity, keeps aliases scoped, and ignores comments, strings and shadowed bindings', async (t) => {
  const source = [
    'export proc compute(value: i32) -> i32 { return value; }',
    'proc main() { compute(1); { let compute = 2; compute; } let text = "compute"; } // compute',
  ].join('\n');
  const project = fs.mkdtempSync(path.join(require('node:os').tmpdir(), 'vitte-rename-'));
  t.after(() => fs.rmSync(project, { recursive: true, force: true }));
  const nestedRoot = path.join(project, 'ambiguous');
  const ambiguousMove = findUniqueModulePathFileMove(
    path.join(nestedRoot, 'thing.vit'),
    path.join(nestedRoot, 'renamed.vit'),
    [project, nestedRoot],
  );
  assert.equal(ambiguousMove.mapping, undefined);
  assert.equal(ambiguousMove.candidateCount, 2, 'nested workspace and module roots must not guess which module path to update');
  const sourcePath = path.join(project, 'api.vit');
  fs.writeFileSync(sourcePath, source);
  const sourceUri = require('node:url').pathToFileURL(sourcePath).toString();
  const sourceDoc = TextDocument.create(sourceUri, 'vit', 1, source);
  const importerText = 'use api::{compute as run};\nproc call() { run(3); }';
  const importerUri = require('node:url').pathToFileURL(path.join(project, 'main.vit')).toString();
  const importerDoc = TextDocument.create(importerUri, 'vit', 1, importerText);
  const environment = { projectRoot: project, workspaceRoots: [project] };

  const localPosition = sourceDoc.positionAt(source.indexOf('compute(1)') + 1);
  const localTarget = await resolveRenameTarget(sourceDoc, localPosition, environment);
  assert.equal(localTarget.target?.kind, 'source');
  const localReferences = await renameReferencesForTarget(sourceDoc, localPosition, localTarget.target, environment, [{ uri: sourceUri, doc: sourceDoc }]);
  assert.deepEqual(localReferences.locations.map(location => sourceDoc.getText(location.range)), ['compute', 'compute']);
  assert.equal(validateRenameName('return')?.includes('mot réservé'), true);
  assert.equal(validateRenameLocations('compute', 'computed', localReferences.locations, [{ uri: sourceUri, doc: sourceDoc }]), undefined);
  const conflictDoc = doc(`${source}\nconst computed = 0;`);
  assert.match(validateRenameLocations('compute', 'computed', localReferences.locations, [{ uri: sourceUri, doc: conflictDoc }]) ?? '', /existe déjà/);

  const aliasPosition = importerDoc.positionAt(importerText.indexOf('run(3)') + 1);
  const aliasTarget = await resolveRenameTarget(importerDoc, aliasPosition, environment);
  assert.equal(aliasTarget.target?.kind, 'import-alias');
  const aliasReferences = await renameReferencesForTarget(importerDoc, aliasPosition, aliasTarget.target, environment, [{ uri: importerUri, doc: importerDoc }]);
  assert.deepEqual(aliasReferences.locations.map(location => importerDoc.getText(location.range)), ['run', 'run']);
  assert.equal(validateRenameLocations('run', 'invoke', aliasReferences.locations, [{ uri: importerUri, doc: importerDoc }]), undefined);

  const sourceTarget = await resolveRenameTarget(sourceDoc, sourceDoc.positionAt(source.indexOf('export proc compute') + 12), environment);
  const crossFile = await renameReferencesForTarget(sourceDoc, sourceDoc.positionAt(source.indexOf('export proc compute') + 12), sourceTarget.target, environment, [
    { uri: sourceUri, doc: sourceDoc },
    { uri: importerUri, doc: importerDoc },
  ]);
  const sourceNames = crossFile.locations.filter(location => location.uri === sourceUri).map(location => sourceDoc.getText(location.range));
  const importedNames = crossFile.locations.filter(location => location.uri === importerUri).map(location => importerDoc.getText(location.range));
  assert.deepEqual(sourceNames, ['compute', 'compute']);
  assert.deepEqual(importedNames, ['compute'], 'renaming the export preserves the local import alias and its uses');

  const commentDoc = doc('// renameMe\nproc main() { let value = "renameMe"; }');
  const invalid = await resolveRenameTarget(commentDoc, { line: 0, character: 5 }, {});
  assert.equal(invalid.target, undefined);
  assert.match(invalid.unavailableReason, /identifiant Vitte/);

  const constantsSource = 'export const limit: i32 = 3;\nproc useLimit() { limit; { let limit = 4; limit; } }';
  const constantsPath = path.join(project, 'constants.vit');
  fs.writeFileSync(constantsPath, constantsSource);
  const constantsUri = require('node:url').pathToFileURL(constantsPath).toString();
  const constantsDoc = TextDocument.create(constantsUri, 'vit', 1, constantsSource);
  const constantsImporterUri = require('node:url').pathToFileURL(path.join(project, 'constant-use.vit')).toString();
  const constantsImporterText = 'use constants::{limit as max};\nproc readLimit() { max; }';
  const constantsImporter = TextDocument.create(constantsImporterUri, 'vit', 1, constantsImporterText);
  const globalPosition = constantsDoc.positionAt(constantsSource.indexOf('limit;', constantsSource.indexOf('proc useLimit')));
  const globalTarget = await resolveRenameTarget(constantsDoc, globalPosition, environment);
  assert.equal(globalTarget.target?.kind, 'source');
  const globalReferences = await renameReferencesForTarget(constantsDoc, globalPosition, globalTarget.target, environment, [
    { uri: constantsUri, doc: constantsDoc },
    { uri: constantsImporterUri, doc: constantsImporter },
  ]);
  assert.equal(globalReferences.locations.filter(location => location.uri === constantsUri).length, 2, 'the local shadow is excluded');
  const importedConstant = globalReferences.locations.find(location => location.uri === constantsImporterUri);
  assert.equal(constantsImporter.getText(importedConstant.range), 'limit', 'the exported name changes while its alias and use stay local');

  const namespacedGlobals = doc('space alpha { export const flag: i32 = 1; proc readAlpha() { flag; } }\nspace beta { proc readBeta() { flag; } }');
  const betaReference = namespacedGlobals.positionAt(namespacedGlobals.getText().lastIndexOf('flag'));
  const unresolvedNamespaceTarget = await resolveRenameTarget(namespacedGlobals, betaReference, {});
  assert.equal(unresolvedNamespaceTarget.target, undefined, 'a same-named global in another namespace is not treated as a reference');
});

test('semantic tokens classify types, mutable bindings, fields and canonical numbers', () => {
  const text = 'form Point { x: i32 }\r\nproc run() -> i32 {\r\n let mut result: i32 = 0XFF;\r\n /* outer\r\n\r\n /* inner */ tail */\r\n return result;\r\n}\r\n';
  const tokens = semanticTokens(text);
  for (const [value, type] of [['i32', 'type'], ['result', 'variable'], ['x', 'property'], ['return', 'keyword'], ['0XFF', 'number']]) assert.ok(tokens.some(t => t.value === value && t.type === type), `${value}: ${type}`);
  const numbers = '0x1.fp+2 1e3 1. 0B101 0O755 1_000 0..4 0..=4';
  assert.deepEqual(semanticTokens(numbers).filter(t => t.type === 'number').map(t => t.value), ['0x1.fp+2', '1e3', '1.', '0B101', '0O755', '1_000', '0', '4', '0', '4']);
});

test('semantic tokenization colors logical word operators and ordinary identifiers', () => {
  const tokens = semanticTokens('proc run(left: bool, right: bool) -> bool { return left and not right or left is true; }');
  for (const word of ['and', 'not', 'or', 'is']) assert.ok(tokens.some(token => token.value === word && token.type === 'operator'), word);
  assert.ok(tokens.some(token => token.value === 'left' && token.type === 'parameter'));
});

test('semantic tokens classify declarations, attributes, variants, modifiers and punctuation while masking strings and comments', () => {
  const text = 'pub async proc run(value: bool) { #[hot::cold] let mut item = value && true; const fixed = 1; /* run(fake) */ let text = "item && run"; }\npick State { Ready, Busy }';
  const tokens = semanticTokens(text);
  const token = (value, type) => tokens.find(candidate => candidate.value === value && (!type || candidate.type === type));
  assert.equal(token('run', 'function')?.modifiers.includes('declaration'), true);
  assert.equal(token('run', 'function')?.modifiers.includes('public'), true);
  assert.equal(token('run', 'function')?.modifiers.includes('async'), true);
  assert.ok(token('hot', 'decorator'));
  assert.ok(token('cold', 'decorator'));
  assert.ok(token('Ready', 'enumMember'));
  assert.ok(token('&&', 'operator'));
  assert.equal(token('item', 'variable')?.modifiers.includes('modification'), true);
  assert.equal(token('fixed', 'variable')?.modifiers.includes('readonly'), true);
  assert.ok(!tokens.some(candidate => candidate.value === 'fake'));
  assert.equal(tokens.filter(candidate => candidate.value === 'item' && candidate.type === 'variable').length, 1, 'string contents do not leak into identifier tokens');
  const legend = getSemanticTokensLegend();
  assert.ok(legend.tokenModifiers.includes('readonly'));
  assert.ok(legend.tokenModifiers.includes('modification'));
});

test('inlay hints show only literal types and uniquely resolved parameter names', async () => {
  const document = doc('proc combine(left: i32, right: bool) -> i32 { return left; }\nproc main() { let count = 3; combine("word", true); }');
  const hints = await provideInlayHints(document, { start: { line: 0, character: 0 }, end: { line: 1, character: 100 } }, {}, {});
  assert.ok(hints.some(hint => hint.label === ': int'));
  assert.ok(hints.some(hint => hint.label === 'left:'));
  assert.ok(hints.some(hint => hint.label === 'right:'));
  assert.ok(!hints.some(hint => /^arg\d+:$/.test(hint.label)));
  assert.deepEqual(await provideInlayHints(document, { start: { line: 0, character: 0 }, end: { line: 1, character: 100 } }, { typeHints: false, parameterHints: false }, {}), []);
});

test('folding and selection ranges follow lexical comments and canonical brace scopes', () => {
  const document = doc('proc outer() {\n  /* comment {\n     body }\n  */\n  let text = "{ ignored }";\n  if true {\n    return 1;\n  }\n}\n');
  const folds = provideFoldingRanges(document);
  assert.ok(folds.some(range => range.startLine === 0 && range.endLine === 8));
  assert.ok(folds.some(range => range.kind === 'comment' && range.startLine === 1 && range.endLine === 3));
  assert.ok(folds.some(range => range.startLine === 5 && range.endLine === 7));
  const selections = provideSelectionRanges(document, [{ line: 6, character: 11 }]);
  const chain = [];
  for (let range = selections[0]; range; range = range.parent) chain.push(document.getText(range.range));
  assert.ok(chain.some(value => value.includes('return 1;')));
  assert.ok(chain.some(value => value.includes('if true')));
  assert.ok(chain.some(value => value.includes('proc outer')));
});

test('document links resolve one workspace module and omit ambiguous or external targets', async () => {
  const workspace = fs.mkdtempSync(path.join(require('node:os').tmpdir(), 'vitte-doclinks-'));
  try {
    fs.writeFileSync(path.join(workspace, 'lib.vit'), 'export proc api() {}');
    const importer = TextDocument.create(require('node:url').pathToFileURL(path.join(workspace, 'main.vit')).toString(), 'vit', 1, 'use lib;\n// use missing;\nlet text = "use missing";');
    const links = await provideDocumentLinks(importer, { workspaceRoots: [workspace] });
    assert.equal(links.length, 1);
    assert.match(links[0].target, /lib\.vit$/);
    assert.equal(importer.getText(links[0].range), 'lib');
    fs.writeFileSync(path.join(workspace, 'lib.vitte'), 'export proc api() {}');
    assert.deepEqual(await provideDocumentLinks(importer, { workspaceRoots: [workspace] }), [], 'ambiguous module mappings are not linked');
  } finally {
    fs.rmSync(workspace, { recursive: true, force: true });
  }
});

test('TextMate highlights current keywords and every canonical numeric form', () => {
  for (const word of ['return', 'give', 'ensures', 'requires']) assert.ok(scopesAt(word, word).some(s => s.startsWith('keyword.')), word);
  for (const literal of ['0XFF', '0B101', '0O755', '0x1.fp+2', '1e3', '1.', '1_000']) {
    const result = grammar.tokenizeLine(literal);
    assert.equal(result.tokens.length, 1, literal);
    assert.ok(result.tokens[0].scopes.some(s => s.startsWith('constant.numeric.')), literal);
  }
  const ranges = grammar.tokenizeLine('0..4').tokens.filter(t => t.scopes.some(s => s.startsWith('constant.numeric.')));
  assert.deepEqual(ranges.map(t => [t.startIndex, t.endIndex]), [[0, 1], [3, 4]]);
});

test('TextMate retains nested comments and raw strings until their real terminator', () => {
  assert.ok(scopesAt('/* outer /* inner */ still_comment */ return', 'still_comment').some(s => s.startsWith('comment.')));
  assert.ok(scopesAt('r##"inner " still_string"## return', 'still_string').some(s => s.startsWith('string.')));
  assert.ok(scopesAt('r"text" return', 'return').includes('keyword.control.vitte'));
  for (const escape of ['\\a', '\\u0041', '\\U0001F600']) assert.ok(scopesAt(`"${escape}"`, escape).includes('constant.character.escape.vitte'), escape);
});

test('snippets use parser-compatible match, pick, import and external declarations', () => {
  const snippets = JSON.parse(fs.readFileSync(path.join(root, 'snippets/vitte.json'), 'utf8'));
  const byPrefix = Object.fromEntries(Object.values(snippets).map(snippet => [snippet.prefix, snippet]));
  assert.ok(byPrefix.match.body.join('\n').includes(' => {'));
  assert.ok(!byPrefix.pick.body.join('\n').includes('case '));
  assert.ok(byPrefix.extern.body.join('\n').startsWith('extern "C" proc '));
  assert.match(byPrefix.use.body.join('\n'), /use .*::\{.*\};/);
  assert.match(byPrefix.module.body.join('\n'), /^space .*;\n\n\$0$/);
  for (const prefix of ['use', 'useas', 'exports', 'type', 'const', 'return']) assert.ok(byPrefix[prefix].body.at(-1).trim().endsWith(';'), prefix);
  const allBodies = Object.values(snippets).map(snippet => snippet.body.join('\n')).join('\n');
  assert.doesNotMatch(allBodies, /\bgive\b|<<<|package\/module\.\$\{/);

  const sharedCatalogPath = path.resolve(root, '../../editors/snippets/catalog.json');
  if (fs.existsSync(sharedCatalogPath)) {
    const catalog = JSON.parse(fs.readFileSync(sharedCatalogPath, 'utf8'));
    const expected = Object.fromEntries(catalog.map(entry => [entry.description, {
      scope: 'vit,vitte', prefix: entry.prefix, body: entry.body.split('\n'), description: entry.description,
    }]));
    assert.deepEqual(snippets, expected, 'the packaged VS Code snippets must be generated from the maintained shared catalog');
  }
});

test('language configuration uses canonical comment pairs and stable block indentation', () => {
  const config = JSON.parse(fs.readFileSync(path.join(root, 'language-configuration.json'), 'utf8'));
  assert.equal(config.comments.lineComment, '//');
  assert.deepEqual(config.comments.blockComment, ['/*', '*/']);
  for (const pair of [['{', '}'], ['[', ']'], ['(', ')']]) assert.ok(config.brackets.some(item => JSON.stringify(item) === JSON.stringify(pair)));
  for (const pair of [['{', '}'], ['[', ']'], ['(', ')'], ['/*', '*/']]) {
    assert.ok(config.autoClosingPairs.some(item => item.open === pair[0] && item.close === pair[1]), `missing auto-close ${pair.join('')}`);
    assert.ok(config.surroundingPairs.some(item => item.open === pair[0] && item.close === pair[1]), `missing surround ${pair.join('')}`);
  }
  assert.ok(config.onEnterRules.some(rule => /proc\|form\|pick/.test(rule.beforeText ?? '')));
  assert.ok(config.indentationRules.increaseIndentPattern.includes('\\{'));
  assert.doesNotMatch(JSON.stringify(config), /<<<|>>>|\bcompiler\||\bquery\|/);
});

test('versioned capability matrix records evidence and gates advertised behavior', () => {
  const matrix = JSON.parse(fs.readFileSync(path.join(root, 'server/src/capability-matrix.json'), 'utf8'));
  assert.equal(matrix.schema, 'vitte-capabilities@1');
  assert.match(matrix.matrixVersion, /^\d+\.\d+\.\d+$/);
  for (const feature of matrix.editorFeatures) {
    assert.ok(feature.id && feature.label && feature.status && feature.description, JSON.stringify(feature));
    if (feature.advertised) {
      assert.ok(feature.evidence.length > 0, `${feature.id} has no evidence`);
      assert.ok(!['planned', 'unavailable'].includes(feature.status), feature.id);
    }
    assert.equal(isCapabilityAdvertised(feature.id), Boolean(feature.advertised && feature.evidence.length));
  }
  const nativeDebug = matrix.editorFeatures.find(feature => feature.id === 'native-debug');
  assert.equal(nativeDebug.advertised, false, 'native debugging is not advertised without passing evidence for the active toolchain/platform');
  assert.equal(nativeDebug.followUp, 'CAP-DBG-001');
  const formatting = matrix.editorFeatures.find(feature => feature.id === 'formatting');
  assert.equal(formatting.status, 'unavailable');
  assert.equal(formatting.advertised, false, 'formatting needs a canonical compatible formatter');
  assert.equal(matrix.editorFeatures.find(feature => feature.id === 'editor-configuration').advertised, true);
  assert.equal(matrix.editorFeatures.find(feature => feature.id === 'on-type-formatting').advertised, false, 'unverified typing rewrites stay disabled');
  assert.equal(matrix.editorFeatures.find(feature => feature.id === 'organize-imports').advertised, false);
  for (const group of [matrix.compilerFrontends, matrix.cliCommands, matrix.platforms]) {
    for (const entry of group) assert.ok(entry.evidence?.length, `${entry.id} has no evidence`);
  }
  assert.ok(matrix.followUps.length > 0);
  assert.ok(matrix.followUps.every(item => item.status === 'planned' && /^CAP-[A-Z]+-\d+$/.test(item.id)));
  const docs = fs.readFileSync(path.resolve(root, '../../docs/reference/capabilities.html'), 'utf8');
  const documentedRows = (docs.match(/<tr><td>/g) ?? []).length;
  const matrixRows = matrix.editorFeatures.length + matrix.compilerFrontends.length + matrix.cliCommands.length + matrix.platforms.length;
  assert.equal(documentedRows, matrixRows, 'the reference page must include every matrix row');
  for (const followUp of matrix.followUps) assert.ok(docs.includes(followUp.id), `documentation is missing ${followUp.id}`);
  assert.ok(docs.includes(matrix.matrixVersion), 'documentation must identify the same matrix version');
});

test('extension manifest exposes the matrix and keeps unverified hints off', () => {
  const manifest = JSON.parse(fs.readFileSync(path.join(root, 'package.json'), 'utf8'));
  const commands = manifest.contributes.commands.map(command => command.command);
  assert.ok(commands.includes('vitte.showCapabilities'));
  assert.ok(commands.includes('vitte.detectProject'));
  assert.ok(commands.includes('vitte.check'));
  assert.ok(commands.includes('vitte.formatDocument'));
  assert.match(manifest.contributes.commands.find(command => command.command === 'vitte.formatDocument').title, /Unavailable/);
  const settings = manifest.contributes.configuration.properties;
  assert.equal(settings['vitte.features.formatting'], undefined);
  assert.equal(settings['vitte.format.onSave'], undefined);
  assert.match(settings['vitte.fmt.path'].description, /playground stdin protocol only/i);
  assert.equal(settings['vitte.suggestions.localEngine.enabled'].default, false);
  assert.equal(settings['vitte.suggestions.ghostTextTop1Stable'].default, false);
  assert.equal(settings['vitte.features.workspaceSymbols'].default, true);
  assert.equal(settings['vitte.features.inlayHints'].default, true);
  assert.match(settings['vitte.features.inlayHints'].description, /source-derived/i);
  assert.equal(settings['vitte.inlayHints.returnHints'].default, false);
  assert.equal(settings['vitte.inlayHints.aliasHints'].default, false);
  assert.ok(!commands.includes('vitte.codelens.runProc'));
  assert.ok(!commands.includes('vitte.codelens.explainCode'));
  assert.equal(manifest.main, './out/extension.js');
  assert.equal(manifest.engines.vscode, '^1.105.0');
  const capabilityMatrix = JSON.parse(fs.readFileSync(path.join(root, 'server/src/capability-matrix.json'), 'utf8'));
  assert.equal(capabilityMatrix.versions.extension, manifest.version);
  assert.equal(capabilityMatrix.versions.languageServer.split('.').slice(0, 2).join('.'), manifest.version.split('.').slice(0, 2).join('.'));
  assert.deepEqual(new Set(manifest.contributes.languages.flatMap(language => language.extensions)), new Set(['.vit', '.vitl', '.vitte']));
  for (const event of ['onLanguage:vit', 'onLanguage:vitte', 'workspaceContains:vitte.toml', 'workspaceContains:.vitte/config']) {
    assert.ok(manifest.activationEvents.includes(event), `missing activation event ${event}`);
  }
  assert.ok(!manifest.activationEvents.includes('*'));
  for (const [key, setting] of Object.entries(settings)) {
    assert.ok(setting.type, `${key} must declare a schema type`);
    assert.ok(setting.description, `${key} must be documented`);
    assert.ok(['application', 'machine', 'window', 'resource', 'machine-overridable', 'language-overridable'].includes(setting.scope), `${key} must have a valid settings scope`);
  }
  assert.equal(settings['vitte.compiler.path'].type, 'string');
  assert.equal(settings['vitte.serverPath'].type, 'string');
  assert.equal(settings['vitte.lsp.path'].type, 'string');
  assert.equal(settings['vitte.lsp.args'].type, 'array');
  assert.equal(settings['vitte.build.args'].type, 'array');
  assert.equal(settings['vitte.build.target'].type, 'string');
  assert.equal(settings['vitte.build.target'].scope, 'resource');
  assert.equal(settings['vitte.project.manifestPath'].type, 'string');
  assert.equal(settings['vitte.modulePaths'].type, 'array');
  assert.equal(settings['vitte.modulePaths'].scope, 'resource');
  assert.equal(settings['vitte.modulePaths'].default.length, 0);
  const taskDefinition = manifest.contributes.taskDefinitions.find(task => task.type === 'vitte');
  assert.ok(taskDefinition.properties.command.enum.includes('check'));
  assert.equal(taskDefinition.properties.programArgs.type, 'array');
  assert.equal(manifest.contributes.problemMatchers.find(matcher => matcher.name === '$vitte').owner, 'vitte');
  assert.ok(!manifest.files, 'package uses .vscodeignore so VSCE includes runtime dependencies by default');
});

test('bundled server serves editor features, navigation and workspace symbols over stdio LSP', { timeout: 15000 }, async (t) => {
  const { spawn } = require('node:child_process');
  const { createMessageConnection, StreamMessageReader, StreamMessageWriter } = require('vscode-jsonrpc/node');
  const workspace = fs.realpathSync(fs.mkdtempSync(path.join(require('node:os').tmpdir(), 'vitte-lsp-workspace-')));
  const workspaceUri = require('node:url').pathToFileURL(workspace).toString();
  fs.writeFileSync(path.join(workspace, 'workspace-indexed.vit'), 'proc workspaceOnly() {}\n');
  fs.writeFileSync(path.join(workspace, 'diskapi.vit'), 'export proc workspaceApi(value: i32) -> i32 { return value; }\n');
  const child = spawn(process.execPath, [path.join(root, 'server/out/src/server.js'), '--stdio'], { cwd: root, stdio: ['pipe', 'pipe', 'pipe'] });
  const connection = createMessageConnection(new StreamMessageReader(child.stdout), new StreamMessageWriter(child.stdin));
  t.after(() => { connection.dispose(); child.kill(); });
  t.after(() => fs.rmSync(workspace, { recursive: true, force: true }));
  child.stderr.resume();
  connection.listen();
  const initialized = await connection.sendRequest('initialize', {
    processId: process.pid,
    rootUri: workspaceUri,
    workspaceFolders: [{ uri: workspaceUri, name: 'vitte-lsp-workspace' }],
    capabilities: {
      workspace: {
        workspaceEdit: { documentChanges: true },
        workspaceFolders: true,
        fileOperations: { willRename: true },
      },
    },
  });
  const capabilityMatrix = JSON.parse(fs.readFileSync(path.join(root, 'server/src/capability-matrix.json'), 'utf8'));
  assert.equal(initialized.serverInfo?.version, capabilityMatrix.versions.languageServer);
  assert.ok(initialized.capabilities.completionProvider);
  assert.ok(initialized.capabilities.completionProvider.triggerCharacters.length > 0);
  assert.equal(initialized.capabilities.completionProvider.resolveProvider, true);
  for (const capability of ['hoverProvider', 'signatureHelpProvider', 'documentSymbolProvider', 'workspaceSymbolProvider', 'definitionProvider', 'declarationProvider', 'typeDefinitionProvider', 'implementationProvider', 'referencesProvider', 'renameProvider', 'codeActionProvider', 'semanticTokensProvider', 'inlayHintProvider', 'codeLensProvider', 'foldingRangeProvider', 'selectionRangeProvider', 'documentLinkProvider']) {
    assert.ok(initialized.capabilities[capability], `missing tested LSP capability ${capability}`);
  }
  assert.equal(initialized.capabilities.documentFormattingProvider, undefined, 'no canonical compatible formatter is available');
  assert.equal(initialized.capabilities.documentRangeFormattingProvider, undefined, 'range formatting is unavailable without the canonical formatter');
  assert.equal(initialized.capabilities.documentOnTypeFormattingProvider, undefined, 'typing must not trigger unverified source rewrites');
  for (const capability of ['callHierarchyProvider', 'typeHierarchyProvider']) {
    assert.equal(initialized.capabilities[capability], undefined, `unverified capability ${capability} must not be advertised`);
  }
  assert.ok(initialized.capabilities.workspace.fileOperations.willRename);
  assert.equal(initialized.capabilities.workspace.workspaceFolders.changeNotifications, true);
  await connection.sendNotification('initialized', {});
  const matrix = await connection.sendRequest('vitte/capabilityMatrix');
  assert.equal(matrix.schema, 'vitte-capabilities@1');
  assert.ok(matrix.editorFeatures.some(feature => feature.id === 'inlay-hints' && feature.advertised && feature.enabled));
  assert.ok(matrix.editorFeatures.filter(feature => feature.advertised).every(feature => feature.evidence.length > 0));
  const uri = 'file:///vitte-protocol-test.vit';
  const text = 'space editor_test;\nproc editorTarget() -> i32 { return 0; }\nret';
  await connection.sendNotification('textDocument/didOpen', { textDocument: { uri, languageId: 'vit', version: 1, text } });
  const completions = await connection.sendRequest('textDocument/completion', { textDocument: { uri }, position: { line: 2, character: 3 } });
  assert.ok((Array.isArray(completions) ? completions : completions.items).some(item => item.label === 'return'));
  const tokens = await connection.sendRequest('textDocument/semanticTokens/full', { textDocument: { uri } });
  assert.ok(tokens.data.length > 0);
  assert.ok(tokens.resultId);
  const changedText = text.replace('return 0', 'return 1000');
  await connection.sendNotification('textDocument/didChange', { textDocument: { uri, version: 2 }, contentChanges: [{ text: changedText }] });
  const delta = await connection.sendRequest('textDocument/semanticTokens/full/delta', { textDocument: { uri }, previousResultId: tokens.resultId });
  assert.ok(delta.resultId && Array.isArray(delta.edits), 'semantic tokens return a versioned delta');
  assert.ok(delta.edits.some(edit => edit.deleteCount > 0 || edit.data?.length > 0));
  const notRenameable = await connection.sendRequest('textDocument/prepareRename', { textDocument: { uri }, position: { line: 2, character: 1 } });
  assert.equal(notRenameable, null, 'unsupported and unresolved tokens have an explicit unavailable prepare-rename result');
  const symbols = await connection.sendRequest('textDocument/documentSymbol', { textDocument: { uri } });
  assert.ok(symbols.some(symbol => symbol.name === 'editorTarget' || symbol.children?.some(child => child.name === 'editorTarget')));
  const workspaceResults = await connection.sendRequest('workspace/symbol', { query: 'editorTarget' });
  assert.ok(!workspaceResults.some(symbol => symbol.name === 'editorTarget'), 'workspace search excludes open documents outside the workspace');
  const diskSymbols = await connection.sendRequest('workspace/symbol', { query: 'workspaceOnly' });
  assert.ok(diskSymbols.some(symbol => symbol.name === 'workspaceOnly' && symbol.location.uri.endsWith('/workspace-indexed.vit')));

  const navigationUri = 'file:///vitte-protocol-navigation.vit';
  const navigationText = [
    'form EditorPoint { x: i32 }',
    'trait EditorReadable { proc read() -> i32; }',
    'impl EditorReadable for EditorPoint { proc read(receiver: EditorPoint) -> i32 { return 1; } }',
    'proc editorUse(point: EditorPoint) -> i32 { point.x; return point.read(); }',
  ].join('\n');
  await connection.sendNotification('textDocument/didOpen', { textDocument: { uri: navigationUri, languageId: 'vit', version: 1, text: navigationText } });
  const navDoc = TextDocument.create(navigationUri, 'vit', 1, navigationText);
  const navAt = (needle, delta = 0) => navDoc.positionAt(navigationText.indexOf(needle) + delta);
  const hover = await connection.sendRequest('textDocument/hover', { textDocument: { uri: navigationUri }, position: navAt('point.x', 1) });
  assert.match(hover.contents.value, /EditorPoint/);
  const definition = await connection.sendRequest('textDocument/definition', { textDocument: { uri: navigationUri }, position: navAt('point.x', 6) });
  assert.deepEqual(definition[0].range.start, navDoc.positionAt(navigationText.indexOf('x: i32')));
  const declaration = await connection.sendRequest('textDocument/declaration', { textDocument: { uri: navigationUri }, position: navAt('point.x', 6) });
  assert.deepEqual(declaration[0].range.start, definition[0].range.start);
  const typeDefinition = await connection.sendRequest('textDocument/typeDefinition', { textDocument: { uri: navigationUri }, position: navAt('point.x', 1) });
  assert.deepEqual(typeDefinition[0].range.start, navDoc.positionAt(navigationText.indexOf('EditorPoint')));
  const implementations = await connection.sendRequest('textDocument/implementation', { textDocument: { uri: navigationUri }, position: navAt('EditorReadable', 2) });
  assert.ok(implementations.some(location => location.range.start.line === 2));
  const references = await connection.sendRequest('textDocument/references', { textDocument: { uri: navigationUri }, position: navAt('editorUse', 2), context: { includeDeclaration: true } });
  assert.ok(references.some(location => location.range.start.line === 3));
  const lenses = await connection.sendRequest('textDocument/codeLens', { textDocument: { uri: navigationUri } });
  assert.ok(lenses.some(lens => lens.command?.title === 'Find references' && lens.command.command === 'vitte.codelens.openRefs'));

  const presentationUri = 'file:///vitte-presentation.vit';
  const presentationText = [
    'proc combine(left: i32, right: bool) -> i32 {',
    '  return left;',
    '}',
    'proc main() {',
    '  let count = 3;',
    '  combine(count, true);',
    '}',
  ].join('\n');
  await connection.sendNotification('textDocument/didOpen', { textDocument: { uri: presentationUri, languageId: 'vit', version: 1, text: presentationText } });
  const presentationDoc = TextDocument.create(presentationUri, 'vit', 1, presentationText);
  const inlayHints = await connection.sendRequest('textDocument/inlayHint', {
    textDocument: { uri: presentationUri },
    range: { start: { line: 0, character: 0 }, end: { line: 6, character: 0 } },
  });
  assert.ok(inlayHints.some(hint => hint.label === ': int'));
  assert.ok(inlayHints.some(hint => hint.label === 'left:'));
  const folds = await connection.sendRequest('textDocument/foldingRange', { textDocument: { uri: presentationUri } });
  assert.ok(folds.some(range => range.startLine === 0 && range.endLine === 2));
  const selection = await connection.sendRequest('textDocument/selectionRange', {
    textDocument: { uri: presentationUri },
    positions: [presentationDoc.positionAt(presentationText.indexOf('return left'))],
  });
  assert.ok(selection[0]?.parent, 'selection ranges include enclosing syntax scopes');

  const importerPath = path.join(workspace, 'disk-use.vit');
  const importerUri = require('node:url').pathToFileURL(importerPath).toString();
  const importerText = 'use diskapi::{workspaceApi as api};\nproc useApi() { api(1); }';
  await connection.sendNotification('textDocument/didOpen', { textDocument: { uri: importerUri, languageId: 'vit', version: 1, text: importerText } });
  const crossFileReferences = await connection.sendRequest('textDocument/references', {
    textDocument: { uri: importerUri },
    position: { line: 1, character: importerText.split('\n')[1].indexOf('api') + 1 },
    context: { includeDeclaration: true },
  });
  assert.ok(crossFileReferences.some(location => location.uri.endsWith('/diskapi.vit')));
  assert.ok(crossFileReferences.some(location => location.uri === importerUri));
  const links = await connection.sendRequest('textDocument/documentLink', { textDocument: { uri: importerUri } });
  assert.equal(links.length, 1);
  assert.ok(links[0].target.endsWith('/diskapi.vit'));

  const workspaceApi = path.join(workspace, 'diskapi.vit');
  const workspaceApiUri = require('node:url').pathToFileURL(workspaceApi).toString();
  const unsavedApiText = [
    'export proc workspaceApi(value: i32) -> i32 { return value; }',
    '// workspaceApi must remain a comment',
    'proc localUse() { let text = "workspaceApi"; workspaceApi(2); }',
  ].join('\n');
  await connection.sendNotification('textDocument/didOpen', { textDocument: { uri: workspaceApiUri, languageId: 'vit', version: 1, text: 'export proc workspaceApi(value: i32) -> i32 { return value; }\n' } });
  await connection.sendNotification('textDocument/didChange', {
    textDocument: { uri: workspaceApiUri, version: 2 },
    contentChanges: [{ text: unsavedApiText }],
  });
  const apiDoc = TextDocument.create(workspaceApiUri, 'vit', 2, unsavedApiText);
  const apiPrepare = await connection.sendRequest('textDocument/prepareRename', {
    textDocument: { uri: workspaceApiUri },
    position: apiDoc.positionAt(unsavedApiText.indexOf('workspaceApi')),
  });
  assert.equal(apiPrepare.placeholder, 'workspaceApi');
  const apiRename = await connection.sendRequest('textDocument/rename', {
    textDocument: { uri: workspaceApiUri },
    position: apiDoc.positionAt(unsavedApiText.indexOf('workspaceApi')),
    newName: 'workspaceRenamed',
  });
  assert.ok(Array.isArray(apiRename.documentChanges));
  const apiChange = apiRename.documentChanges.find(change => change.textDocument.uri === workspaceApiUri);
  const importerChange = apiRename.documentChanges.find(change => change.textDocument.uri === importerUri);
  assert.equal(apiChange.textDocument.version, 2, 'rename edit is tied to the unsaved buffer version');
  assert.equal(apiChange.edits.length, 2, 'only declaration and resolved call are renamed');
  assert.equal(importerChange.textDocument.version, 1);
  assert.equal(importerChange.edits.length, 1, 'aliased import source updates while the local alias remains intact');
  assert.equal(importerChange.edits[0].newText, 'workspaceRenamed');

  const originalImportPosition = importerText.indexOf('workspaceApi');
  const originalImportPrepare = await connection.sendRequest('textDocument/prepareRename', {
    textDocument: { uri: importerUri },
    position: { line: 0, character: originalImportPosition },
  });
  assert.equal(originalImportPrepare.placeholder, 'workspaceApi');
  const originalImportRename = await connection.sendRequest('textDocument/rename', {
    textDocument: { uri: importerUri },
    position: { line: 0, character: originalImportPosition },
    newName: 'renamedFromImport',
  });
  const originalImportEdit = originalImportRename.documentChanges.find(change => change.textDocument.uri === importerUri);
  assert.equal(originalImportEdit.edits.length, 1);
  assert.equal(originalImportEdit.edits[0].newText, 'renamedFromImport');
  assert.equal(originalImportEdit.edits[0].range.start.character, importerText.indexOf('workspaceApi'));

  const modulePath = path.join(workspace, 'oldmodule.vit');
  const moduleUri = require('node:url').pathToFileURL(modulePath).toString();
  const moduleSource = 'space oldmodule;\nexport proc moduleItem() {}\n';
  fs.writeFileSync(modulePath, moduleSource);
  const moduleUsePath = path.join(workspace, 'module-use.vit');
  const moduleUseUri = require('node:url').pathToFileURL(moduleUsePath).toString();
  const nestedModuleDirectory = path.join(workspace, 'oldmodule');
  fs.mkdirSync(nestedModuleDirectory);
  fs.writeFileSync(path.join(nestedModuleDirectory, 'nested.vit'), 'export proc nestedItem() {}\n');
  const moduleUse = 'use oldmodule::{moduleItem};\nuse oldmodule::nested::{nestedItem};\nproc useModule() { moduleItem(); nestedItem(); }';
  fs.writeFileSync(moduleUsePath, moduleUse);
  await connection.sendNotification('textDocument/didOpen', { textDocument: { uri: moduleUri, languageId: 'vit', version: 1, text: moduleSource } });
  const fileRename = await connection.sendRequest('workspace/willRenameFiles', {
    files: [{ oldUri: moduleUri, newUri: require('node:url').pathToFileURL(path.join(workspace, 'renamedmodule.vit')).toString() }],
  });
  assert.ok(Array.isArray(fileRename.documentChanges));
  const moduleChange = fileRename.documentChanges.find(change => change.textDocument.uri === moduleUri);
  const moduleUseChange = fileRename.documentChanges.find(change => change.textDocument.uri === moduleUseUri);
  assert.equal(moduleChange.textDocument.version, 1);
  assert.equal(moduleChange.edits.length, 1);
  assert.equal(moduleChange.edits[0].newText, 'renamedmodule');
  assert.equal(moduleUseChange.edits.length, 1);
  assert.equal(moduleUseChange.edits[0].newText, 'renamedmodule');
  assert.equal(moduleUseChange.edits[0].range.start.line, 0, 'renaming a module file leaves imports resolved to nested files untouched');


  const actionUri = 'file:///vitte-protocol-code-action.vit';
  const actionText = 'space editor_test;\nproc main() {   \n}\n';
  await connection.sendNotification('textDocument/didOpen', { textDocument: { uri: actionUri, languageId: 'vit', version: 1, text: actionText } });
  const actionRange = { start: { line: 1, character: 0 }, end: { line: 1, character: 1 } };
  const whitespaceActions = await connection.sendRequest('textDocument/codeAction', {
    textDocument: { uri: actionUri },
    range: actionRange,
    context: { diagnostics: [{ range: actionRange, message: 'trailing whitespace', severity: 2, code: 'format.trailingWhitespace' }] },
  });
  const trimWhitespace = whitespaceActions.find(action => action.title === 'Vitte: Trim trailing whitespace');
  assert.ok(trimWhitespace);
  assert.equal(trimWhitespace.isPreferred, false, 'a safe edit is still applied only after the user selects it');
  assert.ok(trimWhitespace.title.length <= 80, 'the action title explains the resulting change briefly');
  const syntaxActions = await connection.sendRequest('textDocument/codeAction', {
    textDocument: { uri: actionUri },
    range: actionRange,
    context: { diagnostics: [{ range: actionRange, message: 'syntax error', severity: 1, code: 'PARSE:E0007' }] },
  });
  assert.deepEqual(syntaxActions, [], 'unverified syntax errors must not receive guessed edits');
  await connection.sendRequest('shutdown');
  await connection.sendNotification('exit');
});
