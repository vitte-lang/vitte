# Lints (Packages Migration)

## 1) Package layout lint

Command:

```bash
tools/lint_package_layout.py
```

Strict mode (release):

```bash
tools/lint_package_layout.py --strict
```

Checks:
- package entry conventions (`mod.vit`, `info.vit`)
- legacy naming allowlists

## 2) Legacy import path lint

Command:

```bash
tools/lint_legacy_import_paths.py
```

Checks:
- forbids legacy self-leaf imports (`.../<pkg>/<pkg>`) in `src/`, `tests/`, `examples/`, `book/`, `docs/`
- fails on orphan entries in `tools/legacy_import_path_allowlist.txt`

## 3) Module tree lint

Command:

```bash
tools/lint_module_tree.py
```

Checks:
- enforces top-level buckets: `public/`, `internal/`, `experimental/`
- enforces ownership file (`OWNERS`) for each module directory
- enforces owner format `@team/name`
- forbids glob imports in `public/*` modules

## 4) Critical module contracts lint

Command:

```bash
tools/lint_critical_module_contracts.py
```

Checks (`abi/http/db/core`):
- requires `info.vit` metadata keys: `owner`, `stability`, `since`, `deprecated_in`
- requires `OWNERS` non-empty with `@team/name` format
- enforces owner coherence between `info.vit` and `OWNERS`
- requires `ROLE-CONTRACT` header metadata in `mod.vit`

## 5) Module naming lint

Command:

```bash
tools/lint_module_naming.py
```

Checks:
- scans `src/vitte/packages/**/mod.vit`
- forbids `_` in module path segments unless listed in `tools/module_path_name_legacy_allowlist.txt`

## 6) Experimental modules lint

Command:

```bash
tools/lint_experimental_modules.py
```

Checks:
- each `experimental/*` module must have `OWNERS` (non-empty, `@team/name`)
- each `experimental/*` module must have `JUSTIFICATION.md` (non-empty)

## 7) Public module snapshot coverage lint

Command:

```bash
tools/lint_public_modules_have_snapshots.py
```

Checks:
- each `public/*` module must be referenced by at least one `tests/modules/snapshots/*.cmd`

## 8) Module cache perf gate

Command:

```bash
tools/modules_cache_perf.sh tests/modules/mod_graph/main.vit
```

Checks:
- enforces cold/hot latency thresholds (`COLD_MAX_MS`, `HOT_MAX_MS`)
- enforces hot/cold ratio (`HOT_RATIO_MAX`) when cold run duration is significant (`RATIO_MIN_COLD_MS`)

## 9) Packages governance lint

Command:

```bash
tools/lint_packages_governance.py
```

Checks on `src/vitte/packages/**/mod.vit`:
- `OWNERS` mandatory and non-empty (`@team/name`)
- `info.vit` mandatory with strict `PACKAGE-META`: `owner`, `stability`, `since`, `deprecated_in`
- owner coherence: `info.vit owner` must exist in `OWNERS`
- forbids `_` in path segments unless legacy allowlisted
- forbids legacy self-leaf imports (`vitte/x/x`) in package `.vit` files
- forbids `<pkg>.vit` leaf file when `mod.vit` exists (unless allowlisted)
- forbids glob imports in `packages/public/*`

## CI usage

```bash
make module-starter MODULE=<domain/name> OWNER=@team/name
make package-layout-lint
make package-layout-lint-strict
make module-leaf-file-lint
make legacy-import-path-lint
make module-tree-lint
make module-naming-lint
make critical-module-contract-lint
make experimental-modules-lint
make public-modules-snapshots-lint
make modules-perf-cache
make packages-governance-lint
make packages-report
make packages-contract-snapshots
make packages-gate
make migration-check
make release-modules-gate
```
