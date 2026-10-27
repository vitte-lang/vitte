# =============================================================================
# Vitte Compiler — C17 build
# =============================================================================
#
# Expected layout:
#
#   vitte/
#   ├── src/
#   │   ├── Makefile
#   │   └── bootstrap/
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

SOURCE_ROOT := $(THIS_DIR)/bootstrap

UNICODE_TABLE_GENERATOR := $(ROOT_DIR)/scripts/generate-unicode-xid.py
UNICODE_TABLE := $(SOURCE_ROOT)/unicode/unicode_xid_table.h
# Keep table generation reproducible across Python installations.
UNICODE_DATA_VERSION := 16.0.0

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

# Classification switches, POSIX spawn adapters, standalone ABI mirrors and
# checked varargs forwarding need localized suppressions; keep the remaining
# strict diagnostics enabled.
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
	-Wstrict-overflow=5 \
	-Wno-switch-enum \
	-Wno-cast-qual \
	-Wno-missing-prototypes \
	-Wno-missing-declarations \
	-Wno-format-nonliteral

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

SOURCES := $(shell \
	find "$(SOURCE_ROOT)" \
		-type f \
		-name '*.c' \
		! -name '* *' \
		-print | \
	sort)

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

# The executable-facing CLI modules are linked only into vitte itself.  Test
# binaries exercise compiler subsystems directly and must not inherit process
# entry callbacks that depend on the executable source/module context.
TEST_LIB_OBJECTS := $(filter-out \
	$(OBJ_DIR)/main.o \
	$(OBJ_DIR)/cli/entry.o \
	$(OBJ_DIR)/cli/options.o \
	$(OBJ_DIR)/cli/diagnostics.o \
	$(OBJ_DIR)/cli/pipeline.o \
	$(OBJ_DIR)/cli/execution.o, \
	$(OBJECTS))
FRONTEND_TOKEN_TEST := $(OUT_DIR)/frontend_token_test
FRONTEND_FUZZ_TEST := $(OUT_DIR)/frontend_fuzz_test
FRONTEND_STRESS_TEST := $(OUT_DIR)/frontend_stress_test
IMPORT_GRAPH_TEST := $(OUT_DIR)/import_graph_test
DIAGNOSTIC_RENDER_TEST := $(OUT_DIR)/diagnostic_render_test
DIAGNOSTIC_FUZZ_TEST := $(OUT_DIR)/diagnostic_fuzz_test
DIAGNOSTIC_HELP_TEST := $(ROOT_DIR)/src/tests/diagnostic_help_test.py
DIAGNOSTIC_PARITY_TEST := $(ROOT_DIR)/src/tests/diagnostic_parity_test.py
IMPORT_FUZZ_TEST := $(OUT_DIR)/import_fuzz_test
CONFIG_FUZZ_TEST := $(OUT_DIR)/config_fuzz_test
ROBUSTNESS_TEST := $(OUT_DIR)/robustness_test
CONCURRENT_TEST := $(OUT_DIR)/concurrent_compile_test
TYPE_CONVERSION_TEST := $(OUT_DIR)/type_conversion_test
IR_BACKEND_TEST := $(OUT_DIR)/ir_backend_test
CONFIG_PRECEDENCE_TEST := $(OUT_DIR)/config_precedence_test
FILESYSTEM_PORTABILITY_TEST := $(OUT_DIR)/filesystem_portability_test
CLI_SPEC_TEST := $(OUT_DIR)/cli_spec_test
CLI_DIFFERENTIAL_C_RUNNER := $(OUT_DIR)/cli_differential_c_runner
CLI_DIFFERENTIAL_VITTE_RUNNER ?= $(OUT_DIR)/cli_differential_vitte_runner
CLI_DIFFERENTIAL_TEST := $(ROOT_DIR)/src/tests/cli_differential_test.py
GRAMMAR_PARSER_TEST := $(OUT_DIR)/grammar_parser_test
GRAMMAR_FIXTURES := \
	$(ROOT_DIR)/src/tests/grammar_alignment/let_mut_ok.vit \
	$(ROOT_DIR)/src/tests/grammar_alignment/ref_param_type_marker_ok.vit \
	$(ROOT_DIR)/src/tests/expression_literals_parse.vit \
	$(ROOT_DIR)/src/tests/form_parse.vit \
	$(ROOT_DIR)/src/tests/pick_parse.vit
BOOTSTRAP_REGRESSION_TEST := $(ROOT_DIR)/src/tests/bootstrap_regression_test.py

