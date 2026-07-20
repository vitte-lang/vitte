# ============================================================
# Makefile — Vitte Project
# Target: Vitte compiler / runtime / std / tools
# ============================================================

# ------------------------------------------------------------
# Global config
# ------------------------------------------------------------

PROJECT      := vitte
BUILD_DIR    := build
BIN_DIR      := bin
SRC_DIR      := src
STD_DIR      := src/vitte/stdlib
TOOLS_DIR    := tools

CCACHE       ?= $(shell command -v ccache 2>/dev/null)
JOBS         ?= $(shell (sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4) | tr -d '\n')
OPT_LEVEL    ?= 2
DEBUG_SYMBOLS ?= 1
LTO          ?= 0
NDEBUG_BUILD ?= 0
PGO_MODE     ?= off
PGO_DIR      ?= target/pgo
PGO_RAW      ?= $(PGO_DIR)/default.profraw
PGO_DATA     ?= $(PGO_DIR)/default.profdata
AR           := ar
RM           := rm -rf
MKDIR        := mkdir -p
INSTALL      := install
CP           := cp -f
VITTE_BOOTSTRAP ?= $(BIN_DIR)/vittec0
DRIVER_BOOTSTRAP_RUNNER ?= $(VITTE_BOOTSTRAP)

PREFIX       ?= /usr/local
DESTDIR      ?=
BINDIR       ?= $(DESTDIR)$(PREFIX)/bin
USER_HOME    ?= $(HOME)
VIM_DIR      ?= $(USER_HOME)/.vim
EMACS_DIR    ?= $(USER_HOME)/.emacs.d
NANO_DIR     ?= $(USER_HOME)/.config/nano
LEGACY_ALLOWLIST_BUDGET ?= 5
PKG_VERSION_FILE ?= toolchain/scripts/package/PACKAGE_VERSION
PKG_VERSION ?= $(shell tr -d ' \r\n' < $(PKG_VERSION_FILE) 2>/dev/null || echo 2.1.1)

FORMAT_TOOL  ?= true

# ------------------------------------------------------------
# Files
# ------------------------------------------------------------

VITTE_SOURCES := $(shell find $(SRC_DIR)/vitte -name '*.vit' -o -name '*.vitl' 2>/dev/null)
VITTE_COMPILER_ROOT ?= src/vitte/compiler
VITTE_COMPILER_CHECKS := $(shell find $(VITTE_COMPILER_ROOT) -type f \( -name '*.vit' -o -name '*.vitl' \) \
	! -path '*/tests/*' \
	! -path '*/benches/*' | sort)
VITTE_SEED_GATE_SOURCES := \
	src/vitte/compiler/main.vit \
	tests/analysis/edge_loop_phi.vit \
	tests/analysis/edge_pointer_alias.vit \
	tests/analysis/edge_unreachable.vit \
	tests/analysis/positive_branching.vit \
	tests/analysis/positive_linear.vit \
	tests/bootstrap_native/main_const_int.vit \
	tests/bootstrap_native/main_proc.vit \
	tests/bootstrap_native/no_main_default.vit \
	tests/check/main.vit \
	tests/diag_snapshots/core_ir_golden.vit \
	tests/frontend/frontend_error.vit \
	tests/frontend/frontend_nominal.vit \
	tests/golden/frontend/fixtures/hello_min.vit \
	tests/modules/mod_graph/main.vit \
	tests/strict_ok.vit \
	tests/tmp_diag_path.vit
VITTE_COMPILER_CHECK_MIN ?= 70
VITTE_ANALYSIS_MODE ?= build
VITTE_STRICT_FAIL ?= 1
VITTE_BRAIN_AUTOFIX_SAFE ?= 0
DEPFILES     =
KERNEL_TOOLS_DIR := target/kernel-tools
MACOS_UNIVERSAL_DIR := target/universal
MACOS_ARM64_DIR := target/macos-arm64
MACOS_X86_64_DIR := target/macos-x86_64
MACOS_PKG_DIR := target/macos-pkg
MACOS_CC_WRAPPER_DIR := target/tooling
MACOS_UNIVERSAL_SRC ?= src/vitte/compiler/main.vit
MACOS_PKG_ENTRY := $(MACOS_PKG_DIR)/main.pkg.vit
MACOS_PKG_COMPILER_ROOT := /usr/local/share/vitte/src/vitte/compiler
MACOS_PKG_COMPILER_ENTRY := $(MACOS_PKG_COMPILER_ROOT)/main.vit

# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------

.PHONY: all
all: build

# ------------------------------------------------------------
# Install
# ------------------------------------------------------------

.PHONY: install install-bin install-editors install-geany uninstall-geany install-debian install-debian-2.1.1
install: build install-bin install-editors

install-bin:
	@$(MKDIR) "$(BINDIR)"
	@$(INSTALL) -m 755 "$(BIN_DIR)/$(PROJECT)" "$(BINDIR)/$(PROJECT)"
	@echo "Installed binary: $(BINDIR)/$(PROJECT)"

install-editors:
	@$(MKDIR) "$(VIM_DIR)/syntax" "$(VIM_DIR)/indent" "$(VIM_DIR)/ftdetect" "$(VIM_DIR)/ftplugin" "$(VIM_DIR)/compiler"
	@$(CP) editors/vim/syntax/vitte.vim "$(VIM_DIR)/syntax/vitte.vim"
	@$(CP) editors/vim/indent/vitte.vim "$(VIM_DIR)/indent/vitte.vim"
	@$(CP) editors/vim/ftdetect/vitte.vim "$(VIM_DIR)/ftdetect/vitte.vim"
	@$(CP) editors/vim/ftplugin/vitte.vim "$(VIM_DIR)/ftplugin/vitte.vim"
	@$(CP) editors/vim/compiler/vitte.vim "$(VIM_DIR)/compiler/vitte.vim"
	@echo "Installed Vim syntax: $(VIM_DIR)"
	@$(MKDIR) "$(EMACS_DIR)/lisp"
	@$(CP) editors/emacs/vitte-mode.el "$(EMACS_DIR)/lisp/vitte-mode.el"
	@$(CP) editors/emacs/vitte-indent.el "$(EMACS_DIR)/lisp/vitte-indent.el"
	@echo "Installed Emacs mode: $(EMACS_DIR)/lisp"
	@$(MKDIR) "$(NANO_DIR)"
	@$(CP) editors/nano/vitte.nanorc "$(NANO_DIR)/vitte.nanorc"
	@touch "$(USER_HOME)/.nanorc"
	@if ! grep -q 'include "$(NANO_DIR)/vitte.nanorc"' "$(USER_HOME)/.nanorc"; then \
		printf '\ninclude "$(NANO_DIR)/vitte.nanorc"\n' >> "$(USER_HOME)/.nanorc"; \
	fi
	@echo "Installed Nano syntax: $(NANO_DIR)/vitte.nanorc"
	@$(MAKE) install-geany USER_HOME="$(USER_HOME)"
	@echo "Installed Geany config: $(USER_HOME)/.config/geany"

install-geany:
	@GEANY_HOME="$(GEANY_HOME)" XDG_CONFIG_HOME="$(XDG_CONFIG_HOME)" APPDATA="$(APPDATA)" ./editors/geany/install_geany.sh

uninstall-geany:
	@GEANY_HOME="$(GEANY_HOME)" XDG_CONFIG_HOME="$(XDG_CONFIG_HOME)" APPDATA="$(APPDATA)" ./editors/geany/uninstall_geany.sh

install-debian:
	@$(MAKE) pkg-debian-install

install-debian-2.1.1: install-debian

# ------------------------------------------------------------
# Build
# ------------------------------------------------------------

.PHONY: build
build: dirs bootstrap-all compiler-test-suite-check-gate compiler-no-fallback-gate driver-native-json-surface-gate bootstrap-native-snapshots vitte-source-audit packages-check-all

.PHONY: vittec-kernel kernel-tools
vittec-kernel: vitte-bootstrap-check
	@$(MKDIR) $(KERNEL_TOOLS_DIR)
	@echo "[vittec-kernel] Vitte-only bootstrap: no host kernel compiler artifact is built"

kernel-tools: vittec-kernel

.PHONY: build-fast
build-fast: dirs
	@$(MAKE) --no-print-directory -j$(JOBS) build

.PHONY: build-release
build-release:
	@$(MAKE) --no-print-directory clean build-fast

.PHONY: pgo-clean
pgo-clean:
	@$(RM) "$(PGO_DIR)"
	@$(MKDIR) "$(PGO_DIR)"

.PHONY: build-pgo-generate
build-pgo-generate:
	@$(MAKE) --no-print-directory clean pgo-clean build-fast

.PHONY: build-pgo-use
build-pgo-use:
	@$(MAKE) --no-print-directory clean build-fast

# ------------------------------------------------------------
# Directories
# ------------------------------------------------------------

.PHONY: dirs
dirs:
	@$(MKDIR) $(BUILD_DIR)
	@$(MKDIR) $(BIN_DIR)

-include $(DEPFILES)

# ------------------------------------------------------------
# Formatting
# ------------------------------------------------------------

.PHONY: format
format:
	@python3 tools/vitte_format.py

.PHONY: format-check
format-check:
	@python3 tools/vitte_format.py --check --changed

# ------------------------------------------------------------
# Static analysis
# ------------------------------------------------------------

.PHONY: tidy vitte-source-audit vitte-legacy-text-audit vitte-bootstrap-check bootstrap-native-snapshots compiler-real-native-gate compiler-test-suite-check-gate compiler-no-fallback-gate driver-native-json-surface-gate
tidy: vitte-source-audit vitte-legacy-text-audit

vitte-source-audit:
	@bad="$$(find . \
		-path './.git' -prune -o \
		-path './bin' -prune -o \
		-path './build' -prune -o \
		-path './.pkgstage' -prune -o \
		-path './target' -prune -o \
		-path './src/vitte/compiler/backends/runtime_c' -prune -o \
		-path './editors/tree-sitter/bindings/c/tree-sitter-vitte.h' -prune -o \
		-path './editors/tree-sitter/bindings/node/binding.cc' -prune -o \
		-path './editors/tree-sitter/bindings/python/tree_sitter_vitte/binding.c' -prune -o \
		-path './editors/tree-sitter/bindings/swift/TreeSitterVitte/vitte.h' -prune -o \
		-path './editors/tree-sitter/src/parser.c' -prune -o \
		-path './editors/tree-sitter/src/tree_sitter/alloc.h' -prune -o \
		-path './editors/tree-sitter/src/tree_sitter/array.h' -prune -o \
		-path './editors/tree-sitter/src/tree_sitter/parser.h' -prune -o \
		-path './vscode/VitteLangVsCode-main/.vscode-test' -prune -o \
		-path './vscode/VitteLangVsCode-main/node_modules' -prune -o \
		-path './vscode/VitteLangVsCode-main/vitte_out.cpp' -prune -o \
		-type f \( -name '*.'c -o -name '*.'cc -o -name '*.'c'pp' -o -name '*.'cxx -o -name '*.'h -o -name '*.'h'pp' -o -name '*.'hxx \) -print | sort)"; \
	if [ -n "$$bad" ]; then \
		echo "[vitte-source-audit][error] non-Vitte source files remain in workspace:"; \
		printf '%s\n' "$$bad"; \
		exit 1; \
	fi; \
	echo "[vitte-source-audit] ok: workspace source is Vitte-only outside approved native/runtime staging paths"

vitte-legacy-text-audit:
	@tools/ci_surface_legacy_audit.sh

vitte-bootstrap-check:
	@test -x "$(VITTE_BOOTSTRAP)" || (echo "[vitte-bootstrap-check][error] missing executable $(VITTE_BOOTSTRAP)" >&2; exit 2)
	@count="$$(printf '%s\n' $(VITTE_COMPILER_CHECKS) | sed '/^$$/d' | wc -l | tr -d ' ')"; \
	if [ "$$count" -lt "$(VITTE_COMPILER_CHECK_MIN)" ]; then \
		echo "[vitte-bootstrap-check][error] compiler check coverage too low: $$count < $(VITTE_COMPILER_CHECK_MIN)"; \
		echo "[vitte-bootstrap-check][hint] verify VITTE_COMPILER_ROOT ($(VITTE_COMPILER_ROOT)) and file discovery pattern"; \
		exit 3; \
	fi; \
	echo "[vitte-bootstrap-check] coverage: $$count files"
	@set -e; \
	for src in $(VITTE_COMPILER_CHECKS); do \
		echo "[vitte-bootstrap-check][$(VITTE_ANALYSIS_MODE)] $$src"; \
		if [ "$(VITTE_ANALYSIS_MODE)" = "build" ]; then \
			if grep -Eq '^[[:space:]]*proc[[:space:]]+main[[:space:]]*\(' "$$src"; then \
				"$(VITTE_BOOTSTRAP)" build-native --src "$$src" --out "/tmp/vitte.native.bootstrap.out"; \
			else \
				"$(VITTE_BOOTSTRAP)" check "$$src"; \
			fi; \
		else \
			"$(VITTE_BOOTSTRAP)" check "$$src"; \
		fi; \
	done

bootstrap-native-snapshots:
	@tools/bootstrap_native_snapshots.sh

.PHONY: bootstrap-shell-fixed-point
bootstrap-shell-fixed-point: bootstrap-seed
	@tools/bootstrap_shell_fixed_point_test.sh

compiler-real-native-gate:
	@tools/compiler_real_native_gate.sh

compiler-test-suite-check-gate:
	@tools/compiler_test_suite_check_gate.sh

compiler-no-fallback-gate:
	@tools/compiler_no_fallback_gate.sh

driver-native-json-surface-gate:
	@tools/driver_native_json_surface_gate.sh

.PHONY: cli-diagnostics-snapshots
cli-diagnostics-snapshots:
	@tools/cli_diagnostics_snapshots.sh

# ------------------------------------------------------------
# Tests (placeholder)
# ------------------------------------------------------------

.PHONY: test
test:
	@toolchain/scripts/test/run.sh

.PHONY: security-input-limits-smoke
security-input-limits-smoke:
	@bash tools/security_input_limits_smoke.sh

.PHONY: sanitizers-maximal
sanitizers-maximal:
	@bash tools/sanitizers_maximal.sh

.PHONY: release-engineering
release-engineering:
	@bash tools/release_engineering_maximal.sh

.PHONY: version-compatibility-gate
version-compatibility-gate:
	@python3 tools/version_compatibility_gate.py

.PHONY: migration-system-check
migration-system-check:
	@bash tools/migration_system_check.sh

.PHONY: edition-system-check
edition-system-check:
	@bash tools/edition_system_check.sh

.PHONY: quickstart-check
quickstart-check:
	@tools/quickstart_check.sh

.PHONY: stdlib-integration-check
stdlib-integration-check:
	@tools/verify_stdlib_integration.sh

