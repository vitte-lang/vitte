# =============================================================================
# Vitte — Root Makefile
# =============================================================================
#
# Repository layout:
#
#   src/
#     CMakeLists.txt
#     Makefile
#     runtime/
#     src/
#       api/
#       arena/
#       ast/
#       backend/c17/
#       builtin/
#       cli/
#       codegen/
#       config/
#       constant_fold/
#       diagnostic/
#       driver/
#       filesystem/
#       hir/
#       import/
#       ir/
#       lexer/
#       module/
#       parser/
#       printer/
#       scanner/
#       scope/
#       sema/
#       source/
#       symbol/
#       type/
#       unicode/
#       util/
#       main.c
#     tests/
#
# This Makefile is intended to be invoked from the repository root:
#
#   /Users/vincent/Documents/Github/vitte
#
# =============================================================================

PROJECT := Vitte
TARGET  := vitte

CC ?= cc
AR ?= ar
RANLIB ?= ranlib

PYTHON ?= python3

# -----------------------------------------------------------------------------
# Repository layout
# -----------------------------------------------------------------------------

VITTE_ROOT := src

SRC_ROOT     := $(VITTE_ROOT)/src
RUNTIME_ROOT := $(VITTE_ROOT)/runtime
TEST_DIR     := $(VITTE_ROOT)/tests
DOC_DIR      := $(VITTE_ROOT)/docs

# -----------------------------------------------------------------------------
# Build directories
# -----------------------------------------------------------------------------

BUILD   := build
OBJ_DIR := $(BUILD)/obj
DEP_DIR := $(BUILD)/dep
BIN_DIR := $(BUILD)/bin
TMP_DIR := $(BUILD)/tmp
LOG_DIR := $(BUILD)/logs

TARGET_BIN := $(BIN_DIR)/$(TARGET)

# -----------------------------------------------------------------------------
# Source discovery
# -----------------------------------------------------------------------------

SOURCES := $(shell find $(SRC_ROOT) -type f -name '*.c' | LC_ALL=C sort)

OBJECTS := $(patsubst $(SRC_ROOT)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS    := $(patsubst $(SRC_ROOT)/%.c,$(DEP_DIR)/%.d,$(SOURCES))

MAIN_SOURCE := $(SRC_ROOT)/main.c
MAIN_OBJECT := $(OBJ_DIR)/main.o

LIB_OBJECTS := $(filter-out $(MAIN_OBJECT),$(OBJECTS))

# -----------------------------------------------------------------------------
# Include paths
# -----------------------------------------------------------------------------
#
# IMPORTANT:
#
# Do NOT add:
#
#   -Isrc/include/vitte
#   -Iinclude/vitte
#
# or another directory capable of shadowing libc headers such as <string.h>.
#
# Headers are colocated with their implementation directories and source files
# should normally include sibling headers directly.
#
# SRC_ROOT is sufficient for module-qualified includes such as:
#
#   #include "diagnostic/diagnostic.h"
#   #include "parser/parser.h"
#
# Local "foo.h" includes are resolved relative to foo.c automatically.
#
# -----------------------------------------------------------------------------

CPPFLAGS := \
	-I$(SRC_ROOT) \
	-I$(VITTE_ROOT)

# -----------------------------------------------------------------------------
# Common compiler warnings
# -----------------------------------------------------------------------------

CFLAGS_COMMON := \
	-std=c17 \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Wconversion \
	-Wsign-conversion \
	-Wshadow \
	-Wcast-align \
	-Wwrite-strings \
	-Wundef \
	-Wformat \
	-Wstrict-prototypes \
	-Wold-style-definition \
	-Wimplicit-fallthrough \
	-Wvla \
	-Wpointer-arith \
	-Wbad-function-cast

# -----------------------------------------------------------------------------
# Build modes
# -----------------------------------------------------------------------------

CFLAGS_DEBUG := \
	-O0 \
	-g3 \
	-fno-omit-frame-pointer

CFLAGS_RELEASE := \
	-O2 \
	-DNDEBUG

CFLAGS_ASAN := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fsanitize=address,undefined

CFLAGS_UBSAN := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fsanitize=undefined

MODE ?= debug

LDFLAGS :=
LDLIBS   :=

# constant_fold and generated/runtime code may require libm on Unix.
UNAME_S := $(shell uname -s 2>/dev/null || echo unknown)

ifeq ($(UNAME_S),Linux)
	LDLIBS += -lm
endif

ifeq ($(MODE),release)
	CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_RELEASE)
else ifeq ($(MODE),asan)
	CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_ASAN)
	LDFLAGS += -fsanitize=address,undefined
else ifeq ($(MODE),ubsan)
	CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_UBSAN)
	LDFLAGS += -fsanitize=undefined
else
	CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_DEBUG)
endif

# -----------------------------------------------------------------------------
# Default build
# -----------------------------------------------------------------------------

.PHONY: all
all: $(TARGET_BIN)

$(TARGET_BIN): $(OBJECTS)
	@mkdir -p $(dir $@)
	@printf '\n[LD] %s\n' "$@"
	$(CC) $(LDFLAGS) $(OBJECTS) -o build/bin/vitte -lm
		$(OBJECTS) \
		$(LDFLAGS) \
		$(LDLIBS) \
		-o $@
	@printf '\nBuilt Vitte compiler:\n'
	@printf '  %s\n\n' "$@"