# Link the executable-facing pipeline (without main) for the in-process
# concurrency check. Each worker owns an independent driver context.
CLI_TEST_OBJECTS := $(filter-out $(OBJ_DIR)/main.o,$(OBJECTS))
THREAD_FLAGS ?= -pthread

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
	unicode-table \
	unicode-check \
	reentrancy-check \
	verify-sources \
	verify-headers \
	verify-empty \
	verify-alignment \
	alignment \
	docs \
	docs-check \
	check-ebnf-frontend-sync \
	portability \
	smoke \
	corpus \
	strict \
	debug \
	release \
	sanitize \
	sanitize-test \
	asan \
	ubsan \
	test-frontend-tokens \
	test-unicode-generation \
	test-frontend-fuzz \
	test-frontend-stress \
	test-import-graph \
	test-import-fuzz \
	test-config-fuzz \
	test-robustness \
	fuzz-continuous \
	test-type-conversions \
	test-ir-backend \
	test-config-precedence \
	test-filesystem-portability \
	test-cli-spec \
	test-cli-differential \
	test-grammar \
	test-diagnostics \
	test-diagnostic-help \
	test-diagnostic-parity \
	test-concurrent \
	test-bootstrap-regressions \
	test-frontend \
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
# Frontend tests
# =============================================================================

$(FRONTEND_TOKEN_TEST): \
	$(ROOT_DIR)/src/tests/frontend_token_test.c \
	$(TEST_LIB_OBJECTS) \
	$(UNICODE_TABLE)
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] frontend_token_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/frontend_token_test.c" \
		$(TEST_LIB_OBJECTS) $(LDFLAGS) $(LDLIBS) -o "$@"

$(FRONTEND_FUZZ_TEST): \
	$(ROOT_DIR)/src/tests/frontend_fuzz_test.c \
	$(TEST_LIB_OBJECTS) \
	$(UNICODE_TABLE)
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] frontend_fuzz_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/frontend_fuzz_test.c" \
		$(TEST_LIB_OBJECTS) $(LDFLAGS) $(LDLIBS) -o "$@"

$(FRONTEND_STRESS_TEST): \
	$(ROOT_DIR)/src/tests/frontend_stress_test.c \
	$(TEST_LIB_OBJECTS) \
	$(UNICODE_TABLE)
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] frontend_stress_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/frontend_stress_test.c" \
		$(TEST_LIB_OBJECTS) $(LDFLAGS) $(LDLIBS) -o "$@"

$(GRAMMAR_PARSER_TEST): \
	$(ROOT_DIR)/src/tests/grammar_parser_test.c \
	$(TEST_LIB_OBJECTS)
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] grammar_parser_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/grammar_parser_test.c" \
		$(TEST_LIB_OBJECTS) $(LDFLAGS) $(LDLIBS) -o "$@"

$(IMPORT_GRAPH_TEST): \
	$(ROOT_DIR)/src/tests/import_graph_test.c \
	$(OBJ_DIR)/import/import.o \
	$(OBJ_DIR)/util/util.o
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] import_graph_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/import_graph_test.c" \
		$(OBJ_DIR)/import/import.o $(OBJ_DIR)/util/util.o \
		$(LDFLAGS) $(LDLIBS) -o "$@"

$(DIAGNOSTIC_RENDER_TEST): \
	$(ROOT_DIR)/src/tests/diagnostic_render_test.c \
	$(TEST_LIB_OBJECTS)
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] diagnostic_render_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/diagnostic_render_test.c" \
		$(TEST_LIB_OBJECTS) $(LDFLAGS) $(LDLIBS) -o "$@"

$(DIAGNOSTIC_FUZZ_TEST): \
	$(ROOT_DIR)/src/tests/diagnostic_fuzz_test.c \
	$(TEST_LIB_OBJECTS)
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] diagnostic_fuzz_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/diagnostic_fuzz_test.c" \
		$(TEST_LIB_OBJECTS) $(LDFLAGS) $(LDLIBS) -o "$@"

$(IMPORT_FUZZ_TEST): \
	$(ROOT_DIR)/src/tests/import_fuzz_test.c \
	$(OBJ_DIR)/import/import.o \
	$(OBJ_DIR)/util/util.o
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] import_fuzz_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/import_fuzz_test.c" \
		$(OBJ_DIR)/import/import.o $(OBJ_DIR)/util/util.o \
		$(LDFLAGS) $(LDLIBS) -o "$@"

