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

.PHONY: install install-bin install-editors install-geany uninstall-geany install-debian-2.1.1
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
	@MODE=$${TEST_EXAMPLES_MODE:-check} tools/build_examples_matrix.sh

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

.PHONY: diagnostics-locales-lint
diagnostics-locales-lint:
	@python3 tools/check_diagnostics_locales.py

.PHONY: core-semantic-snapshots
core-semantic-snapshots:
	@MANIFEST=tests/diag_snapshots/core_semantic_manifest.txt tools/diag_snapshots.sh

.PHONY: core-semantic-success
core-semantic-success:
	@MANIFEST=tests/core_semantic_success_manifest.txt tools/check_manifest.sh

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
	@python3 book/grammar/scripts/sync_grammar.py

.PHONY: harden-mod-vits
harden-mod-vits:
	@test -n "$(PACKAGES)" || (echo "usage: make harden-mod-vits PACKAGES='catalog data std/io'" >&2; exit 2)
	@python3 tools/harden_existing_mod_vit.py --write $(PACKAGES)

.PHONY: grammar-check
grammar-check:
	@python3 book/grammar/scripts/sync_grammar.py --check

.PHONY: grammar-test
grammar-test:
	@python3 book/grammar/scripts/validate_examples.py

.PHONY: core-language-test
core-language-test:
	@python3 book/grammar/scripts/validate_examples.py --manifest tests/grammar/core_manifest.txt

.PHONY: core-language-test-update
core-language-test-update:
	@python3 book/grammar/scripts/validate_examples.py --manifest tests/grammar/core_manifest.txt --update-snapshots

.PHONY: grammar-test-update
grammar-test-update:
	@python3 book/grammar/scripts/validate_examples.py --update-snapshots

.PHONY: grammar-docs
grammar-docs:
	@python3 book/grammar/scripts/build_railroad.py

.PHONY: grammar-docs-check
grammar-docs-check:
	@python3 book/grammar/scripts/build_railroad.py --check

.PHONY: grammar-gate
grammar-gate: grammar-check grammar-test grammar-docs-check

.PHONY: core-language-gate
core-language-gate: grammar-check core-language-test core-semantic-success core-semantic-snapshots diagnostics-locales-lint

.PHONY: core-release-gate
core-release-gate: core-language-gate diagnostics-ftl-check

.PHONY: book-qa
book-qa:
	@python3 book/scripts/qa_book.py

.PHONY: book-qa-strict
book-qa-strict:
	@python3 book/scripts/qa_book.py --strict

.PHONY: keywords-normalize
keywords-normalize:
	@python3 book/keywords/scripts/normalize_keywords.py

.PHONY: keywords-lint
keywords-lint:
	@python3 book/keywords/scripts/lint_keywords.py

.PHONY: update-diagnostics-ftl
update-diagnostics-ftl:
	@tools/update_diagnostics_ftl.py

.PHONY: diagnostics-ftl-check
diagnostics-ftl-check:
	@tools/update_diagnostics_ftl.py --check

.PHONY: ci-strict
ci-strict: core-language-gate package-layout-lint legacy-import-path-lint negative-tests diag-snapshots geany-lint highlight-snapshots

.PHONY: ci-fast
ci-fast: core-language-gate package-layout-lint legacy-import-path-lint negative-tests diag-snapshots completions-snapshots wrapper-stage-test geany-lint

.PHONY: ci-completions
ci-completions: completions-check completions-lint completions-snapshots completions-fallback

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
	@grep -q "Vitte=\\*\\.vit;" editors/geany/filetype_extensions.conf

.PHONY: geany-install-check
geany-install-check:
	@tools/check_geany_install.sh

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

.PHONY: packages-report
packages-report:
	@SEARCH_ROOT=src/vitte/packages ENTRY_GLOB=mod.vit OUT_FILE=target/reports/packages_modules_report.txt OUT_JSON=target/reports/packages_modules_report.json DEPENDENCY_OVERLAP_ALLOWLIST=tools/package_dependency_export_overlap_allowlist.txt tools/modules_report.sh

.PHONY: packages-dependency-overlap-lint
packages-dependency-overlap-lint:
	@SEARCH_ROOT=src/vitte/packages ENTRY_GLOB=mod.vit OUT_FILE=target/reports/packages_modules_report.txt OUT_JSON=target/reports/packages_modules_report.json DEPENDENCY_OVERLAP_ALLOWLIST=tools/package_dependency_export_overlap_allowlist.txt FAIL_ON_DEPENDENCY_OVERLAP=1 tools/modules_report.sh

.PHONY: package-check
package-check:
	@test -n "$(SRC)" || (echo "usage: make package-check SRC=src/vitte/packages/<pkg>/mod.vit" >&2; exit 2)
	@bin/vitte check --lang=en --allow-internal --resolve-only "$(SRC)"

.PHONY: modules-perf-cache
modules-perf-cache:
	@tools/modules_cache_perf.sh tests/modules/mod_doctor/main.vit

.PHONY: packages-contract-snapshots
packages-contract-snapshots:
	@tools/packages_contract_snapshots.sh

.PHONY: packages-contract-snapshots-update
packages-contract-snapshots-update:
	@tools/packages_contract_snapshots.sh --update

.PHONY: packages-gate
packages-gate: package-layout-lint-strict packages-governance-lint no-std-lint module-naming-lint legacy-import-path-lint critical-runtime-matrix-lint new-public-packages-snapshots-lint modules-perf-cache packages-dependency-overlap-lint packages-contract-snapshots

.PHONY: modules-ci-strict
modules-ci-strict: modules-tests modules-snapshots modules-contract-snapshots module-tree-lint module-naming-lint critical-module-contract-lint experimental-modules-lint public-modules-snapshots-lint modules-perf-cache legacy-import-path-lint migration-check modules-report
	@$(BIN_DIR)/$(PROJECT) mod contract-diff --lang=en --old tests/modules/api_diff/old_case/main.vit --new tests/modules/api_diff/new_case/main.vit >/tmp/vitte-modules-ci-contract-diff.out 2>&1 || true
	@grep -Fq "[contract-diff] BREAKING" /tmp/vitte-modules-ci-contract-diff.out
	@$(BIN_DIR)/$(PROJECT) mod contract-diff --lang=en --old tests/modules/api_diff/old_case/main.vit --new tests/modules/api_diff/old_case/main.vit >/tmp/vitte-modules-ci-contract-diff-ok.out 2>&1
	@grep -Fq "[contract-diff] OK" /tmp/vitte-modules-ci-contract-diff-ok.out
	@rm -f /tmp/vitte-modules-ci-contract-diff.out /tmp/vitte-modules-ci-contract-diff-ok.out

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

PKG_VERSION ?= 2.1.1

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
pkg-macos:
	@VERSION=$(PKG_VERSION) toolchain/scripts/package/make-macos-pkg.sh

.PHONY: pkg-macos-uninstall
pkg-macos-uninstall:
	@VERSION=$(PKG_VERSION) toolchain/scripts/package/make-macos-uninstall-pkg.sh