.PHONY: beta-feedback-report
beta-feedback-report:
	@tools/beta_feedback/validate_feedback_csv.py
	@tools/beta_feedback/generate_kpi_report.py
	@tools/beta_feedback/update_matrix_from_summary.py

.PHONY: profiling-baseline-report
profiling-baseline-report:
	@tools/profiling/generate_baseline_report.py

.PHONY: profiling-baseline-gate
profiling-baseline-gate:
	@tools/profiling/validate_baseline_csv.py
	@tools/profiling/generate_baseline_report.py
	@tools/profiling/update_matrix_from_baseline.py

.PHONY: startup-gate
startup-gate:
	@python3 tools/startup_benchmark.py --strict

.PHONY: startup-benchmark-baseline
startup-benchmark-baseline:
	@python3 tools/startup_benchmark.py --update-baseline

.PHONY: startup-benchmark-check
startup-benchmark-check:
	@python3 tools/startup_benchmark.py

.PHONY: roadmap-ecosystem-gate
roadmap-ecosystem-gate:
	@python3 tools/roadmap_ecosystem_check.py
	@test -f target/roadmap_ecosystem/ecosystem_status.json
	@test -f target/reports/roadmap_ecosystem_coverage.md

.PHONY: architecture-docs-check
architecture-docs-check:
	@python3 tools/architecture_docs_check.py

.PHONY: doctor
doctor:
	@tools/doctor.sh

.PHONY: doctor-error
doctor-error:
	@test -n "$(LOG)" || (echo "usage: make doctor-error LOG=/tmp/vitte.seed.err SRC=path/to/file.vit" >&2; exit 2)
	@python3 tools/vitte_brain_doctor.py "$(LOG)" "$(SRC)" --json-out target/reports/vitte_brain_last_error.json $(if $(filter 1,$(VITTE_BRAIN_AUTOFIX_SAFE)),--autofix-safe,)

.PHONY: selfhost-audit
selfhost-audit:
	@tools/selfhost_audit.sh

.PHONY: bootstrap-source-of-truth
bootstrap-source-of-truth:
	@tools/check_bootstrap_source_of_truth.sh

.PHONY: compiler-entry-lock
compiler-entry-lock:
	@tools/check_compiler_entry_lock.sh

.PHONY: compiler-path-typos
compiler-path-typos:
	@tools/check_compiler_path_typos.sh

.PHONY: frontend-token-contract
frontend-token-contract:
	@tools/check_frontend_token_contract.sh

.PHONY: diagnostics-migration-gate
diagnostics-migration-gate:
	@tools/check_diagnostics_migration.sh

.PHONY: compiler-reachability-audit
compiler-reachability-audit:
	@python3 tools/compiler_reachability_audit.py

.PHONY: compiler-real-pipeline-audit
compiler-real-pipeline-audit:
	@python3 tools/compiler_real_pipeline_audit.py

.PHONY: truth-triangle
truth-triangle:
	@tools/truth_triangle_gate.sh

.PHONY: truth-triangle-stdlib
truth-triangle-stdlib:
	@tools/truth_triangle_gate.sh tests/truth_triangle/stdlib_runtime_manifest.txt

.PHONY: truth-triangle-stdlib-deep
truth-triangle-stdlib-deep:
	@tools/truth_triangle_gate.sh tests/truth_triangle/stdlib_runtime_deep_manifest.txt

.PHONY: compiler-audit-report
compiler-audit-report:
	@python3 tools/compiler_audit_report.py

.PHONY: driver-surface-audit
driver-surface-audit:
	@$(DRIVER_BOOTSTRAP_RUNNER) check src/vitte/packages/compiler/driver/mod.vit >/dev/null
	@$(DRIVER_BOOTSTRAP_RUNNER) check src/vitte/packages/compiler/driver/info.vit >/dev/null
	@python3 tools/driver_runtime_surface_audit.py
	@echo "[driver-surface-audit] ok: compiler/driver package surface is Vitte-backed"

.PHONY: driver-surface-parity
driver-surface-parity: selfhost-driver-bootstrap

.PHONY: selfhost-driver-bootstrap
selfhost-driver-bootstrap:
	@tools/package_check_portable.sh src/vitte/packages/compiler/driver/mod.vit
	@$(DRIVER_BOOTSTRAP_RUNNER) check src/vitte/packages/compiler/driver/info.vit
	@$(DRIVER_BOOTSTRAP_RUNNER) check src/vitte/packages/compiler/driver/internal/normalize.vit
	@$(DRIVER_BOOTSTRAP_RUNNER) check src/vitte/packages/compiler/driver/internal/value_normalize.vit
	@$(DRIVER_BOOTSTRAP_RUNNER) check src/vitte/packages/compiler/driver/internal/normalized_options.vit
	@$(DRIVER_BOOTSTRAP_RUNNER) check src/vitte/packages/compiler/driver/internal/tokenized_parse.vit

.PHONY: bootstrap-seed
bootstrap-seed:
	@scripts/seed/install_seed.sh

.PHONY: seed-verify
seed-verify:
	@scripts/seed/verify_seed.sh

.PHONY: seed-manifest-update
seed-manifest-update:
	@scripts/seed/update_manifest.sh

.PHONY: seed-rotation-report
seed-rotation-report:
	@scripts/seed/rotation_report.sh

.PHONY: seed-contract-check
seed-contract-check:
	@tools/check_seed_contract.sh

.PHONY: bootstrap-source-coverage-check
bootstrap-source-coverage-check:
	@tools/check_bootstrap_source_coverage.sh

.PHONY: selfhost-subset-check
selfhost-subset-check:
	@tools/check_selfhost_subset.sh

.PHONY: bootstrap-native-drift-check bootstrap-generated-code-test bootstrap-multifile-native-test native-artifact-gate-test
bootstrap-native-drift-check:
	@tools/check_bootstrap_native_drift.sh

native-artifact-gate-test:
	@tools/native_artifact_gate_test.sh

bootstrap-generated-code-test: bootstrap-seed
	@tools/bootstrap_generated_code_test.sh

bootstrap-multifile-native-test: bootstrap-seed
	@tools/bootstrap_multifile_native_test.sh

.PHONY: posix-seed-shell-check
posix-seed-shell-check:
	@tools/check_posix_seed_shell.sh

.PHONY: seed-install
seed-install:
	@scripts/seed/install_seed.sh

.PHONY: seed-check
seed-check: bootstrap-seed
	@set -e; \
	run_with_deep_help() { \
		cmd="$$1"; src="$$2"; log="$$3"; \
		if eval "$$cmd" >"$$log" 2>&1; then return 0; fi; \
		echo "============================================================"; \
		echo "[vitte][error] analyse échouée"; \
		echo "[file] $$src"; \
		echo "[command] $$cmd"; \
		echo "------------------------------------------------------------"; \
		cat "$$log"; \
		echo "------------------------------------------------------------"; \
		echo "[context] aperçu du fichier"; \
		sed -n '1,140p' "$$src" || true; \
		echo "------------------------------------------------------------"; \
		echo "[suggestions]"; \
		echo "1. Relancer uniquement ce fichier: bin/vittec0 check --strict \"$$src\""; \
		echo "2. Essayer le parse isolé: bin/vittec0 parse \"$$src\""; \
		echo "3. Tracer le pipeline: bin/vittec0 --trace-pipeline check --strict \"$$src\""; \
		echo "4. Vérifier points d'entrée: proc main(...) pour build-native"; \
		echo "5. Lancer snapshots diagnostics: make cli-diagnostics-snapshots"; \
		echo "6. Expliquer erreurs connues: make explain-snapshots"; \
		python3 tools/vitte_brain_doctor.py "$$log" "$$src" --json-out target/reports/vitte_brain_seed_check.json $(if $(filter 1,$(VITTE_BRAIN_AUTOFIX_SAFE)),--autofix-safe,) || true; \
		echo "============================================================"; \
		return 1; \
	}; \
	sources="$$(printf '%s\n' $(VITTE_SEED_GATE_SOURCES) | sed '/^$$/d')"; \
	total="$$(printf '%s\n' "$$sources" | sed '/^$$/d' | wc -l | tr -d ' ')"; \
	i=0; \
	printf '%s\n' "$$sources" | while IFS= read -r src; do \
		[ -n "$$src" ] || continue; \
		i=$$((i + 1)); \
		echo "[seed-check][$(VITTE_ANALYSIS_MODE)] ($$i/$$total) $$src"; \
		if [ "$(VITTE_ANALYSIS_MODE)" = "build" ]; then \
			if grep -Eq '^[[:space:]]*proc[[:space:]]+main[[:space:]]*\(' "$$src"; then \
				run_with_deep_help "bin/vittec0 build-native --src \"$$src\" --out \"/tmp/vitte.native.seed.out\"" "$$src" "/tmp/vitte.seed.err"; \
			else \
				run_with_deep_help "bin/vittec0 check --strict \"$$src\"" "$$src" "/tmp/vitte.seed.err"; \
			fi; \
		else \
			run_with_deep_help "bin/vittec0 check --strict \"$$src\"" "$$src" "/tmp/vitte.seed.err"; \
		fi; \
	done

.PHONY: seed-gate
seed-gate: bootstrap-seed
	@set -e; \
	run_with_deep_help() { \
		cmd="$$1"; src="$$2"; log="$$3"; \
		if eval "$$cmd" >"$$log" 2>&1; then return 0; fi; \
		echo "============================================================"; \
		echo "[vitte][error] analyse échouée"; \
		echo "[file] $$src"; \
		echo "[command] $$cmd"; \
		echo "------------------------------------------------------------"; \
		cat "$$log"; \
		echo "------------------------------------------------------------"; \
		echo "[context] aperçu du fichier"; \
		sed -n '1,140p' "$$src" || true; \
		echo "------------------------------------------------------------"; \
		echo "[suggestions]"; \
		echo "1. Relancer ce fichier: bin/vittec0 check --strict \"$$src\""; \
		echo "2. Vérifier en isolé: bin/vittec0 check --strict \"$$src\""; \
		echo "3. Tracer pipeline: bin/vittec0 --trace-pipeline check --strict \"$$src\""; \
		echo "4. Générer snapshots: make diag-snapshots"; \
		echo "5. Exécuter gate compilateur: make compiler-max-gate-fast"; \
		python3 tools/vitte_brain_doctor.py "$$log" "$$src" --json-out target/reports/vitte_brain_seed_gate.json $(if $(filter 1,$(VITTE_BRAIN_AUTOFIX_SAFE)),--autofix-safe,) || true; \
		echo "============================================================"; \
		return 1; \
	}; \
	sources="$$( \
		printf '%s\n' $(VITTE_SEED_GATE_SOURCES) | sed '/^$$/d' | sort \
	)"; \
	total="$$(printf '%s\n' "$$sources" | sed '/^$$/d' | wc -l | tr -d ' ')"; \
	i=0; \
	printf '%s\n' "$$sources" | while IFS= read -r src; do \
		[ -n "$$src" ] || continue; \
		i=$$((i + 1)); \
		echo "[seed-gate][$(VITTE_ANALYSIS_MODE)] ($$i/$$total) $$src"; \
		if [ "$(VITTE_ANALYSIS_MODE)" = "build" ]; then \
			if grep -Eq '^[[:space:]]*proc[[:space:]]+main[[:space:]]*\(' "$$src"; then \
				run_with_deep_help "bin/vittec0 build-native --src \"$$src\" --out \"/tmp/vitte.native.seed.out\"" "$$src" "/tmp/vitte.seed.err"; \
			else \
				run_with_deep_help "bin/vittec0 check --strict \"$$src\"" "$$src" "/tmp/vitte.seed.err"; \
			fi; \
		else \
			run_with_deep_help "bin/vittec0 check --strict \"$$src\"" "$$src" "/tmp/vitte.seed.err"; \
		fi; \
	done

.PHONY: fast-bootstrap
fast-bootstrap:
	@$(MAKE) --no-print-directory seed-verify
	@$(MAKE) --no-print-directory seed-check VITTE_ANALYSIS_MODE=build
	@$(MAKE) --no-print-directory stage0-check

.PHONY: bootstrap-help
bootstrap-help:
	@echo "Bootstrap workflow (recommended):"
	@echo "  1) make seed-verify"
	@echo "  2) make seed-check VITTE_ANALYSIS_MODE=build"
	@echo "  3) make seed-gate VITTE_BRAIN_AUTOFIX_SAFE=1"
	@echo "  4) on failure: make doctor-error LOG=/tmp/vitte.seed.err SRC=path/to/file.vit"
	@echo "  5) local fast path: make fast-bootstrap then make build"
	@echo "  6) keep bootstrap-all for CI/release"
	@echo "  7) parser/typing change: use --trace-pipeline on failing file"
	@echo "  8) regression tracking: target/reports/vitte_brain_seed_check.json"

.PHONY: bootstrap-seed-root-check
bootstrap-seed-root-check:
	@python3 tools/check_bootstrap_seed_root.py
	@python3 tools/bootstrap_seed_root_test.py

.PHONY: bootstrap-hard-gate
bootstrap-hard-gate:
	@python3 tools/bootstrap_hard_gate.py

.PHONY: bootstrap-clean-checkout-gate
bootstrap-clean-checkout-gate:
	@python3 tools/bootstrap_clean_checkout.py
	@test -f target/bootstrap/clean-checkout/report.json

.PHONY: bootstrap-offline-gate
bootstrap-offline-gate:
	@python3 tools/bootstrap_offline.py
	@test -f target/bootstrap/offline/report.json

.PHONY: bootstrap-vitte-hard-gate
bootstrap-vitte-hard-gate: bootstrap-seed-root-check
	@echo "[bootstrap-vitte] strict native bootstrap gate"
	@tools/bootstrap_vitte_hard_gate.sh

.PHONY: bootstrap-v2-hard-gate
bootstrap-v2-hard-gate: bootstrap-vitte-hard-gate

.PHONY: stage0-check stage0-gate
stage0-check: seed-check
stage0-gate: seed-gate

.PHONY: bootstrap-all
bootstrap-all:
	@$(MAKE) --no-print-directory bootstrap-vitte-hard-gate
	@echo "[bootstrap-all] completed via bootstrap_vitte hard gate"

.PHONY: bootstrap-parity
bootstrap-parity:
	@echo "[bootstrap-parity][error] retired; use bootstrap-seed-root-check and bootstrap-native-snapshots" >&2
	@exit 1

.PHONY: bootstrap-verify
bootstrap-verify: bootstrap-all
	@bin/vittec0 --version
	@python3 tools/check_bootstrap_seed_root.py --artifacts
	@bin/vittec0 check tests/bootstrap_native/main_proc.vit
	@bin/vittec0 check tests/bootstrap_native/main_const_int.vit
	@echo "[bootstrap-verify] seed version + artifact contract + bootstrap smoke checks ok"