# -----------------------------------------------------------------------------
# Object compilation
# -----------------------------------------------------------------------------

$(OBJ_DIR)/%.o: $(SRC_ROOT)/%.c
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEP_DIR)/$*.d)
	@printf '[CC] %s\n' "$<"
	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		-MMD \
		-MP \
		-MF $(DEP_DIR)/$*.d \
		-c $< \
		-o $@

-include $(DEPS)

# -----------------------------------------------------------------------------
# Build modes
# -----------------------------------------------------------------------------

.PHONY: debug
debug:
	$(MAKE) MODE=debug all

.PHONY: release
release:
	$(MAKE) MODE=release all

.PHONY: asan
asan:
	$(MAKE) MODE=asan all

.PHONY: ubsan
ubsan:
	$(MAKE) MODE=ubsan all

# -----------------------------------------------------------------------------
# Compiler execution
# -----------------------------------------------------------------------------

.PHONY: run
run: $(TARGET_BIN)
	$(TARGET_BIN)

.PHONY: version
version: $(TARGET_BIN)
	$(TARGET_BIN) --version

.PHONY: cli-help
cli-help: $(TARGET_BIN)
	$(TARGET_BIN) --help

# -----------------------------------------------------------------------------
# Vitte frontend
# -----------------------------------------------------------------------------

.PHONY: compile
compile: $(TARGET_BIN)
ifndef FILE
	$(error FILE is required: make compile FILE=program.vit)
endif
	$(TARGET_BIN) compile "$(FILE)"

.PHONY: vitte-check
vitte-check: $(TARGET_BIN)
ifndef FILE
	$(error FILE is required: make vitte-check FILE=program.vit)
endif
	$(TARGET_BIN) check "$(FILE)"

.PHONY: test-cli-driver
test-cli-driver: $(TARGET_BIN)
	@mkdir -p $(BUILD)
	@set -e; \
	fixture="$(TEST_DIR)/grammar_alignment/let_mut_ok.vit"; \
	printf '[CLI] check and early-stop stages\n'; \
	$(TARGET_BIN) check "$$fixture" \
		>/dev/null \
		2>$(BUILD)/cli-check.stderr; \
	test ! -s $(BUILD)/cli-check.stderr; \
	$(TARGET_BIN) compile "$$fixture" \
		--emit-tokens \
		--stop-after-lexer \
		>$(BUILD)/cli-tokens.txt; \
	test -s $(BUILD)/cli-tokens.txt; \
	$(TARGET_BIN) compile "$$fixture" \
		--emit-ast \
		--stop-after-parser \
		>$(BUILD)/cli-ast.txt; \
	grep -q "AST root kind:" $(BUILD)/cli-ast.txt; \
	printf '[CLI] compile and run native programs\n'; \
	$(TARGET_BIN) compile "$$fixture" \
		-o "$(BUILD)/cli-native"; \
	"$(BUILD)/cli-native"; \
	$(TARGET_BIN) run "$$fixture" --quiet; \
	printf '[CLI] compile and run expo_math test declarations\n'; \
	$(TARGET_BIN) compile "$(TEST_DIR)/expo_math.vit" \
		-o "$(BUILD)/cli-expo-math"; \
	"$(BUILD)/cli-expo-math"; \
	printf '[CLI] compile and run game program\n'; \
	$(TARGET_BIN) compile "$(TEST_DIR)/game.vit" \
		-o "$(BUILD)/cli-game"; \
	printf '6\n' | $(TARGET_BIN) run "$(TEST_DIR)/game.vit" --quiet \
		>$(BUILD)/cli-game.stdout; \
	grep -q "PARTIE TERMINEE" $(BUILD)/cli-game.stdout

.PHONY: emit-c
emit-c: $(TARGET_BIN)
ifndef FILE
	$(error FILE is required: make emit-c FILE=program.vit)
endif
	@mkdir -p $(BUILD)
	$(TARGET_BIN) compile "$(FILE)" --emit-c -o "$(BUILD)/generated.c"
	@printf '\nGenerated C:\n'
	@printf '  %s\n' "$(BUILD)/generated.c"

# -----------------------------------------------------------------------------
# C source verification
# -----------------------------------------------------------------------------

.PHONY: check
check:
	@printf 'Checking Vitte C sources...\n\n'
	@set -e; \
	for file in $(SOURCES); do \
		printf '[CHECK] %s\n' "$$file"; \
		$(CC) \
			$(CPPFLAGS) \
			$(CFLAGS_COMMON) \
			-fsyntax-only \
			"$$file"; \
	done
	@printf '\nAll C translation units passed syntax checking.\n'

# -----------------------------------------------------------------------------
# Source manager test
# -----------------------------------------------------------------------------

SOURCE_TEST_BIN := $(BIN_DIR)/source_manager_test

.PHONY: test-source
test-source:
	@mkdir -p $(BIN_DIR)
	@printf '[TEST-CC] source_manager_test\n'
	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		$(TEST_DIR)/source_manager_test.c \
		$(SRC_ROOT)/source/source.c \
		$(SRC_ROOT)/api/error.c \
		$(LDFLAGS) \
		$(LDLIBS) \
		-o $(SOURCE_TEST_BIN)
	$(SOURCE_TEST_BIN)

