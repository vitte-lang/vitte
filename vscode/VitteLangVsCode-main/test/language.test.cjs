const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
const { test, before } = require('node:test');
const { TextDocument } = require('vscode-languageserver-textdocument');
const { Registry } = require('vscode-textmate');
const oniguruma = require('vscode-oniguruma');
const { scanLexical, maskNonCode, lexicalContextAt } = require('../server/out/src/lexical');
const facts = require('../server/out/src/languageFacts');
const { buildSemanticTokens, getSemanticTokensLegend } = require('../server/out/src/semantic');
const { buildCodeMask } = require('../server/out/src/navigation');
const { provideCompletions, configureCompletions } = require('../server/out/src/completion');
const { indexDocument, getDocumentIndex, clearIndex } = require('../server/out/src/indexer');
const { provideSignatureHelp } = require('../server/out/src/signatureHelp');
const { provideHover } = require('../server/out/src/semantic');
const { provideFormattingEdits, provideRangeFormattingEdits } = require('../server/out/src/formatting');
const { isCapabilityAdvertised } = require('../server/out/src/intellisenseCapabilities');
const root = path.resolve(__dirname, '..');
const doc = (text) => TextDocument.create('file:///vitte-editor-test.vit', 'vit', 1, text);
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
    tokens.push({ value, type: types[data[i + 3]] });
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

