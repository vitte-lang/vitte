# Compiler diagnostic fixtures

`compiler-cases.json` is generated from the compiler's diagnostic contract suite. It contains 255 invalid programs across lexer, parser, semantic, flow, contract, constant-evaluation, lowering and backend families. Its production map includes an invalid witness for every one of the 117 syntax productions in `grammar/vitte.ebnf`. The VS Code client test verifies that this checked-in inventory matches the compiler source of truth.

The manifest also points to five positive programs covering a small valid client module, canonical grammar, contracts, imports and native runtime compilation. The compiler contract suite remains authoritative for detailed code, phase, span, label, related-location, help and applicability assertions across every negative case.

Regenerate the manifest from the repository root with:

```sh
python3 src/tests/diagnostic_contract_test.py --export-fixtures > vscode/VitteLangVsCode-main/test/fixtures/diagnostics/compiler-cases.json
```