# -----------------------------------------------------------------------------
# Diagnostic rendering
# -----------------------------------------------------------------------------

DIAGNOSTIC_RENDER_TEST_BIN := $(BIN_DIR)/diagnostic_render_test

.PHONY: test-diagnostic
test-diagnostic: $(TARGET_BIN)
	@mkdir -p $(BIN_DIR)
	@printf '[TEST-CC] diagnostic_render_test\n'
	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		$(TEST_DIR)/diagnostic_render_test.c \
		$(LIB_OBJECTS) \
		$(LDFLAGS) \
		$(LDLIBS) \
		-o $(DIAGNOSTIC_RENDER_TEST_BIN)
	$(DIAGNOSTIC_RENDER_TEST_BIN)
	@set +e; \
	output=`$(TARGET_BIN) parse \
		$(TEST_DIR)/diagnostic_parser_invalid.vit \
		--quiet --no-color 2>&1`; \
	status=$$?; \
	test "$$status" -eq 1 && \
	printf '%s\n' "$$output" | grep -F 'E0203' >/dev/null && \
	printf '%s\n' "$$output" | grep -F 'phase: parser' >/dev/null && \
	printf '%s\n' "$$output" | grep -F 'help: Add a valid expression' >/dev/null
	@set +e; \
	output=`$(TARGET_BIN) lex \
		$(TEST_DIR)/diagnostic_lexer_invalid.vit \
		--quiet --no-color 2>&1`; \
	status=$$?; \
	test "$$status" -eq 1 && \
	printf '%s\n' "$$output" | grep -F 'E0102' >/dev/null && \
	printf '%s\n' "$$output" | grep -F 'phase: lexer' >/dev/null && \
	printf '%s\n' "$$output" | grep -F 'help: Close the string' >/dev/null
	@set +e; \
	output=`$(TARGET_BIN) --not-a-valid-option 2>&1`; \
	status=$$?; \
	test "$$status" -eq 2 && \
	printf '%s\n' "$$output" | grep -F 'E0011' >/dev/null && \
	printf '%s\n' "$$output" | grep -F 'phase: driver' >/dev/null && \
	printf '%s\n' "$$output" | grep -F 'help: Check the option name' >/dev/null

# -----------------------------------------------------------------------------
# Diagnostic fuzz test
# -----------------------------------------------------------------------------

DIAGNOSTIC_FUZZ_TEST_BIN := $(BIN_DIR)/diagnostic_fuzz_test

.PHONY: test-diagnostic-fuzz
test-diagnostic-fuzz: $(TARGET_BIN)
	@mkdir -p $(BIN_DIR)
	@printf '[TEST-CC] diagnostic_fuzz_test\n'
	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		$(TEST_DIR)/diagnostic_fuzz_test.c \
		$(LIB_OBJECTS) \
		$(LDFLAGS) \
		$(LDLIBS) \
		-o $(DIAGNOSTIC_FUZZ_TEST_BIN)
	$(DIAGNOSTIC_FUZZ_TEST_BIN)

# -----------------------------------------------------------------------------
# Golden diagnostic tests
# -----------------------------------------------------------------------------

.PHONY: test-diagnostic-golden
test-diagnostic-golden: $(TARGET_BIN)
	@mkdir -p $(TMP_DIR)
	@set +e; \
	$(TARGET_BIN) compile \
		$(TEST_DIR)/golden/unknown_symbol.input \
		--stop-after-sema --quiet --no-color \
		>$(TMP_DIR)/diagnostic_unknown_symbol.stdout \
		2>$(TMP_DIR)/diagnostic_unknown_symbol.stderr; \
	status=$$?; \
	set -e; \
	test "$$status" -eq 1 && \
	awk 'NF { last = NR } { lines[NR] = $$0 } END { for (i = 1; i <= last; i++) print lines[i] }' \
		$(TMP_DIR)/diagnostic_unknown_symbol.stderr \
		>$(TMP_DIR)/diagnostic_unknown_symbol.normalized && \
	diff -u $(TEST_DIR)/golden/diagnostic_unknown_symbol.stderr \
		$(TMP_DIR)/diagnostic_unknown_symbol.normalized
	@set +e; \
	$(TARGET_BIN) compile \
		$(TEST_DIR)/sema_calls/local_param_mismatch.vit \
		--stop-after-sema --quiet --no-color \
		>$(TMP_DIR)/diagnostic_type_mismatch.stdout \
		2>$(TMP_DIR)/diagnostic_type_mismatch.stderr; \
	status=$$?; \
	set -e; \
	test "$$status" -eq 1 && \
	awk 'NF { last = NR } { lines[NR] = $$0 } END { for (i = 1; i <= last; i++) print lines[i] }' \
		$(TMP_DIR)/diagnostic_type_mismatch.stderr \
		>$(TMP_DIR)/diagnostic_type_mismatch.normalized && \
	diff -u $(TEST_DIR)/golden/diagnostic_type_mismatch.stderr \
		$(TMP_DIR)/diagnostic_type_mismatch.normalized
	@set +e; \
	$(TARGET_BIN) lex \
		$(TEST_DIR)/diagnostic_lexer_invalid.vit \
		--quiet --no-color \
		>$(TMP_DIR)/diagnostic_lexer_invalid.stdout \
		2>$(TMP_DIR)/diagnostic_lexer_invalid.stderr; \
	status=$$?; \
	set -e; \
	test "$$status" -eq 1 && \
	awk 'NF { last = NR } { lines[NR] = $$0 } END { for (i = 1; i <= last; i++) print lines[i] }' \
		$(TMP_DIR)/diagnostic_lexer_invalid.stderr \
		>$(TMP_DIR)/diagnostic_lexer_invalid.normalized && \
	diff -u $(TEST_DIR)/golden/diagnostic_lexer_invalid.stderr \
		$(TMP_DIR)/diagnostic_lexer_invalid.normalized

