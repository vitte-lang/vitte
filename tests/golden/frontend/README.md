# Frontend Golden Snapshots

`snapshot_version`: `1`

This suite snapshots frontend compiler outputs for a fixed corpus:

- AST JSON (`*.ast.json`)
- HIR JSON (`*.hir.json`)
- MIR JSON (`*.mir.json`)
- shape summaries (`*.shape.json`)
- diagnostics + parse metrics (`*.diag.json`)

## Layout

- Fixtures: `tests/golden/frontend/{valid,invalid,recovery,strict,operators}/*.vit`
- Manifest: `tests/golden/frontend/MANIFEST.txt`
- Config: `tests/golden/frontend/CONFIG.json`
- Snapshots: `tests/golden/frontend/snapshots/**`

## Commands

- Check all: `make test-golden`
- Update snapshots: `make test-golden-update`
- Check critical subset: `make test-golden-critical`

## Comparison modes

- `exact`: strict snapshot byte match (non-regression lock)
- `shape`: stable structural summary match (robust against benign formatting noise)

## Notes

- Runner enforces deterministic compiler flags (`--deterministic`).
- Invalid fixtures assert diagnostics (code/message/span) via `CONFIG.json`.
- Recovery fixtures additionally assert parser metrics and partial AST shape.
