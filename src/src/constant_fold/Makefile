# ==============================================================================
# Vitte Compiler
# src/src/constant_fold/Makefile
#
# Strict standalone build for the constant folding subsystem.
#
# Primary targets:
#
#   make
#   make debug
#   make release
#   make check
#   make syntax
#   make sanitize
#   make asan
#   make ubsan
#   make test
#   make info
#   make clean
#
# Strict validation:
#
#   make CC=clang WERROR=1 check
#
# Important include rule:
#
#   Never add:
#
#       -I$(PROJECT_ROOT)/include/vitte
#
#   because the project contains include/vitte/string.h and that include path
#   could shadow the platform C library <string.h>.
# ==============================================================================

# ------------------------------------------------------------------------------
# Makefile location
# ------------------------------------------------------------------------------

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
THIS_DIR      := $(abspath $(dir $(THIS_MAKEFILE)))

# ------------------------------------------------------------------------------
# Project layout
# ------------------------------------------------------------------------------

PROJECT_ROOT := $(abspath $(THIS_DIR)/../../../..)

SOURCE_ROOT  := $(PROJECT_ROOT)/src
SRC_ROOT     := $(PROJECT_ROOT)/src/src
INCLUDE_ROOT := $(PROJECT_ROOT)/src/include
ROOT_INCLUDE := $(PROJECT_ROOT)/include

BUILD_ROOT ?= $(PROJECT_ROOT)/build
BUILD_DIR  ?= $(BUILD_ROOT)/constant_fold

OBJ_DIR := $(BUILD_DIR)/obj
DEP_DIR := $(BUILD_DIR)/dep
LIB_DIR := $(BUILD_DIR)/lib
BIN_DIR := $(BUILD_DIR)/bin

# ------------------------------------------------------------------------------
# Tools
# ------------------------------------------------------------------------------

CC      ?= cc
AR      ?= ar
RANLIB  ?= ranlib
RM      ?= rm -f
RMDIR   ?= rm -rf
MKDIR_P ?= mkdir -p

# ------------------------------------------------------------------------------
# Shell
# ------------------------------------------------------------------------------

SHELL := /bin/sh

# ------------------------------------------------------------------------------
# Build configuration
# ------------------------------------------------------------------------------

MODE ?= debug

WERROR ?= 0

VERBOSE ?= 0

SANITIZERS ?= address,undefined

# ------------------------------------------------------------------------------
# Output library
# ------------------------------------------------------------------------------

LIB_NAME := libvitte_constant_fold.a
LIBRARY  := $(LIB_DIR)/$(LIB_NAME)

# ------------------------------------------------------------------------------
# Source discovery
# ------------------------------------------------------------------------------

SOURCES := $(sort $(shell find "$(THIS_DIR)" \
	-type f -name '*.c' \
	-not -path '*/tests/*' \
	-not -path '*/test/*' \
	-not -path '*/tools/*' \
	-not -path '*/examples/*' \
	-print))

RELATIVE_SOURCES := $(patsubst $(THIS_DIR)/%,%,$(SOURCES))

OBJECTS := $(addprefix $(OBJ_DIR)/,$(RELATIVE_SOURCES:.c=.o))
DEPS    := $(addprefix $(DEP_DIR)/,$(RELATIVE_SOURCES:.c=.d))

# ------------------------------------------------------------------------------
# Test discovery
# ------------------------------------------------------------------------------

TEST_SOURCES := $(sort $(shell \
	if [ -d "$(THIS_DIR)/tests" ]; then \
		find "$(THIS_DIR)/tests" -type f -name '*.c' -print; \
	fi))

TEST_RELATIVE_SOURCES := $(patsubst $(THIS_DIR)/tests/%,%,$(TEST_SOURCES))

TEST_OBJECTS := \
	$(addprefix $(OBJ_DIR)/tests/,$(TEST_RELATIVE_SOURCES:.c=.o))

TEST_DEPS := \
	$(addprefix $(DEP_DIR)/tests/,$(TEST_RELATIVE_SOURCES:.c=.d))

TEST_BINARY := $(BIN_DIR)/constant_fold_tests