# -----------------------------------------------------------------------------
# Structured diagnostic formats
# -----------------------------------------------------------------------------

.PHONY: test-diagnostic-formats
test-diagnostic-formats: $(TARGET_BIN)
	@set +e; \
	output=`$(TARGET_BIN) check \
		$(TEST_DIR)/diagnostic_parser_invalid.vit \
		--diagnostic=json 2>&1`; \
	status=$$?; \
	test "$$status" -eq 1 && \
	printf '%s\n' "$$output" | grep -F '"schema": "vitte-diagnostics"' >/dev/null && \
	printf '%s\n' "$$output" | grep -F '"code": "E0203"' >/dev/null && \
	printf '%s\n' "$$output" | grep -F '"kind": "help"' >/dev/null && \
	test "$$(printf '%s\n' "$$output" | tail -n 1)" = '}'
	@set +e; \
	output=`$(TARGET_BIN) compile \
		$(TEST_DIR)/diagnostic_sema_multiple_invalid.vit \
		--stop-after-sema --quiet --no-color \
		--diagnostic=json 2>&1`; \
	status=$$?; \
	test "$$status" -eq 1 && \
	printf '%s\n' "$$output" | grep -F '"stored": 2' >/dev/null && \
	test "$$(printf '%s\n' "$$output" | grep -Fc '"code": "E0602"')" -eq 2 && \
	printf '%s\n' "$$output" | grep -F '"kind": "help"' >/dev/null && \
	test "$$(printf '%s\n' "$$output" | tail -n 1)" = '}'
	@set +e; \
	output=`$(TARGET_BIN) check \
		$(TEST_DIR)/diagnostic_parser_invalid.vit \
		--diagnostic=sarif 2>&1`; \
	status=$$?; \
	test "$$status" -eq 1 && \
	printf '%s\n' "$$output" | grep -F '"version": "2.1.0"' >/dev/null && \
	printf '%s\n' "$$output" | grep -F '"ruleId": "E0203"' >/dev/null && \
	test "$$(printf '%s\n' "$$output" | tail -n 1)" = '}'
	@set +e; \
	output=`$(TARGET_BIN) compile \
		$(TEST_DIR)/diagnostic_sema_multiple_invalid.vit \
		--stop-after-sema --quiet --no-color \
		--diagnostic=sarif 2>&1`; \
	status=$$?; \
	test "$$status" -eq 1 && \
	test "$$(printf '%s\n' "$$output" | grep -Fc '"ruleId": "E0602"')" -eq 2 && \
	test "$$(printf '%s\n' "$$output" | tail -n 1)" = '}'
	@set +e; \
	output=`$(TARGET_BIN) --diagnostic=json --not-a-valid-option 2>&1`; \
	status=$$?; \
	test "$$status" -eq 2 && \
	printf '%s\n' "$$output" | grep -F '"code": "E0011"' >/dev/null && \
	printf '%s\n' "$$output" | grep -F '"schema": "vitte-diagnostics"' >/dev/null && \
	! printf '%s\n' "$$output" | grep -F 'Usage:' >/dev/null && \
	test "$$(printf '%s\n' "$$output" | tail -n 1)" = '}'
	@set +e; \
	output=`$(TARGET_BIN) check \
		$(TEST_DIR)/diagnostic_missing_source.vit \
		--diagnostic=json 2>&1`; \
	status=$$?; \
	test "$$status" -eq 1 && \
	printf '%s\n' "$$output" | grep -F '"schema": "vitte-diagnostics"' >/dev/null && \
	printf '%s\n' "$$output" | grep -F '"code": "E0005"' >/dev/null && \
	test "$$(printf '%s\n' "$$output" | tail -n 1)" = '}'

# -----------------------------------------------------------------------------
# Source map / generated C
# -----------------------------------------------------------------------------

SOURCE_MAP_INPUT ?= $(TEST_DIR)/assignment_ok.vit
SOURCE_MAP_C     := $(BUILD)/source-map.c

.PHONY: test-source-map
test-source-map: $(TARGET_BIN)
	@printf '[SKIP] source-map generation requires the disconnected code-generation pipeline\n'

# -----------------------------------------------------------------------------
# Corpus
# -----------------------------------------------------------------------------

.PHONY: test-corpus
test-corpus: $(TARGET_BIN)
	$(PYTHON) \
		$(TEST_DIR)/run_corpus.py \
		--compiler $(TARGET_BIN)

.PHONY: test-unavailable
test-unavailable: $(TARGET_BIN)
	$(PYTHON) \
		$(TEST_DIR)/run_corpus.py \
		--compiler $(TARGET_BIN) \
		--include-unavailable

