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
CXX          := clang++
AR           := ar
RM           := rm -rf
MKDIR        := mkdir -p
INSTALL      := install
CP           := cp -f

PREFIX       ?= /usr/local
DESTDIR      ?=
BINDIR       ?= $(DESTDIR)$(PREFIX)/bin
USER_HOME    ?= $(HOME)
VIM_DIR      ?= $(USER_HOME)/.vim
EMACS_DIR    ?= $(USER_HOME)/.emacs.d
NANO_DIR     ?= $(USER_HOME)/.config/nano

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
LDFLAGS += -lssl -lcrypto -lcurl

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

# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------

.PHONY: all
all: build

# ------------------------------------------------------------
# Install
# ------------------------------------------------------------

.PHONY: install install-bin install-editors
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

# ------------------------------------------------------------
# Build
# ------------------------------------------------------------

.PHONY: build
build: dirs $(BIN_DIR)/$(PROJECT)

$(BIN_DIR)/$(PROJECT): $(OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
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
ci-strict: grammar-check book-qa-strict negative-tests diag-snapshots

.PHONY: ci-fast
ci-fast: grammar-check negative-tests diag-snapshots completions-snapshots wrapper-stage-test

.PHONY: ci-completions
ci-completions: completions-check completions-lint completions-snapshots completions-fallback

.PHONY: runtime-matrix-modules
runtime-matrix-modules:
	@$(BIN_DIR)/$(PROJECT) check --lang=en tests/vitte_packages_runtime_matrix.vit

.PHONY: module-shape-policy
module-shape-policy:
	@tools/check_module_shape_policy.py

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

.PHONY: ci-std-fast
ci-std-fast: std-check extern-abi-host stdlib-api-lint stdlib-profile-snapshots diag-snapshots completions-snapshots wrapper-stage-test

.PHONY: ci-mod-fast
ci-mod-fast: grammar-check diag-snapshots completions-snapshots stdlib-profile-snapshots stdlib-abi-compat modules-tests modules-snapshots same-output-hash

.PHONY: ci-bridge-compat
ci-bridge-compat: ci-mod-fast

PKG_VERSION ?= 2.1.1

.PHONY: pkg-macos
pkg-macos:
	@VERSION=$(PKG_VERSION) toolchain/scripts/package/make-macos-pkg.sh

.PHONY: pkg-macos-uninstall
pkg-macos-uninstall:
	@VERSION=$(PKG_VERSION) toolchain/scripts/package/make-macos-uninstall-pkg.sh

.PHONY: release-check
release-check: build ci-fast ci-completions pkg-macos

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
	@echo "  make install    build + install binary + Vim/Emacs/Nano syntax files"
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
	@echo "  make explain-snapshots assert vitte explain outputs"
	@echo "  make same-output-hash verify deterministic emit hash stability"
	@echo "  make completions-gen regenerate bash/zsh/fish completions from unified spec"
	@echo "  make completions-check verify generated completions are up to date"
	@echo "  make completions-snapshots run completion snapshot assertions"
	@echo "  make completions-snapshots-update update completion golden snapshots"
	@echo "  make completions-lint syntax-check bash/zsh/fish completion files"
	@echo "  make ci-completions run completion check + lint + snapshots + fallback"
	@echo "  make pkg-macos build macOS installer pkg (PKG_VERSION=$(PKG_VERSION))"
	@echo "  make pkg-macos-uninstall build macOS uninstall pkg (PKG_VERSION=$(PKG_VERSION))"
	@echo "  make release-check run build + ci-fast + ci-completions + pkg build"
	@echo "  make ci-mod-fast module-focused CI (grammar + snapshots + module tests)"
	@echo "  make ci-fast-compiler compiler-focused CI with cache skip (grammar + resolve + module snapshots + explain + runtime matrix)"
	@echo "  make platon-editor build and self-test platon editor core"
	@echo "  make std-check  verify std layout"
	@echo "  make clean      remove build artifacts"
	@echo "  make distclean  full cleanup"
	@echo ""
