# ============================================================
# Makefile — Vitte Project
# Target: compiler / runtime / std / tools
# ============================================================

# ------------------------------------------------------------
# Global config
# ------------------------------------------------------------

PROJECT      := vitte
BUILD_DIR    := build
BIN_DIR      := bin
SRC_DIR      := src
STD_DIR      := src/vitte/packages
TOOLS_DIR    := tools

CC           := clang
CXX          ?= clang++
CXX_FALLBACK ?= g++
AUTO_CXX_FALLBACK ?= 1
AR           := ar
RM           := rm -rf
MKDIR        := mkdir -p
INSTALL      := install
CP           := cp -f

# Auto-fallback to g++ when clang++ cannot locate C++ standard headers.
# This keeps local builds working on hosts with partial clang toolchains.
ifeq ($(AUTO_CXX_FALLBACK),1)
ifneq ($(origin CXX),command line)
  CXX_STDLIB_OK := $(shell printf '#include <cstddef>\nint main(){return 0;}\n' | $(CXX) -std=c++20 -x c++ -fsyntax-only - >/dev/null 2>&1; echo $$?)
  ifneq ($(CXX_STDLIB_OK),0)
    ifneq ($(shell command -v $(CXX_FALLBACK) 2>/dev/null),)
      $(warning [make] CXX='$(CXX)' missing C++ std headers; falling back to '$(CXX_FALLBACK)')
      CXX := $(CXX_FALLBACK)
    else
      $(warning [make] CXX='$(CXX)' missing C++ std headers and fallback '$(CXX_FALLBACK)' not found)
    endif
  endif
endif
endif

PREFIX       ?= /usr/local
DESTDIR      ?=
BINDIR       ?= $(DESTDIR)$(PREFIX)/bin
USER_HOME    ?= $(HOME)
VIM_DIR      ?= $(USER_HOME)/.vim
EMACS_DIR    ?= $(USER_HOME)/.emacs.d
NANO_DIR     ?= $(USER_HOME)/.config/nano
LEGACY_ALLOWLIST_BUDGET ?= 5

CFLAGS       := -std=c17 -Wall -Wextra -Werror -O2 -g
CXXFLAGS     := -std=c++20 -Wall -Wextra -Werror -O2 -g
LDFLAGS      :=

# Optional dependency roots (e.g. Homebrew)
ifdef OPENSSL_DIR
  CXXFLAGS += -I$(OPENSSL_DIR)/include
  LDFLAGS  += -L$(OPENSSL_DIR)/lib
endif
ifdef CURL_DIR
  CXXFLAGS += -I$(CURL_DIR)/include
  LDFLAGS  += -L$(CURL_DIR)/lib
endif

# Runtime deps
LDFLAGS += -lssl -lcrypto
CURL_CFLAGS  := $(shell pkg-config --cflags libcurl 2>/dev/null)
CURL_LDFLAGS := $(shell pkg-config --libs libcurl 2>/dev/null)
ifneq ($(strip $(CURL_LDFLAGS)),)
  CXXFLAGS += $(CURL_CFLAGS)
  LDFLAGS  += $(CURL_LDFLAGS)
endif
KERNEL_LDFLAGS := $(filter-out -lcurl,$(LDFLAGS))

CLANG_TIDY   := clang-tidy
FORMAT       := clang-format

# ------------------------------------------------------------
# Files
# ------------------------------------------------------------

C_SOURCES    := $(shell find $(SRC_DIR) -name '*.c')
CPP_SOURCES  := $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS     := \
	$(C_SOURCES:%.c=$(BUILD_DIR)/%.o) \
	$(CPP_SOURCES:%.cpp=$(BUILD_DIR)/%.o)

KERNEL_CPP_SOURCES := $(filter-out src/compiler/backends/runtime/vitte_runtime.cpp,$(CPP_SOURCES))
KERNEL_OBJECTS := $(KERNEL_CPP_SOURCES:%.cpp=$(BUILD_DIR)/kernel/%.o)
KERNEL_TOOLS_DIR := target/kernel-tools

# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------

.PHONY: all
all: build

# ------------------------------------------------------------
# Install
# ------------------------------------------------------------

.PHONY: install install-bin install-editors install-debian-2.1.1
install: build install-bin install-editors

install-bin:
	@$(MKDIR) "$(BINDIR)"
	@$(INSTALL) -m 755 "$(BIN_DIR)/$(PROJECT)" "$(BINDIR)/$(PROJECT)"
	@echo "Installed binary: $(BINDIR)/$(PROJECT)"

install-editors:
	@$(MKDIR) "$(VIM_DIR)/syntax" "$(VIM_DIR)/indent" "$(VIM_DIR)/ftdetect" "$(VIM_DIR)/ftplugin" "$(VIM_DIR)/compiler"
	@$(CP) editors/vim/vitte.vim "$(VIM_DIR)/syntax/vitte.vim"
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

install-debian-2.1.1:
	@toolchain/scripts/install/install-debian-vitte-2.1.1.sh

# ------------------------------------------------------------
# Build
# ------------------------------------------------------------

.PHONY: build
build: dirs $(BIN_DIR)/$(PROJECT)

$(BIN_DIR)/$(PROJECT): $(OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS)

.PHONY: vittec-kernel kernel-tools
vittec-kernel: dirs $(KERNEL_TOOLS_DIR)/vittec-kernel

kernel-tools: vittec-kernel

