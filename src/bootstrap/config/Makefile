# ==============================================================================
# Vitte Compiler - config subsystem
# ==============================================================================
#
# Standalone strict ISO C17 build for:
#
#   src/src/config/
#
# Goals:
#   - strict compilation
#   - deterministic builds
#   - dependency tracking
#   - debug/release/sanitizer profiles
#   - static library generation
#   - public-header validation
#   - syntax-only validation
#   - libc header shadowing detection
#   - Clang and GCC support
#   - no dependency on the global Vitte build system
#
# IMPORTANT:
#
# Never add:
#
#   -I$(PROJECT_ROOT)/include/vitte
#
# because the Vitte project contains headers whose names may collide with
# standard C library headers such as <string.h>.
#
# The public include ROOT is valid:
#
#   -I$(PROJECT_ROOT)/include
#
# ==============================================================================

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
THIS_DIR      := $(abspath $(dir $(THIS_MAKEFILE)))

# ------------------------------------------------------------------------------
# Project layout
# ------------------------------------------------------------------------------

PROJECT_ROOT := $(abspath $(THIS_DIR)/../../../..)

SOURCE_ROOT := $(PROJECT_ROOT)/src
SRC_ROOT    := $(PROJECT_ROOT)/src/src

ROOT_INCLUDE := $(PROJECT_ROOT)/include
SRC_INCLUDE  := $(PROJECT_ROOT)/src/include

BUILD_ROOT ?= $(PROJECT_ROOT)/build
BUILD_DIR  ?= $(BUILD_ROOT)/config

# ------------------------------------------------------------------------------
# Toolchain
# ------------------------------------------------------------------------------

CC ?= cc
AR ?= ar
RANLIB ?= ranlib

RM ?= rm -f
RMDIR ?= rm -rf
MKDIR_P ?= mkdir -p

# ------------------------------------------------------------------------------
# Build controls
# ------------------------------------------------------------------------------

WERROR ?= 0
LTO ?= 0
HARDEN ?= 0
VERBOSE ?= 0

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

HEADERS := $(sort $(shell find "$(THIS_DIR)" \
	-type f -name '*.h' \
	-not -path '*/tests/*' \
	-not -path '*/test/*' \
	-not -path '*/tools/*' \
	-not -path '*/examples/*' \
	-print))

REL_SOURCES := $(patsubst $(THIS_DIR)/%,%,$(SOURCES))
REL_HEADERS := $(patsubst $(THIS_DIR)/%,%,$(HEADERS))

# ------------------------------------------------------------------------------
# Output layout
# ------------------------------------------------------------------------------

DEBUG_DIR    := $(BUILD_DIR)/debug
RELEASE_DIR  := $(BUILD_DIR)/release
SANITIZE_DIR := $(BUILD_DIR)/sanitize
ASAN_DIR     := $(BUILD_DIR)/asan
UBSAN_DIR    := $(BUILD_DIR)/ubsan

DEBUG_OBJ_DIR    := $(DEBUG_DIR)/obj
RELEASE_OBJ_DIR  := $(RELEASE_DIR)/obj
SANITIZE_OBJ_DIR := $(SANITIZE_DIR)/obj
ASAN_OBJ_DIR     := $(ASAN_DIR)/obj
UBSAN_OBJ_DIR    := $(UBSAN_DIR)/obj

DEBUG_LIB    := $(DEBUG_DIR)/libvitte-config.a
RELEASE_LIB  := $(RELEASE_DIR)/libvitte-config.a
SANITIZE_LIB := $(SANITIZE_DIR)/libvitte-config.a
ASAN_LIB     := $(ASAN_DIR)/libvitte-config.a
UBSAN_LIB    := $(UBSAN_DIR)/libvitte-config.a

DEBUG_OBJECTS := \
	$(patsubst $(THIS_DIR)/%.c,$(DEBUG_OBJ_DIR)/%.o,$(SOURCES))

RELEASE_OBJECTS := \
	$(patsubst $(THIS_DIR)/%.c,$(RELEASE_OBJ_DIR)/%.o,$(SOURCES))

SANITIZE_OBJECTS := \
	$(patsubst $(THIS_DIR)/%.c,$(SANITIZE_OBJ_DIR)/%.o,$(SOURCES))

ASAN_OBJECTS := \
	$(patsubst $(THIS_DIR)/%.c,$(ASAN_OBJ_DIR)/%.o,$(SOURCES))

