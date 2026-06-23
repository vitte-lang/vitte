# Changelog - Vitte Bootstrap Toolchain

## [unreleased] - 2026-06-23 - Diagnostics: JSON output

### ✅ 231. `--format json` for `vitte check`
- Added native `--format json` CLI flag to produce machine-readable diagnostics JSON from `vitte check`.
- This enables tooling to consume diagnostics without needing ANSI stripping wrappers.

## [0.3.8] - 2026-06-19 - Installer Payload and Packaging Hardening

### ✅ 228. Complete stdlib payloads in installers
- Added the real `src/vitte/stdlib/` tree to Debian x86_64, Debian i686, Debian i586, Raspberry Pi 4 ARM64, macOS, macOS i686, Windows, and shared package-builder payloads.
- Kept `src/vitte/packages/` as source packages and installed it alongside the standard library instead of treating it as the whole stdlib.
- Included compiler sources with the same installed source layout where legacy installers previously copied only partial payloads.
- Updated Windows NSIS packaging to install the source payload under `share\vitte\src` instead of relying on the removed `data\stdlib` staging path.

### ✅ 229. Packaging build target and cleanup safety
- Switched installer build scripts to use `BUILD_TARGET=bootstrap-all` by default so packaging does not fail on workspace-wide source audits triggered by vendored editor dependencies.
- Limited package clean steps to package-specific staging and output files, preserving unrelated `build/`, `pkgout/`, and tracked release artifacts.
- Normalized Debian `md5sums` generation to use package-relative paths.
- Fixed Debian build-essential detection so package checks test the installed package through `dpkg-query` instead of looking for a non-existent `build-essential` executable.

### ✅ 230. Makefile shell portability
- Replaced Bash-only prerequisite checks in packaging Makefiles with POSIX-compatible `uname` and `command -v` checks.
- Fixed `Makefile.pkg-i686` recipe indentation so `make -f Makefile.pkg-i686 help` parses and runs correctly.
- Updated packaging manifests and distribution text to distinguish source packages from the real standard library path.

## [0.3.7] - 2026-06-18 - Editor Integration Refresh

### ✅ 225. Generated editor highlight pipeline
- Extended `tools/generate_editor_highlights.py` so generated Nano, Vim, and Geany assets share the richer Vitte syntax surface.
- Refreshed editor highlight snapshots and coverage reports for Nano, Vim, and Geany.
- Added `.vitl` recognition to Nano, Vim, and Geany editor integrations alongside `.vit` and `.vitte`.

### ✅ 226. Nano and Vim authoring polish
- Improved Nano highlighting for declarations, exports, signature types, constants, constructors, numeric literals, operators, delimiters, comments, and trailing whitespace.
- Added Nano editing hints for `//` comments and two-space tab insertion.
- Improved Vim syntax highlighting for exports, signature types, escapes, constants, constructors, operators, delimiters, hash comments, and trailing whitespace.
- Expanded Vim filetype support with `.vitl`, `formatprg`, compiler integration, suffix resolution, stronger indentation behavior, and a broader UltiSnips set.

### ✅ 227. Geany integration upgrade
- Expanded Geany filetype configuration with richer keyword coverage, a context check action, and a larger build menu covering file checks, project checks, formatting, parsing, symbol discovery, diagnostics, manifest inspection, file listing, and version reporting.
- Updated Geany extension mappings and common filetype fragments for `.vit`, `.vitte`, and `.vitl`.
- Reworked Geany snippets into complete `[vitte]` and `[Vitte]` sections covering modules, entrypoints, procedures, types, control flow, diagnostics, compiler passes, tests, comments, and role contracts.
- Hardened Geany install/uninstall scripts with idempotent snippet markers, dual filetype-definition installs, working-directory mode rewriting, and clean removal of Vitte sections and extension mappings.

## [0.3.6] - 2026-06-16 - Truth Triangle, Bootstrap Native, and Stdlib Runtime Hardening

### ✅ 217. Stable syntax profile and driver surface parity
- Added the `stable-v1` syntax profile to the compiler driver option catalog.
- Normalized syntax profile aliases through the driver normalization path.
- Extended driver smoke coverage so option catalog, normalization, and tokenized parsing stay aligned.

### ✅ 218. Native bootstrap vertical slices
- Extended the bootstrap native path beyond fixed constants to build simple `main` programs from parsed/native IR return values.
- Added native build/run coverage for a minimal frontend fixture and truth-triangle programs.
- Regenerated native shell snapshots, emission hashes, and seed manifest after bootstrap changes.

