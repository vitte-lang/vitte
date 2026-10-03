# =============================================================================
# Vitte Compiler — C17 build
# =============================================================================
#
# Expected layout:
#
#   vitte/
#   ├── src/
#   │   ├── Makefile
#   │   └── src/
#   │       ├── main.c
#   │       ├── lexer/
#   │       ├── parser/
#   │       ├── sema/
#   │       ├── ...
#   │       └── util/
#   ├── examples/
#   └── target/
#
# Run from:
#
#   cd /Users/vincent/Documents/Github/vitte/src
#   make
#
# Useful targets:
#
#   make
#   make verify
#   make smoke
#   make corpus
#   make strict
#   make debug
#   make release
#   make clean
#   make distclean
#   make install
#
# =============================================================================

CC ?= cc

# -----------------------------------------------------------------------------
# Directories
# -----------------------------------------------------------------------------

THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT_DIR := $(abspath $(THIS_DIR)/..)

SOURCE_ROOT := $(THIS_DIR)/src

TARGET_ROOT ?= $(ROOT_DIR)/target
OUT_DIR ?= $(TARGET_ROOT)/vitte

OBJ_DIR := $(OUT_DIR)/obj
DEP_DIR := $(OUT_DIR)/dep
CORPUS_DIR := $(OUT_DIR)/corpus

BIN ?= $(OUT_DIR)/vitte

INSTALL_DIR ?= $(ROOT_DIR)/bin
INSTALL_BIN ?= $(INSTALL_DIR)/vitte

# -----------------------------------------------------------------------------
# Compiler configuration
# -----------------------------------------------------------------------------

CPPFLAGS ?=

BASE_CFLAGS := \
	-std=c17 \
	-Wall \
	-Wextra \
	-Wpedantic

STRICT_CFLAGS := \
	-Wconversion \
	-Wsign-conversion \
	-Wshadow \
	-Wcast-align \
	-Wcast-qual \
	-Wwrite-strings \
	-Wundef \
	-Wformat=2 \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wmissing-declarations \
	-Wold-style-definition \
	-Wswitch-enum \
	-Wimplicit-fallthrough \
	-Wvla \
	-Wpointer-arith \
	-Wbad-function-cast \
	-Wstrict-overflow=5

ERROR_CFLAGS := \
	-Werror

OPT_CFLAGS ?= -O2

DEBUG_CFLAGS := \
	-O0 \
	-g3

RELEASE_CFLAGS := \
	-O3 \
	-DNDEBUG

SANITIZER_CFLAGS := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fsanitize=address,undefined

SANITIZER_LDFLAGS := \
	-fsanitize=address,undefined

CFLAGS ?= $(BASE_CFLAGS) $(ERROR_CFLAGS) $(OPT_CFLAGS)

# -----------------------------------------------------------------------------
# Includes
# -----------------------------------------------------------------------------
#
# IMPORTANT:
#
# Do NOT add:
#
#   -I$(ROOT_DIR)/include/vitte
#
# because Vitte may contain headers such as:
#
#   include/vitte/string.h
#
# which can shadow the C standard library <string.h>.
#
# -----------------------------------------------------------------------------

INCLUDES := \
	-I$(THIS_DIR) \
	-I$(SOURCE_ROOT) \
	-I$(ROOT_DIR) \
	-I$(ROOT_DIR)/include \
	-I$(THIS_DIR)/include

CPPFLAGS += $(INCLUDES)

# -----------------------------------------------------------------------------
# Linker
# -----------------------------------------------------------------------------

LDFLAGS ?=

# Constant folding and numeric support may use fmod()/libm.
LDLIBS ?= -lm

# -----------------------------------------------------------------------------
# Source discovery
# -----------------------------------------------------------------------------

GIT_TRACKED := $(shell \
	if git -C "$(ROOT_DIR)" rev-parse --is-inside-work-tree >/dev/null 2>&1; then \
		git -C "$(ROOT_DIR)" ls-files 'src/src/**/*.c' 'src/src/*.c' 2>/dev/null | \
			sed 's|^src/||'; \
	fi)