UBSAN_OBJECTS := \
	$(patsubst $(THIS_DIR)/%.c,$(UBSAN_OBJ_DIR)/%.o,$(SOURCES))

DEBUG_DEPS := $(DEBUG_OBJECTS:.o=.d)
RELEASE_DEPS := $(RELEASE_OBJECTS:.o=.d)
SANITIZE_DEPS := $(SANITIZE_OBJECTS:.o=.d)
ASAN_DEPS := $(ASAN_OBJECTS:.o=.d)
UBSAN_DEPS := $(UBSAN_OBJECTS:.o=.d)

# ------------------------------------------------------------------------------
# Compiler identification
# ------------------------------------------------------------------------------

CC_VERSION_TEXT := $(shell $(CC) --version 2>/dev/null | head -n 1)

IS_CLANG := $(shell \
	printf '' | $(CC) -dM -E - 2>/dev/null | \
	grep -q '__clang__' && printf '1' || printf '0')

IS_GCC := $(shell \
	printf '' | $(CC) -dM -E - 2>/dev/null | \
	grep -q '__GNUC__' && \
	! grep -q '__clang__' && \
	printf '1' || printf '0')

# ------------------------------------------------------------------------------
# Language
# ------------------------------------------------------------------------------

C_STANDARD := -std=c17

# ------------------------------------------------------------------------------
# Include paths
# ------------------------------------------------------------------------------

CPPFLAGS_COMMON := \
	-I$(THIS_DIR) \
	-I$(SRC_ROOT) \
	-I$(SOURCE_ROOT) \
	-I$(PROJECT_ROOT) \
	-I$(ROOT_INCLUDE)

ifneq ($(wildcard $(SRC_INCLUDE)),)
CPPFLAGS_COMMON += -I$(SRC_INCLUDE)
endif

# ------------------------------------------------------------------------------
# Preprocessor policy
# ------------------------------------------------------------------------------

CPPFLAGS_COMMON += \
	-D_POSIX_C_SOURCE=200809L

# ------------------------------------------------------------------------------
# Common warnings
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

# Do NOT add -Wswitch-default.
#
# The project intentionally uses:
#
#   -Wswitch-enum
#
# and, with Clang:
#
#   -Wcovered-switch-default
#
# Exhaustive enum switches therefore explicitly enumerate every enum member,
# including INVALID and COUNT sentinels, without redundant default labels.

# ------------------------------------------------------------------------------
# Clang warnings
# ------------------------------------------------------------------------------

WARNINGS_CLANG :=

ifeq ($(IS_CLANG),1)
WARNINGS_CLANG += \
	-Wdocumentation \
	-Wnewline-eof \
	-Wcomma \
	-Wconditional-uninitialized \
	-Wcovered-switch-default
endif

# ------------------------------------------------------------------------------
# GCC warnings
# ------------------------------------------------------------------------------

WARNINGS_GCC :=

ifeq ($(IS_GCC),1)
WARNINGS_GCC += \
	-Wduplicated-cond \
	-Wduplicated-branches \
	-Wlogical-op \
	-Wjump-misses-init
endif

# ------------------------------------------------------------------------------
# Warning-as-error policy
# ------------------------------------------------------------------------------

WERROR_FLAGS :=

ifeq ($(WERROR),1)
WERROR_FLAGS += -Werror
endif

# ------------------------------------------------------------------------------
# Reproducibility
# ------------------------------------------------------------------------------

REPRO_FLAGS :=

REPRO_FLAGS += \
	-ffile-prefix-map=$(PROJECT_ROOT)=. \
	-fmacro-prefix-map=$(PROJECT_ROOT)=.

# ------------------------------------------------------------------------------
# Hardening
# ------------------------------------------------------------------------------

HARDEN_FLAGS :=

ifeq ($(HARDEN),1)
HARDEN_FLAGS += \
	-fstack-protector-strong
endif

# ------------------------------------------------------------------------------
# LTO
# ------------------------------------------------------------------------------

LTO_FLAGS :=

ifeq ($(LTO),1)
LTO_FLAGS += -flto
endif

# ------------------------------------------------------------------------------
# Dependency generation
# ------------------------------------------------------------------------------

DEPFLAGS := \
	-MMD \
	-MP

# ------------------------------------------------------------------------------
# Common C flags
# ------------------------------------------------------------------------------

