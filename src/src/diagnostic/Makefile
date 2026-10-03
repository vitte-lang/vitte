# =============================================================================
# Vitte — Diagnostic subsystem
# =============================================================================
#
# This Makefile builds the diagnostic subsystem as a standalone static library
# and provides strict development, sanitizer, analysis and maintenance targets.
#
# Main project builds may still use the root CMake configuration.
#
# Usage:
#
#   make
#   make debug
#   make release
#   make check
#   make sanitize
#   make clean
#
# =============================================================================

SHELL := /bin/sh

# -----------------------------------------------------------------------------
# Toolchain
# -----------------------------------------------------------------------------

CC      ?= cc
AR      ?= ar
RANLIB  ?= ranlib
RM      ?= rm -f
MKDIR_P ?= mkdir -p

# -----------------------------------------------------------------------------
# Project
# -----------------------------------------------------------------------------

PROJECT := vitte-diagnostic

STATIC_LIBRARY := lib$(PROJECT).a

# -----------------------------------------------------------------------------
# Directories
# -----------------------------------------------------------------------------

THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
SRC_ROOT := $(abspath $(THIS_DIR)/..)
PROJECT_ROOT := $(abspath $(THIS_DIR)/../..)

BUILD_DIR ?= $(THIS_DIR)/build
OBJ_DIR   := $(BUILD_DIR)/obj
DEP_DIR   := $(BUILD_DIR)/dep
LIB_DIR   := $(BUILD_DIR)/lib

TARGET := $(LIB_DIR)/$(STATIC_LIBRARY)

# -----------------------------------------------------------------------------
# Include paths
# -----------------------------------------------------------------------------

CPPFLAGS += \
	-I$(THIS_DIR) \
	-I$(SRC_ROOT) \
	-I$(PROJECT_ROOT) \
	-I$(PROJECT_ROOT)/include \
	-I$(PROJECT_ROOT)/include/vitte

# -----------------------------------------------------------------------------
# Language
# -----------------------------------------------------------------------------

CSTD ?= c17

CFLAGS_BASE := \
	-std=$(CSTD) \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Wshadow \
	-Wconversion \
	-Wsign-conversion \
	-Wcast-align \
	-Wcast-qual \
	-Wformat=2 \
	-Wundef \
	-Wwrite-strings \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wmissing-declarations \
	-Wswitch-enum \
	-Wswitch-default \
	-Wpointer-arith \
	-Wbad-function-cast \
	-Wstrict-overflow=5 \
	-Wnull-dereference \
	-Wdouble-promotion \
	-Wvla \
	-Walloca \
	-Wunused \
	-Wuninitialized

CFLAGS ?=

# -----------------------------------------------------------------------------
# Dependency generation
# -----------------------------------------------------------------------------

DEPFLAGS = -MMD -MP -MF $(DEP_DIR)/$*.d

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

SOURCES := \
	diagnostic.c \
	registry.c \
	engine.c \
	span.c \
	label.c \
	suggestion.c \
	cascade.c \
	render_terminal.c \
	render_json.c \
	render_sarif.c \
	render_lsp.c \
	source_map.c \
	explain.c

# -----------------------------------------------------------------------------
# Optional diagnostic modules
# -----------------------------------------------------------------------------
#
# Add future modules here when implemented:
#
#   code.c
#   severity.c
#   cause.c
#   fixit.c
#   applicability.c
#   provenance.c
#   source_context.c
#

# -----------------------------------------------------------------------------
# Objects / dependencies
# -----------------------------------------------------------------------------

OBJECTS := $(SOURCES:%.c=$(OBJ_DIR)/%.o)
DEPS    := $(SOURCES:%.c=$(DEP_DIR)/%.d)

# -----------------------------------------------------------------------------
# Build modes
# -----------------------------------------------------------------------------

DEBUG_FLAGS := \
	-O0 \
	-g3 \
	-fno-omit-frame-pointer

RELEASE_FLAGS := \
	-O2 \
	-DNDEBUG

STRICT_FLAGS := \
	-Werror

# -----------------------------------------------------------------------------
# Sanitizers
# -----------------------------------------------------------------------------

SANITIZE_FLAGS := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fsanitize=address,undefined

SANITIZE_LDFLAGS := \
	-fsanitize=address,undefined

# -----------------------------------------------------------------------------
# Default
# -----------------------------------------------------------------------------

.PHONY: all

all: debug

# =============================================================================
# Debug
# =============================================================================

.PHONY: debug

debug: CFLAGS += $(CFLAGS_BASE) $(DEBUG_FLAGS)
debug: $(TARGET)

# =============================================================================
# Release
# =============================================================================

.PHONY: release

release: CFLAGS += $(CFLAGS_BASE) $(RELEASE_FLAGS)
release: $(TARGET)

# =============================================================================
# Strict
# =============================================================================

.PHONY: strict

strict: CFLAGS += $(CFLAGS_BASE) $(DEBUG_FLAGS) $(STRICT_FLAGS)
strict: $(TARGET)

# =============================================================================
# Static library
# =============================================================================