test('completion is suppressed inside every comment and string form, including EOF', () => {
  configureCompletions();
  assert.equal(lexicalContextAt('// pro\n', 6), 'comment');
  for (const source of ['// pro', '# pro', '/* outer /* inner */ pro', '<<< pro', 'r##"pro', '"""pro', '"pro', "'pro"]) {
    const document = doc(source);
    assert.notEqual(lexicalContextAt(source, source.length), 'code', source);
    assert.deepEqual(provideCompletions(document, document.positionAt(source.length)), [], source);
  }
  for (const source of ['/* done */', 'r#"done"#', '"""done"""', '<<< done >>>']) assert.equal(lexicalContextAt(source, source.length), 'code', source);
  const document = doc('ret');
  assert.ok(provideCompletions(document, document.positionAt(3)).some(item => item.label === 'return'));
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

test('hover documents known language elements and leaves unknown user symbols empty', () => {
  const source = 'proc main() -> i32 { return missing_value; }';
  const document = doc(source);
  const returnHover = provideHover(document, document.positionAt(source.indexOf('return')));
  assert.match(returnHover.contents.value, /Retourne depuis une fonction/);
  const typeHover = provideHover(document, document.positionAt(source.indexOf('i32')));
  assert.match(typeHover.contents.value, /Type primitif/);
  assert.equal(provideHover(document, document.positionAt(source.indexOf('missing_value') + 2)), null);
});

test('formatter normalizes whitespace and is idempotent', () => {
  const original = doc('proc  main()   {  return 0; }   ');
  const edits = provideFormattingEdits(original);
  assert.equal(edits.length, 1);
  assert.match(edits[0].newText, /proc main\(\)/);
  assert.ok(edits[0].newText.endsWith('\n'));
  const formatted = doc(edits[0].newText);
  assert.deepEqual(provideFormattingEdits(formatted), []);
  const rangeEdits = provideRangeFormattingEdits(original, { start: { line: 0, character: 0 }, end: { line: 0, character: original.getText().length } });
  assert.equal(rangeEdits.length, 1);
  assert.match(rangeEdits[0].newText, /proc main\(\)/);
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
  assert.ok(snippets['match expression'].body.join('\n').includes(' => {'));
  assert.ok(!snippets['pick declaration'].body.join('\n').includes('case '));
  assert.ok(snippets['extern C block'].body.join('\n').startsWith('extern "C" proc '));
  for (const name of ['use declaration', 'use alias', 'export symbols', 'type alias', 'constant', 'return value']) assert.ok(snippets[name].body.at(-1).endsWith(';'), name);
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

test('extension manifest exposes the matrix and keeps heuristic defaults off', () => {
  const manifest = JSON.parse(fs.readFileSync(path.join(root, 'package.json'), 'utf8'));
  const commands = manifest.contributes.commands.map(command => command.command);
  assert.ok(commands.includes('vitte.showCapabilities'));
  const settings = manifest.contributes.configuration.properties;
  assert.equal(settings['vitte.suggestions.localEngine.enabled'].default, false);
  assert.equal(settings['vitte.suggestions.ghostTextTop1Stable'].default, false);
  assert.equal(settings['vitte.features.workspaceSymbols'].default, false);
  assert.equal(settings['vitte.features.inlayHints'].default, false);
  assert.match(settings['vitte.features.inlayHints'].description, /not advertised/i);
  assert.ok(!manifest.files, 'package uses .vscodeignore so VSCE includes runtime dependencies by default');
});

test('bundled server serves completion, semantic tokens and symbols over stdio LSP', { timeout: 15000 }, async (t) => {
  const { spawn } = require('node:child_process');
  const { createMessageConnection, StreamMessageReader, StreamMessageWriter } = require('vscode-jsonrpc/node');
  const child = spawn(process.execPath, [path.join(root, 'server/out/src/server.js'), '--stdio'], { cwd: root, stdio: ['pipe', 'pipe', 'pipe'] });
  const connection = createMessageConnection(new StreamMessageReader(child.stdout), new StreamMessageWriter(child.stdin));
  t.after(() => { connection.dispose(); child.kill(); });
  child.stderr.resume();
  connection.listen();
  const initialized = await connection.sendRequest('initialize', { processId: process.pid, rootUri: null, capabilities: {} });
  assert.ok(initialized.capabilities.completionProvider);
  assert.ok(initialized.capabilities.completionProvider.triggerCharacters.length > 0);
  assert.equal(initialized.capabilities.completionProvider.resolveProvider, undefined);
  for (const capability of ['hoverProvider', 'signatureHelpProvider', 'documentSymbolProvider', 'documentFormattingProvider', 'documentRangeFormattingProvider', 'definitionProvider', 'referencesProvider', 'renameProvider', 'codeActionProvider', 'semanticTokensProvider']) {
    assert.ok(initialized.capabilities[capability], `missing tested LSP capability ${capability}`);
  }
  for (const capability of ['workspaceSymbolProvider', 'inlayHintProvider', 'callHierarchyProvider', 'typeHierarchyProvider']) {
    assert.equal(initialized.capabilities[capability], undefined, `unverified capability ${capability} must not be advertised`);
  }
  await connection.sendNotification('initialized', {});
  const matrix = await connection.sendRequest('vitte/capabilityMatrix');
  assert.equal(matrix.schema, 'vitte-capabilities@1');
  assert.ok(matrix.editorFeatures.some(feature => feature.id === 'inlay-hints' && !feature.advertised && !feature.enabled));
  assert.ok(matrix.editorFeatures.filter(feature => feature.advertised).every(feature => feature.evidence.length > 0));
  const uri = 'file:///vitte-protocol-test.vit';
  const text = 'space editor_test;\nproc editorTarget() -> i32 { return 0; }\nret';
  await connection.sendNotification('textDocument/didOpen', { textDocument: { uri, languageId: 'vit', version: 1, text } });
  const completions = await connection.sendRequest('textDocument/completion', { textDocument: { uri }, position: { line: 2, character: 3 } });
  assert.ok((Array.isArray(completions) ? completions : completions.items).some(item => item.label === 'return'));
  const tokens = await connection.sendRequest('textDocument/semanticTokens/full', { textDocument: { uri } });
  assert.ok(tokens.data.length > 0);
  const symbols = await connection.sendRequest('textDocument/documentSymbol', { textDocument: { uri } });
  assert.ok(symbols.some(symbol => symbol.name === 'editorTarget' || symbol.children?.some(child => child.name === 'editorTarget')));

  const actionUri = 'file:///vitte-protocol-code-action.vit';
  const actionText = 'space editor_test;\nproc main() {   \n}\n';
  await connection.sendNotification('textDocument/didOpen', { textDocument: { uri: actionUri, languageId: 'vit', version: 1, text: actionText } });
  const actionRange = { start: { line: 1, character: 0 }, end: { line: 1, character: 1 } };
  const whitespaceActions = await connection.sendRequest('textDocument/codeAction', {
    textDocument: { uri: actionUri },
    range: actionRange,
    context: { diagnostics: [{ range: actionRange, message: 'trailing whitespace', severity: 2, code: 'format.trailingWhitespace' }] },
  });
  assert.ok(whitespaceActions.some(action => action.title === 'Vitte: Trim trailing whitespace'));
  const syntaxActions = await connection.sendRequest('textDocument/codeAction', {
    textDocument: { uri: actionUri },
    range: actionRange,
    context: { diagnostics: [{ range: actionRange, message: 'syntax error', severity: 1, code: 'PARSE:E0007' }] },
  });
  assert.deepEqual(syntaxActions, [], 'unverified syntax errors must not receive guessed edits');
  await connection.sendRequest('shutdown');
  await connection.sendNotification('exit');
});