CFLAGS_COMMON := \
	$(C_STANDARD) \
	$(WARNINGS_COMMON) \
	$(WARNINGS_CLANG) \
	$(WARNINGS_GCC) \
	$(WERROR_FLAGS) \
	$(REPRO_FLAGS) \
	$(HARDEN_FLAGS)

# ------------------------------------------------------------------------------
# Debug profile
# ------------------------------------------------------------------------------

CFLAGS_DEBUG := \
	-O0 \
	-g3 \
	-fno-omit-frame-pointer \
	-DVITTE_DEBUG=1

# ------------------------------------------------------------------------------
# Release profile
# ------------------------------------------------------------------------------

CFLAGS_RELEASE := \
	-O3 \
	-DNDEBUG \
	-DVITTE_RELEASE=1 \
	$(LTO_FLAGS)

# ------------------------------------------------------------------------------
# ASan + UBSan profile
# ------------------------------------------------------------------------------

CFLAGS_SANITIZE := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-fsanitize=address,undefined \
	-DVITTE_DEBUG=1 \
	-DVITTE_SANITIZE=1

# ------------------------------------------------------------------------------
# AddressSanitizer profile
# ------------------------------------------------------------------------------

CFLAGS_ASAN := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-fsanitize=address \
	-DVITTE_DEBUG=1 \
	-DVITTE_ASAN=1

# ------------------------------------------------------------------------------
# UndefinedBehaviorSanitizer profile
# ------------------------------------------------------------------------------

CFLAGS_UBSAN := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-fsanitize=undefined \
	-DVITTE_DEBUG=1 \
	-DVITTE_UBSAN=1

# ------------------------------------------------------------------------------
# Quiet / verbose command display
# ------------------------------------------------------------------------------

ifeq ($(VERBOSE),1)
Q :=
else
Q := @
endif

# ------------------------------------------------------------------------------
# Archive flags
# ------------------------------------------------------------------------------

ARFLAGS ?= rcs

# ------------------------------------------------------------------------------
# Default target
# ------------------------------------------------------------------------------

.PHONY: all
all: debug

# ------------------------------------------------------------------------------
# Profile targets
# ------------------------------------------------------------------------------

.PHONY: debug
debug: $(DEBUG_LIB)

.PHONY: release
release: $(RELEASE_LIB)

.PHONY: sanitize
sanitize: $(SANITIZE_LIB)

.PHONY: asan
asan: $(ASAN_LIB)

.PHONY: ubsan
ubsan: $(UBSAN_LIB)

.PHONY: profiles
profiles: debug release sanitize asan ubsan

# ------------------------------------------------------------------------------
# Debug compilation
# ------------------------------------------------------------------------------

$(DEBUG_OBJ_DIR)/%.o: $(THIS_DIR)/%.c
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CFLAGS_COMMON) \
		$(CFLAGS_DEBUG) \
		$(DEPFLAGS) \
		-c "$<" \
		-o "$@"

$(DEBUG_LIB): $(DEBUG_OBJECTS)
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(AR) $(ARFLAGS) "$@" $(DEBUG_OBJECTS)
	$(Q)$(RANLIB) "$@"

# ------------------------------------------------------------------------------
# Release compilation
# ------------------------------------------------------------------------------

$(RELEASE_OBJ_DIR)/%.o: $(THIS_DIR)/%.c
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CFLAGS_COMMON) \
		$(CFLAGS_RELEASE) \
		$(DEPFLAGS) \
		-c "$<" \
		-o "$@"

$(RELEASE_LIB): $(RELEASE_OBJECTS)
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(AR) $(ARFLAGS) "$@" $(RELEASE_OBJECTS)
	$(Q)$(RANLIB) "$@"

# ------------------------------------------------------------------------------
# Sanitizer compilation
# ------------------------------------------------------------------------------

$(SANITIZE_OBJ_DIR)/%.o: $(THIS_DIR)/%.c
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CFLAGS_COMMON) \
		$(CFLAGS_SANITIZE) \
		$(DEPFLAGS) \
		-c "$<" \
		-o "$@"

$(SANITIZE_LIB): $(SANITIZE_OBJECTS)
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(AR) $(ARFLAGS) "$@" $(SANITIZE_OBJECTS)
	$(Q)$(RANLIB) "$@"

# ------------------------------------------------------------------------------
# ASan compilation
# ------------------------------------------------------------------------------