# -----------------------------------------------------------------------------
# Grammar alignment (direct lexer/parser API; independent of the CLI driver)
# -----------------------------------------------------------------------------

GRAMMAR_TEST_BIN := $(BIN_DIR)/grammar_parser_test
GRAMMAR_FIXTURES := \
	$(TEST_DIR)/grammar_alignment/let_mut_ok.vit \
	$(TEST_DIR)/grammar_alignment/ref_param_type_marker_ok.vit \
	$(TEST_DIR)/expression_literals_parse.vit \
	$(TEST_DIR)/form_parse.vit \
	$(TEST_DIR)/pick_parse.vit

$(GRAMMAR_TEST_BIN): $(TEST_DIR)/grammar_parser_test.c $(LIB_OBJECTS)
	@mkdir -p $(BIN_DIR)
	@printf '[TEST-CC] grammar_parser_test\n'
	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		$(TEST_DIR)/grammar_parser_test.c \
		$(LIB_OBJECTS) \
		$(LDFLAGS) \
		$(LDLIBS) \
		-o $@

.PHONY: test-grammar
test-grammar: check-ebnf-frontend-sync $(GRAMMAR_TEST_BIN)
	$(GRAMMAR_TEST_BIN) $(GRAMMAR_FIXTURES)

# -----------------------------------------------------------------------------
# Lexer token contract and frontend fuzzing
# -----------------------------------------------------------------------------

FRONTEND_TOKEN_TEST_BIN := $(BIN_DIR)/frontend_token_test
FRONTEND_FUZZ_TEST_BIN := $(BIN_DIR)/frontend_fuzz_test

$(FRONTEND_TOKEN_TEST_BIN): $(TEST_DIR)/frontend_token_test.c $(SRC_ROOT)/lexer/lexer.c
	@mkdir -p $(BIN_DIR)
	@printf '[TEST-CC] frontend_token_test\n'
	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		$(TEST_DIR)/frontend_token_test.c \
		$(SRC_ROOT)/lexer/lexer.c \
		$(LDFLAGS) \
		$(LDLIBS) \
		-o $@

$(FRONTEND_FUZZ_TEST_BIN): $(TEST_DIR)/frontend_fuzz_test.c $(LIB_OBJECTS)
	@mkdir -p $(BIN_DIR)
	@printf '[TEST-CC] frontend_fuzz_test\n'
	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		$(TEST_DIR)/frontend_fuzz_test.c \
		$(LIB_OBJECTS) \
		$(LDFLAGS) \
		$(LDLIBS) \
		-o $@

.PHONY: test-frontend-tokens
test-frontend-tokens: $(FRONTEND_TOKEN_TEST_BIN)
	$(FRONTEND_TOKEN_TEST_BIN)

.PHONY: test-frontend-fuzz
test-frontend-fuzz: $(FRONTEND_FUZZ_TEST_BIN)
	$(FRONTEND_FUZZ_TEST_BIN)

.PHONY: check-ebnf-frontend-sync
check-ebnf-frontend-sync:
	$(PYTHON) scripts/check-ebnf-frontend-sync.py

# -----------------------------------------------------------------------------
# Semantic tests
# -----------------------------------------------------------------------------

.PHONY: test-sema
test-sema: $(TARGET_BIN)
	$(TARGET_BIN) compile \
		$(TEST_DIR)/sema_calls/local_param_ok.vit \
		--stop-after-sema \
		--quiet

	$(TARGET_BIN) compile \
		$(TEST_DIR)/contracts_ok.vit \
		--stop-after-sema \
		--quiet

	$(TARGET_BIN) compile \
		$(TEST_DIR)/sema_calls/import_param_ok.vit \
		--stop-after-sema \
		--quiet

	$(TARGET_BIN) compile \
		$(TEST_DIR)/import_visibility/module_ok.vit \
		--stop-after-sema \
		--quiet

	$(TARGET_BIN) compile \
		$(TEST_DIR)/grammar_alignment/ref_param_type_marker_ok.vit \
		--stop-after-sema \
		--quiet

	@printf '%s\n' \
		'SKIP import_collision/modules_main.vit: qualified colliding imports are not semantically resolved yet'

	$(TARGET_BIN) compile \
		$(TEST_DIR)/sema_calls/u64_literal_and_recursion_ok.vit \
		--stop-after-sema \
		--quiet

	$(TARGET_BIN) compile \
		$(TEST_DIR)/sema_calls/u8_form_members_and_string_pointer_ok.vit \
		--stop-after-sema \
		--quiet

	$(TARGET_BIN) compile \
		$(TEST_DIR)/sema_operators/logical_ok.vit \
		--stop-after-sema \
		--quiet

	@set +e; \
	output=`$(TARGET_BIN) compile \
		$(TEST_DIR)/import_visibility/glob_private.vit \
		--stop-after-sema \
		--quiet --no-color 2>&1`; \
	status=$$?; \
	set -e; \
	test "$$status" -eq 1 && \
	printf '%s\n' "$$output" | grep -F 'E0404' >/dev/null && \
	! printf '%s\n' "$$output" | grep -F 'E0504' >/dev/null

	@set +e; \
	output=`$(TARGET_BIN) compile \
		$(TEST_DIR)/sema_calls/local_param_mismatch.vit \
		--stop-after-sema \
		--quiet --no-color 2>&1`; \
	status=$$?; \
	set -e; \
	test "$$status" -eq 1 && \
	printf '%s\n' "$$output" | grep -F 'E0602' >/dev/null && \
	printf '%s\n' "$$output" | grep -F 'expected i64, found string' >/dev/null && \
	printf '%s\n' "$$output" | grep -F 'help: Make the expression' >/dev/null

	@set +e; \
	output=`$(TARGET_BIN) compile \
		$(TEST_DIR)/sema_calls/signed_variable_to_u64_mismatch.vit \
		--stop-after-sema \
		--quiet --no-color 2>&1`; \
	status=$$?; \
	set -e; \
	test "$$status" -eq 1 && \
	printf '%s\n' "$$output" | grep -F 'E0602' >/dev/null && \
	printf '%s\n' "$$output" | grep -F 'help: Make the expression' >/dev/null