### ✅ 219. Truth triangle gates
- Added `tools/truth_triangle_gate.sh` with manifest-driven `check -> build -> run` validation.
- Added `make truth-triangle` for 20 stable compiler/frontend programs.
- Promoted generics and traits fixtures to stable native run coverage, including the `generics_positive` executable returning `7`.

### ✅ 220. Stdlib/runtime truth triangle
- Added `make truth-triangle-stdlib` with 20 stable stdlib/runtime fixtures.
- Added `make truth-triangle-stdlib-deep` with 20 additional stable stdlib/runtime programs that combine multiple public stdlib calls per executable.
- Replaced the stdlib/runtime baseline fixtures with real stdlib calls across strings, collections, JSON, path, IO, encoding, crypto, compression, math, regex, datetime, OS, memory, async, threading, runtime/ABI, and kernel readiness surfaces.
- Representative covered calls include:
  - `strings.str_length("vitte")` returns `5`.
  - `arithmetic.add_i64(4, 6)` returns `10`.
  - `strings.str_length(io.path_join("src", "vitte"))` returns `9`.
  - `json.json_module_count()` returns `7`.
  - `vector.vector_push(vector.vector_new(2), 7)` returns `1`.
  - `crypto.crypto_module_count()` returns `5`.
- Extended the bootstrap extractor with a narrow sum evaluator for deterministic stdlib terms, allowing deep fixtures such as module-count mixes, metadata length mixes, and path/encoding/math combinations while preserving full manifest stability.

### ✅ 221. MIR optimization gate restoration
- Added the MIR optimization module contract and smoke fixture required by `tools/mir_opt/run_checks.py`.
- Wired the MIR optimization check into `make mir-opt-gate`.
- Verified MIR, stdlib, driver parity, bootstrap snapshots, and truth-triangle gates together.

### ✅ 222. AST interning compiler flow
- Promoted the AST interning package from a standalone package smoke to a compiler-facing flow fixture.
- Added `tests/pkg/compiler_ast_interning_flow.vit`, which parses a mini module, interns the parsed space/proc names, and verifies duplicate handle reuse.
- Added a truth-triangle native vertical slice for `interning.ast_intern_smoke()`, bringing the core truth triangle to 21 stable build/run programs.
- Extended the bootstrap native extractor to allow targeted `src/vitte/packages/...` imports and deterministic AST interning smoke evaluation.
- Added the internal driver pass `compiler_ast_interning.vit`, which converts parsed `compiler_ast.Module` names into interned handles and reports count/reuse/validity.
- Added package and truth-triangle coverage for the internal driver AST interning pass, bringing the core truth triangle to 22 stable build/run programs.

### ✅ 223. Compiler gate closure and ecosystem targets
- Made `make compiler-gate` pass end-to-end after the project scan.
- Aligned `compiler-topology-gate` with the active compiler tree now that legacy `src/vitte/compiler/components` is removed.
- Completed grammar alignment coverage: declarations, statements, expressions, types, and patterns are now all complete and fixture-tested.
- Added the AArch64 backend target profile and wired it into `vitte/compiler/backend/target/mod`.
- Added roadmap ecosystem stdlib contracts for profiling and networking (`socket`, `udp`, `http`).
- Removed stale bootstrap bridge markers from the seed/native payload path so the real compiler pipeline audit now reports no informational bridge markers.
- Expanded the core truth triangle from 22 to 50 stable `check -> build -> run` programs.
- Added 28 native-surface fixtures covering forms, picks, helper procs, generic helpers, module imports, stdlib calls, pattern metrics, async/effects/unsafe/FFI contracts, macro/comptime contracts, and backend target contracts.
- Extended the bootstrap native extractor so deterministic `main` programs can build/run while carrying top-level form, pick, and helper procedure declarations.
- Verified `compiler-gate`, `truth-triangle`, `truth-triangle-stdlib`, `truth-triangle-stdlib-deep`, `packages-gate`, `stdlib` checks, and the real compiler pipeline audit.