$(ASAN_OBJ_DIR)/%.o: $(THIS_DIR)/%.c
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CFLAGS_COMMON) \
		$(CFLAGS_ASAN) \
		$(DEPFLAGS) \
		-c "$<" \
		-o "$@"

$(ASAN_LIB): $(ASAN_OBJECTS)
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(AR) $(ARFLAGS) "$@" $(ASAN_OBJECTS)
	$(Q)$(RANLIB) "$@"

# ------------------------------------------------------------------------------
# UBSan compilation
# ------------------------------------------------------------------------------

$(UBSAN_OBJ_DIR)/%.o: $(THIS_DIR)/%.c
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CFLAGS_COMMON) \
		$(CFLAGS_UBSAN) \
		$(DEPFLAGS) \
		-c "$<" \
		-o "$@"

$(UBSAN_LIB): $(UBSAN_OBJECTS)
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(AR) $(ARFLAGS) "$@" $(UBSAN_OBJECTS)
	$(Q)$(RANLIB) "$@"

# ------------------------------------------------------------------------------
# Syntax-only validation
# ------------------------------------------------------------------------------

.PHONY: syntax
syntax:
	@set -e; \
	if [ -z "$(SOURCES)" ]; then \
		printf '%s\n' "config: no C sources found"; \
		exit 1; \
	fi; \
	for source in $(SOURCES); do \
		printf '%s\n' "CHECK $$source"; \
		$(CC) \
			$(CPPFLAGS_COMMON) \
			$(CFLAGS_COMMON) \
			-fsyntax-only \
			"$$source"; \
	done

# ------------------------------------------------------------------------------
# Strict validation
# ------------------------------------------------------------------------------

.PHONY: check
check:
	@$(MAKE) --no-print-directory \
		CC="$(CC)" \
		WERROR=1 \
		syntax
	@$(MAKE) --no-print-directory \
		CC="$(CC)" \
		WERROR=1 \
		check-header

# ------------------------------------------------------------------------------
# Public header validation
# ------------------------------------------------------------------------------

.PHONY: check-header
check-header:
	@printf '%s\n' \
		'#include "config.h"' \
		'int main(void) {' \
		'    vitte_config_t config;' \
		'    if (!vitte_config_init(&config)) {' \
		'        return 1;' \
		'    }' \
		'    vitte_config_destroy(&config);' \
		'    return 0;' \
		'}' \
	| $(CC) \
		$(CPPFLAGS_COMMON) \
		$(C_STANDARD) \
		$(WARNINGS_COMMON) \
		$(WARNINGS_CLANG) \
		$(WARNINGS_GCC) \
		-Werror \
		-x c \
		-fsyntax-only \
		-

# ------------------------------------------------------------------------------
# Header self-containment
# ------------------------------------------------------------------------------

.PHONY: check-header-self-contained
check-header-self-contained:
	@printf '%s\n' \
		'#include "config.h"' \
		'int main(void) { return 0; }' \
	| $(CC) \
		$(CPPFLAGS_COMMON) \
		$(C_STANDARD) \
		$(WARNINGS_COMMON) \
		$(WARNINGS_CLANG) \
		$(WARNINGS_GCC) \
		-Werror \
		-x c \
		-fsyntax-only \
		-

# ------------------------------------------------------------------------------
# Standard-library header shadowing check
# ------------------------------------------------------------------------------

.PHONY: check-string-header
check-string-header:
	@printf '%s\n' '#include <string.h>' \
	| $(CC) \
		$(CPPFLAGS_COMMON) \
		$(C_STANDARD) \
		-H \
		-E \
		-x c \
		- \
		>/dev/null

# ------------------------------------------------------------------------------
# Additional libc-header checks
# ------------------------------------------------------------------------------

.PHONY: check-libc-headers
check-libc-headers:
	@set -e; \
	for header in \
		assert.h \
		ctype.h \
		errno.h \
		limits.h \
		stddef.h \
		stdint.h \
		stdio.h \
		stdlib.h \
		string.h; \
	do \
		printf '#include <%s>\n' "$$header" \
		| $(CC) \
			$(CPPFLAGS_COMMON) \
			$(C_STANDARD) \
			-E \
			-x c \
			- \
			>/dev/null; \
		printf '%s\n' "OK <$$header>"; \
	done

# ------------------------------------------------------------------------------
# Preprocessor output
# ------------------------------------------------------------------------------

