# Changelog - Vitte Bootstrap Toolchain

## [0.3.1] - 2026-05-21 - Native JSON Parity Contract And Compiler Corpus Gate

### ✅ Native JSON pipeline bridge (bootstrap -> compiler corpus)
- Exposed native JSON surfaces on bootstrap command surface:
  - `parse --dump-ast-json`
  - `check --dump-hir-json`
  - `check --dump-mir-json`
  - `check --diagnostics-json`
- Replaced bootstrap placeholder payloads with deterministic structural metrics extracted from real compiler sources.

### 🔒 Schema contract stabilization
- Introduced stable contract fields across JSON surfaces:
  - `schema=vitte.bootstrap.surface`
  - `schema_version=1.0.0`
  - `surface_version=1.0.0`
  - stable field ordering and normalized shape blocks (`source`, `shape`, surface payload).

### 🧪 Parity gate extension
- `tools/stage_parity_structured.sh` now runs on a compiler corpus (driver, frontend, middle, diagnostics) instead of a single source by default.
- Gate now emits an aggregated report with per-source/per-surface match status in `target/reports/stage_parity/stage1_stage2_parity.json`.

### 🧰 Regression checks
- Added `tools/native_json_schema_contract_test.sh`:
  - fails if JSON flags disappear from stage1/stage2 `--help`
  - fails if `schema_version`/contract fields drift without explicit versioning.
- Added Make target:
  - `make native-json-schema-contract`
- Wired contract test in strict compiler gate (`tools/compiler_max_gate.sh strict` path).

## [0.3.0] - 2026-05-20 - Strict Compiler Driver And Selfhost Gate

### ✅ Follow-up Hardening (2026-05-20)
- **`--stop-after` contract hardened in driver**:
  - centralized CLI stage mapping in `src/vitte/compiler/driver/compiler.vit`
  - exact stop behavior preserved in final result stage (no forced `Done` on stop)
  - added stop-after coverage tests for `parse/hir/typeck/mir/ir/codegen/link/run` in `src/vitte/compiler/tests/pipeline_tests.vit`
- **Driver diagnostics normalization completed**:
  - removed remaining `report.error_text(...)` use in driver mapping path
  - switched to typed diagnostic construction (`severity/code/file/line/column/message`)
  - introduced canonical driver diagnostic report model at `src/vitte/compiler/diagnostics/report.vit`
- **Structured stage1/stage2 parity gate added**:
  - new `tools/stage_parity_structured.sh` compares `parse/check/dump-native-ir` outputs and emits JSON + diff report under `target/reports/stage_parity/`
  - wired into `compiler-max-gate-strict`
- **Bootstrap reproducibility gate added**:
  - new `tools/bootstrap_selfhost_repro.sh` runs multi-pass bootstrap/selfhost drift checks (binary + IR + selfhost-source + version hashes)
  - exposed as `make bootstrap-selfhost-repro`
  - wired into strict compiler max gate
- **Explicit backend mode for stage2 bootstrap**:
  - `toolchain/scripts/bootstrap/stage2.sh` now supports `VITTE_BACKEND_MODE=shell|native`
  - explicit fallback policy controlled by `VITTE_BACKEND_FALLBACK=1|0`
- **Strict CI release pressure increased**:
  - `ci-strict` now includes `selfhost-hard-strict`

### 🎯 Major Changes
- **Strict compiler gate enabled**: `make compiler-max-gate-strict` now runs a compiler-focused strict chain without silent fallback paths.
- **Strict selfhost release gate**: `make selfhost-hard-strict` now validates bootstrap + source lock + reachability + strict compiler gate in one PASS/FAIL target.
- **Driver pipeline wiring**: `src/vitte/compiler/driver/compiler.vit` now calls real frontend/analysis/middle/backend pipeline surfaces instead of placeholder-only stage hops.

### 🔒 Source Of Truth And Contracts
- Added hard entry lock checks for:
  - `COMPILER_SOURCE_ROOT=src/vitte/compiler`
  - `COMPILER_ENTRY_POINT=src/vitte/compiler/driver/compiler.vit`
- Added migration gate for `dignostics` typo references with allowlist-based transition control.
- Added transitive compiler reachability audit from driver entry with explicit allowlist and progressive pruning.

### 🧪 Testing And Gates
- Added strict smoke scripts for:
  - recovery diagnostics
  - determinism
  - incremental/cache consistency
  - crash/diagnostic snapshot path
- Added bootstrap parser surface test for the currently supported driver/stage2 source subset.
- Restored missing diagnostics manifests required by strict gate execution.

### 📚 Documentation
- Updated compiler migration/docs context to reflect strict gates and current driver wiring status.
- Added community/news notes for the latest compiler strictness and pipeline alignment pass.

## [0.2.0] - 2026-05-14 - Bootstrap Sophistication Upgrade

### 🎯 Major Changes
- **4-Stage Bootstrap**: Upgraded from 3-stage to 4-stage bootstrap matching native bootstrap compiler sophistication
  - Stage 0 (Seed): Initial compiler bootstrap
  - Stage 1 (Self-hosted): First compilation with seed
  - Stage 2 (Verification): Second compilation with stage1
  - Stage 3 (Final Verification): Third compilation with stage2
- **Enhanced Verification**: Changed final verification from `stage1 == stage2` to `stage2 == stage3`
- **Native-Equivalent Process**: Now follows the same rigorous verification pattern as native toolchain

### 📁 Files Added
- `toolchain/stage3/src/main.vit` - Stage 3 compiler source
- `toolchain/stage3/README.md` - Stage 3 documentation
- `toolchain/test_bootstrap_reproducibility.sh` - Automated reproducibility tests

### 📝 Files Modified
- `toolchain/src/build.vit` - Added `init_stage3()` function
- `toolchain/src/orchestrator.vit` - Added `create_stage3_phase()`, updated verification
- `toolchain/bootstrap.sh` - Updated bootstrap logic for 4 stages
- `toolchain/BOOTSTRAP_DESIGN.md` - Updated architecture documentation
- `toolchain/README.md` - Updated overview and diagrams
- `toolchain/EXAMPLES.md` - Updated example outputs
- `toolchain/SUMMARY.md` - Updated project description

### 🧪 Testing
- Added automated reproducibility tests
- Verified 4-stage bootstrap execution
- Confirmed final verification logic

### 📚 Documentation
- Updated all documentation to reflect 4-stage process
- Added comprehensive README for stage3
- Enhanced architecture diagrams

### 🔧 Technical Details
- Bootstrap now generates shell scripts (current compiler limitation)
- Verification compares generated scripts for consistency
- Maintains backward compatibility with existing toolchain

### 🎉 Impact
- Bootstrap process now matches sophistication of native compilers
- Enhanced trust in compiler self-hosting capability
- Improved reproducibility verification
- Better alignment with industry standards

---

## [0.1.0] - 2026-05-10 - Initial Bootstrap Toolchain

### ✨ Initial Release
- Complete 3-stage bootstrap toolchain implementation
- Multi-platform support (Linux, macOS, Windows)
- JSON-based configuration system
- Comprehensive documentation
- Build verification and caching