# -----------------------------------------------------------------------------
# Import/export tests
# -----------------------------------------------------------------------------

IMPORT_GRAPH_TEST_BIN := $(BIN_DIR)/import_graph_test

$(IMPORT_GRAPH_TEST_BIN): \
	$(TEST_DIR)/import_graph_test.c \
	$(SRC_ROOT)/import/import.c \
	$(SRC_ROOT)/import/import.h
	@mkdir -p $(BIN_DIR)
	@printf '[TEST-CC] import_graph_test\n'
	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		$(TEST_DIR)/import_graph_test.c \
		$(SRC_ROOT)/import/import.c \
		$(LDFLAGS) \
		$(LDLIBS) \
		-o $@

.PHONY: test-imports
test-imports: $(IMPORT_GRAPH_TEST_BIN)
	$(IMPORT_GRAPH_TEST_BIN)

# -----------------------------------------------------------------------------
# Large-source compiler stress tests
# -----------------------------------------------------------------------------

SCALE_COMPILER ?= $(TARGET_BIN)
SCALE_TIMEOUT ?= 180
SCALE_SOURCE_MIB ?= 48
SCALE_DECLARATIONS ?= 10000

.PHONY: test-scale
test-scale: $(SCALE_COMPILER)
	VITTE_BIN="$(SCALE_COMPILER)" \
		VITTE_SCALE_TIMEOUT="$(SCALE_TIMEOUT)" \
		VITTE_SCALE_SOURCE_MIB="$(SCALE_SOURCE_MIB)" \
		VITTE_SCALE_DECLARATIONS="$(SCALE_DECLARATIONS)" \
		$(PYTHON) $(TEST_DIR)/compiler_scale_test.py

# -----------------------------------------------------------------------------
# Frontend expression statements
# -----------------------------------------------------------------------------

.PHONY: test-frontend
test-frontend: $(TARGET_BIN)
	$(TARGET_BIN) check \
		$(TEST_DIR)/frontend_expr_stmt/expr_stmt_ok.vit \
		--quiet

# -----------------------------------------------------------------------------
# Runtime tests
# -----------------------------------------------------------------------------

RUNTIME_CANDIDATES := \
	$(TEST_DIR)/aggregate_runtime.vit \
	$(TEST_DIR)/contracts_ok.vit \
	$(TEST_DIR)/ir_declaration_runtime.vit \
	$(TEST_DIR)/llvm_array_runtime.vit \
	$(TEST_DIR)/llvm_imports_runtime.vit \
	$(TEST_DIR)/llvm_integer_width_runtime.vit \
	$(TEST_DIR)/llvm_pick_codegen_runtime.vit \
	$(TEST_DIR)/mir_loop_targets_runtime.vit \
	$(TEST_DIR)/streaming_summary_runtime.vit \
	$(TEST_DIR)/string_intrinsics_runtime.vit
# Keep this list empty when every runtime fixture is executable.  Individual
# temporary gaps may still be listed here while a backend is being ported.
RUNTIME_KNOWN_GAPS :=
RUNTIME_EXECUTABLE_CANDIDATES := \
	$(filter-out $(RUNTIME_KNOWN_GAPS),$(RUNTIME_CANDIDATES))
RUNTIME_TESTS := $(wildcard $(RUNTIME_EXECUTABLE_CANDIDATES))
RUNTIME_MISSING := $(filter-out $(RUNTIME_TESTS),$(RUNTIME_EXECUTABLE_CANDIDATES))

.PHONY: test-runtime
test-runtime: $(TARGET_BIN)
	@set -e; \
	for file in $(RUNTIME_MISSING); do \
		printf '[SKIP] %s (fixture absent from this checkout)\n' "$$file"; \
	done; \
	for file in $(RUNTIME_KNOWN_GAPS); do \
		if test -f "$$file"; then \
			printf '[KNOWN-GAP] %s (required frontend/compiler support unavailable)\n' "$$file"; \
		else \
			printf '[SKIP] %s (fixture absent from this checkout)\n' "$$file"; \
		fi; \
	done; \
	for file in $(RUNTIME_TESTS); do \
		printf '[RUNTIME] %s\n' "$$file"; \
		$(TARGET_BIN) run "$$file"; \
	done

# -----------------------------------------------------------------------------
# Complete test suite
# -----------------------------------------------------------------------------