.PHONY: preprocess
preprocess:
	@set -e; \
	for source in $(SOURCES); do \
		name=$$(basename "$$source" .c); \
		output="$(BUILD_DIR)/preprocessed/$$name.i"; \
		mkdir -p "$$(dirname "$$output")"; \
		printf '%s\n' "CPP $$source -> $$output"; \
		$(CC) \
			$(CPPFLAGS_COMMON) \
			$(C_STANDARD) \
			-E \
			"$$source" \
			-o "$$output"; \
	done

# ------------------------------------------------------------------------------
# Assembly inspection
# ------------------------------------------------------------------------------

.PHONY: assembly
assembly:
	@set -e; \
	for source in $(SOURCES); do \
		name=$$(basename "$$source" .c); \
		output="$(BUILD_DIR)/assembly/$$name.s"; \
		mkdir -p "$$(dirname "$$output")"; \
		printf '%s\n' "ASM $$source -> $$output"; \
		$(CC) \
			$(CPPFLAGS_COMMON) \
			$(CFLAGS_COMMON) \
			$(CFLAGS_RELEASE) \
			-S \
			"$$source" \
			-o "$$output"; \
	done

# ------------------------------------------------------------------------------
# Source listing
# ------------------------------------------------------------------------------

.PHONY: list
list:
	@printf '%s\n' "C sources:"
	@for source in $(REL_SOURCES); do \
		printf '  %s\n' "$$source"; \
	done
	@printf '%s\n' "Headers:"
	@for header in $(REL_HEADERS); do \
		printf '  %s\n' "$$header"; \
	done

# ------------------------------------------------------------------------------
# Object listing
# ------------------------------------------------------------------------------

.PHONY: objects
objects:
	@printf '%s\n' "Debug objects:"
	@for object in $(DEBUG_OBJECTS); do \
		printf '  %s\n' "$$object"; \
	done

# ------------------------------------------------------------------------------
# Archive inspection
# ------------------------------------------------------------------------------

.PHONY: archive-list
archive-list: debug
	@$(AR) -t "$(DEBUG_LIB)"

# ------------------------------------------------------------------------------
# Compiler information
# ------------------------------------------------------------------------------

.PHONY: compiler-info
compiler-info:
	@printf '%s\n' "CC                = $(CC)"
	@printf '%s\n' "CC version        = $(CC_VERSION_TEXT)"
	@printf '%s\n' "Clang             = $(IS_CLANG)"
	@printf '%s\n' "GCC               = $(IS_GCC)"
	@printf '%s\n' "C standard        = $(C_STANDARD)"

# ------------------------------------------------------------------------------
# Build information
# ------------------------------------------------------------------------------

.PHONY: info
info:
	@printf '%s\n' "Vitte config build"
	@printf '%s\n' "------------------"
	@printf '%s\n' "THIS_DIR          = $(THIS_DIR)"
	@printf '%s\n' "PROJECT_ROOT      = $(PROJECT_ROOT)"
	@printf '%s\n' "SOURCE_ROOT       = $(SOURCE_ROOT)"
	@printf '%s\n' "SRC_ROOT          = $(SRC_ROOT)"
	@printf '%s\n' "ROOT_INCLUDE      = $(ROOT_INCLUDE)"
	@printf '%s\n' "SRC_INCLUDE       = $(SRC_INCLUDE)"
	@printf '%s\n' "BUILD_DIR         = $(BUILD_DIR)"
	@printf '%s\n' "CC                = $(CC)"
	@printf '%s\n' "CC version        = $(CC_VERSION_TEXT)"
	@printf '%s\n' "Clang             = $(IS_CLANG)"
	@printf '%s\n' "GCC               = $(IS_GCC)"
	@printf '%s\n' "WERROR            = $(WERROR)"
	@printf '%s\n' "LTO               = $(LTO)"
	@printf '%s\n' "HARDEN            = $(HARDEN)"
	@printf '%s\n' "VERBOSE           = $(VERBOSE)"
	@printf '%s\n' "Sources           = $(words $(SOURCES))"
	@printf '%s\n' "Headers           = $(words $(HEADERS))"
	@printf '%s\n' "Debug library     = $(DEBUG_LIB)"
	@printf '%s\n' "Release library   = $(RELEASE_LIB)"
	@printf '%s\n' "Sanitize library  = $(SANITIZE_LIB)"
	@printf '%s\n' "ASan library      = $(ASAN_LIB)"
	@printf '%s\n' "UBSan library     = $(UBSAN_LIB)"