$(TARGET): $(OBJECTS) | $(LIB_DIR)
	@printf '  AR      %s\n' "$@"
	@$(AR) rcs "$@" $(OBJECTS)
	@$(RANLIB) "$@"
	@printf '\n'
	@printf 'Built: %s\n' "$@"

# =============================================================================
# Compilation
# =============================================================================

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR) $(DEP_DIR)
	@printf '  CC      %s\n' "$<"
	@$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		$(DEPFLAGS) \
		-c "$<" \
		-o "$@"

# =============================================================================
# Directories
# =============================================================================

$(BUILD_DIR):
	@$(MKDIR_P) "$@"

$(OBJ_DIR): | $(BUILD_DIR)
	@$(MKDIR_P) "$@"

$(DEP_DIR): | $(BUILD_DIR)
	@$(MKDIR_P) "$@"

$(LIB_DIR): | $(BUILD_DIR)
	@$(MKDIR_P) "$@"

# =============================================================================
# Dependency files
# =============================================================================

-include $(DEPS)

# =============================================================================
# Compile-only verification
# =============================================================================

.PHONY: check

check:
	@printf 'Checking diagnostic sources...\n'
	@set -e; \
	for source in $(SOURCES); do \
		printf '  CHECK   %s\n' "$$source"; \
		$(CC) \
			$(CPPFLAGS) \
			$(CFLAGS_BASE) \
			-fsyntax-only \
			"$$source"; \
	done
	@printf 'Diagnostic source check completed.\n'

# =============================================================================
# Strict compile verification
# =============================================================================

.PHONY: check-strict

check-strict:
	@printf 'Strict checking diagnostic sources...\n'
	@set -e; \
	for source in $(SOURCES); do \
		printf '  CHECK   %s\n' "$$source"; \
		$(CC) \
			$(CPPFLAGS) \
			$(CFLAGS_BASE) \
			$(STRICT_FLAGS) \
			-fsyntax-only \
			"$$source"; \
	done
	@printf 'Strict diagnostic source check completed.\n'

# =============================================================================
# Sanitizer build
# =============================================================================

.PHONY: sanitize

sanitize:
	@$(MAKE) clean
	@$(MAKE) \
		CFLAGS="$(CFLAGS_BASE) $(SANITIZE_FLAGS)" \
		LDFLAGS="$(SANITIZE_LDFLAGS)" \
		$(TARGET)

# =============================================================================
# AddressSanitizer
# =============================================================================

.PHONY: asan

asan:
	@$(MAKE) clean
	@$(MAKE) \
		CFLAGS="$(CFLAGS_BASE) -O1 -g3 -fno-omit-frame-pointer -fsanitize=address" \
		LDFLAGS="-fsanitize=address" \
		$(TARGET)

# =============================================================================
# UndefinedBehaviorSanitizer
# =============================================================================

.PHONY: ubsan

ubsan:
	@$(MAKE) clean
	@$(MAKE) \
		CFLAGS="$(CFLAGS_BASE) -O1 -g3 -fno-omit-frame-pointer -fsanitize=undefined" \
		LDFLAGS="-fsanitize=undefined" \
		$(TARGET)

# =============================================================================
# Clang-specific diagnostics
# =============================================================================

.PHONY: clang-check

clang-check:
	@command -v clang >/dev/null 2>&1 || { \
		printf 'clang not found\n' >&2; \
		exit 1; \
	}
	@printf 'Checking with clang...\n'
	@set -e; \
	for source in $(SOURCES); do \
		printf '  CLANG   %s\n' "$$source"; \
		clang \
			$(CPPFLAGS) \
			$(CFLAGS_BASE) \
			-fsyntax-only \
			"$$source"; \
	done

# =============================================================================
# GCC-specific diagnostics
# =============================================================================

.PHONY: gcc-check

gcc-check:
	@command -v gcc >/dev/null 2>&1 || { \
		printf 'gcc not found\n' >&2; \
		exit 1; \
	}
	@printf 'Checking with GCC...\n'
	@set -e; \
	for source in $(SOURCES); do \
		printf '  GCC     %s\n' "$$source"; \
		gcc \
			$(CPPFLAGS) \
			$(CFLAGS_BASE) \
			-fsyntax-only \
			"$$source"; \
	done

# =============================================================================
# Static analysis — clang
# =============================================================================

.PHONY: analyze

analyze:
	@command -v clang >/dev/null 2>&1 || { \
		printf 'clang not found\n' >&2; \
		exit 1; \
	}
	@printf 'Running clang static analyzer...\n'
	@set -e; \
	for source in $(SOURCES); do \
		printf '  ANALYZE %s\n' "$$source"; \
		clang \
			--analyze \
			$(CPPFLAGS) \
			-std=$(CSTD) \
			"$$source"; \
	done

# =============================================================================
# clang-tidy
# =============================================================================

.PHONY: tidy

tidy:
	@command -v clang-tidy >/dev/null 2>&1 || { \
		printf 'clang-tidy not found\n' >&2; \
		exit 1; \
	}
	@set -e; \
	for source in $(SOURCES); do \
		printf '  TIDY    %s\n' "$$source"; \
		clang-tidy \
			"$$source" \
			-- \
			$(CPPFLAGS) \
			-std=$(CSTD); \
	done