$(CONFIG_FUZZ_TEST): \
	$(ROOT_DIR)/src/tests/config_fuzz_test.c \
	$(OBJ_DIR)/config/config.o \
	$(OBJ_DIR)/filesystem/filesystem.o
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] config_fuzz_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/config_fuzz_test.c" \
		$(OBJ_DIR)/config/config.o $(OBJ_DIR)/filesystem/filesystem.o \
		$(LDFLAGS) $(LDLIBS) -o "$@"

$(ROBUSTNESS_TEST): \
	$(ROOT_DIR)/src/tests/robustness_test.c \
	$(TEST_LIB_OBJECTS)
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] robustness_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/robustness_test.c" \
		$(TEST_LIB_OBJECTS) $(LDFLAGS) $(LDLIBS) -o "$@"

$(CONCURRENT_TEST): \
	$(ROOT_DIR)/src/tests/concurrent_compile_test.c \
	$(CLI_TEST_OBJECTS)
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] concurrent_compile_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THREAD_FLAGS) \
		"$(ROOT_DIR)/src/tests/concurrent_compile_test.c" \
		$(CLI_TEST_OBJECTS) $(LDFLAGS) $(LDLIBS) $(THREAD_FLAGS) -o "$@"

$(TYPE_CONVERSION_TEST): \
	$(ROOT_DIR)/src/tests/type_conversion_test.c \
	$(OBJ_DIR)/type/type.o
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] type_conversion_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/type_conversion_test.c" \
		$(OBJ_DIR)/type/type.o $(LDFLAGS) $(LDLIBS) -o "$@"

$(IR_BACKEND_TEST): \
	$(ROOT_DIR)/src/tests/ir_backend_test.c \
	$(TEST_LIB_OBJECTS)
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] ir_backend_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/ir_backend_test.c" \
		$(TEST_LIB_OBJECTS) $(LDFLAGS) $(LDLIBS) -o "$@"

$(CONFIG_PRECEDENCE_TEST): \
	$(ROOT_DIR)/src/tests/config_precedence_test.c \
	$(OBJ_DIR)/config/config.o \
	$(OBJ_DIR)/filesystem/filesystem.o
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] config_precedence_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/config_precedence_test.c" \
		$(OBJ_DIR)/config/config.o $(OBJ_DIR)/filesystem/filesystem.o \
		$(LDFLAGS) $(LDLIBS) -o "$@"

$(CLI_SPEC_TEST): \
	$(ROOT_DIR)/src/tests/cli_spec_test.c \
	$(OBJ_DIR)/cli/command_spec.o
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] cli_spec_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/cli_spec_test.c" \
		$(OBJ_DIR)/cli/command_spec.o $(LDFLAGS) $(LDLIBS) -o "$@"

$(CLI_DIFFERENTIAL_C_RUNNER): \
	$(ROOT_DIR)/src/tests/cli_differential_c_runner.c \
	$(SOURCE_ROOT)/cli/cli.c \
	$(SOURCE_ROOT)/cli/cli.h
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] cli_differential_c_runner"
	$(CC) $(CPPFLAGS) $(CFLAGS) -I"$(SOURCE_ROOT)" \
		"$(ROOT_DIR)/src/tests/cli_differential_c_runner.c" \
		"$(SOURCE_ROOT)/cli/cli.c" $(LDFLAGS) $(LDLIBS) -o "$@"

test-frontend-tokens: $(FRONTEND_TOKEN_TEST)
	@"$(FRONTEND_TOKEN_TEST)"

test-unicode-generation: unicode-check
	@python3 "$(ROOT_DIR)/scripts/test-unicode-generation.py"

test-frontend-fuzz: $(FRONTEND_FUZZ_TEST)
	@"$(FRONTEND_FUZZ_TEST)"

test-frontend-stress: $(FRONTEND_STRESS_TEST)
	@"$(FRONTEND_STRESS_TEST)"

test-grammar: check-ebnf-frontend-sync $(GRAMMAR_PARSER_TEST)
	@"$(GRAMMAR_PARSER_TEST)" $(GRAMMAR_FIXTURES)

test-import-graph: $(IMPORT_GRAPH_TEST)
	@"$(IMPORT_GRAPH_TEST)"

test-import-fuzz: $(IMPORT_FUZZ_TEST)
	@"$(IMPORT_FUZZ_TEST)"

test-config-fuzz: $(CONFIG_FUZZ_TEST)
	@"$(CONFIG_FUZZ_TEST)"