# ------------------------------------------------------------------------------
# Directory creation
# ------------------------------------------------------------------------------

.PHONY: dirs
dirs:
	@$(MKDIR_P) \
		"$(DEBUG_OBJ_DIR)" \
		"$(RELEASE_OBJ_DIR)" \
		"$(SANITIZE_OBJ_DIR)" \
		"$(ASAN_OBJ_DIR)" \
		"$(UBSAN_OBJ_DIR)"

# ------------------------------------------------------------------------------
# Clean
# ------------------------------------------------------------------------------

.PHONY: clean
clean:
	$(Q)$(RMDIR) "$(BUILD_DIR)"

.PHONY: clean-debug
clean-debug:
	$(Q)$(RMDIR) "$(DEBUG_DIR)"

.PHONY: clean-release
clean-release:
	$(Q)$(RMDIR) "$(RELEASE_DIR)"

.PHONY: clean-sanitize
clean-sanitize:
	$(Q)$(RMDIR) "$(SANITIZE_DIR)"

.PHONY: clean-asan
clean-asan:
	$(Q)$(RMDIR) "$(ASAN_DIR)"

.PHONY: clean-ubsan
clean-ubsan:
	$(Q)$(RMDIR) "$(UBSAN_DIR)"

# ------------------------------------------------------------------------------
# Rebuild
# ------------------------------------------------------------------------------

.PHONY: rebuild
rebuild: clean all

.PHONY: rebuild-release
rebuild-release: clean-release release

# ------------------------------------------------------------------------------
# Full verification
# ------------------------------------------------------------------------------

.PHONY: verify
verify:
	@$(MAKE) --no-print-directory \
		CC="$(CC)" \
		WERROR=1 \
		check
	@$(MAKE) --no-print-directory \
		CC="$(CC)" \
		check-header-self-contained
	@$(MAKE) --no-print-directory \
		CC="$(CC)" \
		check-libc-headers

# ------------------------------------------------------------------------------
# Help
# ------------------------------------------------------------------------------

.PHONY: help
help:
	@printf '%s\n' \
		'Vitte config subsystem' \
		'' \
		'Build targets:' \
		'  make                     Build debug static library' \
		'  make debug               Build debug profile' \
		'  make release             Build release profile' \
		'  make sanitize            Build ASan + UBSan profile' \
		'  make asan                Build AddressSanitizer profile' \
		'  make ubsan               Build UBSan profile' \
		'  make profiles            Build every profile' \
		'' \
		'Validation:' \
		'  make syntax              Syntax-check every .c file' \
		'  make check               Strict -Werror syntax/header check' \
		'  make verify              Full strict validation' \
		'  make check-header        Validate config.h API usage' \
		'  make check-header-self-contained' \
		'                           Verify config.h is self-contained' \
		'  make check-string-header Verify <string.h> resolution' \
		'  make check-libc-headers  Check standard header resolution' \
		'' \
		'Inspection:' \
		'  make info                Display build configuration' \
		'  make compiler-info       Display compiler detection' \
		'  make list                List sources and headers' \
		'  make objects             List debug object files' \
		'  make archive-list        List debug archive contents' \
		'  make preprocess          Generate preprocessed sources' \
		'  make assembly            Generate release assembly' \
		'' \
		'Cleanup:' \
		'  make clean' \
		'  make clean-debug' \
		'  make clean-release' \
		'  make clean-sanitize' \
		'  make clean-asan' \
		'  make clean-ubsan' \
		'  make rebuild' \
		'  make rebuild-release' \
		'' \
		'Options:' \
		'  CC=clang                 Select compiler' \
		'  WERROR=1                 Treat warnings as errors' \
		'  LTO=1                    Enable release LTO' \
		'  HARDEN=1                 Enable stack protection' \
		'  VERBOSE=1                Show compiler commands'

# ------------------------------------------------------------------------------
# Dependency files
# ------------------------------------------------------------------------------

-include $(DEBUG_DEPS)
-include $(RELEASE_DEPS)
-include $(SANITIZE_DEPS)
-include $(ASAN_DEPS)
-include $(UBSAN_DEPS)

# ------------------------------------------------------------------------------
# Make behavior
# ------------------------------------------------------------------------------

.SUFFIXES:

.SECONDARY:

.DELETE_ON_ERROR:
