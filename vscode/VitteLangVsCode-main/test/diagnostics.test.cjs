const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const { test } = require('node:test');
const { normalizeCompilerDiagnostics, storeCompilerSuggestions, compilerSuggestionsFor } = require('../out/utils/compilerDiagnostics');
const { writeCompilerSourceSnapshot } = require('../out/utils/compilerSourceSnapshot');
const { findCompilerRepository } = require('../out/utils/project');
const { lintToPublishable } = require('../server/out/src/lint');

const extensionRoot = path.resolve(__dirname, '..');
const fixtureRoot = path.join(extensionRoot, 'test/fixtures/diagnostics');
const fixtures = JSON.parse(fs.readFileSync(path.join(fixtureRoot, 'compiler-cases.json'), 'utf8'));

test('diagnostic regression inventory covers every canonical grammar production', (t) => {
  const repo = findCompilerRepository(extensionRoot);
  if (!repo) return t.skip('Compiler source tree is not present in this extension checkout.');
  assert.equal(fixtures.schema, 'vitte-diagnostic-fixtures@1');
  assert.equal(fixtures.grammar.length, 117);
  assert.ok(fixtures.negative.length >= 200);
  assert.equal(fixtures.positive.length, 5);
  const cases = new Set(fixtures.negative.map((entry) => entry.name));
  for (const entry of fixtures.grammar) assert.ok(cases.has(entry.case), `${entry.production} has no invalid regression fixture`);
  for (const entry of fixtures.positive) {
    const positivePath = path.join(repo, entry.path);
    assert.ok(fs.existsSync(positivePath), `Missing valid fixture ${positivePath}`);
    const compiler = path.join(repo, 'build/bin', process.platform === 'win32' ? 'vitte.exe' : 'vitte');
    if (fs.existsSync(compiler)) {
      const checked = spawnSync(compiler, ['check', positivePath], { cwd: repo, encoding: 'utf8', timeout: 10000 });
      assert.equal(checked.status, 0, `${entry.name}: ${checked.stdout}${checked.stderr}`);
    }
  }

  const exporter = spawnSync('python3', [path.join(repo, 'src/tests/diagnostic_contract_test.py'), '--export-fixtures'], {
    cwd: repo,
    env: { ...process.env, PYTHONUTF8: '1' },
    encoding: 'utf8',
    timeout: 10000,
  });
  assert.equal(exporter.status, 0, exporter.stderr);
  assert.deepEqual(JSON.parse(exporter.stdout), fixtures, 'Generated compiler diagnostic fixtures changed; regenerate the checked-in client manifest.');
});

test('LSP diagnostics preserve code, title, Unicode primary span, labels, related locations, help and fix applicability', () => {
  const text = '// 😀 before diagnostics\nproc main() -> int { return missing_value; }\n';
  const file = '/tmp/vitte-diagnostics-main.vit';
  const start = text.indexOf('missing_value');
  const startByte = Buffer.byteLength(text.slice(0, start));
  const value = text.split('\n')[1].indexOf('missing_value');
  const payload = [{
    range: { start: { line: 1, character: value }, end: { line: 1, character: value + 13 } },
    severity: 1,
    code: 'E0401',
    source: 'vitte',
    message: 'unknown symbol `missing_value`',
    relatedInformation: [{
      location: { uri: 'file:///tmp/vitte-declaration.vit', range: { start: { line: 4, character: 2 }, end: { line: 4, character: 14 } } },
      message: 'candidate declaration is here',
    }],
    data: {
      help: 'Import or declare the name in this scope.',
      phase: 'name-resolution',
      category: 'name-resolution',
      origin: 'native-sema',
      expected: 'visible symbol',
      found: 'missing_value',
      expectedType: 'i32',
      actualType: 'bool',
      position: { source: file, startByte: startByte, endByte: startByte + Buffer.byteLength('missing_value') },
      labels: [
        { style: 'primary', message: 'The referenced name is not visible.', source: file, startByte, endByte: startByte + 13 },
        { style: 'secondary', message: 'A related source label.', source: file, startByte: Buffer.byteLength(text.slice(0, text.indexOf('proc'))), endByte: Buffer.byteLength(text.slice(0, text.indexOf('proc'))) + 4 },
      ],
      suggestions: [
        { message: 'Use the compiler-proven correction', replacement: 'visible_name', applicability: 'machine-applicable', hasSpan: true, startByte, endByte: startByte + 13 },
        { message: 'Check this possible correction manually', replacement: 'maybe_name', applicability: 'maybe-incorrect', hasSpan: true, startByte, endByte: startByte + 13 },
      ],
    },
  }];

  const [diagnostic] = normalizeCompilerDiagnostics(payload, text, file, '/tmp');
  assert.ok(diagnostic);
  assert.equal(diagnostic.code, 'E0401');
  assert.equal(diagnostic.title, 'unknown symbol `missing_value`');
  assert.equal(diagnostic.start, start);
  assert.equal(diagnostic.end, start + 13);
  assert.equal(text.slice(diagnostic.start, diagnostic.end), 'missing_value');
  assert.equal(diagnostic.labels.find((label) => label.style === 'primary').message, 'The referenced name is not visible.');
  assert.equal(diagnostic.labels.find((label) => label.style === 'secondary').start, text.indexOf('proc'));
  assert.equal(diagnostic.related[0].message, 'candidate declaration is here');
  assert.deepEqual(diagnostic.related[0].lineRange, { startLine: 4, startCharacter: 2, endLine: 4, endCharacter: 14 });
  assert.equal(diagnostic.help, 'Import or declare the name in this scope.');
  assert.equal(diagnostic.expected, 'visible symbol');
  assert.equal(diagnostic.found, 'missing_value');
  assert.equal(diagnostic.expectedType, 'i32');
  assert.equal(diagnostic.actualType, 'bool');
  assert.equal(diagnostic.phase, 'name-resolution');
  assert.equal(diagnostic.category, 'name-resolution');
  assert.equal(diagnostic.origin, 'native-sema');
  assert.deepEqual(diagnostic.suggestions.map((suggestion) => suggestion.applicability), ['machine-applicable', 'maybe-incorrect']);

  storeCompilerSuggestions('file:///tmp/vitte-diagnostics-main.vit', [diagnostic]);
  const suggestions = compilerSuggestionsFor('file:///tmp/vitte-diagnostics-main.vit', 'PARSE:E0401', start, start + 13);
  assert.equal(suggestions.length, 2);
  assert.equal(suggestions[0].replacement, 'visible_name');
  assert.equal(suggestions[1].applicability, 'maybe-incorrect');
  storeCompilerSuggestions('file:///tmp/vitte-diagnostics-main.vit', []);
});

test('cross-file compiler related byte spans remain tagged for source-aware conversion', () => {
  const main = '/tmp/vitte-related-main.vit';
  const imported = '/tmp/vitte-related-import.vit';
  const text = 'proc main() {}';
  const [diagnostic] = normalizeCompilerDiagnostics({ diagnostics: [{
    code: 'E0401',
    message: 'unresolved imported name',
    position: { source: main, start_byte: 5, end_byte: 9 },
    related_locations: [{
      source: imported,
      start_byte: 8,
      end_byte: 14,
      message: 'candidate declaration is here',
    }],
  }] }, text, main, '/tmp');

  assert.ok(diagnostic);
  assert.deepEqual(diagnostic.related[0], {
    source: imported,
    start: 8,
    end: 14,
    message: 'candidate declaration is here',
    offsetUnit: 'utf8-byte',
  });
});

test('unsaved compiler snapshots preserve the source folder and leave disk content untouched', async (t) => {
  const folder = fs.mkdtempSync(path.join(require('node:os').tmpdir(), 'vitte-unsaved-source-'));
  t.after(() => fs.rmSync(folder, { recursive: true, force: true }));
  const original = path.join(folder, 'main.vit');
  fs.writeFileSync(original, 'proc main() {}\n', 'utf8');
  const snapshot = await writeCompilerSourceSnapshot(original, 'proc main() { return missing; }\n');
  t.after(() => fs.rmSync(snapshot, { force: true }));

  assert.notEqual(snapshot, original);
  assert.equal(path.dirname(snapshot), path.dirname(original), 'relative imports retain the source directory');
  assert.equal(path.extname(snapshot), '.vit', 'the compiler sees the canonical source extension');
  assert.match(path.basename(snapshot), /^\.main\.vitte-diagnostics-/);
  assert.equal(fs.readFileSync(snapshot, 'utf8'), 'proc main() { return missing; }\n');
  assert.equal(fs.readFileSync(original, 'utf8'), 'proc main() {}\n', 'unsaved checks do not overwrite the workspace file');
});

test('editor lint does not duplicate compiler-owned syntax or semantic diagnostics', () => {
  const diagnostics = lintToPublishable('proc main() { )\nconst bad__name = 1;\n// TODO: revisit\n', 'file:///main.vit');
  assert.ok(diagnostics.some(diagnostic => diagnostic.code === 'note.todo'));
  assert.ok(diagnostics.every(diagnostic => ['format.tabs', 'format.trailingWhitespace', 'format.lineLength', 'note.todo'].includes(diagnostic.code)));
  assert.ok(!diagnostics.some(diagnostic => diagnostic.severity === 1), 'editor lint must not report compiler-owned syntax errors');
});

test('normalizer accepts legacy compiler JSON envelopes and keeps zero-width insertions', () => {
  const text = 'proc f() { call(1 2); }';
  const at = text.indexOf('2');
  const diagnostic = normalizeCompilerDiagnostics({ diagnostics: [{
    code: 'E0311',
    message: 'missing comma',
    help: 'Separate arguments with a comma.',
    position: { source: 'main.vit', start_byte: at, end_byte: at },
    suggestions: [{ message: 'Insert a comma', replacement: ',', applicability: 'machine-applicable', span: { source: 'main.vit', start_byte: at, end_byte: at } }],
  }] }, text, '/tmp/main.vit', '/tmp')[0];
  assert.ok(diagnostic);
  assert.equal(diagnostic.start, at);
  assert.equal(diagnostic.end, at);
  assert.equal(diagnostic.help, 'Separate arguments with a comma.');
  assert.equal(diagnostic.suggestions[0].start, at);
  assert.equal(diagnostic.suggestions[0].end, at);
});