.PHONY: bootstrap-native-contract
bootstrap-native-contract: seed-verify bootstrap-source-coverage-check selfhost-subset-check posix-seed-shell-check bootstrap-shell-fixed-point bootstrap-native-snapshots selfhost-parity-gate bootstrap-clean-checkout-gate bootstrap-offline-gate bootstrap-verify bootstrap-posix-smoke

.PHONY: bootstrap-native-fast-contract
bootstrap-native-fast-contract: seed-verify bootstrap-source-coverage-check selfhost-subset-check posix-seed-shell-check bootstrap-shell-fixed-point bootstrap-native-snapshots bootstrap-generated-code-test bootstrap-multifile-native-test selfhost-parity-gate bootstrap-native-drift-check

.PHONY: bootstrap-posix-smoke
bootstrap-posix-smoke: bootstrap-all
	@tools/bootstrap_posix_smoke.sh

.PHONY: seed-syntax-test
seed-syntax-test: bootstrap-seed
	@bin/vittec0 check tests/check/main.vit

.PHONY: seed-compat-report
seed-compat-report: bootstrap-seed
	@$(MKDIR) target/reports
	@$(MAKE) --no-print-directory seed-gate > target/reports/seed_compat_report.txt
	@echo "[seed-compat-report] target/reports/seed_compat_report.txt"

.PHONY: stage0-syntax-test stage0-compat-report
stage0-syntax-test: seed-syntax-test
stage0-compat-report: seed-compat-report

.PHONY: bootstrap-migration-status
bootstrap-migration-status:
	@set -e; \
	ok=0; warn=0; \
	check_ok() { \
		label="$$1"; shift; \
		if "$$@" >/dev/null 2>&1; then \
			printf '[bootstrap-migration-status] OK   %s\n' "$$label"; \
			ok=$$((ok+1)); \
		else \
			printf '[bootstrap-migration-status] WARN %s\n' "$$label"; \
			warn=$$((warn+1)); \
		fi; \
	}; \
	check_file() { \
		label="$$1"; path="$$2"; \
		if [ -e "$$path" ]; then \
			printf '[bootstrap-migration-status] OK   %s\n' "$$label"; \
			ok=$$((ok+1)); \
		else \
			printf '[bootstrap-migration-status] WARN %s\n' "$$label"; \
			warn=$$((warn+1)); \
		fi; \
	}; \
	check_ok "Phase0: seed-gate passes" $(MAKE) --no-print-directory seed-gate; \
	check_ok "Phase0: bootstrap-all passes" $(MAKE) --no-print-directory bootstrap-all; \
	check_ok "Phase0: bootstrap-verify passes" $(MAKE) --no-print-directory bootstrap-verify; \
	check_ok "Seed root contract passes" python3 tools/check_bootstrap_seed_root.py; \
	check_ok "Installed seed artifact contract passes" python3 tools/check_bootstrap_seed_root.py --artifacts; \
	check_ok "Host-language bootstrap sources absent" $(MAKE) --no-print-directory vitte-source-audit; \
	check_file "Tracking: migration checklist present" "docs/bootstrap_migration_checklist.md"; \
	printf '[bootstrap-migration-status] summary ok=%s warn=%s\n' "$$ok" "$$warn"; \
	test "$$warn" -eq 0

.PHONY: parse
parse:
	@tools/parse_tests.sh

.PHONY: parse-modules
parse-modules:
	@tools/parse_modules_tests.sh

.PHONY: parse-strict
parse-strict:
	@STRICT_ONLY=1 tools/parse_tests.sh

.PHONY: parse-watch
parse-watch:
	@tools/parse_watch.sh

.PHONY: hir-validate-test
hir-validate-test:
	@$(VITTE_BOOTSTRAP) check src/vitte/compiler/ir/hir.vit >/dev/null || true
	@echo "[hir-validate-test] Vitte HIR surface probed"

.PHONY: hir-validate-fixture
hir-validate-fixture:
	@$(VITTE_BOOTSTRAP) check src/vitte/compiler/ir/mir.vit >/dev/null || true
	@echo "[hir-validate-fixture] Vitte MIR surface probed"

.PHONY: hir-validate
hir-validate: hir-validate-test hir-validate-fixture
	@echo "[hir-validate] ok"

.PHONY: check-tests
check-tests:
	@tools/check_tests.sh

.PHONY: init-templates-smoke
init-templates-smoke:
	@set -e; \
	for tpl in cli service lib-vitte; do \
		td="$$(mktemp -d 2>/dev/null || mktemp -d -t vitte-init)"; \
		"$(BIN_DIR)/$(PROJECT)" init "$$td/app" --template "$$tpl" >/dev/null; \
		"$(BIN_DIR)/$(PROJECT)" check "$$td/app/src/main.vit" >/dev/null; \
		rm -rf "$$td"; \
		echo "[init-templates-smoke] $$tpl ok"; \
	done

.PHONY: dx-hello-prod-bench
dx-hello-prod-bench:
	@python3 tools/dx_hello_prod_bench.py --template cli

.PHONY: dx-hello-prod-gate
dx-hello-prod-gate:
	@python3 tools/dx_hello_prod_bench.py --template cli --strict

.PHONY: lsp-completion-bench
lsp-completion-bench:
	@python3 tools/lsp_completion_bench.py --prefix diag

.PHONY: lsp-completion-gate
lsp-completion-gate:
	@python3 tools/lsp_completion_bench.py --prefix diag --strict

.PHONY: diag-autofix-frequent
diag-autofix-frequent:
	@python3 tools/frequent_diag_autofix_check.py --strict

.PHONY: stress-alloc
stress-alloc:
	@tools/stress_alloc_examples.sh

.PHONY: stress-maximal
stress-maximal:
	@python3 tools/compiler_stress_maximal.py --all --report build/reports/stress.txt

.PHONY: long-term-stability-suite
long-term-stability-suite:
	@python3 tools/long_term_stability_suite.py

.PHONY: core-projects
core-projects:
	@tools/build_core_projects.sh

.PHONY: test-examples
test-examples:
	@MODE=$${TEST_EXAMPLES_MODE:-check} STRICT_EXAMPLES=$${STRICT_EXAMPLES:-0} tools/build_examples_matrix.sh

.PHONY: arduino-projects
arduino-projects:
	@tools/build_arduino_projects.sh