.PHONY: test
test: \
	test-cli-driver \
	test-source \
	test-diagnostic \
	test-diagnostic-fuzz \
	test-diagnostic-golden \
	test-diagnostic-formats \
	test-source-map \
	test-grammar \
	test-frontend-tokens \
	test-frontend-fuzz \
	test-sema \
	test-imports \
	test-scale \
	test-frontend \
	test-corpus
	@printf '\nVitte test suite completed successfully.\n'

# -----------------------------------------------------------------------------
# Sanitizers
# -----------------------------------------------------------------------------

.PHONY: test-sanitizers
test-sanitizers:
	$(MAKE) clean
	$(MAKE) MODE=asan -j4 all
	$(MAKE) MODE=asan \
		test-source \
		test-diagnostic \
		test-diagnostic-fuzz \
		test-frontend-tokens \
		test-frontend-fuzz \
		test-diagnostic-golden
	@printf '\nASan/UBSan tests completed successfully.\n'

# -----------------------------------------------------------------------------
# Strict release verification
# -----------------------------------------------------------------------------

.PHONY: verify
verify:
	$(MAKE) clean
	$(MAKE) MODE=debug all
	$(MAKE) check
	$(MAKE) test
	$(MAKE) clean
	$(MAKE) MODE=release all
	$(MAKE) version
	$(MAKE) cli-help
	@printf '\nVitte verification completed successfully.\n'

# -----------------------------------------------------------------------------
# Build information
# -----------------------------------------------------------------------------

.PHONY: info
info:
	@printf 'Project:        %s\n' "$(PROJECT)"
	@printf 'Target:         %s\n' "$(TARGET_BIN)"
	@printf 'Compiler:       %s\n' "$(CC)"
	@printf 'Mode:           %s\n' "$(MODE)"
	@printf 'Host OS:        %s\n' "$(UNAME_S)"
	@printf 'Vitte root:     %s\n' "$(VITTE_ROOT)"
	@printf 'Source root:    %s\n' "$(SRC_ROOT)"
	@printf 'Runtime root:   %s\n' "$(RUNTIME_ROOT)"
	@printf 'Test root:      %s\n' "$(TEST_DIR)"
	@printf 'Build root:     %s\n' "$(BUILD)"
	@printf 'C sources:      %s\n' "$(words $(SOURCES))"
	@printf 'Objects:        %s\n' "$(words $(OBJECTS))"
	@printf 'Test .vit:      %s\n' \
		"$(shell find $(TEST_DIR) -type f -name '*.vit' | wc -l | tr -d ' ')"

# -----------------------------------------------------------------------------
# Binary information
# -----------------------------------------------------------------------------

.PHONY: binary-info
binary-info: $(TARGET_BIN)
	@printf 'Binary:\n'
	@ls -lh $(TARGET_BIN)
	@printf '\nFile information:\n'
	@file $(TARGET_BIN)
	@printf '\nVersion:\n'
	@$(TARGET_BIN) --version

# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

.PHONY: install
install: $(TARGET_BIN)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 \
		$(TARGET_BIN) \
		$(DESTDIR)$(BINDIR)/$(TARGET)
	$(MAKE) -C modules/json install \
		PREFIX="$(PREFIX)" \
		DESTDIR="$(DESTDIR)"

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	$(MAKE) -C modules/json uninstall \
		PREFIX="$(PREFIX)" \
		DESTDIR="$(DESTDIR)"

# -----------------------------------------------------------------------------
# Cleaning
# -----------------------------------------------------------------------------

.PHONY: clean
clean:
	rm -rf $(BUILD)

.PHONY: clean-target
clean-target:
	rm -rf target

.PHONY: distclean
distclean: clean clean-target

# =============================================================================
# macOS builds
# =============================================================================

MACOS_CC ?= clang

MACOS_ARM64_DIR    := target/macos-arm64
MACOS_X86_64_DIR   := target/macos-x86_64
MACOS_UNIVERSAL_DIR := target/universal

MACOS_ARM64_BIN    := $(MACOS_ARM64_DIR)/vitte
MACOS_X86_64_BIN   := $(MACOS_X86_64_DIR)/vitte
MACOS_UNIVERSAL_BIN := $(MACOS_UNIVERSAL_DIR)/vitte

# Historical Mac OS X 10.4 / iMac 2006 profile.  The compiler and SDK are
# intentionally overridable because modern Xcode installations no longer ship
# an i386-capable toolchain or the 10.4 SDK.
MACOS2006_DIR := target/macos2006-i386
MACOS2006_BIN := $(MACOS2006_DIR)/vitte
MACOS2006_CC ?= gcc-4.0
MACOS2006_SDK ?= MacOSX10.4u.sdk
MACOS2006_DEPLOYMENT_TARGET ?= 10.4

ifneq ($(strip $(VITTE_MACOS_LEGACY_CC)),)
MACOS2006_CC := $(VITTE_MACOS_LEGACY_CC)
endif
ifneq ($(strip $(VITTE_MACOS_LEGACY_SDK)),)
MACOS2006_SDK := $(VITTE_MACOS_LEGACY_SDK)
endif
ifneq ($(strip $(MACOSX_DEPLOYMENT_TARGET)),)
MACOS2006_DEPLOYMENT_TARGET := $(MACOSX_DEPLOYMENT_TARGET)
endif
ifneq ($(strip $(VITTE_MACOS_LEGACY_OUT)),)
MACOS2006_BIN := $(VITTE_MACOS_LEGACY_OUT)
endif