# ------------------------------------------------------------------------------
# Include paths
# ------------------------------------------------------------------------------

CPPFLAGS_COMMON := \
	-I$(THIS_DIR) \
	-I$(SRC_ROOT) \
	-I$(SOURCE_ROOT) \
	-I$(PROJECT_ROOT) \
	-I$(ROOT_INCLUDE)

ifneq ($(wildcard $(INCLUDE_ROOT)),)
CPPFLAGS_COMMON += -I$(INCLUDE_ROOT)
endif

# ------------------------------------------------------------------------------
# Language
# ------------------------------------------------------------------------------

CSTD := -std=c17

# ------------------------------------------------------------------------------
# Baseline warnings
#
# Deliberately:
#
#   - keep -Wswitch-enum
#   - do NOT enable -Wswitch-default
#
# With Clang we additionally use -Wcovered-switch-default.
#
# This allows exhaustive enum switches without contradictory requirements:
#
#   -Wswitch-enum           -> all enum values should be explicit
#   -Wcovered-switch-default -> unnecessary default is diagnosed
# ------------------------------------------------------------------------------

WARNINGS_COMMON := \
	-Wall \
	-Wextra \
	-Wpedantic \
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

# ------------------------------------------------------------------------------
# Compiler detection
# ------------------------------------------------------------------------------

CC_VERSION_TEXT := $(shell $(CC) --version 2>/dev/null | head -n 1)

IS_CLANG := $(shell \
	printf '%s\n' "$(CC_VERSION_TEXT)" | \
	grep -qi clang && printf '1' || printf '0')

IS_GCC := $(shell \
	printf '%s\n' "$(CC_VERSION_TEXT)" | \
	grep -Eqi 'gcc|GNU' && \
	! printf '%s\n' "$(CC_VERSION_TEXT)" | grep -qi clang && \
	printf '1' || printf '0')

# ------------------------------------------------------------------------------
# Compiler-specific warnings
# ------------------------------------------------------------------------------

WARNINGS_COMPILER :=

ifeq ($(IS_CLANG),1)

WARNINGS_COMPILER += \
	-Wdocumentation \
	-Wnewline-eof \
	-Wcomma \
	-Wconditional-uninitialized \
	-Wcovered-switch-default

endif

ifeq ($(IS_GCC),1)

WARNINGS_COMPILER += \
	-Wduplicated-cond \
	-Wduplicated-branches \
	-Wlogical-op

endif

# ------------------------------------------------------------------------------
# Werror
# ------------------------------------------------------------------------------

ifeq ($(WERROR),1)
WARNINGS_WERROR := -Werror
else
WARNINGS_WERROR :=
endif

# ------------------------------------------------------------------------------
# Common compile flags
# ------------------------------------------------------------------------------

CFLAGS_COMMON := \
	$(CSTD) \
	$(WARNINGS_COMMON) \
	$(WARNINGS_COMPILER) \
	$(WARNINGS_WERROR)

# ------------------------------------------------------------------------------
# Build profiles
# ------------------------------------------------------------------------------

CFLAGS_debug := \
	-O0 \
	-g3 \
	-fno-omit-frame-pointer \
	-DVITTE_DEBUG=1

CFLAGS_release := \
	-O3 \
	-DNDEBUG \
	-DVITTE_RELEASE=1

CFLAGS_size := \
	-Os \
	-DNDEBUG \
	-DVITTE_RELEASE=1 \
	-DVITTE_OPTIMIZE_SIZE=1

CFLAGS_sanitize := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-fsanitize=$(SANITIZERS) \
	-DVITTE_DEBUG=1 \
	-DVITTE_SANITIZE=1

CFLAGS_asan := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-fsanitize=address \
	-DVITTE_DEBUG=1 \
	-DVITTE_ASAN=1

CFLAGS_ubsan := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-fsanitize=undefined \
	-DVITTE_DEBUG=1 \
	-DVITTE_UBSAN=1

# ------------------------------------------------------------------------------
# Link flags
# ------------------------------------------------------------------------------

LDFLAGS_COMMON :=

LDFLAGS_debug :=

LDFLAGS_release :=

LDFLAGS_size :=

LDFLAGS_sanitize := \
	-fsanitize=$(SANITIZERS)

LDFLAGS_asan := \
	-fsanitize=address

LDFLAGS_ubsan := \
	-fsanitize=undefined

# constant.c currently uses fmod().
#
# macOS provides libm functionality through the system libraries and accepts
# -lm. Unix-like systems conventionally require it explicitly.
LDLIBS_COMMON := -lm

# ------------------------------------------------------------------------------
# Selected flags
# ------------------------------------------------------------------------------

CFLAGS_MODE  := $(CFLAGS_$(MODE))
LDFLAGS_MODE := $(LDFLAGS_$(MODE))

ifeq ($(strip $(CFLAGS_MODE)),)
$(error Unknown MODE '$(MODE)'. Valid modes: debug release size sanitize asan ubsan)
endif

CPPFLAGS := $(CPPFLAGS_COMMON)
CFLAGS   := $(CFLAGS_COMMON) $(CFLAGS_MODE)
LDFLAGS  := $(LDFLAGS_COMMON) $(LDFLAGS_MODE)
LDLIBS   := $(LDLIBS_COMMON)

# ------------------------------------------------------------------------------
# Dependency generation
# ------------------------------------------------------------------------------

DEPFLAGS = -MMD -MP -MF $(DEP_DIR)/$*.d -MT $@

TEST_DEPFLAGS = -MMD -MP -MF $(DEP_DIR)/tests/$*.d -MT $@

# ------------------------------------------------------------------------------
# Quiet / verbose commands
# ------------------------------------------------------------------------------

ifeq ($(VERBOSE),1)

Q :=

define say
	@:
endef

else

Q := @

define say
	@printf '  %-8s %s\n' "$(1)" "$(2)"
endef

endif

# ------------------------------------------------------------------------------
# Default target
# ------------------------------------------------------------------------------

.DEFAULT_GOAL := all

# ------------------------------------------------------------------------------
# Phony targets
# ------------------------------------------------------------------------------

.PHONY: \
	all \
	debug \
	release \
	size \
	sanitize \
	asan \
	ubsan \
	lib \
	syntax \
	check \
	check-header \
	check-string-header \
	check-sources \
	test \
	run-tests \
	info \
	print-sources \
	print-objects \
	print-includes \
	print-flags \
	clean \
	distclean \
	rebuild \
	help

# ------------------------------------------------------------------------------
# Main targets
# ------------------------------------------------------------------------------

all: lib

lib: $(LIBRARY)

debug:
	$(Q)$(MAKE) MODE=debug all

release:
	$(Q)$(MAKE) MODE=release all

size:
	$(Q)$(MAKE) MODE=size all

sanitize:
	$(Q)$(MAKE) MODE=sanitize all

asan:
	$(Q)$(MAKE) MODE=asan all

ubsan:
	$(Q)$(MAKE) MODE=ubsan all

# ------------------------------------------------------------------------------
# Library
# ------------------------------------------------------------------------------

$(LIBRARY): $(OBJECTS)
	$(call say,AR,$@)
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(AR) rcs "$@" $(OBJECTS)
	$(Q)$(RANLIB) "$@"

# ------------------------------------------------------------------------------
# Object compilation
# ------------------------------------------------------------------------------

$(OBJ_DIR)/%.o: $(THIS_DIR)/%.c
	$(call say,CC,$<)
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(MKDIR_P) "$(dir $(DEP_DIR)/$*.d)"
	$(Q)$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		$(DEPFLAGS) \
		-c "$<" \
		-o "$@"

# ------------------------------------------------------------------------------
# Syntax-only validation
# ------------------------------------------------------------------------------

syntax:
	@set -e; \
	if [ -z "$(SOURCES)" ]; then \
		printf '%s\n' "error: no C sources found in $(THIS_DIR)"; \
		exit 1; \
	fi; \
	for source in $(SOURCES); do \
		printf '  %-8s %s\n' "SYNTAX" "$$source"; \
		$(CC) \
			$(CPPFLAGS) \
			$(CFLAGS) \
			-fsyntax-only \
			"$$source"; \
	done