ifeq ($(strip $(GIT_TRACKED)),)

SOURCES := $(shell \
	find "$(SOURCE_ROOT)" \
		-type f \
		-name '*.c' \
		! -name '* *' \
		-print | \
	sort)

else

SOURCES := $(addprefix $(THIS_DIR)/,$(filter %.c,$(GIT_TRACKED)))

endif

HEADERS := $(shell \
	find "$(SOURCE_ROOT)" \
		-type f \
		-name '*.h' \
		! -name '* *' \
		-print | \
	sort)

CHECKED_FILES := $(shell \
	find "$(THIS_DIR)" \
		-type f \
		\( \
			-name '*.c' \
			-o -name '*.h' \
			-o -name 'Makefile' \
			-o -name 'CMakeLists.txt' \
			-o -name 'README.md' \
		\) \
		! -name '* *' \
		-print | \
	sort)

# -----------------------------------------------------------------------------
# Object mapping
# -----------------------------------------------------------------------------

OBJECTS := $(patsubst $(SOURCE_ROOT)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

DEPS := $(patsubst $(SOURCE_ROOT)/%.c,$(DEP_DIR)/%.d,$(SOURCES))

# -----------------------------------------------------------------------------
# Shell
# -----------------------------------------------------------------------------

SHELL := /bin/sh

# -----------------------------------------------------------------------------
# Phony targets
# -----------------------------------------------------------------------------

.PHONY: \
	all \
	build \
	rebuild \
	verify \
	verify-sources \
	verify-headers \
	verify-empty \
	verify-alignment \
	alignment \
	smoke \
	corpus \
	strict \
	debug \
	release \
	sanitize \
	asan \
	ubsan \
	install \
	uninstall \
	clean \
	distclean \
	print-config \
	print-sources \
	print-headers \
	help

# =============================================================================
# Default
# =============================================================================

all: alignment verify $(BIN)

build: $(BIN)

# =============================================================================
# Compiler binary
# =============================================================================

$(BIN): $(OBJECTS)
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[LD] $@"
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o "$@"

# =============================================================================
# Object compilation
# =============================================================================

$(OBJ_DIR)/%.o: $(SOURCE_ROOT)/%.c
	@mkdir -p "$(dir $@)"
	@mkdir -p "$(dir $(DEP_DIR)/$*.d)"
	@printf '%s\n' "[CC] $<"
	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		-MMD \
		-MP \
		-MF "$(DEP_DIR)/$*.d" \
		-c "$<" \
		-o "$@"

# =============================================================================
# Dependency files
# =============================================================================

-include $(DEPS)

# =============================================================================
# Verification
# =============================================================================

verify: \
	verify-sources \
	verify-headers \
	verify-empty \
	verify-alignment

verify-sources:
	@test -n "$(strip $(SOURCES))" || { \
		printf '%s\n' "error: no C source files found"; \
		exit 1; \
	}
	@printf '%s\n' "[VERIFY] C translation units"
	@set -e; \
	for src in $(SOURCES); do \
		printf '  %-72s' "$$src"; \
		$(CC) \
			$(CPPFLAGS) \
			$(CFLAGS) \
			-fsyntax-only \
			"$$src"; \
		printf ' OK\n'; \
	done

verify-headers:
	@test -n "$(strip $(HEADERS))" || { \
		printf '%s\n' "error: no headers found"; \
		exit 1; \
	}
	@printf '%s\n' "[VERIFY] public/internal headers"
	@set -e; \
	for header in $(HEADERS); do \
		printf '  %-72s' "$$header"; \
		printf '#include "%s"\nint main(void) { return 0; }\n' "$$header" | \
			$(CC) \
				$(CPPFLAGS) \
				$(CFLAGS) \
				-x c \
				-fsyntax-only \
				-; \
		printf ' OK\n'; \
	done

verify-empty:
	@test -n "$(strip $(CHECKED_FILES))" || { \
		printf '%s\n' "error: no project files found"; \
		exit 1; \
	}
	@printf '%s\n' "[VERIFY] empty files"
	@empty="$$( \
		find "$(THIS_DIR)" \
			-type f \
			\( \
				-name '*.c' \
				-o -name '*.h' \
				-o -name 'Makefile' \
				-o -name 'CMakeLists.txt' \
				-o -name 'README.md' \
			\) \
			-size 0 \
			-print \
	)"; \
	if [ -n "$$empty" ]; then \
		printf '%s\n' "error: empty project files:"; \
		printf '%s\n' "$$empty"; \
		exit 1; \
	fi

verify-alignment: alignment

# =============================================================================
# Language/source alignment
# =============================================================================

alignment:
	@printf '%s\n' "[VERIFY] language alignment"
	@test -f "$(ROOT_DIR)/src/docs/language_coverage.md" || { \
		printf '%s\n' \
			"error: missing $(ROOT_DIR)/src/docs/language_coverage.md"; \
		exit 1; \
	}
	@if grep \
		-R \
		-nE \
		'(^|[^[:alnum:]_])(TODO|FIXME)([^[:alnum:]_]|$$)' \
		"$(SOURCE_ROOT)" \
		--include='*.c' \
		--include='*.h'; \
	then \
		printf '%s\n' \
			"error: TODO/FIXME marker found in compiler source"; \
		exit 1; \
	fi

# =============================================================================
# Strict build
# =============================================================================

strict:
	@$(MAKE) clean
	@$(MAKE) \
		CFLAGS="$(BASE_CFLAGS) $(STRICT_CFLAGS) $(ERROR_CFLAGS) -O2" \
		all

# =============================================================================
# Debug build
# =============================================================================

debug:
	@$(MAKE) clean
	@$(MAKE) \
		CFLAGS="$(BASE_CFLAGS) $(STRICT_CFLAGS) $(ERROR_CFLAGS) $(DEBUG_CFLAGS)" \
		$(BIN)

# =============================================================================
# Release build
# =============================================================================

release:
	@$(MAKE) clean
	@$(MAKE) \
		CFLAGS="$(BASE_CFLAGS) $(STRICT_CFLAGS) $(ERROR_CFLAGS) $(RELEASE_CFLAGS)" \
		$(BIN)

# =============================================================================
# Sanitizers
# =============================================================================

sanitize:
	@$(MAKE) clean
	@$(MAKE) \
		CFLAGS="$(BASE_CFLAGS) $(STRICT_CFLAGS) $(SANITIZER_CFLAGS)" \
		LDFLAGS="$(LDFLAGS) $(SANITIZER_LDFLAGS)" \
		$(BIN)

asan:
	@$(MAKE) clean
	@$(MAKE) \
		CFLAGS="$(BASE_CFLAGS) -O1 -g3 -fno-omit-frame-pointer -fsanitize=address" \
		LDFLAGS="$(LDFLAGS) -fsanitize=address" \
		$(BIN)

ubsan:
	@$(MAKE) clean
	@$(MAKE) \
		CFLAGS="$(BASE_CFLAGS) -O1 -g3 -fno-omit-frame-pointer -fsanitize=undefined" \
		LDFLAGS="$(LDFLAGS) -fsanitize=undefined" \
		$(BIN)

# =============================================================================
# Smoke tests
# =============================================================================

smoke: verify $(BIN)
	@printf '%s\n' "[SMOKE] version"
	@"$(BIN)" --version

	@printf '%s\n' "[SMOKE] successful frontend/semantic checks"

	@"$(BIN)" check "$(ROOT_DIR)/examples/hello.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_visibility/direct_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_visibility/module_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/export_forms/direct_star_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/export_forms/module_star_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/export_forms/direct_alias_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/export_forms/module_alias_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_export_coverage/coverage_explicit_main.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_export_coverage/coverage_star_main.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_export_coverage/coverage_chain_main.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_export_coverage/multi_decl_main.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_export_coverage/module_alias_rich_main.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/sema_calls/local_param_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/sema_calls/import_param_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/frontend_expr_stmt/expr_stmt_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/sema_operators/logical_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/grammar_alignment/let_mut_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/grammar_alignment/ref_param_type_marker_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/assignment_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/contracts_ok.vit"

	@printf '%s\n' "[SMOKE] contractual diagnostics"

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/contracts_type_error.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		printf '%s\n' \
			"error: contracts_type_error.vit unexpectedly succeeded"; \
		exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_REQUIRES" "$$tmp" || { \
		cat "$$tmp"; \
		exit 1; \
	}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/contracts_false.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		printf '%s\n' \
			"error: contracts_false.vit unexpectedly succeeded"; \
		exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_REQUIRES_FALSE" "$$tmp" || { \
		cat "$$tmp"; \
		exit 1; \
	}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@printf '%s\n' "[SMOKE] import visibility diagnostics"

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_visibility/ambiguous_symbol_form_main.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_DRIVER_E_IMPORT_USE_CONTRACT" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	grep -q \
		"use 'use contract_lib::{value}' to import symbol 'value'" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_visibility/direct_private.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_SEMA_E_IMPORT_PRIVATE" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_visibility/glob_private.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_SEMA_E_IMPORT_PRIVATE" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_visibility/module_private.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_SEMA_E_IMPORT_PRIVATE_MEMBER" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@printf '%s\n' "[SMOKE] import collision success cases"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_collision/local_main.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_collision/modules_main.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_collision/symbol_alias_main.vit"

	@printf '%s\n' "[SMOKE] import collision diagnostics"

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_collision/glob_conflict_main.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_SEMA_E_IMPORT_CONFLICT" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@printf '%s\n' "[SMOKE] import/export contract"

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_export_coverage/module_path_unsupported.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_DRIVER_E_IMPORT_USE_CONTRACT" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	grep -q \
		"use 'use multi_decl_lib::{first}' to import symbol 'first'" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@printf '%s\n' "[SMOKE] procedure call diagnostics"

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/sema_calls/local_param_mismatch.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_SEMA_E_CALL" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	grep -q \
		"procedure argument type mismatch" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/sema_calls/import_param_mismatch.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_SEMA_E_CALL" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	grep -q \
		"procedure argument type mismatch" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@printf '%s\n' "[SMOKE] operator diagnostics"

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/sema_operators/logical_type_mismatch.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_SEMA_E_OPERATOR" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	grep -q \
		"invalid operand types for operator" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@printf '%s\n' "[SMOKE] export diagnostics"

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/export_forms/missing_export.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_SEMA_E_EXPORT" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/export_forms/reexport_group_unsupported.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_PARSER_E_REEXPORT" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/export_forms/reexport_module_alias_unsupported.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_SEMA_E_REEXPORT" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/export_forms/reexport_symbol_unsupported.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_SEMA_E_REEXPORT" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@printf '%s\n' "[SMOKE] native builds"

	@"$(BIN)" build \
		"$(ROOT_DIR)/examples/hello.vit" \
		-o "$(OUT_DIR)/hello"

	@"$(OUT_DIR)/hello"

	@"$(BIN)" build \
		"$(ROOT_DIR)/src/tests/frontend_expr_stmt/expr_stmt_ok.vit" \
		-o "$(OUT_DIR)/expr_stmt_ok"

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	"$(OUT_DIR)/expr_stmt_ok" >"$$tmp"; \
	grep -q "^expr-stmt-ok$$" "$$tmp" || { \
		cat "$$tmp"; \
		exit 1; \
	}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@"$(BIN)" build \
		"$(ROOT_DIR)/src/tests/sema_operators/logical_ok.vit" \
		-o "$(OUT_DIR)/logical_ok"

	@"$(OUT_DIR)/logical_ok"

	@"$(BIN)" build \
		"$(ROOT_DIR)/src/tests/grammar_alignment/let_mut_ok.vit" \
		-o "$(OUT_DIR)/let_mut_ok"

	@"$(OUT_DIR)/let_mut_ok"

	@"$(BIN)" build \
		"$(ROOT_DIR)/src/tests/assignment_ok.vit" \
		-o "$(OUT_DIR)/assignment_ok"

	@"$(OUT_DIR)/assignment_ok"; \
	status=$$?; \
	test "$$status" -eq 2

	@"$(BIN)" build \
		"$(ROOT_DIR)/src/tests/aggregate_runtime.vit" \
		-o "$(OUT_DIR)/aggregate_runtime"

	@"$(OUT_DIR)/aggregate_runtime"

	@"$(BIN)" build \
		"$(ROOT_DIR)/src/tests/string_intrinsics_runtime.vit" \
		-o "$(OUT_DIR)/string_intrinsics_runtime"

	@"$(OUT_DIR)/string_intrinsics_runtime"

	@mkdir -p "$(OUT_DIR)"

	@cd "$(ROOT_DIR)" && \
		"$(abspath $(BIN))" build \
			src/tests/lazy_expressions_runtime.vit \
			-o "$(abspath $(OUT_DIR))/lazy_expressions_runtime"

	@cd "$(ROOT_DIR)" && \
		"$(abspath $(OUT_DIR))/lazy_expressions_runtime"

	@printf '%s\n' "[SMOKE] all tests passed"

# =============================================================================
# Corpus
# =============================================================================

corpus: verify $(BIN)
	@printf '%s\n' "[CORPUS] preparing"
	@mkdir -p "$(CORPUS_DIR)"

	@"$(BIN)" check \
		"$(ROOT_DIR)/examples/hello.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/frontend_expr_stmt/expr_stmt_ok.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_export_coverage/multi_decl_main.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_export_coverage/module_alias_rich_main.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_export_coverage/coverage_chain_main.vit"

	@printf '%s\n' "[CORPUS] C17 emission"

	@"$(BIN)" emit-c \
		"$(ROOT_DIR)/examples/hello.vit" \
		-o "$(CORPUS_DIR)/hello.c"

	@"$(BIN)" emit-c \
		"$(ROOT_DIR)/src/tests/import_export_coverage/multi_decl_main.vit" \
		-o "$(CORPUS_DIR)/multi_decl_main.c"

	@"$(BIN)" emit-c \
		"$(ROOT_DIR)/src/tests/import_export_coverage/module_alias_rich_main.vit" \
		-o "$(CORPUS_DIR)/module_alias_rich_main.c"

	@"$(BIN)" emit-c \
		"$(ROOT_DIR)/src/tests/import_export_coverage/coverage_chain_main.vit" \
		-o "$(CORPUS_DIR)/coverage_chain_main.c"

	@"$(BIN)" emit-c \
		"$(ROOT_DIR)/src/tests/pick_parse.vit" \
		-o "$(CORPUS_DIR)/pick_parse.c"

	@"$(BIN)" emit-c \
		"$(ROOT_DIR)/src/tests/form_parse.vit" \
		-o "$(CORPUS_DIR)/form_parse.c"

	@printf '%s\n' "[CORPUS] generated C syntax verification"

	@set -e; \
	for generated in "$(CORPUS_DIR)"/*.c; do \
		printf '  %-72s' "$$generated"; \
		$(CC) \
			-std=c17 \
			-Wall \
			-Wextra \
			-Wpedantic \
			-fsyntax-only \
			"$$generated"; \
		printf ' OK\n'; \
	done

	@printf '%s\n' "[CORPUS] native builds"

	@"$(BIN)" build \
		"$(ROOT_DIR)/examples/hello.vit" \
		-o "$(CORPUS_DIR)/hello"

	@"$(BIN)" build \
		"$(ROOT_DIR)/src/tests/import_export_coverage/multi_decl_main.vit" \
		-o "$(CORPUS_DIR)/multi_decl_main"

	@"$(BIN)" build \
		"$(ROOT_DIR)/src/tests/import_export_coverage/module_alias_rich_main.vit" \
		-o "$(CORPUS_DIR)/module_alias_rich_main"

	@"$(BIN)" build \
		"$(ROOT_DIR)/src/tests/import_export_coverage/coverage_chain_main.vit" \
		-o "$(CORPUS_DIR)/coverage_chain_main"

	@printf '%s\n' "[CORPUS] expected failures"

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_collision/glob_conflict_main.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_SEMA_E_IMPORT_CONFLICT" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_export_coverage/module_path_unsupported.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_DRIVER_E_IMPORT_USE_CONTRACT" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/sema_calls/import_param_mismatch.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q \
		"VITTE_SEMA_E_CALL" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@printf '%s\n' "[CORPUS] complete"

# =============================================================================
# Install
# =============================================================================

install: $(BIN)
	@mkdir -p "$(INSTALL_DIR)"
	@cp "$(BIN)" "$(INSTALL_BIN)"
	@chmod 755 "$(INSTALL_BIN)"
	@printf '%s\n' \
		"Installed compiler binary: $(INSTALL_BIN)"

uninstall:
	@rm -f "$(INSTALL_BIN)"
	@printf '%s\n' \
		"Removed compiler binary: $(INSTALL_BIN)"

# =============================================================================
# Rebuild
# =============================================================================

rebuild:
	@$(MAKE) clean
	@$(MAKE) all

# =============================================================================
# Clean
# =============================================================================

clean:
	@printf '%s\n' "[CLEAN] $(OUT_DIR)"
	@rm -rf "$(OUT_DIR)"

distclean:
	@printf '%s\n' "[DISTCLEAN] $(TARGET_ROOT)"
	@rm -rf "$(TARGET_ROOT)"

# =============================================================================
# Introspection
# =============================================================================

print-config:
	@printf '%s\n' \
		"CC              = $(CC)" \
		"CPPFLAGS        = $(CPPFLAGS)" \
		"CFLAGS          = $(CFLAGS)" \
		"LDFLAGS         = $(LDFLAGS)" \
		"LDLIBS          = $(LDLIBS)" \
		"THIS_DIR        = $(THIS_DIR)" \
		"ROOT_DIR        = $(ROOT_DIR)" \
		"SOURCE_ROOT     = $(SOURCE_ROOT)" \
		"TARGET_ROOT     = $(TARGET_ROOT)" \
		"OUT_DIR         = $(OUT_DIR)" \
		"OBJ_DIR         = $(OBJ_DIR)" \
		"DEP_DIR         = $(DEP_DIR)" \
		"BIN             = $(BIN)" \
		"SOURCES         = $(words $(SOURCES))" \
		"HEADERS         = $(words $(HEADERS))"

print-sources:
	@printf '%s\n' $(SOURCES)

print-headers:
	@printf '%s\n' $(HEADERS)

# =============================================================================
# Help
# =============================================================================

help:
	@printf '%s\n' \
		"Vitte compiler build" \
		"" \
		"Targets:" \
		"  all            Verify and build compiler" \
		"  build          Build compiler" \
		"  rebuild        Clean then rebuild" \
		"  verify         Syntax/header/project verification" \
		"  alignment      Verify language/source alignment" \
		"  smoke          Run compiler smoke/regression suite" \
		"  corpus         Build/check representative corpus" \
		"  strict         Strict warning build" \
		"  debug          Debug build" \
		"  release        Optimized release build" \
		"  sanitize       ASan + UBSan build" \
		"  asan           AddressSanitizer build" \
		"  ubsan          UndefinedBehaviorSanitizer build" \
		"  install        Install to repository bin/" \
		"  uninstall      Remove repository bin/vitte" \
		"  clean          Remove Vitte compiler output" \
		"  distclean      Remove complete target directory" \
		"  print-config   Display resolved build configuration" \
		"  print-sources  Display compiler source list" \
		"  print-headers  Display compiler header list"