.PHONY: release-check
release-check: build core-release-gate ci-fast package-layout-lint-strict legacy-import-allowlist-empty ci-completions pkg-macos

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
	@echo "  make install    build + install binary + Vim/Emacs/Nano/Geany syntax files"
	@echo "  make install-debian-2.1.1 install vitte on Debian/Ubuntu (deps + build + install via installer profile 2.1.1)"
	@echo "  make install-bin install only the vitte binary (PREFIX=$(PREFIX))"
	@echo "  make install-editors install syntax configs for Vim/Emacs/Nano/Geany in HOME (override USER_HOME=..., PREFIX=...)"
	@echo "  make install-geany install Geany Vitte config only (VITTE_GEANY_WD_MODE=file|project|current)"
	@echo "  make uninstall-geany remove Geany Vitte config from user profile"
	@echo "  make format     run clang-format"
	@echo "  make tidy       run clang-tidy"
	@echo "  make test       run tests (std/test)"
	@echo "  make grammar-sync regenerate grammar surface artifacts from src/vitte/grammar/vitte.ebnf"
	@echo "  make grammar-check fail if grammar generated artifacts are out of sync"
	@echo "  make grammar-test validate grammar corpus + diagnostics snapshots"
	@echo "  make core-language-test validate the focused core language corpus"
	@echo "  make core-semantic-success validate focused passing core semantic examples"
	@echo "  make core-semantic-snapshots validate focused core semantic diagnostics"
	@echo "  make diagnostics-locales-lint validate locale files against centralized diagnostics"
	@echo "  make update-diagnostics-ftl synchronize diagnostics locales from the central table"
	@echo "  make diagnostics-ftl-check fail if diagnostics locales are out of sync"
	@echo "  make grammar-docs regenerate railroad SVG diagrams"
	@echo "  make grammar-gate run grammar-check + grammar-test"
	@echo "  make core-language-gate run grammar-check + core-language-test + core semantic gates + diagnostics locales lint"
	@echo "  make core-release-gate run the protected language contract gate for release-facing work"
	@echo "  make keywords-normalize apply strict keyword template on book/keywords/*.md"
	@echo "  make keywords-lint validate keyword quality sections/diagnostics/links/score"
	@echo "  make test-examples build/check all examples/*.vit"
	@echo "  make extern-abi validate #[extern] ABI (host profile)"
	@echo "  make extern-abi-arduino validate #[extern] ABI (arduino)"
	@echo "  make extern-abi-kernel validate #[extern] ABI (kernel grub)"
	@echo "  make extern-abi-kernel-uefi validate #[extern] ABI (kernel uefi)"
	@echo "  make extern-abi-all validate #[extern] ABI (all std vs host)"
	@echo "  make quasi-empty-package-tests run checks for newly hardened quasi-empty package modules"
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
	@echo "  make explain-snapshots assert vitte explain outputs"
	@echo "  make same-output-hash verify deterministic emit hash stability"
	@echo "  make completions-gen regenerate bash/zsh/fish completions from unified spec"
	@echo "  make completions-check verify generated completions are up to date"
	@echo "  make completions-snapshots run completion snapshot assertions"
	@echo "  make completions-snapshots-update update completion golden snapshots"
	@echo "  make completions-lint syntax-check bash/zsh/fish completion files"
	@echo "  make ci-completions run completion check + lint + snapshots + fallback"
	@echo "  make ci-mod-fast module-focused CI (grammar + snapshots + module tests)"
	@echo "  make ci-fast-compiler compiler-focused CI with cache skip (grammar + resolve + module snapshots + explain + runtime matrix)"
	@echo "  make vittec-kernel build target/kernel-tools/vittec-kernel (no curl runtime)"
	@echo "  make vitteos-bin-quality run /bin quality checks + matrix report"
	@echo "  make vitteos-bin-runnable-check assert bin/vitte is host-runnable (non-regression arch/format guard)"
	@echo "  make vitteos-bin-runtime run runtime-smoke probes and update runtime column"
	@echo "  make pkg-debian build Debian .deb installer (PACKAGE_PROFILE=full, PKG_VERSION=$(PKG_VERSION))"
	@echo "  make pkg-debian-min build Debian .deb installer (PACKAGE_PROFILE=minimal, PKG_VERSION=$(PKG_VERSION))"
	@echo "  make pkg-debian-audit audit generated Debian .deb content and largest files"
	@echo "  make pkg-debian-install build and install Debian .deb locally via dpkg"
	@echo "  make pkg-macos build macOS installer pkg (PKG_VERSION=$(PKG_VERSION))"
	@echo "  make pkg-macos-uninstall build macOS uninstall pkg (PKG_VERSION=$(PKG_VERSION))"
	@echo "  make release-check run build + core-release-gate + ci-fast + ci-completions + pkg build"
	@echo "  make reports-index build target/reports/index.json (unified reports registry)"
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