$(KERNEL_TOOLS_DIR)/vittec-kernel: $(KERNEL_OBJECTS)
	@$(MKDIR) $(KERNEL_TOOLS_DIR)
	$(CXX) $^ -o $@ $(KERNEL_LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/%.o: %.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ------------------------------------------------------------
# Directories
# ------------------------------------------------------------

.PHONY: dirs
dirs:
	@$(MKDIR) $(BUILD_DIR)
	@$(MKDIR) $(BIN_DIR)

# ------------------------------------------------------------
# Formatting
# ------------------------------------------------------------

.PHONY: format
format:
	$(FORMAT) -i $(C_SOURCES) $(CPP_SOURCES)

# ------------------------------------------------------------
# Static analysis
# ------------------------------------------------------------

.PHONY: tidy
tidy:
	$(CLANG_TIDY) \
		$(CPP_SOURCES) \
		-- \
		$(CXXFLAGS)

# ------------------------------------------------------------
# Tests (placeholder)
# ------------------------------------------------------------

.PHONY: test
test:
	@toolchain/scripts/test/run.sh

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
hir-validate-test: dirs
	$(CXX) $(CXXFLAGS) -Isrc \
		tools/hir_validate_test.cpp \
		src/compiler/ir/validate.cpp \
		src/compiler/ir/hir.cpp \
		src/compiler/frontend/diagnostics.cpp \
		src/compiler/frontend/ast.cpp \
		-o $(BIN_DIR)/hir_validate_test

.PHONY: hir-validate-fixture
hir-validate-fixture: dirs
	$(CXX) $(CXXFLAGS) -Isrc \
		tools/hir_validate_fixture.cpp \
		src/compiler/frontend/lexer.cpp \
		src/compiler/frontend/parser.cpp \
		src/compiler/frontend/diagnostics.cpp \
		src/compiler/frontend/ast.cpp \
		src/compiler/frontend/disambiguate.cpp \
		src/compiler/frontend/lower_hir.cpp \
		src/compiler/ir/hir.cpp \
		src/compiler/ir/validate.cpp \
		-o $(BIN_DIR)/hir_validate_fixture

.PHONY: hir-validate
hir-validate: hir-validate-test hir-validate-fixture
	@$(BIN_DIR)/hir_validate_test
	@$(BIN_DIR)/hir_validate_fixture

.PHONY: check-tests
check-tests:
	@tools/check_tests.sh

.PHONY: stress-alloc
stress-alloc:
	@tools/stress_alloc_examples.sh

.PHONY: core-projects
core-projects:
	@tools/build_core_projects.sh

.PHONY: test-examples
test-examples:
	@tools/build_examples_matrix.sh

.PHONY: arduino-projects
arduino-projects:
	@tools/build_arduino_projects.sh

.PHONY: negative-tests
negative-tests:
	@tools/negative_tests.sh

.PHONY: diag-snapshots
diag-snapshots:
	@tools/diag_snapshots.sh

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
	@python3 docs/book/scripts/sync_grammar_surface.py

.PHONY: grammar-check
grammar-check:
	@python3 docs/book/scripts/sync_grammar_surface.py --check

.PHONY: book-qa
book-qa:
	@python3 docs/book/scripts/qa_book.py

.PHONY: book-qa-strict
book-qa-strict:
	@python3 docs/book/scripts/qa_book.py --strict

.PHONY: update-diagnostics-ftl
update-diagnostics-ftl:
	@tools/update_diagnostics_ftl.py

.PHONY: ci-strict
ci-strict: grammar-check book-qa-strict package-layout-lint legacy-import-path-lint negative-tests diag-snapshots

.PHONY: ci-fast
ci-fast: grammar-check package-layout-lint legacy-import-path-lint negative-tests diag-snapshots completions-snapshots wrapper-stage-test

.PHONY: ci-completions
ci-completions: completions-check completions-lint completions-snapshots completions-fallback

.PHONY: runtime-matrix-modules
runtime-matrix-modules:
	@$(BIN_DIR)/$(PROJECT) check --lang=en tests/vitte_packages_runtime_matrix.vit

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

.PHONY: repro
repro:
	@tools/repro_compare.sh

.PHONY: repro-generate
repro-generate:
	@tools/repro_generate_cpp.sh

# ------------------------------------------------------------
# Stdlib checks
# ------------------------------------------------------------

.PHONY: std-check
std-check:
	@echo "Checking std layout…"
	@test -d $(STD_DIR)/core
	@test -d $(STD_DIR)/io
	@test -d $(STD_DIR)/math
	@test -d $(STD_DIR)/test
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

.PHONY: std-core-tests
std-core-tests:
	@tools/test_std_core.sh

.PHONY: stdlib-api-lint
stdlib-api-lint:
	@tools/lint_stdlib_api.py

.PHONY: stdlib-profile-snapshots
stdlib-profile-snapshots:
	@tools/stdlib_profile_snapshots.sh

.PHONY: stdlib-abi-compat
stdlib-abi-compat:
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

.PHONY: export-policy-lint
export-policy-lint:
	@tools/modules_contract_snapshots.sh

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

.PHONY: array-export-snapshot-lint
array-export-snapshot-lint:
	@tools/lint_array_export_snapshot.py

.PHONY: ast-export-snapshot-lint
ast-export-snapshot-lint:
	@tools/lint_ast_export_snapshot.py

.PHONY: core-mod-lint
core-mod-lint:
	@tools/lint_core_mod_contracts.py

.PHONY: core-no-internal-exports-lint
core-no-internal-exports-lint:
	@python3 tools/lint_core_no_internal_exports.py

.PHONY: core-no-entry-lint
core-no-entry-lint:
	@python3 tools/lint_core_no_entry.py

.PHONY: core-export-naming-lint
core-export-naming-lint:
	@python3 tools/lint_core_export_naming.py

.PHONY: core-sensitive-imports-lint
core-sensitive-imports-lint:
	@python3 tools/lint_core_sensitive_imports.py

.PHONY: core-alias-pkg-lint
core-alias-pkg-lint:
	@python3 tools/lint_core_alias_pkg.py

.PHONY: modules-use-as-strict-lint
modules-use-as-strict-lint:
	@python3 tools/lint_modules_use_as_strict.py

.PHONY: core-compat-contracts-lint
core-compat-contracts-lint:
	@python3 tools/lint_core_compat_contracts.py

.PHONY: core-contract-snapshots
core-contract-snapshots:
	@python3 tools/generate_contract_snapshots_from_mod.py --packages=core
	@tools/core_contract_snapshots.sh

.PHONY: core-contract-snapshots-update
core-contract-snapshots-update:
	@tools/core_contract_snapshots.sh --update

.PHONY: core-facade-snapshot
core-facade-snapshot:
	@tools/core_facade_snapshot.sh

.PHONY: core-facade-snapshot-update
core-facade-snapshot-update:
	@tools/core_facade_snapshot.sh --update

.PHONY: core-smoke
core-smoke:
	@tools/core_smoke.sh

.PHONY: core-no-side-effects-test
core-no-side-effects-test:
	@tools/core_smoke.sh SRC=tests/core/vitte_core_no_side_effects.vit

.PHONY: core-profile-strict-test
core-profile-strict-test:
	@tools/core_smoke.sh SRC=tests/core/core_profile_strict.vit

.PHONY: core-analyze-json
core-analyze-json:
	@tools/analyze_core_json.sh

.PHONY: core-contract-diff
core-contract-diff:
	@tools/core_contract_diff.sh

.PHONY: core-bench
core-bench:
	@tools/core_bench.sh

.PHONY: core-fuzz
core-fuzz:
	@tools/core_fuzz.sh

.PHONY: core-gtk-e2e
core-gtk-e2e:
	@tools/gtk_core_quickfix_e2e.sh

.PHONY: core-only-ci
core-only-ci: core-mod-lint core-no-internal-exports-lint core-no-entry-lint core-export-naming-lint core-sensitive-imports-lint core-contract-snapshots core-facade-snapshot core-smoke core-no-side-effects-test core-profile-strict-test core-analyze-json core-compat-contracts-lint

.PHONY: core-strict-ci
core-strict-ci: core-only-ci core-alias-pkg-lint modules-use-as-strict-lint core-contract-diff core-bench core-fuzz core-gtk-e2e

.PHONY: std-mod-lint
std-mod-lint:
	@python3 tools/lint_std_mod_contracts.py

.PHONY: std-no-internal-exports-lint
std-no-internal-exports-lint:
	@python3 tools/lint_std_no_internal_exports.py

.PHONY: std-graph-lint
std-graph-lint:
	@python3 tools/lint_std_graph.py

.PHONY: std-no-side-effects-lint
std-no-side-effects-lint:
	@python3 tools/lint_std_no_side_effects.py

.PHONY: std-alias-pkg-lint
std-alias-pkg-lint:
	@python3 tools/lint_std_alias_pkg.py

.PHONY: std-sensitive-imports-lint
std-sensitive-imports-lint:
	@python3 tools/lint_std_sensitive_imports.py

.PHONY: std-export-naming-lint
std-export-naming-lint:
	@python3 tools/lint_std_export_naming.py

.PHONY: std-compat-contracts-lint
std-compat-contracts-lint:
	@python3 tools/lint_std_compat_contracts.py

.PHONY: std-contract-snapshots
std-contract-snapshots:
	@python3 tools/generate_contract_snapshots_from_mod.py --packages=std
	@tools/std_contract_snapshots.sh

.PHONY: std-contract-snapshots-update
std-contract-snapshots-update:
	@tools/std_contract_snapshots.sh --update

.PHONY: std-facade-snapshot
std-facade-snapshot:
	@tools/std_facade_snapshot.sh

.PHONY: std-facade-snapshot-update
std-facade-snapshot-update:
	@tools/std_facade_snapshot.sh --update

.PHONY: std-smoke
std-smoke:
	@tools/std_smoke.sh

.PHONY: std-no-side-effects-test
std-no-side-effects-test:
	@SRC=tests/std/vitte_std_no_side_effects.vit tools/std_smoke.sh

.PHONY: std-profile-strict-test
std-profile-strict-test:
	@SRC=tests/std/std_profile_strict.vit tools/std_smoke.sh

.PHONY: std-analyze-json
std-analyze-json:
	@tools/analyze_std_json.sh

.PHONY: std-contract-diff
std-contract-diff:
	@tools/std_contract_diff.sh

.PHONY: std-bench
std-bench:
	@tools/std_bench.sh
	@tools/std_import_latency_bench.sh

.PHONY: std-fuzz
std-fuzz:
	@tools/std_fuzz.sh

.PHONY: std-docsgen
std-docsgen:
	@tools/std_docsgen.sh

.PHONY: std-symbol-index
std-symbol-index:
	@python3 tools/std_symbol_search.py --index --repo .

.PHONY: std-symbol-search
std-symbol-search:
	@python3 tools/std_symbol_search.py --repo . --query "$(Q)"

.PHONY: std-profile-matrix
std-profile-matrix:
	@tools/std_profile_matrix.sh

.PHONY: std-ci-report
std-ci-report:
	@tools/std_ci_report.sh

.PHONY: std-only-ci
std-only-ci: std-mod-lint std-no-internal-exports-lint std-graph-lint std-no-side-effects-lint std-sensitive-imports-lint std-export-naming-lint std-contract-snapshots std-facade-snapshot std-smoke std-no-side-effects-test std-profile-strict-test std-analyze-json std-compat-contracts-lint std-profile-matrix std-ci-report

.PHONY: std-strict-ci
std-strict-ci: std-only-ci std-alias-pkg-lint std-contract-diff std-bench std-fuzz std-docsgen std-symbol-index

.PHONY: log-mod-lint
log-mod-lint:
	@python3 tools/lint_log_mod_contracts.py

.PHONY: log-no-internal-exports-lint
log-no-internal-exports-lint:
	@python3 tools/lint_log_no_internal_exports.py

.PHONY: log-no-side-effects-lint
log-no-side-effects-lint:
	@python3 tools/lint_log_no_side_effects.py

.PHONY: log-alias-pkg-lint
log-alias-pkg-lint:
	@python3 tools/lint_log_alias_pkg.py

.PHONY: log-sensitive-imports-lint
log-sensitive-imports-lint:
	@python3 tools/lint_log_sensitive_imports.py

.PHONY: log-export-naming-lint
log-export-naming-lint:
	@python3 tools/lint_log_export_naming.py

.PHONY: log-compat-contracts-lint
log-compat-contracts-lint:
	@python3 tools/lint_log_compat_contracts.py

.PHONY: log-contract-snapshots
log-contract-snapshots:
	@python3 tools/generate_contract_snapshots_from_mod.py --packages=log
	@tools/log_contract_snapshots.sh

.PHONY: log-contract-snapshots-update
log-contract-snapshots-update:
	@tools/log_contract_snapshots.sh --update

.PHONY: log-facade-snapshot
log-facade-snapshot:
	@tools/log_facade_snapshot.sh

.PHONY: log-facade-snapshot-update
log-facade-snapshot-update:
	@tools/log_facade_snapshot.sh --update

.PHONY: log-smoke
log-smoke:
	@tools/log_smoke.sh

.PHONY: log-no-side-effects-test
log-no-side-effects-test:
	@SRC=tests/log/vitte_log_no_side_effects.vit tools/log_smoke.sh

.PHONY: log-profile-strict-test
log-profile-strict-test:
	@SRC=tests/log/log_profile_strict.vit tools/log_smoke.sh

.PHONY: log-analyze-json
log-analyze-json:
	@tools/analyze_log_json.sh

.PHONY: log-contract-diff
log-contract-diff:
	@tools/log_contract_diff.sh

.PHONY: log-bench
log-bench:
	@tools/log_bench.sh
	@tools/log_macro_bench.sh

.PHONY: log-fuzz
log-fuzz:
	@tools/log_fuzz.sh

.PHONY: log-docsgen
log-docsgen:
	@tools/log_docsgen.sh

.PHONY: log-profile-matrix
log-profile-matrix:
	@tools/log_profile_matrix.sh

.PHONY: log-ci-report
log-ci-report:
	@tools/log_ci_report.sh

.PHONY: log-only-ci
log-only-ci: log-mod-lint log-no-internal-exports-lint log-no-side-effects-lint log-sensitive-imports-lint log-export-naming-lint log-contract-snapshots log-facade-snapshot log-smoke log-no-side-effects-test log-profile-strict-test log-analyze-json log-compat-contracts-lint log-profile-matrix log-ci-report

.PHONY: log-strict-ci
log-strict-ci: log-only-ci log-alias-pkg-lint log-contract-diff log-bench log-fuzz log-docsgen

.PHONY: fs-mod-lint
fs-mod-lint:
	@python3 tools/lint_fs_mod_contracts.py

.PHONY: fs-no-internal-exports-lint
fs-no-internal-exports-lint:
	@python3 tools/lint_fs_no_internal_exports.py

.PHONY: fs-no-side-effects-lint
fs-no-side-effects-lint:
	@python3 tools/lint_fs_no_side_effects.py

.PHONY: fs-alias-pkg-lint
fs-alias-pkg-lint:
	@python3 tools/lint_fs_alias_pkg.py

.PHONY: fs-sensitive-imports-lint
fs-sensitive-imports-lint:
	@python3 tools/lint_fs_sensitive_imports.py

.PHONY: fs-export-naming-lint
fs-export-naming-lint:
	@python3 tools/lint_fs_export_naming.py

.PHONY: fs-compat-contracts-lint
fs-compat-contracts-lint:
	@python3 tools/lint_fs_compat_contracts.py

.PHONY: fs-contract-snapshots
fs-contract-snapshots:
	@python3 tools/generate_contract_snapshots_from_mod.py --packages=fs
	@tools/fs_contract_snapshots.sh

.PHONY: fs-contract-snapshots-update
fs-contract-snapshots-update:
	@tools/fs_contract_snapshots.sh --update

.PHONY: fs-facade-snapshot
fs-facade-snapshot:
	@tools/fs_facade_snapshot.sh

.PHONY: fs-facade-snapshot-update
fs-facade-snapshot-update:
	@tools/fs_facade_snapshot.sh --update

.PHONY: fs-smoke
fs-smoke:
	@tools/fs_smoke.sh

.PHONY: fs-no-side-effects-test
fs-no-side-effects-test:
	@SRC=tests/fs/vitte_fs_no_side_effects.vit tools/fs_smoke.sh

.PHONY: fs-profile-strict-test
fs-profile-strict-test:
	@SRC=tests/fs/fs_profile_strict.vit tools/fs_smoke.sh

.PHONY: fs-analyze-json
fs-analyze-json:
	@tools/analyze_fs_json.sh

.PHONY: fs-contract-diff
fs-contract-diff:
	@tools/fs_contract_diff.sh

.PHONY: fs-bench
fs-bench:
	@tools/fs_bench.sh
	@tools/fs_macro_bench.sh

.PHONY: fs-fuzz
fs-fuzz:
	@tools/fs_fuzz.sh

.PHONY: fs-docsgen
fs-docsgen:
	@tools/fs_docsgen.sh

.PHONY: fs-profile-matrix
fs-profile-matrix:
	@tools/fs_profile_matrix.sh

.PHONY: fs-ci-report
fs-ci-report:
	@tools/fs_ci_report.sh

.PHONY: fs-only-ci
fs-only-ci: fs-mod-lint fs-no-internal-exports-lint fs-no-side-effects-lint fs-sensitive-imports-lint fs-export-naming-lint fs-contract-snapshots fs-facade-snapshot fs-smoke fs-no-side-effects-test fs-profile-strict-test fs-analyze-json fs-compat-contracts-lint fs-profile-matrix fs-ci-report

.PHONY: fs-strict-ci
fs-strict-ci: fs-only-ci fs-alias-pkg-lint fs-contract-diff fs-bench fs-fuzz fs-docsgen

.PHONY: db-mod-lint
db-mod-lint:
	@python3 tools/lint_db_mod_contracts.py

.PHONY: db-no-internal-exports-lint
db-no-internal-exports-lint:
	@python3 tools/lint_db_no_internal_exports.py

.PHONY: db-no-side-effects-lint
db-no-side-effects-lint:
	@python3 tools/lint_db_no_side_effects.py

.PHONY: db-alias-pkg-lint
db-alias-pkg-lint:
	@python3 tools/lint_db_alias_pkg.py

.PHONY: db-sensitive-imports-lint
db-sensitive-imports-lint:
	@python3 tools/lint_db_sensitive_imports.py

.PHONY: db-export-naming-lint
db-export-naming-lint:
	@python3 tools/lint_db_export_naming.py

.PHONY: db-compat-contracts-lint
db-compat-contracts-lint:
	@python3 tools/lint_db_compat_contracts.py

.PHONY: db-sql-injection-lint
db-sql-injection-lint:
	@python3 tools/lint_db_sql_injection.py

.PHONY: db-migration-compat-lint
db-migration-compat-lint:
	@python3 tools/lint_db_migration_compat.py

.PHONY: db-contract-snapshots
db-contract-snapshots:
	@python3 tools/generate_contract_snapshots_from_mod.py --packages=db
	@tools/db_contract_snapshots.sh

.PHONY: db-contract-snapshots-update
db-contract-snapshots-update:
	@tools/db_contract_snapshots.sh --update

.PHONY: db-facade-snapshot
db-facade-snapshot:
	@tools/db_facade_snapshot.sh

.PHONY: db-facade-snapshot-update
db-facade-snapshot-update:
	@tools/db_facade_snapshot.sh --update

.PHONY: db-smoke
db-smoke:
	@tools/db_smoke.sh

.PHONY: db-no-side-effects-test
db-no-side-effects-test:
	@SRC=tests/db/vitte_db_no_side_effects.vit tools/db_smoke.sh

.PHONY: db-profile-strict-test
db-profile-strict-test:
	@SRC=tests/db/db_profile_strict.vit tools/db_smoke.sh

.PHONY: db-analyze-json
db-analyze-json:
	@tools/analyze_db_json.sh

.PHONY: db-contract-diff
db-contract-diff:
	@tools/db_contract_diff.sh

.PHONY: db-bench
db-bench:
	@tools/db_bench.sh
	@tools/db_macro_bench.sh

.PHONY: db-fuzz
db-fuzz:
	@tools/db_fuzz.sh

.PHONY: db-docsgen
db-docsgen:
	@tools/db_docsgen.sh

.PHONY: db-profile-matrix
db-profile-matrix:
	@tools/db_profile_matrix.sh

.PHONY: db-ci-report
db-ci-report:
	@tools/db_ci_report.sh

.PHONY: db-only-ci
db-only-ci: db-mod-lint db-no-internal-exports-lint db-no-side-effects-lint db-sensitive-imports-lint db-export-naming-lint db-sql-injection-lint db-contract-snapshots db-facade-snapshot db-smoke db-no-side-effects-test db-profile-strict-test db-analyze-json db-compat-contracts-lint db-profile-matrix db-ci-report

.PHONY: db-strict-ci
db-strict-ci: db-only-ci db-alias-pkg-lint db-contract-diff db-migration-compat-lint db-bench db-fuzz db-docsgen

.PHONY: http-mod-lint
http-mod-lint:
	@python3 tools/lint_http_mod_contracts.py

.PHONY: http-client-mod-lint
http-client-mod-lint:
	@python3 tools/lint_http_client_mod_contracts.py

.PHONY: http-no-internal-exports-lint
http-no-internal-exports-lint:
	@python3 tools/lint_http_no_internal_exports.py

.PHONY: http-no-side-effects-lint
http-no-side-effects-lint:
	@python3 tools/lint_http_no_side_effects.py

.PHONY: http-alias-pkg-lint
http-alias-pkg-lint:
	@python3 tools/lint_http_alias_pkg.py

.PHONY: http-sensitive-imports-lint
http-sensitive-imports-lint:
	@python3 tools/lint_http_sensitive_imports.py

.PHONY: http-export-naming-lint
http-export-naming-lint:
	@python3 tools/lint_http_export_naming.py

.PHONY: http-compat-contracts-lint
http-compat-contracts-lint:
	@python3 tools/lint_http_compat_contracts.py

.PHONY: http-security-lint
http-security-lint:
	@python3 tools/lint_http_security.py

.PHONY: http-contract-snapshots
http-contract-snapshots:
	@python3 tools/generate_contract_snapshots_from_mod.py --packages=http
	@tools/http_contract_snapshots.sh

.PHONY: http-contract-snapshots-update
http-contract-snapshots-update:
	@tools/http_contract_snapshots.sh --update

.PHONY: http-facade-snapshot
http-facade-snapshot:
	@tools/http_facade_snapshot.sh

.PHONY: http-facade-snapshot-update
http-facade-snapshot-update:
	@tools/http_facade_snapshot.sh --update

.PHONY: http-client-contract-snapshots
http-client-contract-snapshots:
	@tools/http_client_contract_snapshots.sh

.PHONY: http-client-contract-snapshots-update
http-client-contract-snapshots-update:
	@tools/http_client_contract_snapshots.sh --update

.PHONY: http-client-facade-snapshot
http-client-facade-snapshot:
	@tools/http_client_facade_snapshot.sh

.PHONY: http-client-facade-snapshot-update
http-client-facade-snapshot-update:
	@tools/http_client_facade_snapshot.sh --update

.PHONY: http-smoke
http-smoke:
	@tools/http_smoke.sh

.PHONY: http-client-smoke
http-client-smoke:
	@tools/http_client_smoke.sh

.PHONY: http-no-side-effects-test
http-no-side-effects-test:
	@SRC=tests/http/vitte_http_no_side_effects.vit tools/http_smoke.sh

.PHONY: http-client-no-side-effects-test
http-client-no-side-effects-test:
	@SRC=tests/http_client/vitte_http_client_no_side_effects.vit tools/http_client_smoke.sh

.PHONY: http-profile-strict-test
http-profile-strict-test:
	@SRC=tests/http/http_profile_strict.vit tools/http_smoke.sh

.PHONY: http-client-profile-strict-test
http-client-profile-strict-test:
	@SRC=tests/http_client/http_client_profile_strict.vit tools/http_client_smoke.sh

.PHONY: http-analyze-json
http-analyze-json:
	@tools/analyze_http_json.sh

.PHONY: http-client-analyze-json
http-client-analyze-json:
	@tools/analyze_http_client_json.sh

.PHONY: http-contract-diff
http-contract-diff:
	@tools/http_contract_diff.sh

.PHONY: http-bench
http-bench:
	@tools/http_bench.sh
	@tools/http_macro_bench.sh

.PHONY: http-client-bench
http-client-bench:
	@tools/http_client_bench.sh
	@tools/http_client_macro_bench.sh

.PHONY: http-fuzz
http-fuzz:
	@tools/http_fuzz.sh

.PHONY: http-client-fuzz
http-client-fuzz:
	@tools/http_client_fuzz.sh

.PHONY: http-docsgen
http-docsgen:
	@tools/http_docsgen.sh

.PHONY: http-client-docsgen
http-client-docsgen:
	@tools/http_client_docsgen.sh

.PHONY: http-profile-matrix
http-profile-matrix:
	@tools/http_profile_matrix.sh

.PHONY: http-client-profile-matrix
http-client-profile-matrix:
	@tools/http_client_profile_matrix.sh

.PHONY: http-ci-report
http-ci-report:
	@tools/http_ci_report.sh

.PHONY: http-client-ci-report
http-client-ci-report:
	@tools/http_client_ci_report.sh

.PHONY: http-only-ci
http-only-ci: http-mod-lint http-no-internal-exports-lint http-no-side-effects-lint http-sensitive-imports-lint http-export-naming-lint http-security-lint http-contract-snapshots http-facade-snapshot http-smoke http-no-side-effects-test http-profile-strict-test http-analyze-json http-compat-contracts-lint http-profile-matrix http-ci-report

.PHONY: http-strict-ci
http-strict-ci: http-only-ci http-alias-pkg-lint http-contract-diff http-bench http-fuzz http-docsgen

.PHONY: http-client-only-ci
http-client-only-ci: http-client-mod-lint http-no-internal-exports-lint http-no-side-effects-lint http-sensitive-imports-lint http-export-naming-lint http-security-lint http-client-contract-snapshots http-client-facade-snapshot http-client-smoke http-client-no-side-effects-test http-client-profile-strict-test http-client-analyze-json http-compat-contracts-lint http-client-profile-matrix http-client-ci-report

.PHONY: http-client-strict-ci
http-client-strict-ci: http-client-only-ci http-alias-pkg-lint http-contract-diff http-client-bench http-client-fuzz http-client-docsgen

.PHONY: process-mod-lint
process-mod-lint:
	@python3 tools/lint_process_mod_contracts.py

.PHONY: process-no-internal-exports-lint
process-no-internal-exports-lint:
	@python3 tools/lint_process_no_internal_exports.py

.PHONY: process-no-side-effects-lint
process-no-side-effects-lint:
	@python3 tools/lint_process_no_side_effects.py

.PHONY: process-alias-pkg-lint
process-alias-pkg-lint:
	@python3 tools/lint_process_alias_pkg.py

.PHONY: process-sensitive-imports-lint
process-sensitive-imports-lint:
	@python3 tools/lint_process_sensitive_imports.py

.PHONY: process-export-naming-lint
process-export-naming-lint:
	@python3 tools/lint_process_export_naming.py

.PHONY: process-security-lint
process-security-lint:
	@python3 tools/lint_process_security.py

.PHONY: process-compat-contracts-lint
process-compat-contracts-lint:
	@python3 tools/lint_process_compat_contracts.py

.PHONY: process-contract-snapshots
process-contract-snapshots:
	@python3 tools/generate_contract_snapshots_from_mod.py --packages=process
	@tools/process_contract_snapshots.sh

.PHONY: process-contract-snapshots-update
process-contract-snapshots-update:
	@tools/process_contract_snapshots.sh --update

.PHONY: process-facade-snapshot
process-facade-snapshot:
	@tools/process_facade_snapshot.sh

.PHONY: process-facade-snapshot-update
process-facade-snapshot-update:
	@tools/process_facade_snapshot.sh --update

.PHONY: process-smoke
process-smoke:
	@tools/process_smoke.sh

.PHONY: process-no-side-effects-test
process-no-side-effects-test:
	@SRC=tests/process/vitte_process_no_side_effects.vit tools/process_smoke.sh

.PHONY: process-profile-strict-test
process-profile-strict-test:
	@SRC=tests/process/process_profile_strict.vit tools/process_smoke.sh

.PHONY: process-analyze-json
process-analyze-json:
	@tools/analyze_process_json.sh

.PHONY: process-contract-diff
process-contract-diff:
	@tools/process_contract_diff.sh

.PHONY: process-bench
process-bench:
	@tools/process_bench.sh
	@tools/process_macro_bench.sh

.PHONY: process-fuzz
process-fuzz:
	@tools/process_fuzz.sh

.PHONY: process-docsgen
process-docsgen:
	@tools/process_docsgen.sh

.PHONY: process-profile-matrix
process-profile-matrix:
	@tools/process_profile_matrix.sh

.PHONY: process-ci-report
process-ci-report:
	@tools/process_ci_report.sh

.PHONY: process-only-ci
process-only-ci: process-mod-lint process-no-internal-exports-lint process-no-side-effects-lint process-sensitive-imports-lint process-export-naming-lint process-security-lint process-contract-snapshots process-facade-snapshot process-smoke process-no-side-effects-test process-profile-strict-test process-analyze-json process-compat-contracts-lint process-profile-matrix process-ci-report

.PHONY: process-strict-ci
process-strict-ci: process-only-ci process-alias-pkg-lint process-contract-diff process-bench process-fuzz process-docsgen

.PHONY: json-mod-lint
json-mod-lint:
	@python3 tools/lint_json_mod_contracts.py

.PHONY: json-no-internal-exports-lint
json-no-internal-exports-lint:
	@python3 tools/lint_json_no_internal_exports.py

.PHONY: json-no-side-effects-lint
json-no-side-effects-lint:
	@python3 tools/lint_json_no_side_effects.py

.PHONY: json-alias-pkg-lint
json-alias-pkg-lint:
	@python3 tools/lint_json_alias_pkg.py

.PHONY: json-sensitive-imports-lint
json-sensitive-imports-lint:
	@python3 tools/lint_json_sensitive_imports.py

.PHONY: json-export-naming-lint
json-export-naming-lint:
	@python3 tools/lint_json_export_naming.py

.PHONY: json-compat-contracts-lint
json-compat-contracts-lint:
	@python3 tools/lint_json_compat_contracts.py

.PHONY: json-security-lint
json-security-lint:
	@python3 tools/lint_json_security.py

.PHONY: json-contract-snapshots
json-contract-snapshots:
	@python3 tools/generate_contract_snapshots_from_mod.py --packages=json
	@tools/json_contract_snapshots.sh

.PHONY: json-contract-snapshots-update
json-contract-snapshots-update:
	@tools/json_contract_snapshots.sh --update

.PHONY: json-facade-snapshot
json-facade-snapshot:
	@tools/json_facade_snapshot.sh

.PHONY: json-facade-snapshot-update
json-facade-snapshot-update:
	@tools/json_facade_snapshot.sh --update

.PHONY: json-smoke
json-smoke:
	@tools/json_smoke.sh

.PHONY: json-no-side-effects-test
json-no-side-effects-test:
	@SRC=tests/json/vitte_json_no_side_effects.vit tools/json_smoke.sh

.PHONY: json-profile-strict-test
json-profile-strict-test:
	@SRC=tests/json/json_profile_strict.vit tools/json_smoke.sh

.PHONY: json-analyze-json
json-analyze-json:
	@tools/analyze_json_json.sh

.PHONY: json-contract-diff
json-contract-diff:
	@tools/json_contract_diff.sh

.PHONY: json-bench
json-bench:
	@tools/json_bench.sh
	@tools/json_macro_bench.sh

.PHONY: json-fuzz
json-fuzz:
	@tools/json_fuzz.sh

.PHONY: json-docsgen
json-docsgen:
	@tools/json_docsgen.sh

.PHONY: json-profile-matrix
json-profile-matrix:
	@tools/json_profile_matrix.sh

.PHONY: json-ci-report
json-ci-report:
	@tools/json_ci_report.sh

.PHONY: json-only-ci
json-only-ci: json-mod-lint json-no-internal-exports-lint json-no-side-effects-lint json-sensitive-imports-lint json-export-naming-lint json-security-lint json-contract-snapshots json-facade-snapshot json-smoke json-no-side-effects-test json-profile-strict-test json-analyze-json json-compat-contracts-lint json-profile-matrix json-ci-report

.PHONY: json-strict-ci
json-strict-ci: json-only-ci json-alias-pkg-lint json-contract-diff json-bench json-fuzz json-docsgen

.PHONY: yaml-mod-lint
yaml-mod-lint:
	@python3 tools/lint_yaml_mod_contracts.py

.PHONY: yaml-no-internal-exports-lint
yaml-no-internal-exports-lint:
	@python3 tools/lint_yaml_no_internal_exports.py

.PHONY: yaml-no-side-effects-lint
yaml-no-side-effects-lint:
	@python3 tools/lint_yaml_no_side_effects.py

.PHONY: yaml-alias-pkg-lint
yaml-alias-pkg-lint:
	@python3 tools/lint_yaml_alias_pkg.py

.PHONY: yaml-sensitive-imports-lint
yaml-sensitive-imports-lint:
	@python3 tools/lint_yaml_sensitive_imports.py

.PHONY: yaml-export-naming-lint
yaml-export-naming-lint:
	@python3 tools/lint_yaml_export_naming.py

.PHONY: yaml-compat-contracts-lint
yaml-compat-contracts-lint:
	@python3 tools/lint_yaml_compat_contracts.py

.PHONY: yaml-security-lint
yaml-security-lint:
	@python3 tools/lint_yaml_security.py

.PHONY: yaml-contract-snapshots
yaml-contract-snapshots:
	@python3 tools/generate_contract_snapshots_from_mod.py --packages=yaml
	@tools/yaml_contract_snapshots.sh

.PHONY: yaml-contract-snapshots-update
yaml-contract-snapshots-update:
	@tools/yaml_contract_snapshots.sh --update

.PHONY: yaml-facade-snapshot
yaml-facade-snapshot:
	@tools/yaml_facade_snapshot.sh

.PHONY: yaml-facade-snapshot-update
yaml-facade-snapshot-update:
	@tools/yaml_facade_snapshot.sh --update

.PHONY: yaml-smoke
yaml-smoke:
	@tools/yaml_smoke.sh

.PHONY: yaml-no-side-effects-test
yaml-no-side-effects-test:
	@SRC=tests/yaml/vitte_yaml_no_side_effects.vit tools/yaml_smoke.sh

.PHONY: yaml-profile-strict-test
yaml-profile-strict-test:
	@SRC=tests/yaml/yaml_profile_strict.vit tools/yaml_smoke.sh

.PHONY: yaml-analyze-json
yaml-analyze-json:
	@tools/analyze_yaml_json.sh

.PHONY: yaml-contract-diff
yaml-contract-diff:
	@tools/yaml_contract_diff.sh

.PHONY: yaml-bench
yaml-bench:
	@tools/yaml_bench.sh
	@tools/yaml_macro_bench.sh

.PHONY: yaml-fuzz
yaml-fuzz:
	@tools/yaml_fuzz.sh

.PHONY: yaml-docsgen
yaml-docsgen:
	@tools/yaml_docsgen.sh

.PHONY: yaml-profile-matrix
yaml-profile-matrix:
	@tools/yaml_profile_matrix.sh

.PHONY: yaml-ci-report
yaml-ci-report:
	@tools/yaml_ci_report.sh

.PHONY: yaml-only-ci
yaml-only-ci: yaml-mod-lint yaml-no-internal-exports-lint yaml-no-side-effects-lint yaml-sensitive-imports-lint yaml-export-naming-lint yaml-security-lint yaml-contract-snapshots yaml-facade-snapshot yaml-smoke yaml-no-side-effects-test yaml-profile-strict-test yaml-analyze-json yaml-compat-contracts-lint yaml-profile-matrix yaml-ci-report

.PHONY: yaml-strict-ci
yaml-strict-ci: yaml-only-ci yaml-alias-pkg-lint yaml-contract-diff yaml-bench yaml-fuzz yaml-docsgen

.PHONY: test-mod-lint
test-mod-lint:
	@python3 tools/lint_test_mod_contracts.py

.PHONY: test-no-internal-exports-lint
test-no-internal-exports-lint:
	@python3 tools/lint_test_no_internal_exports.py

.PHONY: test-no-side-effects-lint
test-no-side-effects-lint:
	@python3 tools/lint_test_no_side_effects.py

.PHONY: test-alias-pkg-lint
test-alias-pkg-lint:
	@python3 tools/lint_test_alias_pkg.py

.PHONY: test-sensitive-imports-lint
test-sensitive-imports-lint:
	@python3 tools/lint_test_sensitive_imports.py

.PHONY: test-export-naming-lint
test-export-naming-lint:
	@python3 tools/lint_test_export_naming.py

.PHONY: test-compat-contracts-lint
test-compat-contracts-lint:
	@python3 tools/lint_test_compat_contracts.py

.PHONY: test-security-lint
test-security-lint:
	@python3 tools/lint_test_security.py

.PHONY: test-contract-snapshots
test-contract-snapshots:
	@python3 tools/generate_contract_snapshots_from_mod.py --packages=test
	@tools/test_contract_snapshots.sh

.PHONY: test-contract-snapshots-update
test-contract-snapshots-update:
	@tools/test_contract_snapshots.sh --update

.PHONY: test-facade-snapshot
test-facade-snapshot:
	@tools/test_facade_snapshot.sh

.PHONY: test-facade-snapshot-update
test-facade-snapshot-update:
	@tools/test_facade_snapshot.sh --update

.PHONY: test-smoke
test-smoke:
	@tools/test_smoke.sh

.PHONY: test-no-side-effects-test
test-no-side-effects-test:
	@SRC=tests/test/vitte_test_no_side_effects.vit tools/test_smoke.sh

.PHONY: test-profile-strict-test
test-profile-strict-test:
	@SRC=tests/test/test_profile_strict.vit tools/test_smoke.sh

.PHONY: test-analyze-json
test-analyze-json:
	@tools/analyze_test_json.sh

.PHONY: test-contract-diff
test-contract-diff:
	@tools/test_contract_diff.sh

.PHONY: test-bench
test-bench:
	@tools/test_bench.sh
	@tools/test_macro_bench.sh

.PHONY: test-fuzz
test-fuzz:
	@tools/test_fuzz.sh

.PHONY: test-docsgen
test-docsgen:
	@tools/test_docsgen.sh

.PHONY: test-profile-matrix
test-profile-matrix:
	@tools/test_profile_matrix.sh

.PHONY: test-ci-report
test-ci-report:
	@tools/test_ci_report.sh

.PHONY: test-only-ci
test-only-ci: test-mod-lint test-no-internal-exports-lint test-no-side-effects-lint test-sensitive-imports-lint test-export-naming-lint test-security-lint test-contract-snapshots test-facade-snapshot test-smoke test-no-side-effects-test test-profile-strict-test test-analyze-json test-compat-contracts-lint test-profile-matrix test-ci-report

.PHONY: test-strict-ci
test-strict-ci: test-only-ci test-alias-pkg-lint test-contract-diff test-bench test-fuzz test-docsgen

.PHONY: lint-mod-lint
lint-mod-lint:
	@python3 tools/lint_lint_mod_contracts.py

.PHONY: lint-no-internal-exports-lint
lint-no-internal-exports-lint:
	@python3 tools/lint_lint_no_internal_exports.py

.PHONY: lint-no-side-effects-lint
lint-no-side-effects-lint:
	@python3 tools/lint_lint_no_side_effects.py

.PHONY: lint-alias-pkg-lint
lint-alias-pkg-lint:
	@python3 tools/lint_lint_alias_pkg.py

.PHONY: lint-sensitive-imports-lint
lint-sensitive-imports-lint:
	@python3 tools/lint_lint_sensitive_imports.py

.PHONY: lint-export-naming-lint
lint-export-naming-lint:
	@python3 tools/lint_lint_export_naming.py

.PHONY: lint-compat-contracts-lint
lint-compat-contracts-lint:
	@python3 tools/lint_lint_compat_contracts.py

.PHONY: lint-security-lint
lint-security-lint:
	@python3 tools/lint_lint_security.py

.PHONY: lint-contract-snapshots
lint-contract-snapshots:
	@python3 tools/generate_contract_snapshots_from_mod.py --packages=lint
	@tools/lint_contract_snapshots_pkg.sh

.PHONY: lint-contract-snapshots-update
lint-contract-snapshots-update:
	@tools/lint_contract_snapshots_pkg.sh --update

.PHONY: lint-facade-snapshot
lint-facade-snapshot:
	@tools/lint_facade_snapshot_pkg.sh

.PHONY: lint-facade-snapshot-update
lint-facade-snapshot-update:
	@tools/lint_facade_snapshot_pkg.sh --update

.PHONY: lint-smoke
lint-smoke:
	@tools/lint_smoke_pkg.sh

.PHONY: lint-no-side-effects-test
lint-no-side-effects-test:
	@SRC=tests/lint/vitte_lint_no_side_effects.vit tools/lint_smoke_pkg.sh

.PHONY: lint-profile-strict-test
lint-profile-strict-test:
	@SRC=tests/lint/lint_profile_strict.vit tools/lint_smoke_pkg.sh

.PHONY: lint-analyze-json
lint-analyze-json:
	@tools/analyze_lint_json.sh

.PHONY: lint-contract-diff
lint-contract-diff:
	@tools/lint_contract_diff_pkg.sh

.PHONY: lint-bench
lint-bench:
	@tools/lint_bench_pkg.sh
	@tools/lint_macro_bench_pkg.sh

.PHONY: lint-fuzz
lint-fuzz:
	@tools/lint_fuzz_pkg.sh

.PHONY: lint-docsgen
lint-docsgen:
	@tools/lint_docsgen_pkg.sh

.PHONY: lint-profile-matrix
lint-profile-matrix:
	@tools/lint_profile_matrix_pkg.sh

.PHONY: lint-ci-report
lint-ci-report:
	@tools/lint_ci_report_pkg.sh

.PHONY: lint-only-ci
lint-only-ci: lint-mod-lint lint-no-internal-exports-lint lint-no-side-effects-lint lint-sensitive-imports-lint lint-export-naming-lint lint-security-lint lint-contract-snapshots lint-facade-snapshot lint-smoke lint-no-side-effects-test lint-profile-strict-test lint-analyze-json lint-compat-contracts-lint lint-profile-matrix lint-ci-report

.PHONY: lint-strict-ci
lint-strict-ci: lint-only-ci lint-alias-pkg-lint lint-contract-diff lint-bench lint-fuzz lint-docsgen

.PHONY: contracts-refresh
contracts-refresh:
	@python3 tools/generate_contract_snapshots_from_mod.py --packages=core,std,log,fs,db,http,process,json,yaml,test,lint
	@python3 tools/generate_contract_lockfiles.py

.PHONY: contract-lockfiles-lint
contract-lockfiles-lint:
	@python3 tools/lint_contract_lockfiles.py

.PHONY: facade-role-contracts-lint
facade-role-contracts-lint:
	@python3 tools/lint_facade_role_contracts.py

.PHONY: facade-thin-lint
facade-thin-lint:
	@python3 tools/lint_facade_thin.py

.PHONY: diag-namespace-lint
diag-namespace-lint:
	@python3 tools/lint_diagnostic_namespace_ownership.py

.PHONY: alias-pkg-global-lint
alias-pkg-global-lint:
	@python3 tools/lint_alias_pkg_generic.py --roots=src/vitte/packages/core,src/vitte/packages/std,src/vitte/packages/log,src/vitte/packages/fs,src/vitte/packages/db,src/vitte/packages/http,src/vitte/packages/http_client,src/vitte/packages/process,src/vitte/packages/json,src/vitte/packages/yaml,src/vitte/packages/test,src/vitte/packages/lint --tag=alias-pkg-global

.PHONY: no-side-effects-global-lint
no-side-effects-global-lint:
	@python3 tools/lint_no_side_effects_generic.py --roots=src/vitte/packages/core,src/vitte/packages/std,src/vitte/packages/log,src/vitte/packages/fs,src/vitte/packages/db,src/vitte/packages/http,src/vitte/packages/http_client,src/vitte/packages/process,src/vitte/packages/json,src/vitte/packages/yaml,src/vitte/packages/test,src/vitte/packages/lint --tag=no-side-effects-global

.PHONY: analyze-json-unify
analyze-json-unify:
	@python3 tools/normalize_analyze_report.py --in target/reports/core_analyze.json --out target/reports/core_analyze.json --package=vitte/core --diag-prefix=VITTE-C
	@python3 tools/normalize_analyze_report.py --in target/reports/std_analyze.json --out target/reports/std_analyze.json --package=vitte/std --diag-prefix=VITTE-S
	@python3 tools/normalize_analyze_report.py --in target/reports/log_analyze.json --out target/reports/log_analyze.json --package=vitte/log --diag-prefix=VITTE-L
	@python3 tools/normalize_analyze_report.py --in target/reports/fs_analyze.json --out target/reports/fs_analyze.json --package=vitte/fs --diag-prefix=VITTE-F
	@python3 tools/normalize_analyze_report.py --in target/reports/db_analyze.json --out target/reports/db_analyze.json --package=vitte/db --diag-prefix=VITTE-D
	@python3 tools/normalize_analyze_report.py --in target/reports/http_analyze.json --out target/reports/http_analyze.json --package=vitte/http --diag-prefix=VITTE-H
	@python3 tools/normalize_analyze_report.py --in target/reports/http_client_analyze.json --out target/reports/http_client_analyze.json --package=vitte/http_client --diag-prefix=VITTE-C
	@python3 tools/normalize_analyze_report.py --in target/reports/process_analyze.json --out target/reports/process_analyze.json --package=vitte/process --diag-prefix=VITTE-P
	@python3 tools/normalize_analyze_report.py --in target/reports/json_analyze.json --out target/reports/json_analyze.json --package=vitte/json --diag-prefix=VITTE-J
	@python3 tools/normalize_analyze_report.py --in target/reports/yaml_analyze.json --out target/reports/yaml_analyze.json --package=vitte/yaml --diag-prefix=VITTE-Y
	@python3 tools/normalize_analyze_report.py --in target/reports/test_analyze.json --out target/reports/test_analyze.json --package=vitte/test --diag-prefix=VITTE-T
	@python3 tools/normalize_analyze_report.py --in target/reports/lint_analyze.json --out target/reports/lint_analyze.json --package=vitte/lint --diag-prefix=VITTE-I

.PHONY: diagnostics-index
diagnostics-index:
	@python3 tools/build_diagnostics_index.py

.PHONY: reports-index
reports-index:
	@python3 tools/reports_index.py

.PHONY: contracts-dashboard
contracts-dashboard:
	@python3 tools/contracts_dashboard.py

.PHONY: quickfix-schema-snapshots
quickfix-schema-snapshots:
	@python3 tools/quickfix_schema_snapshots.py

.PHONY: diagnostics-stability-lint
diagnostics-stability-lint:
	@python3 tools/lint_diagnostics_stability.py

.PHONY: lint-rule-ownership-lint
lint-rule-ownership-lint:
	@python3 tools/lint_lint_rule_ownership.py

.PHONY: docs-sync-gate
docs-sync-gate:
	@python3 tools/docs_sync_gate.py

.PHONY: security-gates-report
security-gates-report:
	@tools/security_gates_report.sh

.PHONY: security-hardening-gate
security-hardening-gate:
	@python3 tools/security_hardening_gate.py --min-score=$${SECURITY_HARDENING_MIN:-75}

.PHONY: security-baseline-diff
security-baseline-diff: security-gates-report
	@python3 tools/security_baseline_diff.py

.PHONY: security-baseline-update
security-baseline-update: security-gates-report
	@python3 tools/security_baseline_diff.py --update

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

.PHONY: api-nonregression-doctors
api-nonregression-doctors:
	@python3 tools/api_nonregression_doctors.py

.PHONY: api-diff-explainer
api-diff-explainer:
	@python3 tools/api_diff_explainer.py

.PHONY: breaking-removal-table
breaking-removal-table:
	@python3 tools/test_breaking_removal_table.py

.PHONY: deprecated-wrappers-budget-lint
deprecated-wrappers-budget-lint:
	@python3 tools/lint_deprecated_wrappers_budget.py

.PHONY: cross-package-smoke
cross-package-smoke:
	@tools/cross_package_smoke.sh

.PHONY: doctors
doctors:
	@tools/doctors_dashboard.sh

.PHONY: symbol-index
symbol-index:
	@python3 tools/build_symbol_index.py

.PHONY: perf-regression
perf-regression:
	@python3 tools/perf_regression_check.py --threshold-pct=15

.PHONY: perf-regression-robust
perf-regression-robust:
	@profile="$${PERF_MACHINE_PROFILE:-}"; \
	if [ -z "$$profile" ]; then \
	  if [ -n "$${CI:-}" ]; then profile=ci; else profile=dev; fi; \
	fi; \
	echo "[perf-regression-robust] profile=$$profile"; \
	python3 tools/perf_regression_robust.py --profile="$$profile" --threshold-pct=20 --variance-cap-pct=75

.PHONY: perf-budget
perf-budget:
	@python3 tools/perf_budget_check.py

.PHONY: analyze-schema-snapshot
analyze-schema-snapshot: analyze-json-unify
	@python3 tools/analyze_schema_snapshot.py

.PHONY: analyze-schema-snapshot-update
analyze-schema-snapshot-update: analyze-json-unify
	@python3 tools/analyze_schema_snapshot.py --update

.PHONY: network-process-profile-matrix
network-process-profile-matrix:
	@tools/network_process_profile_matrix.sh

.PHONY: semantic-contract-tests
semantic-contract-tests:
	@tools/semantic_contract_tests.sh

.PHONY: semantic-golden-cross-package
semantic-golden-cross-package:
	@python3 tools/semantic_golden_cross_package.py

.PHONY: gtk-quickfix-e2e
gtk-quickfix-e2e:
	@tools/gtk_quickfix_e2e.sh

.PHONY: gtk-workflow-snapshots
gtk-workflow-snapshots:
	@tools/gtk_workflow_snapshots.sh

.PHONY: generate-highlights
generate-highlights:
	@python3 tools/generate_editor_highlights.py

.PHONY: highlight-snapshot-vim
highlight-snapshot-vim: generate-highlights
	@tools/highlight_snapshot_vim.sh

.PHONY: highlight-snapshot-emacs
highlight-snapshot-emacs: generate-highlights
	@tools/highlight_snapshot_emacs.sh

.PHONY: highlight-snapshot-nano
highlight-snapshot-nano: generate-highlights
	@tools/highlight_snapshot_nano.sh

.PHONY: highlight-snapshots-update
highlight-snapshots-update: generate-highlights
	@tools/highlight_snapshot_vim.sh --update
	@tools/highlight_snapshot_emacs.sh --update
	@tools/highlight_snapshot_nano.sh --update

.PHONY: highlights-coverage
highlights-coverage: generate-highlights
	@python3 tools/highlights_coverage.py

.PHONY: tree-sitter-vitte-validate
tree-sitter-vitte-validate:
	@python3 tools/tree_sitter_vitte_validate.py

.PHONY: tree-sitter-vitte-smoke
tree-sitter-vitte-smoke:
	@tools/tree_sitter_vitte_smoke.sh

.PHONY: tree-sitter-vitte-ci
tree-sitter-vitte-ci: tree-sitter-vitte-validate tree-sitter-vitte-smoke
	@echo "[tree-sitter-vitte-ci] OK"

.PHONY: highlights-ci
highlights-ci: highlight-snapshot-vim highlight-snapshot-emacs highlight-snapshot-nano highlights-coverage tree-sitter-vitte-validate
	@echo "[highlights-ci] OK"

.PHONY: fuzz-corpus-manage
fuzz-corpus-manage:
	@python3 tools/fuzz_corpus_manager.py --write-corpus --minimize

.PHONY: symbol-index-daemon-once
symbol-index-daemon-once:
	@python3 tools/symbol_index_daemon.py run-once

.PHONY: debian-packaging-hardening
debian-packaging-hardening:
	@tools/debian_packaging_hardening.sh

.PHONY: repro-report
repro-report:
	@python3 tools/repro_report.py

.PHONY: deprecation-lifecycle-enforcer
deprecation-lifecycle-enforcer:
	@python3 tools/deprecation_lifecycle_enforcer.py --current-version=$${VITTE_VERSION:-3.0.0}

.PHONY: pr-risk-budget
pr-risk-budget:
	@python3 tools/pr_risk_budget.py

.PHONY: release-doctor
release-doctor:
	@python3 tools/release_doctor.py

.PHONY: packages-only-ci
packages-only-ci: contracts-refresh contract-lockfiles-lint facade-role-contracts-lint facade-thin-lint diag-namespace-lint alias-pkg-global-lint no-side-effects-global-lint core-only-ci std-only-ci log-only-ci fs-only-ci db-only-ci http-only-ci http-client-only-ci process-only-ci json-only-ci yaml-only-ci test-only-ci lint-only-ci diagnostics-index contracts-dashboard reports-index security-gates-report cross-package-smoke symbol-index analyze-json-unify analyze-schema-snapshot quickfix-schema-snapshots diagnostics-stability-lint api-nonregression-doctors api-diff-explainer lint-rule-ownership-lint breaking-removal-table

.PHONY: packages-strict-ci
packages-strict-ci: packages-only-ci core-strict-ci std-strict-ci log-strict-ci fs-strict-ci db-strict-ci http-strict-ci http-client-strict-ci process-strict-ci json-strict-ci yaml-strict-ci test-strict-ci lint-strict-ci semantic-contract-tests semantic-golden-cross-package highlights-ci deprecated-wrappers-budget-lint deprecation-lifecycle-enforcer plugin-abi-compat plugin-manifest-lint plugin-sandbox-lint plugin-binary-abi fuzz-corpus-manage symbol-index-daemon-once perf-regression-robust perf-budget security-hardening-gate security-baseline-diff docs-sync-gate gtk-quickfix-e2e gtk-workflow-snapshots

.PHONY: packages-changed-ci
packages-changed-ci:
	@tools/changed_packages_ci.sh

.PHONY: packages-impacted-strict
packages-impacted-strict:
	@targets="$$(python3 tools/packages_impacted_strict.py --base-ref=$${BASE_REF:-HEAD~1})"; \
	echo "[packages-impacted-strict] targets: $$targets"; \
	make -s $$targets

.PHONY: package-ci-cached
package-ci-cached:
	@tools/pkg_ci_cached.sh "$(PKG)" "$${TARGET:-$(PKG)-only-ci}"

.PHONY: release-readiness
release-readiness: packages-strict-ci doctors release-doctor contracts-dashboard reports-index security-gates-report security-hardening-gate security-baseline-diff perf-regression-robust perf-budget plugin-abi-compat plugin-manifest-lint plugin-sandbox-lint plugin-binary-abi debian-packaging-hardening repro-report analyze-json-unify analyze-schema-snapshot api-diff-explainer docs-sync-gate pr-risk-budget
	@echo "[release-readiness] OK"

.PHONY: new-public-packages-snapshots-lint
new-public-packages-snapshots-lint:
	@tools/lint_new_public_packages_have_snapshots.py

.PHONY: no-std-lint
no-std-lint:
	@tools/lint_no_std_imports.py --roots src/vitte/packages

.PHONY: modules-report
modules-report:
	@tools/modules_report.sh

.PHONY: packages-report
packages-report:
	@SEARCH_ROOT=src/vitte/packages ENTRY_GLOB=mod.vit OUT_FILE=target/reports/packages_modules_report.txt OUT_JSON=target/reports/packages_modules_report.json tools/modules_report.sh

.PHONY: modules-perf-cache
modules-perf-cache:
	@tools/modules_cache_perf.sh tests/modules/mod_graph/main.vit

.PHONY: packages-contract-snapshots
packages-contract-snapshots:
	@tools/packages_contract_snapshots.sh

.PHONY: packages-contract-snapshots-update
packages-contract-snapshots-update:
	@tools/packages_contract_snapshots.sh --update

.PHONY: packages-gate
packages-gate: package-layout-lint-strict packages-governance-lint no-std-lint module-naming-lint legacy-import-path-lint critical-runtime-matrix-lint new-public-packages-snapshots-lint modules-perf-cache packages-report packages-contract-snapshots

.PHONY: modules-ci-strict
modules-ci-strict: modules-tests modules-snapshots modules-contract-snapshots array-export-snapshot-lint ast-export-snapshot-lint module-tree-lint module-naming-lint critical-module-contract-lint experimental-modules-lint public-modules-snapshots-lint modules-perf-cache legacy-import-path-lint migration-check modules-report
	@$(BIN_DIR)/$(PROJECT) mod contract-diff --lang=en --old tests/modules/api_diff/old_case/main.vit --new tests/modules/api_diff/new_case/main.vit >/tmp/vitte-modules-ci-contract-diff.out 2>&1 || true
	@grep -Fq "[contract-diff] BREAKING" /tmp/vitte-modules-ci-contract-diff.out
	@$(BIN_DIR)/$(PROJECT) mod contract-diff --lang=en --old tests/modules/api_diff/old_case/main.vit --new tests/modules/api_diff/old_case/main.vit >/tmp/vitte-modules-ci-contract-diff-ok.out 2>&1
	@grep -Fq "[contract-diff] OK" /tmp/vitte-modules-ci-contract-diff-ok.out
	@rm -f /tmp/vitte-modules-ci-contract-diff.out /tmp/vitte-modules-ci-contract-diff-ok.out

.PHONY: completions-gen
completions-gen:
	@python3 tools/generate_completions.py

.PHONY: completions-check
completions-check:
	@python3 tools/generate_completions.py --check

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

.PHONY: crash-report-snapshots
crash-report-snapshots:
	@tools/crash_report_snapshots.sh

.PHONY: ci-std-fast
ci-std-fast: std-check extern-abi-host stdlib-api-lint stdlib-profile-snapshots diag-snapshots completions-snapshots wrapper-stage-test

.PHONY: ci-mod-fast
ci-mod-fast: grammar-check diag-snapshots completions-snapshots stdlib-profile-snapshots stdlib-abi-compat modules-tests modules-snapshots same-output-hash

.PHONY: ci-bridge-compat
ci-bridge-compat: ci-mod-fast

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

PKG_VERSION ?= 2.1.1

.PHONY: pkg-debian
pkg-debian: release-readiness
	@VERSION=$(PKG_VERSION) toolchain/scripts/package/make-debian-deb.sh

.PHONY: pkg-debian-install
pkg-debian-install: pkg-debian
	@sudo dpkg -i pkgout/vitte_$(PKG_VERSION)_$$(dpkg --print-architecture).deb

.PHONY: pkg-macos
pkg-macos:
	@VERSION=$(PKG_VERSION) toolchain/scripts/package/make-macos-pkg.sh

.PHONY: pkg-macos-uninstall
pkg-macos-uninstall:
	@VERSION=$(PKG_VERSION) toolchain/scripts/package/make-macos-uninstall-pkg.sh

.PHONY: release-check
release-check: build ci-fast package-layout-lint-strict legacy-import-allowlist-empty ci-completions pkg-macos

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

.PHONY: alloc-ci-short
alloc-ci-short:
	@MODE=short tools/ci_alloc_matrix.sh

.PHONY: alloc-ci-nightly
alloc-ci-nightly:
	@MODE=nightly tools/ci_alloc_matrix.sh

.PHONY: modules-weekly-deny-legacy
modules-weekly-deny-legacy:
	@DENY_LEGACY_SELF_LEAF=1 ALERTS_FUZZ_NIGHTLY=1 ALLOC_FUZZ_NIGHTLY=1 tools/modules_tests.sh

.PHONY: modules-weekly-legacy-warn-only
modules-weekly-legacy-warn-only:
	@LEGACY_SELF_LEAF_WARN_ONLY=1 tools/modules_tests.sh

.PHONY: release-modules-gate
release-modules-gate: modules-ci-strict modules-contract-snapshots modules-report legacy-import-allowlist-empty

.PHONY: packages-changed-ci
packages-changed-ci:
	@tools/changed_packages_ci.sh

.PHONY: packages-impacted-strict
packages-impacted-strict:
	@targets="$$(python3 tools/packages_impacted_strict.py --base-ref=$${BASE_REF:-HEAD~1})"; \
	echo "[packages-impacted-strict] targets: $$targets"; \
	make -s $$targets

.PHONY: platon-editor
platon-editor:
	@./bin/vitte build platon-editor/editor_core.vit -o platon-editor/editor_core
	@platon-editor/editor_core

.PHONY: vitte-ide
vitte-ide: dirs
	$(CXX) $(CXXFLAGS) apps/vitte_ide_cpp/vitte_ide.cpp -o $(BIN_DIR)/vitte-ide -lncurses -pthread

.PHONY: vitte-ide-plugin-analyzer
vitte-ide-plugin-analyzer: plugins/vitte_analyzer_pack.so

plugins/vitte_analyzer_pack.so: plugins/vitte_analyzer_pack.cpp apps/vitte_ide_gtk/plugin_api.hpp
	$(CXX) $(CXXFLAGS) -fPIC -shared plugins/vitte_analyzer_pack.cpp -o plugins/vitte_analyzer_pack.so

.PHONY: vitte-ide-gtk
vitte-ide-gtk: dirs vitte-ide-plugin-analyzer
	$(CXX) $(CXXFLAGS) apps/vitte_ide_gtk/vitte_ide_gtk.cpp -o $(BIN_DIR)/vitte-ide-gtk $(shell pkg-config --cflags --libs gtk+-3.0) -pthread -ldl

.PHONY: tui-ide-snapshots
tui-ide-snapshots:
	@tools/tui_ide_snapshots.sh

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
	$(RM) .cache .clangd

# ------------------------------------------------------------
# Help
# ------------------------------------------------------------

.PHONY: help
help:
	@echo ""
	@echo "Vitte Makefile targets:"
	@echo ""
	@echo "  make            build everything"
	@echo "  make install    build + install binary + Vim/Emacs/Nano syntax files"
	@echo "  make install-debian-2.1.1 install vitte on Debian/Ubuntu (deps + build + install via installer profile 2.1.1)"
	@echo "  make install-bin install only the vitte binary (PREFIX=$(PREFIX))"
	@echo "  make install-editors install syntax configs for Vim/Emacs/Nano in HOME"
	@echo "  make format     run clang-format"
	@echo "  make tidy       run clang-tidy"
	@echo "  make test       run tests (std/test)"
	@echo "  make test-examples build/check all examples/*.vit"
	@echo "  make extern-abi validate #[extern] ABI (host profile)"
	@echo "  make extern-abi-arduino validate #[extern] ABI (arduino)"
	@echo "  make extern-abi-kernel validate #[extern] ABI (kernel grub)"
	@echo "  make extern-abi-kernel-uefi validate #[extern] ABI (kernel uefi)"
	@echo "  make extern-abi-all validate #[extern] ABI (all std vs host)"
	@echo "  make std-core-tests run std/core regression tests"
	@echo "  make stdlib-api-lint check stable stdlib ABI surface entries"
	@echo "  make stdlib-profile-snapshots check stdlib profile allow/deny matrix"
	@echo "  make stdlib-abi-compat block ABI removals from v1 to v2"
	@echo "  make ci-std-fast std-focused CI (stdlib + snapshots + wrappers)"
	@echo "  make ci-bridge-compat alias of ci-mod-fast for native liaison compatibility"
	@echo "  make modules-tests run module graph/doctor fixtures"
	@echo "  make module-shape-policy enforce single module layout (<name>.vit xor <name>/mod.vit)"
	@echo "  make modules-snapshots assert mod graph/doctor outputs"
	@echo "  make modules-snapshots-update regenerate modules snapshot files (.must/.diagjson/.codes/.fr)"
	@echo "  make modules-snapshots-bless regenerate modules snapshots and print diffs"
	@echo "  make core-mod-lint enforce vitte/core facade invariants and no side-effects"
	@echo "  make core-contract-snapshots validate tests/modules/contracts/core snapshots"
	@echo "  make core-contract-snapshots-update refresh core contracts snapshot hash"
	@echo "  make core-facade-snapshot validate sorted core facade API snapshot + hash"
	@echo "  make core-facade-snapshot-update refresh core facade API snapshot + hash"
	@echo "  make core-smoke run vitte/core smoke check fixture"
	@echo "  make core-no-side-effects-test ensure import-only core check fixture passes"
	@echo "  make core-profile-strict-test run strict core profile fixture"
	@echo "  make core-analyze-json emit target/reports/core_analyze.json"
	@echo "  make core-contract-diff compare core facade baseline vs current exports"
	@echo "  make core-bench run micro benchmark harness for core helpers"
	@echo "  make core-fuzz run placeholder fuzz harness with versioned seeds"
	@echo "  make core-gtk-e2e generate GTK core quickfix interaction snapshot"
	@echo "  make core-only-ci run core-focused lint + snapshots + smoke + analyze-json"
	@echo "  make core-strict-ci run core-only-ci + strict alias/import lint + bench/fuzz"
	@echo "  make std-mod-lint enforce vitte/std facade invariants and diagnostics namespace"
	@echo "  make std-no-side-effects-lint enforce no 'entry' across src/vitte/packages/std"
	@echo "  make std-contract-snapshots validate tests/modules/contracts/std snapshots"
	@echo "  make std-contract-snapshots-update refresh std contracts snapshot hash"
	@echo "  make std-facade-snapshot validate sorted std facade API snapshot + hash"
	@echo "  make std-facade-snapshot-update refresh std facade API snapshot + hash"
	@echo "  make std-smoke run vitte/std smoke check fixture"
	@echo "  make std-no-side-effects-test ensure import-only std check fixture passes"
	@echo "  make std-profile-strict-test run strict std profile fixture"
	@echo "  make std-analyze-json emit target/reports/std_analyze.json"
	@echo "  make std-contract-diff compare std facade baseline vs current exports"
	@echo "  make std-bench run std micro + import latency benchmarks"
	@echo "  make std-fuzz run std fuzz seed harness"
	@echo "  make std-docsgen generate docs/std API markdown index"
	@echo "  make std-symbol-index generate persistent std symbol index"
	@echo "  make std-symbol-search Q='sym:foo kind:proc module:std/base' query std symbol index"
	@echo "  make std-only-ci run std-focused lint + snapshots + smoke + analyze-json"
	@echo "  make std-strict-ci run std-only-ci + strict alias lint + contract diff + bench/fuzz/docs"
	@echo "  make log-mod-lint enforce vitte/log facade invariants and diagnostics namespace"
	@echo "  make log-no-side-effects-lint enforce no 'entry' across src/vitte/packages/log"
	@echo "  make log-contract-snapshots validate tests/modules/contracts/log snapshots"
	@echo "  make log-facade-snapshot validate sorted log facade API snapshot + hash"
	@echo "  make log-smoke run vitte/log smoke check fixture"
	@echo "  make log-no-side-effects-test ensure import-only log check fixture passes"
	@echo "  make log-profile-strict-test run strict log profile fixture"
	@echo "  make log-analyze-json emit target/reports/log_analyze.json"
	@echo "  make log-contract-diff compare log facade baseline vs current exports"
	@echo "  make log-bench run log micro + macro benchmarks"
	@echo "  make log-fuzz run log fuzz seed harness"
	@echo "  make log-docsgen generate docs/log API markdown index"
	@echo "  make log-only-ci run log-focused lint + snapshots + smoke + analyze-json"
	@echo "  make log-strict-ci run log-only-ci + strict alias lint + contract diff + bench/fuzz/docs"
	@echo "  make fs-mod-lint enforce vitte/fs facade invariants and diagnostics namespace"
	@echo "  make fs-no-side-effects-lint enforce no 'entry' across src/vitte/packages/fs"
	@echo "  make fs-contract-snapshots validate tests/modules/contracts/fs snapshots"
	@echo "  make fs-facade-snapshot validate sorted fs facade API snapshot + hash"
	@echo "  make fs-smoke run vitte/fs smoke check fixture"
	@echo "  make fs-no-side-effects-test ensure import-only fs check fixture passes"
	@echo "  make fs-profile-strict-test run strict fs profile fixture"
	@echo "  make fs-analyze-json emit target/reports/fs_analyze.json"
	@echo "  make fs-contract-diff compare fs facade baseline vs current exports"
	@echo "  make fs-bench run fs micro + macro benchmarks"
	@echo "  make fs-fuzz run fs fuzz seed harness"
	@echo "  make fs-docsgen generate docs/fs API markdown index"
	@echo "  make fs-only-ci run fs-focused lint + snapshots + smoke + analyze-json"
	@echo "  make fs-strict-ci run fs-only-ci + strict alias lint + contract diff + bench/fuzz/docs"
	@echo "  make db-mod-lint enforce vitte/db facade invariants and diagnostics namespace"
	@echo "  make db-no-side-effects-lint enforce no 'entry' across src/vitte/packages/db"
	@echo "  make db-contract-snapshots validate tests/modules/contracts/db snapshots"
	@echo "  make db-facade-snapshot validate sorted db facade API snapshot + hash"
	@echo "  make db-smoke run vitte/db smoke check fixture"
	@echo "  make db-no-side-effects-test ensure import-only db check fixture passes"
	@echo "  make db-profile-strict-test run strict db profile fixture"
	@echo "  make db-analyze-json emit target/reports/db_analyze.json"
	@echo "  make db-contract-diff compare db facade baseline vs current exports"
	@echo "  make db-bench run db micro + macro benchmarks"
	@echo "  make db-fuzz run db fuzz seed harness"
	@echo "  make db-docsgen generate docs/db API markdown index"
	@echo "  make db-only-ci run db-focused lint + snapshots + smoke + analyze-json"
	@echo "  make db-strict-ci run db-only-ci + strict alias lint + migration-compat + bench/fuzz/docs"
	@echo "  make http-mod-lint enforce vitte/http facade invariants and diagnostics namespace"
	@echo "  make http-client-mod-lint enforce vitte/http_client facade invariants and diagnostics namespace"
	@echo "  make http-contract-snapshots validate tests/modules/contracts/http snapshots"
	@echo "  make http-facade-snapshot validate sorted http facade API snapshot + hash"
	@echo "  make http-smoke run vitte/http smoke check fixture"
	@echo "  make http-no-side-effects-test ensure import-only http check fixture passes"
	@echo "  make http-profile-strict-test run strict http profile fixture"
	@echo "  make http-analyze-json emit target/reports/http_analyze.json"
	@echo "  make http-contract-diff compare http facade baseline vs current exports"
	@echo "  make http-bench run http micro + macro benchmarks"
	@echo "  make http-fuzz run http fuzz seed harness"
	@echo "  make http-docsgen generate docs/http API markdown index"
	@echo "  make http-only-ci run http-focused lint + snapshots + smoke + analyze-json"
	@echo "  make http-strict-ci run http-only-ci + strict alias lint + contract diff + bench/fuzz/docs"
	@echo "  make json-mod-lint enforce vitte/json facade invariants and diagnostics namespace"
	@echo "  make json-contract-snapshots validate tests/modules/contracts/json snapshots"
	@echo "  make json-facade-snapshot validate sorted json facade API snapshot + hash"
	@echo "  make json-smoke run vitte/json smoke check fixture"
	@echo "  make json-no-side-effects-test ensure import-only json check fixture passes"
	@echo "  make json-profile-strict-test run strict json profile fixture"
	@echo "  make json-analyze-json emit target/reports/json_analyze.json"
	@echo "  make json-contract-diff compare json facade baseline vs current exports"
	@echo "  make json-bench run json micro + macro benchmarks"
	@echo "  make json-fuzz run json fuzz seed harness"
	@echo "  make json-docsgen generate docs/json API markdown index"
	@echo "  make json-only-ci run json-focused lint + snapshots + smoke + analyze-json"
	@echo "  make json-strict-ci run json-only-ci + strict alias lint + contract diff + bench/fuzz/docs"
	@echo "  make yaml-mod-lint enforce vitte/yaml facade invariants and diagnostics namespace"
	@echo "  make yaml-contract-snapshots validate tests/modules/contracts/yaml snapshots"
	@echo "  make yaml-facade-snapshot validate sorted yaml facade API snapshot + hash"
	@echo "  make yaml-smoke run vitte/yaml smoke check fixture"
	@echo "  make yaml-no-side-effects-test ensure import-only yaml check fixture passes"
	@echo "  make yaml-profile-strict-test run strict yaml profile fixture"
	@echo "  make yaml-analyze-json emit target/reports/yaml_analyze.json"
	@echo "  make yaml-contract-diff compare yaml facade baseline vs current exports"
	@echo "  make yaml-bench run yaml micro + macro benchmarks"
	@echo "  make yaml-fuzz run yaml fuzz seed harness"
	@echo "  make yaml-docsgen generate docs/yaml API markdown index"
	@echo "  make yaml-only-ci run yaml-focused lint + snapshots + smoke + analyze-json"
	@echo "  make yaml-strict-ci run yaml-only-ci + strict alias lint + contract diff + bench/fuzz/docs"
	@echo "  make test-mod-lint enforce vitte/test facade invariants and diagnostics namespace"
	@echo "  make test-contract-snapshots validate tests/modules/contracts/test snapshots"
	@echo "  make test-facade-snapshot validate sorted test facade API snapshot + hash"
	@echo "  make test-smoke run vitte/test smoke check fixture"
	@echo "  make test-no-side-effects-test ensure import-only test check fixture passes"
	@echo "  make test-profile-strict-test run strict test profile fixture"
	@echo "  make test-analyze-json emit target/reports/test_analyze.json"
	@echo "  make test-contract-diff compare test facade baseline vs current exports"
	@echo "  make test-bench run test micro + macro benchmarks"
	@echo "  make test-fuzz run test fuzz seed harness"
	@echo "  make test-docsgen generate docs/test API markdown index"
	@echo "  make test-only-ci run test-focused lint + snapshots + smoke + analyze-json"
	@echo "  make test-strict-ci run test-only-ci + strict alias lint + contract diff + bench/fuzz/docs"
	@echo "  make lint-mod-lint enforce vitte/lint facade invariants and diagnostics namespace"
	@echo "  make lint-contract-snapshots validate tests/modules/contracts/lint snapshots"
	@echo "  make lint-facade-snapshot validate sorted lint facade API snapshot + hash"
	@echo "  make lint-smoke run vitte/lint smoke check fixture"
	@echo "  make lint-no-side-effects-test ensure import-only lint check fixture passes"
	@echo "  make lint-profile-strict-test run strict lint profile fixture"
	@echo "  make lint-analyze-json emit target/reports/lint_analyze.json"
	@echo "  make lint-contract-diff compare lint facade baseline vs current exports"
	@echo "  make lint-bench run lint micro + macro benchmarks"
	@echo "  make lint-fuzz run lint fuzz seed harness"
	@echo "  make lint-docsgen generate docs/lint API markdown index"
	@echo "  make lint-only-ci run lint-focused lint + snapshots + smoke + analyze-json"
	@echo "  make lint-strict-ci run lint-only-ci + strict alias lint + contract diff + bench/fuzz/docs"
	@echo "  make http-client-contract-snapshots validate tests/modules/contracts/http_client snapshots"
	@echo "  make http-client-facade-snapshot validate sorted http_client facade API snapshot + hash"
	@echo "  make http-client-smoke run vitte/http_client smoke check fixture"
	@echo "  make http-client-no-side-effects-test ensure import-only http_client check fixture passes"
	@echo "  make http-client-profile-strict-test run strict http_client profile fixture"
	@echo "  make http-client-analyze-json emit target/reports/http_client_analyze.json"
	@echo "  make http-client-bench run http_client micro + macro benchmarks"
	@echo "  make http-client-fuzz run http_client fuzz seed harness"
	@echo "  make http-client-docsgen generate docs/http_client API markdown index"
	@echo "  make http-client-only-ci run http_client-focused lint + snapshots + smoke + analyze-json"
	@echo "  make http-client-strict-ci run http-client-only-ci + strict alias lint + contract diff + bench/fuzz/docs"
	@echo "  make process-mod-lint enforce vitte/process facade invariants and diagnostics namespace"
	@echo "  make process-contract-snapshots validate tests/modules/contracts/process snapshots"
	@echo "  make process-facade-snapshot validate sorted process facade API snapshot + hash"
	@echo "  make process-smoke run vitte/process smoke check fixture"
	@echo "  make process-no-side-effects-test ensure import-only process check fixture passes"
	@echo "  make process-profile-strict-test run strict process profile fixture"
	@echo "  make process-analyze-json emit target/reports/process_analyze.json"
	@echo "  make process-contract-diff compare process facade baseline vs current exports"
	@echo "  make process-bench run process micro + macro benchmarks"
	@echo "  make process-fuzz run process fuzz seed harness"
	@echo "  make process-docsgen generate docs/process API markdown index"
	@echo "  make process-only-ci run process-focused lint + snapshots + smoke + analyze-json"
	@echo "  make process-strict-ci run process-only-ci + strict alias lint + contract diff + bench/fuzz/docs"
	@echo "  make contracts-refresh regenerate contracts snapshots/hash/lockfiles from mod.vit"
	@echo "  make contract-lockfiles-lint validate generated contract lockfiles consistency"
	@echo "  make facade-role-contracts-lint enforce unified ROLE-CONTRACT template across key facades"
	@echo "  make facade-thin-lint enforce lightweight facades (mod.vit)"
	@echo "  make diag-namespace-lint enforce diagnostic namespace ownership per package"
	@echo "  make alias-pkg-global-lint run unified alias *_pkg lint engine"
	@echo "  make no-side-effects-global-lint run unified no-side-effects lint engine"
	@echo "  make diagnostics-index build central diagnostics index JSON/Markdown"
	@echo "  make contracts-dashboard build markdown dashboard for contracts/hash/status"
	@echo "  make reports-index build unified target/reports/index.json for IDE/pipeline"
	@echo "  make security-gates-report build consolidated multi-package security gates report"
	@echo "  make security-hardening-gate enforce SSRF/TLS/CRLF/process/fs hardening minimum score"
	@echo "  make security-baseline-diff compare security gates report against versioned baseline"
	@echo "  make security-baseline-update refresh versioned security baseline from current report"
	@echo "  make analyze-json-unify normalize analyze reports to stable schema v1.0"
	@echo "  make analyze-schema-snapshot verify global analyze schema snapshot"
	@echo "  make analyze-schema-snapshot-update update global analyze schema snapshot"
	@echo "  make api-nonregression-doctors enforce doctor/quickfix facade API presence"
	@echo "  make network-process-profile-matrix emit core/desktop/system matrix for http/http_client/process"
	@echo "  make cross-package-smoke run HTTP->process->log->fs smoke scenario"
	@echo "  make doctors run all API doctors and emit dashboard report"
	@echo "  make symbol-index build persistent symbol index for IDE/compiler sharing"
	@echo "  make symbol-index-daemon-once build shared symbol-index shards/manifest"
	@echo "  make generate-highlights regenerate vim/emacs/nano/tree-sitter highlight assets from grammar"
	@echo "  make highlight-snapshot-vim verify vim highlight snapshot"
	@echo "  make highlight-snapshot-emacs verify emacs highlight snapshot"
	@echo "  make highlight-snapshot-nano verify nano highlight snapshot"
	@echo "  make highlight-snapshots-update refresh editor highlight snapshots"
	@echo "  make highlights-coverage write target/reports/highlights_coverage.json and enforce strict constructs"
	@echo "  make highlights-ci run highlight snapshots + coverage gate (vim+emacs strict required)"
	@echo "  make tree-sitter-vitte-validate static-validate VITTE tree-sitter grammar/query/corpus presence"
	@echo "  make tree-sitter-vitte-smoke run tree-sitter generate/test when CLI is available"
	@echo "  make tree-sitter-vitte-ci run validate + smoke for VITTE tree-sitter parser"
	@echo "  make semantic-contract-tests run behavior-oriented contract checks (not just exports)"
	@echo "  make semantic-golden-cross-package run golden behavior check HTTP->Process->Log->FS->DB"
	@echo "  make fuzz-corpus-manage run centralized fuzz corpus dedup/minimize/triage report"
	@echo "  make perf-regression compare bench outputs with baseline threshold"
	@echo "  make perf-regression-robust run median/warmup/variance-aware perf gate"
	@echo "  make perf-budget enforce per-package p95 budgets from tools/perf_budget.json"
	@echo "  make plugin-abi-compat validate plugin command ABI surface"
	@echo "  make plugin-manifest-lint validate plugins/plugin.toml command schema declarations"
	@echo "  make plugin-sandbox-lint validate per-command plugin permission policy"
	@echo "  make plugin-binary-abi compile+dlopen plugin shared library and validate ABI"
	@echo "  make deprecation-lifecycle-enforcer fail on expired deprecated wrappers by version"
	@echo "  make debian-packaging-hardening validate desktop/metainfo/assets packaging readiness"
	@echo "  make repro-report generate build provenance/reproducibility report JSON"
	@echo "  make pr-risk-budget compute PR risk budget (breaking/perf/security/docs)"
	@echo "  make release-doctor aggregate contracts/perf/security/docs/ABI gates with failing reports"
	@echo "  make gtk-workflow-snapshots emit GTK workflow snapshots marker file"
	@echo "  make gtk-quickfix-e2e validate GTK quickfix flow markers (Problems->preview->apply->recheck)"
	@echo "  make breaking-removal-table run table-driven breaking removal detection across key packages"
	@echo "  make deprecated-wrappers-budget-lint enforce deprecated wrapper removal budget policy"
	@echo "  make packages-only-ci run unified fast package gate for core/std/log/fs/db/http/http_client/process/json/yaml/test/lint"
	@echo "  make packages-strict-ci run full strict gate including perf/plugin/deprecation checks"
	@echo "  make packages-changed-ci run only impacted package CI targets (BASE_REF=HEAD~1 by default)"
	@echo "  make release-readiness run contracts+security+perf+ABI+schema aggregate gate"
	@echo "  make explain-snapshots assert vitte explain outputs"
	@echo "  make same-output-hash verify deterministic emit hash stability"
	@echo "  make completions-gen regenerate bash/zsh/fish completions from unified spec"
	@echo "  make completions-check verify generated completions are up to date"
	@echo "  make completions-snapshots run completion snapshot assertions"
	@echo "  make completions-snapshots-update update completion golden snapshots"
	@echo "  make completions-lint syntax-check bash/zsh/fish completion files"
	@echo "  make ci-completions run completion check + lint + snapshots + fallback"
	@echo "  make pkg-debian build Debian .deb installer (PKG_VERSION=$(PKG_VERSION))"
	@echo "  make pkg-debian-install build and install Debian .deb locally via dpkg"
	@echo "  make pkg-macos build macOS installer pkg (PKG_VERSION=$(PKG_VERSION))"
	@echo "  make pkg-macos-uninstall build macOS uninstall pkg (PKG_VERSION=$(PKG_VERSION))"
	@echo "  make release-check run build + ci-fast + ci-completions + pkg build"
	@echo "  make packages-changed-ci run package CI only for changed packages (BASE_REF=HEAD~1 by default)"
	@echo "  make packages-impacted-strict run strict CI on changed packages + reverse dependency closure"
	@echo "  make ci-mod-fast module-focused CI (grammar + snapshots + module tests)"
	@echo "  make ci-fast-compiler compiler-focused CI with cache skip (grammar + resolve + module snapshots + explain + runtime matrix)"
	@echo "  make vittec-kernel build target/kernel-tools/vittec-kernel (no curl runtime)"
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
	@echo "  make vitteos-status regenerate vitteos-status.md from checks/snapshots"
	@echo "  make vitteos-new-module MODULE=vitteos/<path> generate module template files"
	@echo "  make vitteos-quick run quick local loop (issues + domain + orphan + space + arch-contract + header + targeted + smoke)"
	@echo "  make vitteos-ci run VitteOS CI chain (soft scripts + bin gate + issues + domain + orphan + space + arch-contract + header + targeted + smoke + adr)"
	@echo "  make vitteos-ci-strict run strict VitteOS CI chain (strict scripts + bin gate + issues + domain + orphan + space + arch-contract + header + targeted + smoke + adr)"
	@echo "  make vitteos-ci-local run local VitteOS CI chain without external /bin migration dataset"
	@echo "  make vitteos-ci-min run minimal fast checks for pre-commit local loop"
	@echo "  make platon-editor build and self-test platon editor core"
	@echo "  make vitte-ide build C++ TUI IDE for Vitte projects (auto-check/build/debug/suggestions)"
	@echo "  make vitte-ide-gtk build C++ GTK IDE (Geany-like GUI with tabs/tree/problems)"
	@echo "  make std-check  verify std layout"
	@echo "  make clean      remove build artifacts"
	@echo "  make distclean  full cleanup"
	@echo ""