MACOS_COMMON_FLAGS := \
	$(CPPFLAGS) \
	$(CFLAGS_COMMON) \
	-O2 \
	-DNDEBUG

.PHONY: macos-arm64-bin
macos-arm64-bin:
	@printf 'Building Vitte for macOS arm64...\n'
	@mkdir -p $(MACOS_ARM64_DIR)
	$(MACOS_CC) \
		$(MACOS_COMMON_FLAGS) \
		-arch arm64 \
		-mmacosx-version-min=11.0 \
		$(SOURCES) \
		$(LDLIBS) \
		-o $(MACOS_ARM64_BIN)
	@printf 'Architectures: '
	@lipo -archs $(MACOS_ARM64_BIN)
	@printf 'Built: %s\n' "$(MACOS_ARM64_BIN)"

.PHONY: macos-x86_64-bin
macos-x86_64-bin:
	@printf 'Building Vitte for macOS x86_64...\n'
	@mkdir -p $(MACOS_X86_64_DIR)
	$(MACOS_CC) \
		$(MACOS_COMMON_FLAGS) \
		-arch x86_64 \
		-mmacosx-version-min=10.13 \
		$(SOURCES) \
		$(LDLIBS) \
		-o $(MACOS_X86_64_BIN)
	@printf 'Architectures: '
	@lipo -archs $(MACOS_X86_64_BIN)
	@printf 'Built: %s\n' "$(MACOS_X86_64_BIN)"

.PHONY: macos-universal-bin
macos-universal-bin: macos-arm64-bin macos-x86_64-bin
	@printf 'Creating universal Vitte binary...\n'
	@mkdir -p $(MACOS_UNIVERSAL_DIR)
	lipo -create \
		$(MACOS_ARM64_BIN) \
		$(MACOS_X86_64_BIN) \
		-output $(MACOS_UNIVERSAL_BIN)
	@printf 'Architectures: '
	@lipo -archs $(MACOS_UNIVERSAL_BIN)
	@printf 'Binary: '
	@file $(MACOS_UNIVERSAL_BIN)
	@printf 'Built: %s\n' "$(MACOS_UNIVERSAL_BIN)"

.PHONY: macos2006-i386-bin
macos2006-i386-bin:
	@printf 'Building Vitte for Mac OS X 2006 i386...\n'
	@mkdir -p $(dir $(MACOS2006_BIN))
	$(MACOS2006_CC) \
		$(MACOS_COMMON_FLAGS) \
		-arch i386 \
		-isysroot $(MACOS2006_SDK) \
		-mmacosx-version-min=$(MACOS2006_DEPLOYMENT_TARGET) \
		$(SOURCES) \
		$(LDLIBS) \
		-o $(MACOS2006_BIN)
	@printf 'Architectures: '
	@lipo -archs $(MACOS2006_BIN)
	@printf 'Built: %s\n' "$(MACOS2006_BIN)"

.PHONY: macos-verify
macos-verify: macos-arm64-bin
	@printf '\nVerifying macOS arm64 binary...\n'
	@file $(MACOS_ARM64_BIN)
	@lipo -archs $(MACOS_ARM64_BIN)
	@$(MACOS_ARM64_BIN) --version
	@$(MACOS_ARM64_BIN) --help >/dev/null
	@printf 'macOS arm64 binary verified.\n'

# =============================================================================
# Help
# =============================================================================

.PHONY: help
help:
	@printf '%s\n' \
		'Vitte build system' \
		'' \
		'Build:' \
		'  make                         Build debug compiler' \
		'  make debug                   Build debug compiler' \
		'  make release                 Build optimized compiler' \
		'  make asan                    Build with ASan + UBSan' \
		'  make ubsan                   Build with UBSan' \
		'' \
		'Compiler:' \
		'  make run                     Run compiler' \
		'  make version                 Run vitte --version' \
		'  make cli-help                Run vitte --help' \
		'  make compile FILE=x.vit      Run vitte build' \
		'  make vitte-check FILE=x.vit  Run vitte check' \
		'  make emit-c FILE=x.vit       Generate C17 source' \
		'' \
		'Verification:' \
		'  make check                   Syntax-check every C source' \
		'  make test                    Run complete test suite' \
		'  make test-corpus             Run corpus tests' \
		'  make test-grammar            Run grammar tests' \
		'  make test-sema               Run semantic tests' \
		'  make test-imports            Run import/export tests' \
		'  make test-scale              Compile large Vitte source stress cases' \
		'  make test-runtime            Build runtime test programs' \
		'  make test-sanitizers         Run sanitizer tests' \
		'  make verify                  Debug + tests + release verification' \
		'' \
		'macOS:' \
		'  make macos-arm64-bin         Build native Apple Silicon binary' \
		'  make macos-x86_64-bin        Build Intel macOS binary' \
		'  make macos-universal-bin     Build universal arm64+x86_64 binary' \
		'  make macos-verify            Verify Apple Silicon binary' \
		'' \
		'Utility:' \
		'  make info                    Show build information' \
		'  make binary-info             Inspect generated compiler' \
		'  make install                 Install Vitte' \
		'  make uninstall               Uninstall Vitte' \
		'  make clean                   Remove build/' \
		'  make clean-target            Remove target/' \
		'  make distclean               Remove all generated build files'