# ------------------------------------------------------------------------------
# Strict check
# ------------------------------------------------------------------------------

check:
	$(Q)$(MAKE) \
		CC="$(CC)" \
		MODE=debug \
		WERROR=1 \
		syntax
	$(Q)$(MAKE) \
		CC="$(CC)" \
		MODE=debug \
		WERROR=1 \
		check-header
	$(Q)$(MAKE) \
		CC="$(CC)" \
		MODE=debug \
		WERROR=1 \
		check-string-header

# ------------------------------------------------------------------------------
# Public header check
# ------------------------------------------------------------------------------

check-header:
	$(call say,CHECK,constant.h)
	$(Q)printf '%s\n' \
		'#include "constant.h"' \
		'int main(void) {' \
		'    vitte_constant_t value = vitte_constant_make_i32(42);' \
		'    return vitte_constant_is_valid(&value) ? 0 : 1;' \
		'}' \
		| $(CC) \
			$(CPPFLAGS) \
			$(CFLAGS) \
			-x c \
			-fsyntax-only \
			-

# ------------------------------------------------------------------------------
# libc <string.h> shadowing check
#
# The Vitte repository contains include/vitte/string.h.
#
# The include path must never cause:
#
#     #include <string.h>
#
# to resolve to that project header.
#
# We use compiler include tracing and reject the result if the Vitte header is
# present in the trace.
# ------------------------------------------------------------------------------

check-string-header:
	$(call say,CHECK,system-string.h)
	$(Q)set -e; \
	trace_file="$(BUILD_DIR)/string-header.trace"; \
	$(MKDIR_P) "$(BUILD_DIR)"; \
	printf '%s\n' \
		'#include <string.h>' \
		'int main(void) { return 0; }' \
		| $(CC) \
			$(CPPFLAGS) \
			$(CSTD) \
			-H \
			-x c \
			-fsyntax-only \
			- \
			2>"$$trace_file"; \
	if grep -F "$(PROJECT_ROOT)/include/vitte/string.h" "$$trace_file" >/dev/null 2>&1; then \
		printf '%s\n' \
			"error: <string.h> resolved through Vitte include/vitte/string.h"; \
		cat "$$trace_file"; \
		exit 1; \
	fi; \
	printf '%s\n' "system <string.h> resolution: OK"

# ------------------------------------------------------------------------------
# Source consistency check
#
# constant.c is expected to use constant.h as the public type/API authority.
#
# This target deliberately warns rather than rewriting source code.
# ------------------------------------------------------------------------------

check-sources:
	$(call say,CHECK,source-layout)
	$(Q)set -e; \
	source="$(THIS_DIR)/constant.c"; \
	header="$(THIS_DIR)/constant.h"; \
	test -f "$$source" || { \
		printf '%s\n' "error: missing $$source"; \
		exit 1; \
	}; \
	test -f "$$header" || { \
		printf '%s\n' "error: missing $$header"; \
		exit 1; \
	}; \
	if ! grep -E '^[[:space:]]*#[[:space:]]*include[[:space:]]*"constant\.h"' "$$source" >/dev/null 2>&1; then \
		printf '%s\n' \
			"warning: constant.c does not yet include constant.h"; \
		printf '%s\n' \
			"warning: synchronize constant.c before treating constant.h as the single public API authority"; \
	fi

# ------------------------------------------------------------------------------
# Tests
# ------------------------------------------------------------------------------

ifeq ($(strip $(TEST_SOURCES)),)

test:
	@printf '%s\n' \
		"constant_fold: no tests found under $(THIS_DIR)/tests"

run-tests: test

else

$(OBJ_DIR)/tests/%.o: $(THIS_DIR)/tests/%.c
	$(call say,TEST-CC,$<)
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(MKDIR_P) "$(dir $(DEP_DIR)/tests/$*.d)"
	$(Q)$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		$(TEST_DEPFLAGS) \
		-c "$<" \
		-o "$@"

$(TEST_BINARY): $(OBJECTS) $(TEST_OBJECTS)
	$(call say,LINK,$@)
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(CC) \
		$(LDFLAGS) \
		$(OBJECTS) \
		$(TEST_OBJECTS) \
		$(LDLIBS) \
		-o "$@"