test-robustness: $(ROBUSTNESS_TEST)
	@"$(ROBUSTNESS_TEST)"

FUZZ_SECONDS ?= 60
.PHONY: fuzz-continuous
fuzz-continuous: $(FRONTEND_FUZZ_TEST) $(IMPORT_FUZZ_TEST) $(CONFIG_FUZZ_TEST)
	@python3 "$(ROOT_DIR)/scripts/run-bootstrap-fuzz.py" \
		--seconds "$(FUZZ_SECONDS)" \
		--iterations "$${VITTE_FUZZ_ITERATIONS:-1000}" \
		"$(FRONTEND_FUZZ_TEST)" "$(IMPORT_FUZZ_TEST)" "$(CONFIG_FUZZ_TEST)"

test-type-conversions: $(TYPE_CONVERSION_TEST)
	@"$(TYPE_CONVERSION_TEST)"

test-ir-backend: $(IR_BACKEND_TEST)
	@"$(IR_BACKEND_TEST)"

test-config-precedence: $(CONFIG_PRECEDENCE_TEST)
	@"$(CONFIG_PRECEDENCE_TEST)"

$(FILESYSTEM_PORTABILITY_TEST): \
	$(ROOT_DIR)/src/tests/filesystem_portability_test.c \
	$(OBJ_DIR)/filesystem/filesystem.o \
	$(OBJ_DIR)/platform/platform.o
	@mkdir -p "$(dir $@)"
	@printf '%s\n' "[TEST-CC] filesystem_portability_test"
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		"$(ROOT_DIR)/src/tests/filesystem_portability_test.c" \
		$(OBJ_DIR)/filesystem/filesystem.o $(OBJ_DIR)/platform/platform.o \
		$(LDFLAGS) $(LDLIBS) -o "$@"

test-filesystem-portability: $(FILESYSTEM_PORTABILITY_TEST)
	@"$(FILESYSTEM_PORTABILITY_TEST)"

test-cli-spec: $(CLI_SPEC_TEST)
	@"$(CLI_SPEC_TEST)"

# The Vitte runner is built by the native compiler toolchain. Keep this target
# explicit until that compiler can build cli.vitte as part of its own graph.
test-cli-differential: $(CLI_DIFFERENTIAL_C_RUNNER)
	@test -x "$(CLI_DIFFERENTIAL_VITTE_RUNNER)" || { \
		echo "Missing executable native Vitte runner: $(CLI_DIFFERENTIAL_VITTE_RUNNER)" >&2; \
		echo "Build src/tests/cli_differential_vitte_runner.vitte with the native compiler, then rerun." >&2; \
		exit 2; \
	}
	@python3 "$(CLI_DIFFERENTIAL_TEST)" "$(CLI_DIFFERENTIAL_C_RUNNER)" "$(CLI_DIFFERENTIAL_VITTE_RUNNER)"

test-diagnostics: $(DIAGNOSTIC_RENDER_TEST) $(DIAGNOSTIC_FUZZ_TEST) $(BIN) $(DIAGNOSTIC_HELP_TEST) $(DIAGNOSTIC_PARITY_TEST)
	@"$(DIAGNOSTIC_RENDER_TEST)"
	@"$(DIAGNOSTIC_FUZZ_TEST)"
	@python3 "$(DIAGNOSTIC_HELP_TEST)" "$(BIN)"
	@python3 "$(DIAGNOSTIC_PARITY_TEST)" "$(BIN)"

test-diagnostic-help: $(BIN) $(DIAGNOSTIC_HELP_TEST)
	@python3 "$(DIAGNOSTIC_HELP_TEST)" "$(BIN)"

test-diagnostic-parity: $(BIN) $(DIAGNOSTIC_PARITY_TEST)
	@python3 "$(DIAGNOSTIC_PARITY_TEST)" "$(BIN)"

test-concurrent: $(CONCURRENT_TEST)
	@"$(CONCURRENT_TEST)" \
		"$(ROOT_DIR)/examples/hello.vit" \
		"$(ROOT_DIR)/src/tests/sema_calls/local_param_ok.vit"

test-bootstrap-regressions: $(BIN) $(BOOTSTRAP_REGRESSION_TEST)
	@python3 "$(BOOTSTRAP_REGRESSION_TEST)" "$(BIN)"