.PHONY: quasi-empty-package-tests
quasi-empty-package-tests:
	@set -e; \
	for f in \
		tests/catalog/*.vit \
		tests/data/*.vit \
		tests/kernel/*.vit \
		tests/module_index/*.vit \
		tests/platform/*.vit \
		tests/release_guard/*.vit \
		tests/docsgen_modules/*.vit \
		tests/contracts_registry/*.vit \
		tests/migration_playbook/*.vit \
		tests/fuzzkit/*.vit \
		tests/async/*.vit \
		tests/env/*.vit \
		tests/jsonpath/*.vit \
		tests/openapi/*.vit \
		tests/testkit_modules/*.vit \
		tests/std/io/*.vit \
		tests/std/net/*.vit \
		tests/std/data/*.vit \
		tests/std/async/*.vit; do \
		echo "[quasi-empty-package-tests] check $$f"; \
		$(BIN_DIR)/$(PROJECT) check "$$f"; \
	done

.PHONY: negative-tests
negative-tests:
	@tools/negative_tests.sh

.PHONY: diag-snapshots
diag-snapshots:
	@tools/diag_snapshots.sh

.PHONY: diag-snapshots-portable
diag-snapshots-portable:
	@BIN="$(CURDIR)/bin/vittec0" tools/diag_snapshots.sh

.PHONY: negative-tests-portable
negative-tests-portable:
	@BIN="$(CURDIR)/bin/vittec0" tools/negative_tests.sh

.PHONY: frontend-diagnostics-test diagnostics-portable diagnostics-local compiler-diagnostics-local
frontend-diagnostics-test:
	@python3 tests/diagnostics/run_frontend_diagnostic_tests.py
	@python3 tests/diagnostics/input_matrix_test.py
	@tools/apply_fixits_recompile_test.py

diagnostics-portable diagnostics-local compiler-diagnostics-local: frontend-diagnostics-test diagnostics-locales-lint diagnostics-ftl-check diagnostics-fluent-gate diagnostic-quality diag-snapshots-portable negative-tests-portable

.PHONY: compiler-smoke-portable
compiler-smoke-portable: diagnostics-portable

.PHONY: native-binaries-doctor
native-binaries-doctor:
	@tools/doctor.sh | grep -E '^(bin/vitte|bin/vittec)'

.PHONY: diagnostics-locales-lint
diagnostics-locales-lint:
	@python3 tools/check_diagnostics_locales.py

.PHONY: diagnostics-fluent-gate
diagnostics-fluent-gate:
	@tools/diagnostics_fluent_gate.sh

.PHONY: core-semantic-snapshots
core-semantic-snapshots:
	@MANIFEST=tests/diag_snapshots/core_semantic_manifest.txt tools/diag_snapshots.sh

.PHONY: core-semantic-success
core-semantic-success:
	@MANIFEST=tests/core_semantic_success_manifest.txt tools/check_manifest.sh

.PHONY: core-semantic-imports-snapshots
core-semantic-imports-snapshots:
	@MANIFEST=tests/diag_snapshots/core_semantic_imports_manifest.txt tools/diag_snapshots.sh

.PHONY: core-semantic-imports-success
core-semantic-imports-success:
	@MANIFEST=tests/core_semantic_imports_success_manifest.txt tools/check_manifest.sh

.PHONY: core-semantic-resolve-snapshots
core-semantic-resolve-snapshots:
	@MANIFEST=tests/diag_snapshots/core_semantic_resolve_manifest.txt tools/diag_snapshots.sh

.PHONY: core-semantic-resolve-success
core-semantic-resolve-success:
	@MANIFEST=tests/core_semantic_resolve_success_manifest.txt tools/check_manifest.sh

.PHONY: core-semantic-returns-snapshots
core-semantic-returns-snapshots:
	@MANIFEST=tests/diag_snapshots/core_semantic_returns_manifest.txt tools/diag_snapshots.sh

.PHONY: core-semantic-returns-success
core-semantic-returns-success:
	@MANIFEST=tests/core_semantic_returns_success_manifest.txt tools/check_manifest.sh

.PHONY: core-semantic-match-snapshots
core-semantic-match-snapshots:
	@MANIFEST=tests/diag_snapshots/core_semantic_match_manifest.txt tools/diag_snapshots.sh

.PHONY: core-semantic-match-success
core-semantic-match-success:
	@MANIFEST=tests/core_semantic_match_success_manifest.txt tools/check_manifest.sh

.PHONY: core-semantic-share-snapshots
core-semantic-share-snapshots:
	@MANIFEST=tests/diag_snapshots/core_semantic_share_manifest.txt tools/diag_snapshots.sh

.PHONY: core-semantic-share-success
core-semantic-share-success:
	@MANIFEST=tests/core_semantic_share_success_manifest.txt tools/check_manifest.sh

.PHONY: core-semantic-entry-snapshots
core-semantic-entry-snapshots:
	@MANIFEST=tests/diag_snapshots/core_semantic_entry_manifest.txt tools/diag_snapshots.sh

.PHONY: core-semantic-entry-success
core-semantic-entry-success:
	@MANIFEST=tests/core_semantic_entry_success_manifest.txt tools/check_manifest.sh

.PHONY: explicit-generics-snapshots
explicit-generics-snapshots:
	@MANIFEST=tests/diag_snapshots/explicit_generics_manifest.txt tools/diag_snapshots.sh

.PHONY: core-ir-golden-snapshots
core-ir-golden-snapshots:
	@MANIFEST=tests/diag_snapshots/core_ir_golden_manifest.txt tools/diag_snapshots.sh

.PHONY: core-semantic-themes
core-semantic-themes: \
	core-semantic-imports-success \
	core-semantic-imports-snapshots \
	core-semantic-resolve-success \
	core-semantic-resolve-snapshots \
	core-semantic-returns-success \
	core-semantic-returns-snapshots \
	core-semantic-match-success \
	core-semantic-match-snapshots \
	core-semantic-share-success \
	core-semantic-share-snapshots \
	core-semantic-entry-success \
	core-semantic-entry-snapshots

.PHONY: resolve-tests
resolve-tests:
	@TEST_DIR=tests/diag_snapshots/resolve tools/diag_snapshots.sh

.PHONY: explain-snapshots
explain-snapshots:
	@tools/explain_snapshots.sh

.PHONY: wrapper-stage-test
wrapper-stage-test:
	@tools/wrapper_stage_test.sh

.PHONY: grammar-sync
grammar-sync:
	@python3 docs/book/grammar/scripts/sync_grammar.py
	@python3 docs/book/grammar/scripts/sync_precedence.py

.PHONY: harden-mod-vits
harden-mod-vits:
	@test -n "$(PACKAGES)" || (echo "usage: make harden-mod-vits PACKAGES='catalog data std/io'" >&2; exit 2)
	@python3 tools/harden_existing_mod_vit.py --write $(PACKAGES)

.PHONY: grammar-check
grammar-check:
	@python3 docs/book/grammar/scripts/sync_grammar.py --check
	@python3 docs/book/grammar/scripts/sync_precedence.py --check

.PHONY: grammar-test
grammar-test:
	@python3 docs/book/grammar/scripts/validate_examples.py
	@python3 tools/parser_precedence_property_test.py

.PHONY: grammar-test-portable
grammar-test-portable:
	@VITTE_BIN="$(CURDIR)/bin/vittec0" python3 docs/book/grammar/scripts/validate_examples.py
	@VITTE_BIN="$(CURDIR)/bin/vittec0" python3 tools/parser_precedence_property_test.py

.PHONY: grammar-alignment-test
grammar-alignment-test:
	@python3 tools/grammar_alignment_checker.py

.PHONY: frontend-syntax-test
frontend-syntax-test:
	@python3 tools/frontend_syntax_check.py tests/frontend_syntax/valid
	@if python3 tools/frontend_syntax_check.py tests/frontend_syntax/invalid >/tmp/frontend_syntax_invalid.out 2>&1; then \
		echo "[frontend-syntax-test][error] invalid fixtures unexpectedly parsed"; \
		cat /tmp/frontend_syntax_invalid.out; \
		exit 1; \
	fi
	@echo "[frontend-syntax-test] OK"

.PHONY: driver-report-runtime-test
driver-report-runtime-test:
	@python3 tools/driver_report_runtime_test.py

.PHONY: core-language-test
core-language-test:
	@python3 docs/book/grammar/scripts/validate_examples.py --strict-core --manifest tests/grammar/core_manifest.txt
	@python3 tools/parser_precedence_property_test.py

.PHONY: core-language-test-portable
core-language-test-portable:
	@VITTE_BIN="$(CURDIR)/bin/vittec0" python3 docs/book/grammar/scripts/validate_examples.py --strict-core --manifest tests/grammar/core_manifest.txt
	@VITTE_BIN="$(CURDIR)/bin/vittec0" python3 tools/parser_precedence_property_test.py

.PHONY: core-language-test-update
core-language-test-update:
	@python3 docs/book/grammar/scripts/validate_examples.py --strict-core --manifest tests/grammar/core_manifest.txt --update-snapshots

.PHONY: grammar-test-update
grammar-test-update:
	@python3 docs/book/grammar/scripts/validate_examples.py --update-snapshots

.PHONY: parser-recovery-golden
parser-recovery-golden:
	@python3 docs/book/grammar/scripts/validate_examples.py --strict-core --manifest tests/grammar/recovery_manifest.txt

.PHONY: parser-recovery-golden-portable
parser-recovery-golden-portable:
	@VITTE_BIN="$(CURDIR)/bin/vittec0" python3 docs/book/grammar/scripts/validate_examples.py --strict-core --manifest tests/grammar/recovery_manifest.txt

.PHONY: test-golden
test-golden:
	@tools/golden_runner --check

.PHONY: test-golden-update
test-golden-update:
	@tools/golden_runner --update

.PHONY: test-golden-critical
test-golden-critical:
	@tools/golden_runner --check --subset critical

.PHONY: parser-sync-coverage
parser-sync-coverage:
	@python3 tools/parser_sync_coverage_report.py --check

.PHONY: grammar-coverage
grammar-coverage:
	@python3 tools/parser_sync_coverage_report.py --check

.PHONY: frontend-lexer-test
frontend-lexer-test:
	@bin/vitte check src/vitte/compiler/tests/lexer_tests.vit
	@python3 tools/lexer_ebnf_surface_check.py

.PHONY: frontend-ast-test
frontend-ast-test:
	@bin/vitte check src/vitte/compiler/tests/ast_tests.vit
	@python3 tools/ast_coverage_gate.py

.PHONY: hir-lowering-test
hir-lowering-test:
	@bin/vitte check src/vitte/compiler/tests/hir_tests.vit
	@python3 tools/ast_hir_lowering_audit.py

.PHONY: mir-lowering-test
mir-lowering-test:
	@bin/vitte check src/vitte/compiler/tests/mir_tests.vit

.PHONY: sema-analysis-test
sema-analysis-test:
	@bin/vitte check src/vitte/compiler/tests/sema_tests.vit

.PHONY: const-eval-analysis-test
const-eval-analysis-test:
	@bin/vitte check src/vitte/compiler/tests/const_eval_tests.vit

.PHONY: typeck-analysis-test
typeck-analysis-test:
	@bin/vitte check src/vitte/compiler/tests/typeck_tests.vit

.PHONY: borrowck-analysis-test
borrowck-analysis-test:
	@bin/vitte check src/vitte/compiler/tests/borrowck_tests.vit

.PHONY: frontend-token-consistency
frontend-token-consistency:
	@python3 tools/check_frontend_token_consistency.py

.PHONY: grammar-docs
grammar-docs:
	@python3 docs/book/grammar/scripts/build_railroad.py

.PHONY: grammar-docs-check
grammar-docs-check:
	@python3 docs/book/grammar/scripts/build_railroad.py --check

.PHONY: grammar-gate
grammar-gate: grammar-check grammar-test grammar-docs-check

.PHONY: core-forbidden-syntax-lint
core-forbidden-syntax-lint:
	@python3 tools/lint_core_forbidden_syntax.py

.PHONY: strict-core-guard-test
strict-core-guard-test:
	@tools/strict_core_guard_test.sh

.PHONY: strict-core-guard-test-portable
strict-core-guard-test-portable:
	@BIN="$(CURDIR)/bin/vittec0" tools/strict_core_guard_test.sh

.PHONY: parser-lexer-fuzz-smoke
parser-lexer-fuzz-smoke:
	@python3 tools/parser_lexer_fuzz_smoke.py --cases 80 --seed 1337

.PHONY: core-language-gate
core-language-gate: grammar-check grammar-test core-language-test parser-recovery-golden grammar-coverage frontend-lexer-test frontend-ast-test hir-lowering-test mir-lowering-test sema-analysis-test const-eval-analysis-test typeck-analysis-test borrowck-analysis-test frontend-token-consistency strict-core-guard-test core-forbidden-syntax-lint core-ir-golden-snapshots core-semantic-success core-semantic-snapshots diagnostics-locales-lint

.PHONY: core-semantic-success-portable
core-semantic-success-portable:
	@BIN="$(CURDIR)/bin/vittec0" MANIFEST=tests/core_semantic_success_manifest.txt tools/check_manifest.sh

.PHONY: core-semantic-snapshots-portable
core-semantic-snapshots-portable:
	@BIN="$(CURDIR)/bin/vittec0" MANIFEST=tests/diag_snapshots/core_semantic_manifest.txt tools/diag_snapshots.sh

.PHONY: core-language-gate-portable
core-language-gate-portable: grammar-check grammar-test-portable core-language-test-portable parser-recovery-golden-portable grammar-coverage frontend-ast-test hir-lowering-test mir-lowering-test sema-analysis-test const-eval-analysis-test typeck-analysis-test borrowck-analysis-test frontend-token-consistency strict-core-guard-test-portable core-forbidden-syntax-lint core-ir-golden-snapshots core-semantic-success-portable core-semantic-snapshots-portable diagnostics-locales-lint

.PHONY: core-release-gate
core-release-gate: core-language-gate diagnostics-ftl-check

.PHONY: book-qa
book-qa:
	@python3 docs/book/scripts/qa_book.py

.PHONY: book-qa-strict
book-qa-strict:
	@python3 docs/book/scripts/qa_book.py --strict

.PHONY: docs-serve
docs-serve:
	@echo "[docs-serve] serving docs/ at http://127.0.0.1:8000"
	@cd docs && python3 -m http.server 8000

.PHONY: site-html
site-html:
	@python3 tools/render_site_html.py --root docs

.PHONY: docs-phase1-smoke
docs-phase1-smoke:
	@bash tools/docs_pipeline.sh phase1
	@tools/docs/verify_local_pages.sh

.PHONY: docs-maximal
docs-maximal:
	@tools/docs_maximal.sh all "$${PHASE:-phase3}"

.PHONY: docs-maximal-build
docs-maximal-build:
	@tools/docs_maximal.sh build "$${PHASE:-phase3}"

.PHONY: docs-maximal-validate
docs-maximal-validate:
	@tools/docs_maximal.sh validate "$${PHASE:-phase3}"

.PHONY: keywords-normalize
keywords-normalize:
	@python3 docs/book/chapters/keywords/scripts/normalize_keywords.py

.PHONY: keywords-lint
keywords-lint:
	@python3 docs/book/chapters/keywords/scripts/lint_keywords.py

.PHONY: make-targets-doc
make-targets-doc:
	@python3 tools/generate_make_targets_doc.py

.PHONY: make-targets-doc-check
make-targets-doc-check:
	@python3 tools/generate_make_targets_doc.py --check

.PHONY: docs-paths-check
docs-paths-check:
	@python3 tools/docs_paths_check.py

.PHONY: bootstrap-contracts-index-check
bootstrap-contracts-index-check:
	@tools/bootstrap_contracts_index_check.sh

.PHONY: update-diagnostics-ftl
update-diagnostics-ftl:
	@tools/update_diagnostics_ftl.py

.PHONY: diagnostics-ftl-check
diagnostics-ftl-check:
	@tools/update_diagnostics_ftl.py --check

.PHONY: ci-strict
ci-strict: core-language-gate package-layout-lint legacy-import-path-lint negative-tests diag-snapshots geany-lint highlight-snapshots repo-hygiene-check make-targets-doc-check docs-paths-check selfhost-hard-strict

.PHONY: ci-fast
ci-fast: core-language-gate package-layout-lint legacy-import-path-lint negative-tests diag-snapshots diagnostic-quality completions-snapshots wrapper-stage-test geany-lint repo-hygiene-check make-targets-doc-check docs-paths-check

.PHONY: repo-hygiene-check
repo-hygiene-check:
	@tools/repo_hygiene_check.sh

.PHONY: ci-completions
ci-completions: completions-check completions-lint completions-snapshots completions-fallback

.PHONY: dx-adoption
dx-adoption: init-templates-smoke diag-autofix-frequent lsp-completion-gate dx-hello-prod-gate

.PHONY: generate-highlights
generate-highlights:
	@python3 tools/generate_editor_highlights.py

.PHONY: highlight-snapshots
highlight-snapshots:
	@tools/highlight_snapshot_vim.sh
	@tools/highlight_snapshot_emacs.sh
	@tools/highlight_snapshot_nano.sh
	@tools/highlight_snapshot_geany.sh

.PHONY: highlight-snapshots-update
highlight-snapshots-update:
	@tools/highlight_snapshot_vim.sh --update
	@tools/highlight_snapshot_emacs.sh --update
	@tools/highlight_snapshot_nano.sh --update
	@tools/highlight_snapshot_geany.sh --update

.PHONY: highlights-ci
highlights-ci: generate-highlights highlight-snapshots
	@python3 tools/highlights_coverage.py

.PHONY: geany-lint
geany-lint:
	@python3 tools/generate_editor_highlights.py
	@python3 tools/highlights_snapshot.py --editor geany
	@grep -q "AUTO-GENERATED by tools/generate_editor_highlights.py" editors/geany/filetypes.vitte.conf
	@grep -q "^\\[build-menu\\]" editors/geany/filetypes.vitte.conf
	@grep -q "vitte check" editors/geany/filetypes.vitte.conf
	@grep -Eq "^(Vitte|vitte)=.*\\*\\.vit;.*\\*\\.vitte;" editors/geany/filetype_extensions.conf

.PHONY: geany-install-check
geany-install-check:
	@tools/check_geany_install.sh

.PHONY: runtime-matrix-modules
runtime-matrix-modules:
	@if [ -f tests/vitte_packages_runtime_matrix.vit ]; then \
		$(BIN_DIR)/$(PROJECT) check --lang=en tests/vitte_packages_runtime_matrix.vit; \
	else \
		echo "[runtime-matrix] skip: tests/vitte_packages_runtime_matrix.vit not present in this Vitte-only checkout"; \
	fi

.PHONY: module-shape-policy
module-shape-policy:
	@tools/check_module_shape_policy.py

.PHONY: package-layout-lint
package-layout-lint:
	@tools/lint_package_layout.py

.PHONY: package-layout-lint-strict
package-layout-lint-strict:
	@tools/lint_package_layout.py --strict --enforce-mod-only

.PHONY: module-leaf-file-lint
module-leaf-file-lint:
	@tools/lint_package_layout.py --enforce-mod-only

.PHONY: legacy-import-path-lint
legacy-import-path-lint:
	@tools/lint_legacy_import_paths.py --max-entries=$(LEGACY_ALLOWLIST_BUDGET)

.PHONY: experimental-modules-lint
experimental-modules-lint:
	@tools/lint_experimental_modules.py

.PHONY: public-modules-snapshots-lint
public-modules-snapshots-lint:
	@tools/lint_public_modules_have_snapshots.py

.PHONY: critical-module-contract-lint
critical-module-contract-lint:
	@tools/lint_critical_module_contracts.py

.PHONY: legacy-import-allowlist-empty
legacy-import-allowlist-empty:
	@if grep -E -v '^[[:space:]]*(#|$$)' tools/legacy_import_path_allowlist.txt >/dev/null; then \
		echo "[legacy-import-allowlist-empty][error] tools/legacy_import_path_allowlist.txt must be empty for release"; \
		echo "[legacy-import-allowlist-empty][error] remove temporary exceptions before release"; \
		exit 1; \
	fi

.PHONY: ci-fast-compiler
ci-fast-compiler:
	@tools/ci_fast_compiler.sh

.PHONY: compiler-intrinsic-surface-audit
compiler-intrinsic-surface-audit:
	@python3 tools/compiler_intrinsic_surface_audit.py

.PHONY: compiler-src-critical
compiler-src-critical:
	@tools/compile_all_compiler_files.sh

.PHONY: native-json-schema-contract
native-json-schema-contract:
	@tools/native_json_schema_contract_test.sh

.PHONY: bootstrap-selfhost-repro
bootstrap-selfhost-repro:
	@tools/bootstrap_selfhost_repro.sh

.PHONY: runtime-stdlib-real
runtime-stdlib-real:
	@python3 tools/runtime_stdlib_real_checks.py

.PHONY: runtime-abi-contract
runtime-abi-contract:
	@python3 tools/runtime_abi_contract.py
	@test -f target/runtime_stdlib_real/real_checks.json
	@test -f target/reports/runtime_stdlib_real.md

.PHONY: selfhost-completion-audit
selfhost-completion-audit:
	@python3 tools/selfhost_completion_audit.py
	@test -f target/selfhost_completion/selfhost_completion.json
	@test -f target/reports/selfhost_completion.md

.PHONY: selfhost-completion-strict
selfhost-completion-strict:
	@python3 tools/selfhost_completion_audit.py --strict-complete
	@test -f target/selfhost_completion/selfhost_completion.json
	@test -f target/reports/selfhost_completion.md

.PHONY: selfhost-parity-gate
selfhost-parity-gate:
	@python3 tools/selfhost_completion_audit.py --require-parity
	@test -f target/selfhost_completion/selfhost_completion.json
	@test -f target/reports/selfhost_completion.md

.PHONY: spec-normative-check
spec-normative-check:
	@python3 tools/spec_normative_check.py
	@test -f target/reports/spec_normative_coverage.md

.PHONY: language-maturity-gate
language-maturity-gate: runtime-stdlib-real selfhost-completion-audit format-check package-manager-gate lsp-gate spec-normative-check
	@echo "[language-maturity-gate] PASS"

.PHONY: compiler-max-gate-fast
compiler-max-gate-fast:
	@tools/compiler_max_gate.sh fast

.PHONY: compiler-power-gate
compiler-power-gate:
	@$(MAKE) --no-print-directory diagnostic-quality
	@$(MAKE) --no-print-directory compiler-test-suite-check-gate
	@$(MAKE) --no-print-directory analysis-gate
	@$(MAKE) --no-print-directory type-system-gate
	@$(MAKE) --no-print-directory backend-gate
	@echo "[compiler-power-gate] PASS"

.PHONY: compiler-max-gate
compiler-max-gate:
	@tools/compiler_max_gate.sh full

.PHONY: selfhost-hard
selfhost-hard:
	@$(MAKE) --no-print-directory build
	@$(MAKE) --no-print-directory bootstrap-source-of-truth
	@$(MAKE) --no-print-directory compiler-entry-lock
	@$(MAKE) --no-print-directory compiler-path-typos
	@$(MAKE) --no-print-directory selfhost-audit
	@$(MAKE) --no-print-directory compiler-max-gate-fast
	@echo "[selfhost-hard] PASS"

.PHONY: compiler-max-gate-strict
compiler-max-gate-strict:
	@tools/compiler_max_gate.sh strict

.PHONY: selfhost-hard-strict
selfhost-hard-strict:
	@$(MAKE) --no-print-directory build
	@$(MAKE) --no-print-directory bootstrap-source-of-truth
	@$(MAKE) --no-print-directory compiler-entry-lock
	@$(MAKE) --no-print-directory diagnostics-migration-gate
	@$(MAKE) --no-print-directory compiler-reachability-audit
	@$(MAKE) --no-print-directory selfhost-audit
	@$(MAKE) --no-print-directory compiler-max-gate-strict
	@echo "[selfhost-hard-strict] PASS"

.PHONY: repro
repro:
	@tools/repro_compare.sh

.PHONY: repro-generate
repro-generate:
	@echo "[repro-generate] host repro generation retired; use Vitte reducer artifacts"

# ------------------------------------------------------------
# Stdlib checks
# ------------------------------------------------------------

.PHONY: std-check
std-check:
	@echo "Checking std layout…"
	@test -d $(STD_DIR)/core
	@test -d $(STD_DIR)/io
	@test -d $(STD_DIR)/math
	@test -d $(STD_DIR)/tests
	@echo "std layout OK"

.PHONY: extern-abi
extern-abi:
	@tools/validate_extern_abi.py --profile host --strict-warnings

.PHONY: extern-abi-host
extern-abi-host:
	@tools/validate_extern_abi.py --profile host --strict-warnings

.PHONY: extern-abi-arduino
extern-abi-arduino:
	@tools/validate_extern_abi.py --profile arduino --strict-warnings

.PHONY: extern-abi-kernel
extern-abi-kernel:
	@tools/validate_extern_abi.py --profile kernel --kernel-variant grub --strict-warnings

.PHONY: extern-abi-kernel-uefi
extern-abi-kernel-uefi:
	@tools/validate_extern_abi.py --profile kernel --kernel-variant uefi --strict-warnings

.PHONY: extern-abi-all
extern-abi-all:
	@tools/validate_extern_abi.py --profile all

.PHONY: interop-headers-gen
interop-headers-gen:
	@python3 toolchain/scripts/interop/generate_interop_headers.py

.PHONY: interop-headers-check
interop-headers-check:
	@python3 toolchain/scripts/interop/generate_interop_headers.py --check

.PHONY: interop-headers-snapshot-update
interop-headers-snapshot-update:
	@python3 toolchain/scripts/interop/generate_interop_headers.py --update-snapshot

.PHONY: interop-abi-matrix
interop-abi-matrix:
	@python3 tools/interop_abi_matrix.py

.PHONY: stdlib-api-lint
stdlib-api-lint:
	@tools/lint_stdlib_api.py

.PHONY: stdlib-profile-snapshots
stdlib-profile-snapshots:
	@tools/stdlib_profile_snapshots.sh

.PHONY: stdlib-abi-compat
stdlib-abi-compat:
	@tools/lint_stdlib_api.py --surface docs/stdlib_abi_surface_v2.txt
	@tools/check_stdlib_abi_compat.py

.PHONY: modules-tests
modules-tests:
	@tools/modules_tests.sh

.PHONY: modules-snapshots
modules-snapshots:
	@tools/modules_snapshots.sh

.PHONY: modules-snapshots-update
modules-snapshots-update:
	@tools/modules_snapshots.sh --update

.PHONY: modules-snapshots-bless
modules-snapshots-bless:
	@tools/modules_snapshots.sh --bless

.PHONY: modules-contract-snapshots
modules-contract-snapshots:
	@tools/modules_contract_snapshots.sh

.PHONY: modules-contract-snapshots-update
modules-contract-snapshots-update:
	@tools/modules_contract_snapshots.sh --update

.PHONY: module-tree-lint
module-tree-lint:
	@tools/lint_module_tree.py

.PHONY: module-naming-lint
module-naming-lint:
	@tools/lint_module_naming.py

.PHONY: packages-governance-lint
packages-governance-lint:
	@tools/lint_packages_governance.py

.PHONY: critical-runtime-matrix-lint
critical-runtime-matrix-lint:
	@tools/lint_critical_runtime_matrix.py

.PHONY: new-public-packages-snapshots-lint
new-public-packages-snapshots-lint:
	@tools/lint_new_public_packages_have_snapshots.py

.PHONY: no-std-lint
no-std-lint:
	@tools/lint_no_std_imports.py --roots src/vitte/packages

.PHONY: modules-report
modules-report:
	@tools/modules_report.sh

.PHONY: book-links-check
book-links-check:
	@python3 docs/book/scripts/check_links.py --book-root docs/book --report target/reports/book_links_report.txt

.PHONY: book-structure-check
book-structure-check:
	@python3 docs/book/scripts/check_structure.py --book-root docs/book

.PHONY: book-length-check
book-length-check:
	@python3 docs/book/scripts/check_chapter_length.py --chapters-dir docs/book/chapters --min-pages 10 --max-pages 30 --words-per-page 350 --report target/reports/chapter_length_report.txt

.PHONY: packages-report
packages-report:
	@if [ -x tools/modules_report.sh ]; then \
		SEARCH_ROOT=src/vitte/packages ENTRY_GLOB=mod.vit OUT_FILE=target/reports/packages_modules_report.txt OUT_JSON=target/reports/packages_modules_report.json DEPENDENCY_OVERLAP_ALLOWLIST=tools/package_dependency_export_overlap_allowlist.txt tools/modules_report.sh; \
	else \
		echo "[packages-report] SKIP: modules report tool absent"; \
	fi

.PHONY: packages-dependency-overlap-lint
packages-dependency-overlap-lint:
	@if [ -x tools/modules_report.sh ]; then \
		SEARCH_ROOT=src/vitte/packages ENTRY_GLOB=mod.vit OUT_FILE=target/reports/packages_modules_report.txt OUT_JSON=target/reports/packages_modules_report.json DEPENDENCY_OVERLAP_ALLOWLIST=tools/package_dependency_export_overlap_allowlist.txt FAIL_ON_DEPENDENCY_OVERLAP=1 tools/modules_report.sh; \
	else \
		echo "[packages-dependency-overlap-lint] SKIP: modules report tool absent"; \
	fi

.PHONY: package-check
package-check:
	@test -n "$(SRC)" || (echo "usage: make package-check SRC=src/vitte/packages/<pkg>/mod.vit" >&2; exit 2)
	@tools/package_check_portable.sh "$(SRC)"

.PHONY: packages-check-all
packages-check-all:
	@tools/package_check_all.sh

.PHONY: pkg-matrix
pkg-matrix:
	@tools/pkg_matrix.sh

.PHONY: pkg-cli-integration
pkg-cli-integration:
	@tools/pkg_cli_integration.sh

.PHONY: modules-perf-cache
modules-perf-cache:
	@if [ -x tools/modules_cache_perf.sh ] && [ -f tests/modules/mod_doctor/main.vit ]; then \
		tools/modules_cache_perf.sh tests/modules/mod_doctor/main.vit; \
	else \
		echo "[modules-perf-cache] SKIP: modules cache perf fixture/tool absent"; \
	fi

.PHONY: packages-contract-snapshots
packages-contract-snapshots:
	@tools/packages_contract_snapshots.sh

.PHONY: packages-contract-snapshots-update
packages-contract-snapshots-update:
	@tools/packages_contract_snapshots.sh --update

.PHONY: packages-gate
packages-gate: package-layout-lint-strict packages-governance-lint no-std-lint module-naming-lint legacy-import-path-lint critical-runtime-matrix-lint new-public-packages-snapshots-lint modules-perf-cache packages-dependency-overlap-lint packages-contract-snapshots

.PHONY: packages-only-ci
packages-only-ci: packages-governance-lint packages-check-all pkg-matrix pkg-cli-integration

.PHONY: packages-strict-ci
packages-strict-ci: package-layout-lint-strict packages-governance-lint no-std-lint module-naming-lint packages-check-all pkg-matrix pkg-cli-integration

.PHONY: modules-ci-strict
modules-ci-strict: modules-tests modules-snapshots modules-contract-snapshots module-tree-lint module-naming-lint critical-module-contract-lint experimental-modules-lint public-modules-snapshots-lint modules-perf-cache legacy-import-path-lint migration-check modules-report
	@if $(BIN_DIR)/$(PROJECT) --help 2>&1 | grep -Fq "mod contract-diff"; then \
		$(BIN_DIR)/$(PROJECT) mod contract-diff --lang=en --old tests/modules/api_diff/old_case/main.vit --new tests/modules/api_diff/new_case/main.vit >/tmp/vitte-modules-ci-contract-diff.out 2>&1 || true; \
		grep -Fq "[contract-diff] BREAKING" /tmp/vitte-modules-ci-contract-diff.out; \
		$(BIN_DIR)/$(PROJECT) mod contract-diff --lang=en --old tests/modules/api_diff/old_case/main.vit --new tests/modules/api_diff/old_case/main.vit >/tmp/vitte-modules-ci-contract-diff-ok.out 2>&1; \
		grep -Fq "[contract-diff] OK" /tmp/vitte-modules-ci-contract-diff-ok.out; \
		rm -f /tmp/vitte-modules-ci-contract-diff.out /tmp/vitte-modules-ci-contract-diff-ok.out; \
	else \
		echo "[modules-ci-strict] skip: mod contract-diff not available in current bin/vitte"; \
	fi

.PHONY: completions-gen
completions-gen:
	@python3 tools/generate_completions.py --mode $${VITTE_COMPLETIONS_MODE:-static}

.PHONY: completions-check
completions-check:
	@python3 tools/generate_completions.py --check --mode $${VITTE_COMPLETIONS_MODE:-static}

.PHONY: completions-snapshots
completions-snapshots:
	@tools/completions_snapshots.sh

.PHONY: completions-snapshots-update
completions-snapshots-update:
	@tools/completions_snapshots.sh --update

.PHONY: completions-lint
completions-lint:
	@bash -n completions/bash/vitte
	@zsh -n completions/zsh/_vitte
	@if command -v fish >/dev/null 2>&1; then \
		fish -n completions/fish/vitte.fish; \
	else \
		echo "fish not installed, syntax check skipped"; \
	fi

.PHONY: completions-fallback
completions-fallback:
	@tools/completions_fallback_test.sh

.PHONY: same-output-hash
same-output-hash:
	@tools/same_output_hash_test.sh

.PHONY: ci-std-fast
ci-std-fast: std-check extern-abi-host stdlib-api-lint stdlib-profile-snapshots diag-snapshots completions-snapshots wrapper-stage-test

.PHONY: ci-mod-fast
ci-mod-fast: grammar-check diag-snapshots completions-snapshots stdlib-profile-snapshots stdlib-abi-compat modules-tests modules-snapshots same-output-hash

.PHONY: ci-bridge-compat
ci-bridge-compat: ci-mod-fast

.PHONY: all-tests
all-tests:
	@tools/run_all_tests.sh

.PHONY: all-tests-group
all-tests-group:
	@ALL_TESTS_GROUP=$${GROUP:-all} tools/run_all_tests.sh

.PHONY: all-tests-ci-group-guard
all-tests-ci-group-guard:
	@targets="$$(ALL_TESTS_LIST_ONLY=1 ALL_TESTS_GROUP=ci tools/run_all_tests.sh)"; \
	if printf "%s\n" "$$targets" | grep -qx "ci-completions"; then \
		echo "[all-tests][error] ci group must not include ci-completions"; \
		exit 2; \
	fi

.PHONY: vitteos-bin-quality
vitteos-bin-quality:
	@tools/vitteos_bin_ci_quality.sh

.PHONY: vitteos-bin-runnable-check
vitteos-bin-runnable-check:
	@tools/vitteos_bin_runnable_check.sh

.PHONY: vitteos-bin-runtime
vitteos-bin-runtime:
	@tools/vitteos_bin_runtime_check.sh --apply

.PHONY: vitteos-bin-matrix
vitteos-bin-matrix:
	@tools/vitteos_bin_matrix_report.sh

.PHONY: vitteos-bin-gate
vitteos-bin-gate:
	@tools/vitteos_bin_uniformity_check.sh
	@tools/vitteos_bin_vit_check.sh
	@tools/vitteos_bin_lint.sh
	@tools/vitteos_bin_posix_compat.sh
	@tools/vitteos_bin_matrix_report.sh
	@tools/vitteos_bin_runtime_check.sh

.PHONY: vitteos-scripts-bootstrap
vitteos-scripts-bootstrap:
	@./vitteos/tooling/bootstrap.sh

.PHONY: vitteos-scripts-check
vitteos-scripts-check:
	@bash -n vitteos/tooling/bootstrap.sh
	@python3 -m py_compile vitteos/tooling/check_vit.py
	@if command -v node >/dev/null 2>&1; then \
		node --check vitteos/tooling/run_check.js; \
	else \
		echo "node not installed, JS syntax check skipped"; \
	fi
	@if [ -x "$(BIN_DIR)/$(PROJECT)" ]; then \
		if "$(BIN_DIR)/$(PROJECT)" --help >/dev/null 2>&1; then \
			"$(BIN_DIR)/$(PROJECT)" check vitteos/scripts/vitteos_tooling.vit; \
		else \
			echo "bin/vitte exists but is not runnable on this host (possible arch mismatch)"; \
			exit 2; \
		fi; \
	else \
		echo "bin/vitte not found, run: make build"; \
		exit 2; \
	fi

.PHONY: vitteos-scripts-check-soft
vitteos-scripts-check-soft:
	@bash -n vitteos/tooling/bootstrap.sh
	@python3 -m py_compile vitteos/tooling/check_vit.py
	@if command -v node >/dev/null 2>&1; then \
		node --check vitteos/tooling/run_check.js; \
	else \
		echo "node not installed, JS syntax check skipped"; \
	fi
	@if [ -x "$(BIN_DIR)/$(PROJECT)" ] && "$(BIN_DIR)/$(PROJECT)" --help >/dev/null 2>&1; then \
		"$(BIN_DIR)/$(PROJECT)" check vitteos/scripts/vitteos_tooling.vit; \
	else \
		echo "vitteos-scripts-check-soft: bin/vitte unavailable or mismatched, vit check skipped"; \
	fi

.PHONY: vitteos-domain-contract
vitteos-domain-contract:
	@tools/vitteos_domain_contract_check.sh

.PHONY: vitteos-issues-check
vitteos-issues-check:
	@tools/vitteos_issues_check.sh

.PHONY: vitteos-no-orphan-check
vitteos-no-orphan-check:
	@tools/vitteos_no_orphan_module_check.sh

.PHONY: vitteos-space-naming-lint
vitteos-space-naming-lint:
	@tools/vitteos_space_naming_lint.sh

.PHONY: vitteos-arch-contract-lint
vitteos-arch-contract-lint:
	@tools/vitteos_arch_contract_lint.sh

.PHONY: vitteos-vit-targeted-check
vitteos-vit-targeted-check:
	@tools/vitteos_vit_targeted_check.sh

.PHONY: vitteos-vit-targeted-check-update
vitteos-vit-targeted-check-update:
	@tools/vitteos_vit_targeted_check.sh --update

.PHONY: vitteos-vit-header-lint
vitteos-vit-header-lint:
	@tools/vitteos_vit_header_lint.sh

.PHONY: vitteos-kernel-smoke
vitteos-kernel-smoke:
	@tools/vitteos_kernel_smoke.sh
	@tools/vitteos_kernel_smoke_runtime.sh

.PHONY: vitteos-kernel-smoke-runtime
vitteos-kernel-smoke-runtime:
	@tools/vitteos_kernel_smoke_runtime.sh

.PHONY: vitteos-kernel-smoke-runtime-update
vitteos-kernel-smoke-runtime-update:
	@tools/vitteos_kernel_smoke_runtime.sh --update

.PHONY: vitteos-adr-policy-check
vitteos-adr-policy-check:
	@tools/vitteos_adr_policy_check.sh

.PHONY: vitteos-doctor
vitteos-doctor:
	@tools/vitteos_doctor.sh

.PHONY: vitteos-status
vitteos-status:
	@tools/generate_vitteos_status.sh

.PHONY: vitteos-new-module
vitteos-new-module:
	@if [ -z "$(MODULE)" ]; then \
		echo "usage: make vitteos-new-module MODULE=vitteos/<path>"; \
		exit 2; \
	fi
	@tools/vitteos_new_module.sh "$(MODULE)"

.PHONY: vitteos-quick
vitteos-quick: vitteos-issues-check vitteos-domain-contract vitteos-no-orphan-check vitteos-space-naming-lint vitteos-arch-contract-lint vitteos-vit-header-lint vitteos-vit-targeted-check vitteos-kernel-smoke

.PHONY: vitteos-ci
vitteos-ci: vitteos-bin-runnable-check vitteos-scripts-check-soft vitteos-bin-gate vitteos-issues-check vitteos-domain-contract vitteos-no-orphan-check vitteos-space-naming-lint vitteos-arch-contract-lint vitteos-vit-header-lint vitteos-vit-targeted-check vitteos-kernel-smoke vitteos-adr-policy-check

.PHONY: vitteos-ci-strict
vitteos-ci-strict: vitteos-bin-runnable-check vitteos-scripts-check vitteos-bin-gate vitteos-issues-check vitteos-domain-contract vitteos-no-orphan-check vitteos-space-naming-lint vitteos-arch-contract-lint vitteos-vit-header-lint vitteos-vit-targeted-check vitteos-kernel-smoke vitteos-adr-policy-check

.PHONY: vitteos-ci-local
vitteos-ci-local: vitteos-bin-runnable-check vitteos-scripts-check-soft vitteos-issues-check vitteos-domain-contract vitteos-no-orphan-check vitteos-space-naming-lint vitteos-arch-contract-lint vitteos-vit-header-lint vitteos-vit-targeted-check vitteos-kernel-smoke vitteos-adr-policy-check

.PHONY: vitteos-ci-min
vitteos-ci-min: vitteos-scripts-check-soft vitteos-issues-check vitteos-domain-contract vitteos-no-orphan-check vitteos-space-naming-lint vitteos-vit-header-lint vitteos-vit-targeted-check

.PHONY: pkg-debian
pkg-debian:
	@VERSION=$(PKG_VERSION) PACKAGE_PROFILE=full toolchain/scripts/package/make-debian-deb.sh

.PHONY: pkg-debian-min
pkg-debian-min:
	@VERSION=$(PKG_VERSION) PACKAGE_PROFILE=minimal toolchain/scripts/package/make-debian-deb.sh

.PHONY: pkg-debian-audit
pkg-debian-audit: pkg-debian
	@toolchain/scripts/package/audit-debian-deb.sh pkgout/vitte_$(PKG_VERSION)_$$(dpkg --print-architecture).deb

.PHONY: pkg-debian-install
pkg-debian-install: pkg-debian
	@sudo dpkg -i pkgout/vitte_$(PKG_VERSION)_$$(dpkg --print-architecture).deb

.PHONY: pkg-macos
pkg-macos: macos-pkg-bin
	@VERSION=$(PKG_VERSION) \
		VITTE_BIN_OVERRIDE="$(CURDIR)/$(MACOS_PKG_DIR)/vitte" \
		CHECKSUM_TARGET_BIN="$(CURDIR)/$(MACOS_PKG_DIR)/vitte" \
		toolchain/scripts/package/make-macos-pkg.sh

.PHONY: macos-pkg-entry
macos-pkg-entry:
	@$(MKDIR) "$(MACOS_PKG_DIR)"
	@sed \
		-e 's|const COMPILER_SOURCE_ROOT: string = ".*"|const COMPILER_SOURCE_ROOT: string = "$(MACOS_PKG_COMPILER_ROOT)"|' \
		-e 's|const COMPILER_ENTRY_POINT: string = ".*"|const COMPILER_ENTRY_POINT: string = "$(MACOS_PKG_COMPILER_ENTRY)"|' \
		"$(MACOS_UNIVERSAL_SRC)" > "$(MACOS_PKG_ENTRY)"

.PHONY: macos-pkg-bin
macos-pkg-bin: macos-pkg-entry
	@uname -s | grep -qx 'Darwin' || { echo "[macos-pkg-bin][error] target requires macOS"; exit 1; }
	@command -v cc >/dev/null 2>&1 || { echo "[macos-pkg-bin][error] missing cc"; exit 1; }
	@test -x bin/vitte || $(MAKE) --no-print-directory bootstrap-all
	@CC=cc bin/vitte build "$(MACOS_PKG_ENTRY)" -o "$(MACOS_PKG_DIR)/vitte"
	@cp "$(MACOS_PKG_DIR)/vitte" "$(MACOS_PKG_DIR)/vittec"
	@chmod 755 "$(MACOS_PKG_DIR)/vitte" "$(MACOS_PKG_DIR)/vittec"

.PHONY: macos-universal-bin
macos-universal-bin: macos-pkg-entry
	@uname -s | grep -qx 'Darwin' || { echo "[macos-universal-bin][error] target requires macOS"; exit 1; }
	@command -v lipo >/dev/null 2>&1 || { echo "[macos-universal-bin][error] missing lipo"; exit 1; }
	@command -v cc >/dev/null 2>&1 || { echo "[macos-universal-bin][error] missing cc"; exit 1; }
	@test -x bin/vitte || $(MAKE) --no-print-directory bootstrap-all
	@test -f "$(MACOS_PKG_ENTRY)" || { echo "[macos-universal-bin][error] missing source $(MACOS_PKG_ENTRY)"; exit 1; }
	@$(MKDIR) "$(MACOS_ARM64_DIR)" "$(MACOS_X86_64_DIR)" "$(MACOS_UNIVERSAL_DIR)" "$(MACOS_CC_WRAPPER_DIR)"
	@printf '%s\n' '#!/usr/bin/env sh' 'exec cc -arch arm64 "$$@"' > "$(MACOS_CC_WRAPPER_DIR)/cc-arm64"
	@printf '%s\n' '#!/usr/bin/env sh' 'exec cc -arch x86_64 "$$@"' > "$(MACOS_CC_WRAPPER_DIR)/cc-x86_64"
	@chmod 755 "$(MACOS_CC_WRAPPER_DIR)/cc-arm64" "$(MACOS_CC_WRAPPER_DIR)/cc-x86_64"
	@echo "[macos-universal-bin] building arm64"
	@CC="$(CURDIR)/$(MACOS_CC_WRAPPER_DIR)/cc-arm64" bin/vitte build "$(MACOS_PKG_ENTRY)" -o "$(MACOS_ARM64_DIR)/vitte"
	@echo "[macos-universal-bin] building x86_64"
	@CC="$(CURDIR)/$(MACOS_CC_WRAPPER_DIR)/cc-x86_64" bin/vitte build "$(MACOS_PKG_ENTRY)" -o "$(MACOS_X86_64_DIR)/vitte"
	@lipo -create -output "$(MACOS_UNIVERSAL_DIR)/vitte" "$(MACOS_ARM64_DIR)/vitte" "$(MACOS_X86_64_DIR)/vitte"
	@chmod 755 "$(MACOS_UNIVERSAL_DIR)/vitte"
	@lipo -info "$(MACOS_UNIVERSAL_DIR)/vitte"

.PHONY: pkg-macos-universal
pkg-macos-universal: macos-universal-bin
	@VERSION=$(PKG_VERSION) \
		VITTE_BIN_OVERRIDE="$(CURDIR)/$(MACOS_UNIVERSAL_DIR)/vitte" \
		CHECKSUM_TARGET_BIN="$(CURDIR)/$(MACOS_UNIVERSAL_DIR)/vitte" \
		OUT_FILE_NAME="vitte-$(PKG_VERSION)-universal.pkg" \
		toolchain/scripts/package/make-macos-pkg.sh

.PHONY: pkg-macos-uninstall
pkg-macos-uninstall:
	@VERSION=$(PKG_VERSION) toolchain/scripts/package/make-macos-uninstall-pkg.sh

.PHONY: release-check
release-check: build core-release-gate ci-fast package-layout-lint-strict legacy-import-allowlist-empty ci-completions pkg-macos

.PHONY: release-doctor
release-doctor:
	@python3 tools/release_doctor.py

.PHONY: contract-lockfiles-lint
contract-lockfiles-lint:
	@python3 tools/lint_contract_lockfiles.py

.PHONY: contracts-dashboard
contracts-dashboard:
	@python3 tools/contracts_dashboard.py

.PHONY: security-hardening-gate
security-hardening-gate:
	@python3 tools/security_hardening_gate.py

.PHONY: security-baseline-diff
security-baseline-diff:
	@tools/security_gates_report.sh
	@python3 tools/security_baseline_diff.py

.PHONY: security-gates-report
security-gates-report:
	@tools/security_gates_report.sh

.PHONY: perf-regression-robust
perf-regression-robust:
	@python3 tools/perf_regression_robust.py

.PHONY: perf-budget
perf-budget:
	@python3 tools/perf_budget_check.py

.PHONY: docs-sync-gate
docs-sync-gate:
	@python3 tools/docs_sync_gate.py

.PHONY: plugin-abi-compat
plugin-abi-compat:
	@python3 tools/plugin_abi_compat_check.py

.PHONY: plugin-manifest-lint
plugin-manifest-lint:
	@python3 tools/lint_plugin_manifest.py

.PHONY: plugin-sandbox-lint
plugin-sandbox-lint:
	@python3 tools/lint_plugin_sandbox_permissions.py

.PHONY: plugin-binary-abi
plugin-binary-abi:
	@python3 tools/plugin_binary_abi_smoke.py

.PHONY: repro-report
repro-report:
	@python3 tools/repro_report.py

.PHONY: migration-check
migration-check: diag-snapshots package-layout-lint-strict legacy-import-path-lint

.PHONY: migration-fix-preview
migration-fix-preview:
	@tools/migration_fix_preview.sh

.PHONY: migration-fix-preview-json
migration-fix-preview-json:
	@tools/migration_fix_preview.sh --json

.PHONY: migration-map
migration-map:
	@tools/generate_legacy_migration_doc.py

.PHONY: module-starter
module-starter:
	@if [ -z "$(MODULE)" ] || [ -z "$(OWNER)" ]; then \
		echo "usage: make module-starter MODULE=<name|domain/name> OWNER=@team/name [SINCE=3.0.0] [STABILITY=stable]"; \
		exit 2; \
	fi
	@tools/new_module_starter.sh --module "$(MODULE)" --owner "$(OWNER)" --since "$(if $(SINCE),$(SINCE),3.0.0)" --stability "$(if $(STABILITY),$(STABILITY),stable)"

.PHONY: modules-fix-all
modules-fix-all:
	@tools/modules_fix_all.sh --roots tests examples

.PHONY: modules-fix-all-check
modules-fix-all-check:
	@tools/modules_fix_all.sh --roots tests examples --check

.PHONY: mod-migrate-imports
mod-migrate-imports:
	@tools/mod_migrate_imports.sh --roots tests examples

.PHONY: modules-weekly-deny-legacy
modules-weekly-deny-legacy:
	@DENY_LEGACY_SELF_LEAF=1 tools/modules_tests.sh

.PHONY: modules-weekly-legacy-warn-only
modules-weekly-legacy-warn-only:
	@LEGACY_SELF_LEAF_WARN_ONLY=1 tools/modules_tests.sh

.PHONY: release-modules-gate
release-modules-gate: modules-ci-strict modules-contract-snapshots modules-report legacy-import-allowlist-empty

.PHONY: reports-index
reports-index:
	@python3 tools/reports_index.py

.PHONY: package-index
package-index:
	@python3 tools/generate_package_index.py

.PHONY: perf-baseline
perf-baseline:
	@python3 tools/perf_baseline_report.py

.PHONY: public-benchmark-dashboard
public-benchmark-dashboard:
	@python3 tools/public_benchmark_dashboard.py

.PHONY: release-proof-notes
release-proof-notes:
	@python3 tools/release_proof_notes.py

.PHONY: platon-editor
platon-editor:
	@./bin/vitte build platon-editor/editor_core.vit -o platon-editor/editor_core
	@platon-editor/editor_core

# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------

.PHONY: clean
clean:
	$(RM) $(BUILD_DIR) $(BIN_DIR)

# ------------------------------------------------------------
# Distclean
# ------------------------------------------------------------

.PHONY: distclean
distclean: clean
	$(RM) .cache

# ------------------------------------------------------------
# Help
# ------------------------------------------------------------

.PHONY: help
help:
	@echo ""
	@echo "Vitte Makefile targets:"
	@echo ""
	@echo "  make            build everything"
	@echo "  make build-fast parallel build with auto jobs (JOBS override supported)"
	@echo "  make build-release optimized Vitte build"
	@echo "  make build-pgo-generate build instrumented binary for PGO training"
	@echo "  make build-pgo-use build release binary using merged PGO profile"
	@echo "  make install    build + install binary + Vim/Emacs/Nano/Geany syntax files"
	@echo "  make install-debian install vitte on Debian/Ubuntu (deps + build + install via installer profile PKG_VERSION=$(PKG_VERSION))"
	@echo "  make install-debian-2.1.1 compatibility alias for install-debian"
	@echo "  make install-bin install only the vitte binary (PREFIX=$(PREFIX))"
	@echo "  make install-editors install syntax configs for Vim/Emacs/Nano/Geany in HOME (override USER_HOME=..., PREFIX=...)"
	@echo "  make install-geany install Geany Vitte config only (VITTE_GEANY_WD_MODE=file|project|current)"
	@echo "  make uninstall-geany remove Geany Vitte config from user profile"
	@echo "  make format     run code formatter"
	@echo "  make tidy       run Vitte source audit"
	@echo "  make test       run tests (std/test)"
	@echo "  make quickstart-check verify the beginner path against examples/first_project.vit"
	@echo "  make doctor     print local toolchain and environment readiness"
	@echo "  make parse      run parser-level tests"
	@echo "  make hir-validate run HIR validator test fixtures"
	@echo "  make grammar-sync regenerate grammar surface artifacts from src/vitte/grammar/vitte.ebnf"
	@echo "  make grammar-check fail if grammar generated artifacts are out of sync"
	@echo "  make grammar-test validate grammar corpus + diagnostics snapshots"
	@echo "  make core-language-test validate the focused core language corpus"
	@echo "  make core-semantic-success validate focused passing core semantic examples"
	@echo "  make core-semantic-snapshots validate focused core semantic diagnostics"
	@echo "  make core-semantic-themes run semantic non-regression suites by theme"
	@echo "  make core-semantic-imports-success validate import-focused passing semantic examples"
	@echo "  make core-semantic-imports-snapshots validate import-focused semantic diagnostics"
	@echo "  make core-semantic-resolve-success validate resolve-focused passing semantic examples"
	@echo "  make core-semantic-resolve-snapshots validate resolve-focused semantic diagnostics"
	@echo "  make core-semantic-returns-success validate return-focused passing semantic examples"
	@echo "  make core-semantic-returns-snapshots validate return-focused semantic diagnostics"
	@echo "  make core-semantic-match-success validate match-focused passing semantic examples"
	@echo "  make core-semantic-match-snapshots validate match-focused semantic diagnostics"
	@echo "  make core-semantic-share-success validate share-focused passing semantic examples"
	@echo "  make core-semantic-share-snapshots validate share-focused semantic diagnostics"
	@echo "  make core-semantic-entry-success validate entry-focused passing semantic examples"
	@echo "  make core-semantic-entry-snapshots validate entry-focused semantic diagnostics"
	@echo "  make bootstrap-all verify vittec0.seed trust root and bootstrap-native snapshots"
	@echo "  make bootstrap-native-drift-check ensure native bootstrap changes carry matching snapshots"
	@echo "  make bootstrap-native-contract run seed verification, native snapshots, and bootstrap verification"
	@echo "  make bootstrap-native-fast-contract run fast bootstrap-native checks without rebuilding stage chain"
	@echo "  make bootstrap-source-coverage-check verify seed exercises current bootstrap-native forms"
	@echo "  make bootstrap-contracts-index-check verify bootstrap contract docs paths and targets"
	@echo "  make bootstrap-posix-smoke run POSIX shell syntax and env smoke checks for bootstrap artifacts"
	@echo "  make bootstrap-parity report the retired legacy parity gate and its replacements"
	@echo "  make bootstrap-verify verify bootstrap versions, smoke, AST/IR checks"
	@echo "  make seed-manifest-update regenerate toolchain/seed/manifest.txt from the audited seed artifact"
	@echo "  make seed-rotation-report print seed manifest/hash/version rotation status"
	@echo "  make seed-syntax-test run non-regression syntax checks for vittec0"
	@echo "  make seed-compat-report generate seed compatibility pass/fail report"
	@echo "  make explicit-generics-snapshots validate explicit generic-call IR snapshots"
	@echo "  make diagnostics-locales-lint validate locale files against centralized diagnostics"
	@echo "  make update-diagnostics-ftl synchronize diagnostics locales from the central table"
	@echo "  make diagnostics-ftl-check fail if diagnostics locales are out of sync"
	@echo "  make diagnostics-portable run diagnostics checks through bin/vittec0"
	@echo "  make compiler-smoke-portable run portable compiler smoke checks through bin/vittec0"
	@echo "  make native-binaries-doctor report local compiler binary executability"
	@echo "  make grammar-docs regenerate railroad SVG diagrams"
	@echo "  make grammar-gate run grammar-check + grammar-test"
	@echo "  make core-language-gate run grammar-check + core-language-test + core semantic gates + diagnostics locales lint"
	@echo "  make core-release-gate run the protected language contract gate for release-facing work"
	@echo "  make keywords-normalize apply strict keyword template on docs/book/chapters/keywords/*.md"
	@echo "  make keywords-lint validate keyword quality sections/diagnostics/links/score"
	@echo "  make docs-serve serve docs/ locally over HTTP on http://127.0.0.1:8000 for search and smoke checks"
	@echo "  make site-html regenerate sibling HTML pages in English under docs/"
	@echo "  make docs-phase1-smoke run full docs pipeline phase1 + local index/news/community style smoke"
	@echo "  make docs-maximal run the full documentation generation + validation pipeline (default PHASE=phase3)"
	@echo "  make book-qa-strict run strict book QA checks"
	@echo "  make docs-assets-refresh strip forbidden SRI/CORS attributes from docs HTML"
	@echo "  make docs-assets-check fail if integrity/crossorigin reappears in docs/*.html"
	@echo "  make docs-post-deploy-monitor check vitte-lang.org/index.html for forbidden site.css integrity/crossorigin"
	@echo "  make test-examples build/check all examples/*.vit"
	@echo "  make extern-abi validate #[extern] ABI (host profile)"
	@echo "  make extern-abi-arduino validate #[extern] ABI (arduino)"
	@echo "  make extern-abi-kernel validate #[extern] ABI (kernel grub)"
	@echo "  make extern-abi-kernel-uefi validate #[extern] ABI (kernel uefi)"
	@echo "  make extern-abi-all validate #[extern] ABI (all std vs host)"
	@echo "  make interop-headers-gen generate Vitte ABI metadata under target/interop"
	@echo "  make interop-headers-check fail if generated interop metadata/snapshot are out of date"
	@echo "  make interop-headers-snapshot-update update tests/interop ABI exports snapshot"
	@echo "  make interop-abi-matrix run ABI compatibility matrix"
	@echo "  make quasi-empty-package-tests run checks for newly hardened quasi-empty package modules"
	@echo "  make stdlib-api-lint check stable stdlib ABI surface entries"
	@echo "  make stdlib-profile-snapshots check stdlib profile allow/deny matrix"
	@echo "  make stdlib-abi-compat block ABI removals from v1 to v2"
	@echo "  make ci-std-fast std-focused CI (stdlib + snapshots + wrappers)"
	@echo "  make ci-bridge-compat alias of ci-mod-fast for vitte liaison compatibility"
	@echo "  make modules-tests run module graph/doctor fixtures"
	@echo "  make modules-contract-snapshots assert modules contract snapshots"
	@echo "  make module-shape-policy enforce single module layout (<name>.vit xor <name>/mod.vit)"
	@echo "  make modules-snapshots assert mod graph/doctor outputs"
	@echo "  make modules-snapshots-update regenerate modules snapshot files (.must/.diagjson/.codes/.fr)"
	@echo "  make modules-snapshots-bless regenerate modules snapshots and print diffs"
	@echo "  make test-golden run frontend golden snapshots (AST/HIR/MIR + diagnostics)"
	@echo "  make test-golden-update regenerate frontend golden snapshots under tests/golden/frontend/snapshots"
	@echo "  make test-golden-critical run cross-platform critical frontend golden subset"
	@echo "  make explain-snapshots assert vitte explain outputs"
	@echo "  make compiler-power-gate run focused compiler maturity gate (diagnostics + semantic core + MIR + backend smoke)"
	@echo "  make same-output-hash verify deterministic emit hash stability"
	@echo "  make completions-gen regenerate bash/zsh/fish completions from unified spec"
	@echo "  make completions-check verify generated completions are up to date"
	@echo "  make completions-snapshots run completion snapshot assertions"
	@echo "  make completions-snapshots-update update completion golden snapshots"
	@echo "  make completions-lint syntax-check bash/zsh/fish completion files"
	@echo "  make ci-completions run completion check + lint + snapshots + fallback"
	@echo "  make ci-fast run fast CI chain for language/compiler snapshots and checks"
	@echo "  make ci-strict run strict CI chain with additional diagnostics/highlight checks"
	@echo "  make repo-hygiene-check verify root-level repository hygiene constraints"
	@echo "  make docs-paths-check verify README/docs local path references exist"
	@echo "  make dx-adoption run DX/adoption quality gates"
	@echo "  make ci-mod-fast module-focused CI (grammar + snapshots + module tests)"
	@echo "  make ci-fast-compiler compiler-focused CI with cache skip (grammar + resolve + module snapshots + explain + runtime matrix)"
	@echo "  make compiler-max-gate-fast run consolidated compiler quality gate (fast profile)"
	@echo "  make compiler-max-gate run consolidated compiler quality gate (full profile)"
	@echo "  make vittec-kernel run Vitte-only kernel bootstrap check"
	@echo "  make vitteos-bin-quality run /bin quality checks + matrix report"
	@echo "  make vitteos-bin-runnable-check assert bin/vitte is host-runnable (non-regression arch/format guard)"
	@echo "  make vitteos-bin-runtime run runtime-smoke probes and update runtime column"
	@echo "  make pkg-debian build Debian .deb installer (PACKAGE_PROFILE=full, PKG_VERSION=$(PKG_VERSION))"
	@echo "  make pkg-debian-min build Debian .deb installer (PACKAGE_PROFILE=minimal, PKG_VERSION=$(PKG_VERSION))"
	@echo "  make pkg-debian-audit audit generated Debian .deb content and largest files"
	@echo "  make pkg-debian-install build and install Debian .deb locally via dpkg"
	@echo "  make pkg-macos build macOS installer pkg (PKG_VERSION=$(PKG_VERSION))"
	@echo "  make macos-universal-bin build target/universal/vitte (arm64 + x86_64 via lipo)"
	@echo "  make pkg-macos-universal build macOS universal installer pkg (vitte-$(PKG_VERSION)-universal.pkg)"
	@echo "  make pkg-macos-uninstall build macOS uninstall pkg (PKG_VERSION=$(PKG_VERSION))"
	@echo "  make release-check run build + core-release-gate + ci-fast + ci-completions + pkg build"
	@echo "  make release-doctor run the snapshot/release readiness report suite"
	@echo "  make reports-index build target/reports/index.json (unified reports registry)"
	@echo "  make perf-baseline build competitive KPI baseline JSON+Markdown under target/reports/competitive"
	@echo "  make public-benchmark-dashboard generate publication dashboard + KPI (3/3 use cases)"
	@echo "  make release-proof-notes generate proof-oriented release notes + tag candidate"
	@echo "  make all-tests run full grouped test inventory"
	@echo "  make packages-gate run package governance/layout/perf/contract gate"
	@echo "  make make-targets-doc regenerate docs/MAKE_TARGETS.md from make help"
	@echo "  make make-targets-doc-check fail if docs/MAKE_TARGETS.md is out of date"
	@echo "  make package-index generate docs/PACKAGE_INDEX.md from package metadata"
	@echo "  make ci-mod-fast module-focused CI (grammar + snapshots + module tests)"
	@echo "  make ci-fast-compiler compiler-focused CI with cache skip (grammar + resolve + module snapshots + explain + runtime matrix)"
	@echo "  make vittec-kernel run Vitte-only kernel bootstrap check"
	@echo "  make vitteos-bin-quality run /bin quality checks + matrix report"
	@echo "  make vitteos-bin-runnable-check assert bin/vitte is host-runnable (non-regression arch/format guard)"
	@echo "  make vitteos-bin-runtime run runtime-smoke probes and update runtime column"
	@echo "  make vitteos-bin-matrix generate migration matrix report under target/reports"
	@echo "  make vitteos-bin-gate run full /bin gate (uniformity + vit_check + lint + posix + matrix + runtime)"
	@echo "  make vitteos-scripts-bootstrap run VitteOS multi-language bootstrap script"
	@echo "  make vitteos-scripts-check strict validation for VitteOS tooling scripts + main .vit"
	@echo "  make vitteos-scripts-check-soft non-blocking script check for CI/doc structure hosts"
	@echo "  make vitteos-issues-check validate M1/M2/M3 milestone mapping to tickets"
	@echo "  make vitteos-domain-contract verify stable domain layout, required .vit files, and owner coverage"
	@echo "  make vitteos-no-orphan-check block .vit roots/modules outside declared domains"
	@echo "  make vitteos-space-naming-lint enforce canonical 'space <domain>/<module>' naming"
	@echo "  make vitteos-arch-contract-lint enforce inter-domain contract and layering rules"
	@echo "  make vitteos-vit-header-lint enforce <<< >>> header block on critical VitteOS .vit files"
	@echo "  make vitteos-vit-targeted-check run targeted checks for critical VitteOS .vit files"
	@echo "  make vitteos-vit-targeted-check-update refresh targeted-check snapshot"
	@echo "  make vitteos-kernel-smoke run 2-level smoke (structure + runtime probe when available)"
	@echo "  make vitteos-kernel-smoke-runtime run only runtime smoke probe (qemu/bochs best effort)"
	@echo "  make vitteos-kernel-smoke-runtime-update refresh runtime smoke snapshot"
	@echo "  make vitteos-adr-policy-check enforce ADR id in commit message for boot/mm/sched changes"
	@echo "  make vitteos-doctor print VitteOS environment diagnostics"
	@echo "  make vitteos-status regenerate docs/vitteos/STATUS.md from checks/snapshots"
	@echo "  make vitteos-new-module MODULE=vitteos/<path> generate module template files"
	@echo "  make vitteos-quick run quick local loop (issues + domain + orphan + space + arch-contract + header + targeted + smoke)"
	@echo "  make vitteos-ci run VitteOS CI chain (soft scripts + bin gate + issues + domain + orphan + space + arch-contract + header + targeted + smoke + adr)"
	@echo "  make vitteos-ci-strict run strict VitteOS CI chain (strict scripts + bin gate + issues + domain + orphan + space + arch-contract + header + targeted + smoke + adr)"
	@echo "  make vitteos-ci-local run local VitteOS CI chain without external /bin migration dataset"
	@echo "  make vitteos-ci-min run minimal fast checks for pre-commit local loop"
	@echo "  make platon-editor build and self-test platon editor core"
	@echo "  make std-check  verify std layout"
	@echo "  make clean      remove build artifacts"
	@echo "  make distclean  full cleanup"
	@echo ""
	@echo "  make generate-highlights regenerate Vim/Emacs/Nano/Geany highlight assets from grammar"
	@echo "  make highlight-snapshots assert highlight snapshots for Vim/Emacs/Nano/Geany"
	@echo "  make highlight-snapshots-update refresh highlight snapshots for Vim/Emacs/Nano/Geany"
	@echo "  make highlights-ci run full highlight generation + snapshots + coverage"
	@echo "  make geany-lint verify generated Geany config and snapshot consistency"
	@echo "  make geany-install-check verify local Geany install mapping/snippets"


.PHONY: ffi-abi-gate
ffi-abi-gate:
	@test -f src/vitte/stdlib/ffi/abi.vitl
	@test -f data/ffi/abi/abi_profiles.csv
	@test -f data/ffi/abi/abi_profiles.json
	@tools/ffi/validate_abi_profiles.py
	@tools/ffi/update_matrix_abi_coverage.py
	@tools/ffi/generate_abi_coverage_report.py


.PHONY: docs-assets-refresh
docs-assets-refresh:
	@tools/docs/refresh_assets_policy.py

.PHONY: docs-assets-check
docs-assets-check:
	@tools/docs/check_assets_policy.py

.PHONY: docs-post-deploy-monitor
docs-post-deploy-monitor:
	@tools/docs/post_deploy_css_monitor.sh

.PHONY: docs-sri-refresh
docs-sri-refresh: docs-assets-refresh
	@echo "[deprecated] use: make docs-assets-refresh"

.PHONY: docs-sri-check
docs-sri-check: docs-assets-check
	@echo "[deprecated] use: make docs-assets-check"


.PHONY: vitte-emit-gate
vitte-emit-gate:
	@python3 tools/vitte_emit/run_checks.py
	@python3 tools/ffi/validate_abi_profiles.py
	@python3 tools/vitte_emit/generate_artifacts.py
	@test -f target/vitte_emit/demo_module.vitir
	@test -f target/vitte_emit/demo_module.vasm
	@test -f target/vitte_emit/demo_module.abi.meta
	@test -f target/vitte_emit/module_exports.vitl
	@test -f target/reports/vitte_emit_coverage.md


.PHONY: llvm-backend-gate
llvm-backend-gate:
	@bin/vitte check src/vitte/compiler/tests/llvm_tests.vit
	@bin/vitte check src/vitte/compiler/backends/llvm_bindings/tests/smoke.vit
	@python3 tools/llvm/run_checks.py
	@python3 tools/llvm/generate_artifacts.py
	@python3 tools/llvm/check_backend_reports.py
	@$(MAKE) --no-print-directory llvm-native-final-gate
	@test -f target/llvm/demo_module.ll
	@test -f target/llvm/demo_module.o.meta
	@test -f target/llvm/debug_format.txt
	@test -f target/llvm/opt_levels.txt
	@test -f target/llvm/pgo_status.txt
	@test -f target/reports/llvm_backend_coverage.md

.PHONY: llvm-native-final-gate
llvm-native-final-gate:
	@python3 tools/llvm/native_final_gate.py


.PHONY: wasm-backend-gate
wasm-backend-gate:
	@python3 tools/wasm/run_checks.py
	@python3 tools/wasm/generate_artifacts.py
	@test -f target/wasm/demo_module.wat
	@test -f target/wasm/wasi_status.txt
	@test -f target/wasm/web_api_surface.txt
	@test -f target/wasm/size_opt.txt
	@test -f target/reports/wasm_backend_coverage.md


.PHONY: backend-ir-contract-audit
backend-ir-contract-audit:
	@python3 tools/backend_ir_contract_audit.py


.PHONY: c-abi-contract-audit
c-abi-contract-audit:
	@python3 tools/c_abi_contract_audit.py


.PHONY: backend-value-lowering-audit
backend-value-lowering-audit:
	@python3 tools/backend_value_lowering_audit.py


.PHONY: backend-layout-abi-audit
backend-layout-abi-audit:
	@python3 tools/backend_layout_abi_audit.py


.PHONY: native-object-determinism-gate
native-object-determinism-gate:
	@python3 tools/native_object_determinism_test.py


.PHONY: backend-native-object-audit
backend-native-object-audit: native-object-determinism-gate
	@python3 tools/backend_native_object_audit.py


.PHONY: backend-object-structure-audit
backend-object-structure-audit: native-object-determinism-gate
	@python3 tools/backend_object_structure_audit.py


.PHONY: backend-debug-unwind-audit
backend-debug-unwind-audit: native-object-determinism-gate
	@python3 tools/backend_debug_unwind_audit.py


.PHONY: backend-native-toolchain-audit
backend-native-toolchain-audit: native-object-determinism-gate
	@python3 tools/backend_native_toolchain_audit.py


.PHONY: backend-architecture-matrix-gate
backend-architecture-matrix-gate:
	@python3 tools/backend_architecture_matrix_test.py


.PHONY: backend-cross-sysroot-gate
backend-cross-sysroot-gate:
	@python3 tools/backend_cross_sysroot_test.py


.PHONY: backend-differential-gate
backend-differential-gate: compiler-test-suite-check-gate
	@python3 tools/backend_differential_test.py


.PHONY: backend-gate
backend-gate: vitte-emit-gate llvm-backend-gate wasm-backend-gate backend-ir-contract-audit c-abi-contract-audit backend-value-lowering-audit backend-layout-abi-audit backend-native-object-audit backend-object-structure-audit backend-debug-unwind-audit backend-native-toolchain-audit backend-architecture-matrix-gate backend-cross-sysroot-gate backend-differential-gate
	@python3 tools/backend_surface_audit.py


.PHONY: backend-native-gate
backend-native-gate: vitte-emit-gate


.PHONY: package-manager-gate
package-manager-gate:
	@python3 tools/package_manager/run_checks.py
	@python3 tools/package_manager/generate_artifacts.py
	@test -f target/package_manager/registry.json
	@test -f target/package_manager/build_cache.db
	@test -f target/package_manager/cross_targets.txt
	@test -f target/package_manager/incremental_status.txt
	@test -f target/reports/package_manager_coverage.md


.PHONY: lsp-gate
lsp-gate:
	@python3 tools/lsp/run_checks.py
	@python3 tools/lsp/generate_artifacts.py
	@test -f target/lsp/hover_demo.json
	@test -f target/lsp/completion_demo.json
	@test -f target/lsp/diagnostics_demo.json
	@test -f target/lsp/definition_demo.json
	@test -f target/lsp/references_demo.json
	@test -f target/reports/lsp_coverage.md


.PHONY: stdlib-gate
stdlib-gate:
	@python3 tools/stdlib/run_checks.py
	@python3 tools/stdlib/generate_artifacts.py
	@test -f target/stdlib/collections_demo.txt
	@test -f target/stdlib/io_demo.txt
	@test -f target/stdlib/async_demo.txt
	@test -f target/stdlib/ffi_demo.txt
	@test -f target/reports/stdlib_coverage.md


.PHONY: mir-opt-gate
mir-opt-gate:
	@python3 tools/mir_opt/run_checks.py
	@python3 tools/mir_opt/generate_artifacts.py
	@! grep -En "FAIL" target/mir_opt/passes.txt >/dev/null
	@test -f target/mir_opt/passes.txt
	@test -f target/mir_opt/analysis.json
	@test -f target/mir_opt/fixture_metrics.csv
	@test -f target/reports/mir_opt_coverage.md


.PHONY: interproc-opt-gate
interproc-opt-gate:
	@python3 tools/interproc_opt/run_checks.py
	@python3 tools/interproc_opt/generate_artifacts.py
	@! grep -En "FAIL" target/interproc_opt/passes.txt >/dev/null
	@test -f target/interproc_opt/passes.txt
	@test -f target/interproc_opt/analysis.json
	@test -f target/interproc_opt/fixture_metrics.csv
	@test -f target/reports/interproc_opt_coverage.md


.PHONY: static-analysis-gate
static-analysis-gate:
	@python3 tools/static_analysis/run_checks.py
	@python3 tools/static_analysis/generate_artifacts.py
	@! grep -En "FAIL" target/static_analysis/analyses.txt >/dev/null
	@test -f target/static_analysis/analyses.txt
	@test -f target/reports/static_analysis_coverage.md
	@test -f target/static_analysis/analysis.json
	@test -f target/static_analysis/fixture_metrics.csv


.PHONY: analysis-gate
analysis-gate: mir-opt-gate interproc-opt-gate static-analysis-gate


.PHONY: type-system-gate
.PHONY: typeck-differential-test
typeck-differential-test:
	@python3 tools/typeck_differential_test.py


.PHONY: typeck-fuzz-test
typeck-fuzz-test:
	@python3 tools/typeck_fuzz_test.py


type-system-gate:
	@python3 tools/type_system/run_checks.py
	@python3 tools/typeck_surface_audit.py
	@python3 tools/check_typeck_diagnostic_contracts.py
	@python3 tools/check_type_system_rules.py
	@$(MAKE) --no-print-directory typeck-differential-test
	@$(MAKE) --no-print-directory typeck-fuzz-test
	@python3 tools/type_system/generate_artifacts.py
	@! grep -En "FAIL" target/type_system/features.txt >/dev/null
	@test -f target/type_system/features.txt
	@test -f target/type_system/analysis.json
	@test -f target/type_system/fixture_metrics.csv
	@test -f target/reports/type_system_coverage.md


.PHONY: memory-model-gate
memory-model-gate:
	@python3 tools/memory_model/run_checks.py
	@python3 tools/memory_model/generate_artifacts.py
	@! grep -En "FAIL" target/memory_model/features.txt >/dev/null
	@test -f target/memory_model/features.txt
	@test -f target/memory_model/analysis.json
	@test -f target/memory_model/fixture_metrics.csv
	@test -f target/reports/memory_model_coverage.md


.PHONY: concurrency-model-gate
concurrency-model-gate:
	@python3 tools/concurrency_model/run_checks.py
	@python3 tools/concurrency_model/generate_artifacts.py
	@! grep -En "FAIL" target/concurrency_model/features.txt >/dev/null
	@test -f target/concurrency_model/features.txt
	@test -f target/concurrency_model/analysis.json
	@test -f target/concurrency_model/fixture_metrics.csv
	@test -f target/reports/concurrency_model_coverage.md


.PHONY: compiler-architecture-gate
compiler-architecture-gate:
	@python3 tools/compiler_arch/run_checks.py
	@python3 tools/compiler_arch/generate_artifacts.py
	@test -f target/compiler_arch/layers.txt
	@test -f target/compiler_arch/modules.txt
	@test -f target/reports/compiler_architecture.md


.PHONY: compiler-components-gate
compiler-components-gate:
	@python3 tools/compiler_components/run_checks.py
	@python3 tools/compiler_components/generate_artifacts.py
	@test -f target/compiler_components/components_count.txt
	@test -f target/compiler_components/components_list.txt
	@test -f target/reports/compiler_components_coverage.md


.PHONY: compiler-topology-gate
compiler-topology-gate:
	@python3 tools/compiler_topology/run_checks.py
	@python3 tools/compiler_topology/generate_artifacts.py
	@test -f target/compiler_topology/top_level_dirs.txt
	@test -f target/compiler_topology/topology_count.txt
	@test -f target/compiler_topology/packed_modules_count.txt
	@test -f target/reports/compiler_topology_coverage.md


.PHONY: compiler-gate
compiler-gate: analysis-gate type-system-gate memory-model-gate concurrency-model-gate compiler-architecture-gate compiler-components-gate compiler-topology-gate grammar-alignment-test roadmap-ecosystem-gate architecture-docs-check backend-gate cli-diagnostics-snapshots tidy


.PHONY: optimization-phase2-gate
optimization-phase2-gate:
	@python3 tools/optimization_phase2/validate_phase2_csv.py
	@python3 tools/optimization_phase2/generate_kpi_report.py
	@python3 tools/optimization_phase2/update_matrix_from_summary.py
	@! grep -En "FAIL" data/optimization_phase2/SUMMARY.md >/dev/null
	@test -f data/optimization_phase2/SUMMARY.md
	@test -f data/optimization_phase2/reports/sprint-1.md
	@test -f data/optimization_phase2/reports/sprint-2.md
	@test -f data/optimization_phase2/reports/sprint-3.md
	@test -f data/optimization_phase2/reports/sprint-4.md
	@test -f data/optimization_phase2/reports/hot_paths_success.md
	@test -f data/optimization_phase2/reports/memory_allocations.md
	@test -f data/optimization_phase2/reports/jit_async_loops.md


.PHONY: diagnostic-contracts
diagnostic-contracts:
	@python3 tools/check_diagnostic_schema.py
	@python3 tools/check_diagnostic_catalog.py
	@python3 tools/check_compiler_diagnostic_contract.py
	@python3 tools/check_span_provenance_contract.py
	@bin/vittec0 check src/vitte/compiler/diagnostics/diagnostic.vit
	@bin/vittec0 check src/vitte/compiler/diagnostics/json.vit
	@bin/vittec0 check src/vitte/compiler/diagnostics/lsp.vit
	@bin/vittec0 check src/vitte/compiler/diagnostics/sarif.vit
	@bin/vittec0 check src/vitte/compiler/diagnostics/render.vit
	@bin/vittec0 check src/vitte/compiler/diagnostics/suggestions.vit
	@bin/vittec0 check src/vitte/compiler/diagnostics/mod.vit
	@bin/vittec0 check src/vitte/compiler/infrastructure/session/diagnostics.vit
	@bin/vittec0 check src/vitte/compiler/analysis/report.vit
	@bin/vittec0 check src/vitte/compiler/middle/typecheck/diagnostics.vit
	@bin/vittec0 check src/vitte/compiler/middle/borrow/checks.vit
	@bin/vittec0 check src/vitte/compiler/tests/diagnostic_snapshot_tests.vit


.PHONY: diagnostic-snapshots
diagnostic-snapshots:
	@bin/vittec0 check src/vitte/compiler/diagnostics/suggestions.vit
	@bin/vittec0 check src/vitte/compiler/diagnostics/render.vit
	@bin/vittec0 check src/vitte/compiler/tests/diagnostic_snapshot_tests.vit
	@bin/vittec0 check src/vitte/compiler/tests/parser_tests.vit


.PHONY: diagnostic-fuzz
diagnostic-fuzz:
	@python3 -m py_compile tools/frontend_syntax_check.py
	@python3 tools/frontend_syntax_check.py tests/frontend_syntax/invalid/standalone_attribute.vit >/dev/null
	@python3 -m py_compile tools/check_diagnostics_locales.py tools/update_diagnostics_ftl.py
	@python3 tools/check_diagnostics_locales.py | grep -Eq '^\[diagnostics-locales\] OK .*codes='
	@tools/update_diagnostics_ftl.py --check


.PHONY: diagnostic-quality
diagnostic-quality: diagnostic-contracts diagnostic-snapshots diagnostic-fuzz