### ✅ 224. Runtime, self-host, tooling, and normative spec rails
- Added `make runtime-stdlib-real`, which runs host-backed probes for real file IO, path normalization, loopback HTTP, threading locks, memory allocation, and crypto vectors.
- Added `make selfhost-completion-audit` to build `vittec0 -> vittec1 -> vittec2 -> vittec3`, record parity/payload status, and keep the current shell-payload transition explicit.
- Added `make selfhost-completion-strict` as the release-grade gate that fails until byte parity and payload-free self-hosting are both true.
- Added a deterministic Vitte formatter with changed-file checking through `make format-check`.
- Added `docs/spec/normative.md` plus `make spec-normative-check` for the required normative sections: memory, modules/imports, ABI, unsafe, traits/generics, effects/async, macros/comptime, and version compatibility.
- Added `make language-maturity-gate` to tie runtime, self-host audit, formatter, package manager, LSP, and normative spec checks into one maturity rail.

## [0.3.5] - 2026-06-13 - Fluent Diagnostics, Localized Explain, and Installer Language Selection

### ✅ 213. Expanded Fluent diagnostic catalog
- Expanded the public diagnostics contract to 1,388 stable diagnostic codes across syntax, lexer/parser, names/modules, type checking, generics, traits, ownership, lifetimes, const evaluation, macros, HIR/MIR/IR, backend, linker, runtime, driver, bootstrap, and resource-limit families.
- Added centralized locale and diagnostics catalog metadata in:
  - `tools/diagnostics_locales.py`
  - `tools/diagnostic_catalog_data.py`
- Regenerated `src/vitte/compiler/infrastructure/diagnostics/fluent_catalog.vit` from `locales/*/diagnostics.ftl`.

### ✅ 214. Fifteen-language Fluent coverage
- Added/validated diagnostic catalogs for:
  - `en`, `fr`, `es`, `de`, `it`, `pt-BR`, `nl`, `pl`, `ru`, `uk`, `zh-CN`, `ja`, `ko`, `tr`, `ar`.
- Localized Fluent diagnostic messages and generated explain summaries in each supported language instead of leaving new locale catalogs as English fallbacks.
- Preserved Vitte language keywords and technical mode names in localized messages where translating them would make diagnostics less actionable.
- Every public code now has:
  - `CODE`
  - `CODE.summary`
  - `CODE.cause`
  - `CODE.step1`
  - `CODE.fix`
  - `CODE.example`
- Hardened `tools/check_diagnostics_locales.py` and `tools/update_diagnostics_ftl.py` so missing message or explanation keys fail validation.

### ✅ 215. User-first explain output
- Added localized `diagnostic_explain_text_lang(code, lang)` and kept `diagnostic_explain_text(code)` as the English default.
- `vitte explain CODE --lang <locale>` now routes through the selected locale in the compiler driver.
- Explain output now includes:
  - language
  - localized message
  - summary
  - cause
  - step 1
  - fix
  - example
  - docs and quick-fix metadata
- Normal diagnostic rendering now includes a first action line:
  - `step 1:` in human output
  - `step_1` in compact text output

### ✅ 216. Installer and shell integration
- Windows NSIS installer now offers language selection for the supported locale set, installs Fluent catalogs, and writes the selected language to the user Vitte config.
- Debian, macOS, and prefix installers now include `locales/` in the installed share tree.
- `env.sh` now derives `VITTE_LANG` from the system locale when the user has not configured one explicitly.
- Shell completions for `--lang` now expose the full supported locale set.

### 🧪 Diagnostics regression coverage
- Added explain snapshots under `tests/explain_snapshots/` for:
  - `E0001`
  - `TYPECK_E_ASSIGN_MISMATCH`
  - `BORROWCK_E_USE_AFTER_MOVE`
  - `E0001` in French
- Added `tools/generate_explain_snapshots.py` with `--check` mode.
- Strengthened `src/vitte/compiler/tests/diagnostic_snapshot_tests.vit` to require localized explain context and `step 1` rendering.

## [0.3.4] - 2026-05-23 - Compiler Health, Recovery, and Incremental Stability

### ✅ 203. Full internal metrics system
- Added compiler health metrics output for `vittec metrics ...` including phase timings, phase memory, cache hit/miss estimates, AST/MIR size, diagnostics count, backend timings, and linker timings.
- Added project path metrics mode (`vittec metrics build project/`) with auto-discovery of `.vit` files and aggregate compile-set metrics.

### ✅ 204. Compiler health dashboard
- Added health dashboard JSON surface with tests/fuzz/stress/self-host/determinism/benchmark trend status and regression alerts.

### ✅ 205. Advanced diagnostic engine
- Added advanced diagnostics helpers for multi-span style enrichment, typo/import suggestions, lifetime explanation hints, borrow-trace visualization, color/unicode-safe rendering helpers.