test-frontend: test-frontend-tokens test-unicode-generation test-frontend-fuzz test-frontend-stress test-grammar test-import-graph test-import-fuzz test-config-fuzz test-robustness test-type-conversions test-ir-backend test-config-precedence test-filesystem-portability test-cli-spec test-diagnostics test-concurrent test-bootstrap-regressions

# =============================================================================
# Dependency files
# =============================================================================

-include $(DEPS)

# =============================================================================
# Verification
# =============================================================================

verify: \
	unicode-check \
	test-unicode-generation \
	reentrancy-check \
	docs-check \
	verify-sources \
	verify-headers \
	verify-empty \
	verify-alignment

unicode-table:
	@command -v python3 >/dev/null 2>&1 || { \
		printf '%s\n' "error: python3 is required to generate Unicode XID tables"; \
		exit 1; \
	}
	@python3 "$(UNICODE_TABLE_GENERATOR)" \
		--expected-version "$(UNICODE_DATA_VERSION)" \
		--output "$(UNICODE_TABLE)"

unicode-check:
	@command -v python3 >/dev/null 2>&1 || { \
		printf '%s\n' "error: python3 is required to verify Unicode XID tables"; \
		exit 1; \
	}
	@python3 "$(UNICODE_TABLE_GENERATOR)" \
		--expected-version "$(UNICODE_DATA_VERSION)" \
		--check \
		--output "$(UNICODE_TABLE)"

reentrancy-check:
	@python3 "$(ROOT_DIR)/scripts/check-bootstrap-reentrancy.py"

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
# Generated documentation and portability probes
# =============================================================================

docs:
	@python3 "$(ROOT_DIR)/docs/book/grammar/scripts/sync_grammar.py"
	@python3 "$(ROOT_DIR)/scripts/generate-bootstrap-cli.py"
	@python3 "$(ROOT_DIR)/scripts/generate-bootstrap-reference.py"

docs-check:
	@python3 "$(ROOT_DIR)/docs/book/grammar/scripts/sync_grammar.py" --check
	@python3 "$(ROOT_DIR)/scripts/generate-bootstrap-cli.py" --check
	@python3 "$(ROOT_DIR)/scripts/generate-bootstrap-reference.py" --check
	@python3 "$(ROOT_DIR)/scripts/check-ebnf-frontend-sync.py"

check-ebnf-frontend-sync:
	@python3 "$(ROOT_DIR)/scripts/check-ebnf-frontend-sync.py"

portability:
	@python3 "$(ROOT_DIR)/scripts/check-portability.py" \
		$(if $(PORTABILITY_STRICT),--strict,)

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