# =============================================================================
# cppcheck
# =============================================================================

.PHONY: cppcheck

cppcheck:
	@command -v cppcheck >/dev/null 2>&1 || { \
		printf 'cppcheck not found\n' >&2; \
		exit 1; \
	}
	@cppcheck \
		--enable=warning,style,performance,portability \
		--std=c17 \
		--inline-suppr \
		--error-exitcode=1 \
		$(CPPFLAGS) \
		$(SOURCES)

# =============================================================================
# Source inventory
# =============================================================================

.PHONY: sources

sources:
	@printf '%s\n' $(SOURCES)

# =============================================================================
# Object inventory
# =============================================================================

.PHONY: objects

objects:
	@printf '%s\n' $(OBJECTS)

# =============================================================================
# Information
# =============================================================================

.PHONY: info

info:
	@printf 'Project root : %s\n' "$(PROJECT_ROOT)"
	@printf 'Source root  : %s\n' "$(SRC_ROOT)"
	@printf 'Diagnostic   : %s\n' "$(THIS_DIR)"
	@printf 'Build        : %s\n' "$(BUILD_DIR)"
	@printf 'Compiler     : %s\n' "$(CC)"
	@printf 'C standard   : %s\n' "$(CSTD)"
	@printf 'Target       : %s\n' "$(TARGET)"
	@printf 'Sources      : %s\n' "$(words $(SOURCES))"

# =============================================================================
# Verify source inventory
# =============================================================================

.PHONY: verify-sources

verify-sources:
	@set -e; \
	failed=0; \
	for source in $(SOURCES); do \
		if [ ! -f "$$source" ]; then \
			printf 'Missing source: %s\n' "$$source" >&2; \
			failed=1; \
		fi; \
	done; \
	if [ "$$failed" -ne 0 ]; then \
		exit 1; \
	fi
	@printf 'All diagnostic sources are present.\n'

# =============================================================================
# Verify headers
# =============================================================================

HEADERS := \
	diagnostic.h \
	registry.h \
	engine.h \
	span.h \
	label.h \
	suggestion.h \
	cascade.h \
	render_terminal.h \
	render_json.h \
	render_sarif.h \
	render_lsp.h \
	source_map.h \
	explain.h

.PHONY: verify-headers

verify-headers:
	@set -e; \
	failed=0; \
	for header in $(HEADERS); do \
		if [ ! -f "$$header" ]; then \
			printf 'Missing header: %s\n' "$$header" >&2; \
			failed=1; \
		fi; \
	done; \
	if [ "$$failed" -ne 0 ]; then \
		exit 1; \
	fi
	@printf 'All diagnostic headers are present.\n'

# =============================================================================
# Full structural verification
# =============================================================================

.PHONY: verify

verify: verify-sources verify-headers
	@printf 'Diagnostic subsystem structure is complete.\n'

# =============================================================================
# Development audit
# =============================================================================

.PHONY: audit

audit: verify check
	@printf '\n'
	@printf 'Diagnostic subsystem audit completed.\n'

# =============================================================================
# Maximum verification
# =============================================================================

.PHONY: audit-strict

audit-strict: verify check-strict
	@printf '\n'
	@printf 'Strict diagnostic subsystem audit completed.\n'

# =============================================================================
# Clean
# =============================================================================

.PHONY: clean

clean:
	@printf 'Cleaning %s...\n' "$(BUILD_DIR)"
	@$(RM) -r "$(BUILD_DIR)"

# =============================================================================
# Rebuild
# =============================================================================

.PHONY: rebuild

rebuild:
	@$(MAKE) clean
	@$(MAKE) all

# =============================================================================
# Release rebuild
# =============================================================================

.PHONY: rebuild-release

rebuild-release:
	@$(MAKE) clean
	@$(MAKE) release

# =============================================================================
# Help
# =============================================================================

.PHONY: help

help:
	@printf '%s\n' \
		'Vitte diagnostic subsystem' \
		'' \
		'Targets:' \
		'  all              Build debug library' \
		'  debug            Debug build' \
		'  release          Optimized release build' \
		'  strict           Debug build with -Werror' \
		'  check            Syntax-check every source' \
		'  check-strict     Syntax-check with -Werror' \
		'  sanitize         ASan + UBSan build' \
		'  asan             AddressSanitizer build' \
		'  ubsan            UndefinedBehaviorSanitizer build' \
		'  clang-check      Compile-check using clang' \
		'  gcc-check        Compile-check using GCC' \
		'  analyze          Run clang static analyzer' \
		'  tidy             Run clang-tidy' \
		'  cppcheck         Run cppcheck' \
		'  verify           Verify expected source/header files' \
		'  audit            Verify + compile check' \
		'  audit-strict     Verify + strict compile check' \
		'  sources          Print source files' \
		'  objects          Print object files' \
		'  info             Print build configuration' \
		'  rebuild          Clean and rebuild debug' \
		'  rebuild-release  Clean and rebuild release' \
		'  clean            Remove local build directory' \
		'  help             Show this help'