### ✅ 206. Error recovery quality system
- Added parser recovery coverage for blocks/expressions/imports, explicit synchronization points, and recovery metrics with primary-error preservation contract.

### ✅ 207. Language server stability suite
- Added LSP stability suite for incremental diagnostics behavior, concurrent edit safety, malformed input robustness, workspace reload and rename stress behavior.

### ✅ 208. Incremental compilation complete (local-change focused)
- Added fine-grain invalidation helpers for local-change recompilation planning and domain-level invalidation reporting:
  - dependency invalidation
  - query invalidation
  - MIR cache invalidation
  - backend cache invalidation
  - diagnostics invalidation
  - symbol invalidation
- Added pipeline test coverage for minimal-local-change recompilation contract.

### ✅ 209. Distributed build foundation
- Added distributed build groundwork with:
  - remote cache entry model
  - deterministic compile-unit hashing
  - dependency snapshot serialization
  - build job graph export
  - deterministic artifact hash contract

### ✅ 210. Formal type soundness roadmap
- Added formal roadmap primitives for:
  - typing judgments
  - ownership semantics rules
  - progress/preservation groundwork status
  - MIR semantic mapping readiness
  - unsafe escape analysis baseline

### ✅ 211. Verified backend pipeline
- Added backend verification report and checks for:
  - MIR -> backend invariants
  - ABI assertions
  - stack safety checks
  - control-flow verification
  - object validation

### ✅ 212. Compiler chaos testing
- Added compiler chaos suite baseline covering:
  - missing files
  - invalid sysroot-like target
  - malformed package/profile combinations
  - random backend failure surface
  - linker-like failure surface
- Contract: explicit diagnostics and no silent corruption accepted by suite.

## [0.3.3] - 2026-05-22 - Professional Seed Delivery Gates (40->120)

### ✅ Pipeline and quality gates (seed path)
- Added professional gate coverage from compiler validation through delivery:
  - no-stub gate, phase coverage checks, import cycle tests, visibility checks,
    ABI/toolchain checks, deterministic build checks, and strict CI gate wiring.
- Added structured artifact outputs under `build/` for tokens/AST/HIR/MIR/backend logs/diagnostics.

### ✅ Self-host and bootstrap baseline
- Added executable self-host checks:
  - stage0 -> stage1 -> stage2 chain
  - behavior comparison reports + artifact hash reports.
- Added bootstrap documentation set under `docs/bootstrap/` with stage roles, reproducibility and troubleshooting.

### ✅ Release/package/workspace baseline
- Added minimal operational commands on seed command surface:
  - `release build|verify`
  - `package init|add|build|test`
  - `workspace check --strict`
  - `doctor`, `compat-check`, `migrate`, `stress`, `ci-gate --strict`.
- Added release archive/checksum flow and delivery layout baseline for installable artifacts.

### ✅ Docs and truthfulness policy
- Added compiler docs/spec/compat matrix pages matching current implemented subset.
- Hardened rule: features are considered real only when pipeline-connected, test-covered, documented, and proven end-to-end.

## [0.3.2] - 2026-05-22 - Compiler Hardening, Native Panic Boundary, Expanded Targets

### ✅ Compiler pipeline hardening
- Enforced strict stage status handling with explicit fatal behavior for missing critical phases.
- Extended runtime pipeline assertions against silent success paths.
- Strengthened linker/runtime integration by auto-wiring the C runtime payload in linker outputs.

### ✅ Native panic boundary (runtime ABI)
- Added native panic boundary ABI primitives and integrated them in compiler entry flow:
  - begin / end / triggered / code / reset.
- Added concrete runtime C implementation for panic boundary state tracking.
- Unified ICE rendering path for driver/CLI boundary behavior.

### ✅ Target and configuration expansion
- Introduced `TargetConfig` with canonical metadata:
  - pointer size, endianness, ABI, linker path, object format.
- Added broad alias coverage for modern and legacy target names, normalized to supported backend triples.

### ✅ Compiler logger
- Added `CompilerLogger` with levels:
  - `trace`, `debug`, `info`, `warn`, `error`.
- Wired CLI controls:
  - `--verbose`
  - `--trace-pipeline`

### ✅ Community/Docs
- Updated public release communication in News and Community pages.
- Added explicit maintainer signal from `@goginx` for compiler direction and stability policy.

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
