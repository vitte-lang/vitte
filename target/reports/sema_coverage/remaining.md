# Sema Remaining Coverage

## Covered by fixtures

- valid basic resolution
- imports, aliases, glob imports and multi-file summaries
- visibility public/private/internal/package/external
- duplicate binding, undeclared target and unknown symbol diagnostics
- missing module, inaccessible symbol and import cycle contracts
- warning-only and mixed error/warning snapshots

## Still to replace with compiler-executed fixtures

- real import-cycle graph detection when implemented by the module resolver
- explicit export validation for missing exported names
- duplicate field, variant and trait method diagnostics in sema instead of typeck helper contracts
- machine-generated JSON diagnostics from `vitte check --error-format=json` for each fixture