test: $(TEST_BINARY)
	$(call say,TEST,$(TEST_BINARY))
	$(Q)"$(TEST_BINARY)"

run-tests: test

endif

# ------------------------------------------------------------------------------
# Information
# ------------------------------------------------------------------------------

info:
	@printf '%s\n' \
		"Vitte constant_fold build" \
		"" \
		"Makefile:       $(THIS_MAKEFILE)" \
		"Directory:      $(THIS_DIR)" \
		"Project root:   $(PROJECT_ROOT)" \
		"Source root:    $(SOURCE_ROOT)" \
		"src/src root:   $(SRC_ROOT)" \
		"Include root:   $(INCLUDE_ROOT)" \
		"Root include:   $(ROOT_INCLUDE)" \
		"Build root:     $(BUILD_ROOT)" \
		"Build dir:      $(BUILD_DIR)" \
		"Library:        $(LIBRARY)" \
		"" \
		"Compiler:       $(CC)" \
		"Compiler info:  $(CC_VERSION_TEXT)" \
		"Clang:          $(IS_CLANG)" \
		"GCC:            $(IS_GCC)" \
		"Mode:           $(MODE)" \
		"Werror:         $(WERROR)" \
		"Verbose:        $(VERBOSE)" \
		"Sanitizers:     $(SANITIZERS)" \
		"" \
		"Source count:   $(words $(SOURCES))" \
		"Test count:     $(words $(TEST_SOURCES))"

print-sources:
	@for source in $(SOURCES); do \
		printf '%s\n' "$$source"; \
	done

print-objects:
	@for object in $(OBJECTS); do \
		printf '%s\n' "$$object"; \
	done

print-includes:
	@printf '%s\n' "$(CPPFLAGS)"

print-flags:
	@printf '%s\n' \
		"CPPFLAGS=$(CPPFLAGS)" \
		"CFLAGS=$(CFLAGS)" \
		"LDFLAGS=$(LDFLAGS)" \
		"LDLIBS=$(LDLIBS)"

# ------------------------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------------------------

clean:
	$(call say,CLEAN,$(BUILD_DIR))
	$(Q)$(RMDIR) "$(BUILD_DIR)"

distclean: clean

rebuild:
	$(Q)$(MAKE) clean
	$(Q)$(MAKE) all

# ------------------------------------------------------------------------------
# Help
# ------------------------------------------------------------------------------

help:
	@printf '%s\n' \
		"Vitte constant_fold Makefile" \
		"" \
		"Targets:" \
		"  all                  Build the static library" \
		"  lib                  Build libvitte_constant_fold.a" \
		"  debug                Build MODE=debug" \
		"  release              Build MODE=release" \
		"  size                 Build MODE=size" \
		"  sanitize             Build with ASan + UBSan" \
		"  asan                 Build with AddressSanitizer" \
		"  ubsan                Build with UndefinedBehaviorSanitizer" \
		"  syntax               Run syntax-only compilation" \
		"  check                Strict WERROR syntax/header/include checks" \
		"  check-header         Validate constant.h independently" \
		"  check-string-header  Ensure libc <string.h> is not shadowed" \
		"  check-sources        Check constant.c/header synchronization" \
		"  test                 Build and execute tests when present" \
		"  info                 Display resolved build configuration" \
		"  print-sources        Display discovered source files" \
		"  print-objects        Display generated object paths" \
		"  print-includes       Display include flags" \
		"  print-flags          Display compiler/linker flags" \
		"  clean                Remove this subsystem build directory" \
		"  rebuild              Clean and rebuild" \
		"" \
		"Variables:" \
		"  CC=clang" \
		"  MODE=debug|release|size|sanitize|asan|ubsan" \
		"  WERROR=0|1" \
		"  VERBOSE=0|1" \
		"  BUILD_ROOT=/path" \
		"  SANITIZERS=address,undefined" \
		"" \
		"Recommended strict check:" \
		"  make CC=clang WERROR=1 check"

# ------------------------------------------------------------------------------
# Dependency files
# ------------------------------------------------------------------------------

-include $(DEPS)
-include $(TEST_DEPS)