sanitize-test:
	@$(MAKE) clean
	@$(MAKE) \
		CFLAGS="$(BASE_CFLAGS) $(STRICT_CFLAGS) $(SANITIZER_CFLAGS)" \
		LDFLAGS="$(LDFLAGS) $(SANITIZER_LDFLAGS)" \
		test-frontend

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
		"$(ROOT_DIR)/src/tests/import_export_coverage/coverage_chain_main.vit"

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
	grep -q "E0515" "$$tmp" || { \
		cat "$$tmp"; \
		exit 1; \
	}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/contracts_false.vit"

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
		"E0402" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	grep -q \
		"Glob import brings in" \
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
		"E0404" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_visibility/glob_private.vit"

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
		"E0404" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@printf '%s\n' "[SMOKE] import collision success cases"

	@tmp="$$(mktemp)"; \
	trap 'rm -f "$$tmp"' EXIT HUP INT TERM; \
	if "$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_collision/local_main.vit" \
		>"$$tmp" 2>&1; \
	then \
		cat "$$tmp"; \
		exit 1; \
	fi; \
	grep -q "E0402" "$$tmp" || { \
		cat "$$tmp"; \
		exit 1; \
	}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

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
		"E0402" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@printf '%s\n' "[SMOKE] import/export contract"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/import_export_coverage/module_path_unsupported.vit"

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
		"E0602" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	grep -q \
		"expected" \
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
		"E0602" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	grep -q \
		"expected" \
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
		"E0512" \
		"$$tmp" || { \
			cat "$$tmp"; \
			exit 1; \
		}; \
	grep -q \
		"invalid operand type" \
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
	grep -q "E0401" "$$tmp" || { \
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
	grep -q "E0201" "$$tmp" || { \
		cat "$$tmp"; \
		exit 1; \
	}; \
	rm -f "$$tmp"; \
	trap - EXIT HUP INT TERM

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/export_forms/reexport_module_alias_unsupported.vit"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/export_forms/reexport_symbol_unsupported.vit"

	@printf '%s\n' "[SMOKE] native builds"

	@"$(BIN)" compile \
		"$(ROOT_DIR)/examples/hello.vit" \
		-o "$(OUT_DIR)/hello"

	@"$(OUT_DIR)/hello"

	@"$(BIN)" compile \
		"$(ROOT_DIR)/src/tests/frontend_expr_stmt/expr_stmt_ok.vit" \
		-o "$(OUT_DIR)/expr_stmt_ok"

	@"$(OUT_DIR)/expr_stmt_ok"

	@"$(BIN)" compile \
		"$(ROOT_DIR)/src/tests/sema_operators/logical_ok.vit" \
		-o "$(OUT_DIR)/logical_ok"

	@"$(OUT_DIR)/logical_ok"

	@"$(BIN)" compile \
		"$(ROOT_DIR)/src/tests/grammar_alignment/let_mut_ok.vit" \
		-o "$(OUT_DIR)/let_mut_ok"

	@"$(OUT_DIR)/let_mut_ok"

	@"$(BIN)" compile \
		"$(ROOT_DIR)/src/tests/assignment_ok.vit" \
		-o "$(OUT_DIR)/assignment_ok"

	@"$(OUT_DIR)/assignment_ok"; \
	status=$$?; \
	test "$$status" -eq 0

	@"$(BIN)" compile \
		"$(ROOT_DIR)/src/tests/aggregate_runtime.vit" \
		-o "$(OUT_DIR)/aggregate_runtime"

	@"$(OUT_DIR)/aggregate_runtime"

	@"$(BIN)" compile \
		"$(ROOT_DIR)/src/tests/string_intrinsics_runtime.vit" \
		-o "$(OUT_DIR)/string_intrinsics_runtime"

	@"$(OUT_DIR)/string_intrinsics_runtime"

	@"$(BIN)" check \
		"$(ROOT_DIR)/src/tests/lazy_expressions_runtime.vit"

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
		"$(ROOT_DIR)/src/tests/import_export_coverage/coverage_chain_main.vit"

	@printf '%s\n' "[CORPUS] native builds"

	@"$(BIN)" compile \
		"$(ROOT_DIR)/examples/hello.vit" \
		-o "$(CORPUS_DIR)/hello"

	# The coverage chain intentionally contains nominally colliding type names
	# in separate modules.  The frontend checks it; native emission is kept to
	# the collision-free hello fixture until backend namespace mangling is wired
	# through the AST emitter.

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
		"E0402" \
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
		"E0602" \
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
		"  unicode-check  Verify the pinned Unicode XID table" \
		"  test-unicode-generation  Check deterministic Unicode table generation" \
		"  reentrancy-check Audit mutable file-scope API state" \
		"  alignment      Verify language/source alignment" \
		"  docs           Regenerate bootstrap token/diagnostic/option reference" \
		"  docs-check     Verify generated docs and EBNF/frontend synchronization" \
		"  portability    Probe available C17 compilers and Windows boundary" \
		"  smoke          Run compiler smoke/regression suite" \
		"  corpus         Build/check representative corpus" \
		"  test-frontend  Lexer/parser fuzz/stress, diagnostics, Unicode, object, imports, and bootstrap regressions" \
		"  test-config-fuzz  Fuzz configuration adapters" \
		"  test-robustness   Check allocation, size and path limits" \
		"  fuzz-continuous   Repeat lexer/parser/import/config fuzzing" \
		"  strict         Strict warning build" \
		"  debug          Debug build" \
		"  release        Optimized release build" \
		"  sanitize       ASan + UBSan build" \
		"  sanitize-test  ASan + UBSan frontend and fuzz tests" \
		"  test-cli-spec  Verify CLI table, help, parser and shell completion parity" \
		"  test-cli-differential  Compare C and native Vitte CLI parser behavior" \
		"  test-diagnostic-help   Check contextual help in terminal, JSON and SARIF" \
		"  test-diagnostic-parity Compare all diagnostic formats and explain codes" \
		"  asan           AddressSanitizer build" \
		"  ubsan          UndefinedBehaviorSanitizer build" \
		"  install        Install to repository bin/" \
		"  uninstall      Remove repository bin/vitte" \
		"  clean          Remove Vitte compiler output" \
		"  distclean      Remove complete target directory" \
		"  print-config   Display resolved build configuration" \
		"  print-sources  Display compiler source list" \
		"  print-headers  Display compiler header list"
