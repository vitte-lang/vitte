# =============================================================================
# Vitte Compiler - Codegen
# =============================================================================
#
# Standalone strict C17 build for:
#
#   src/src/codegen/
#
# Targets:
#   debug / release / sanitize / asan / ubsan
#   syntax / check / check-header
#   check-string-header
#   list / info / compiler-info
#
# IMPORTANT:
#   Never use:
#
#       -I$(PROJECT_ROOT)/include/vitte
#
#   because it can shadow the system <string.h>.
#
# =============================================================================

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
THIS_DIR      := $(abspath $(dir $(THIS_MAKEFILE)))

PROJECT_ROOT := $(abspath $(THIS_DIR)/../../../..)

SOURCE_ROOT := $(PROJECT_ROOT)/src
SRC_ROOT    := $(PROJECT_ROOT)/src/src

ROOT_INCLUDE := $(PROJECT_ROOT)/include
SRC_INCLUDE  := $(PROJECT_ROOT)/src/include

BUILD_ROOT ?= $(PROJECT_ROOT)/build
BUILD_DIR  ?= $(BUILD_ROOT)/codegen

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

DEBUG_LIB    := $(DEBUG_DIR)/libvitte-codegen.a
RELEASE_LIB  := $(RELEASE_DIR)/libvitte-codegen.a
SANITIZE_LIB := $(SANITIZE_DIR)/libvitte-codegen.a
ASAN_LIB     := $(ASAN_DIR)/libvitte-codegen.a
UBSAN_LIB    := $(UBSAN_DIR)/libvitte-codegen.a

# =============================================================================
# Toolchain
# =============================================================================

CC      ?= cc
AR      ?= ar
RANLIB  ?= ranlib

RM      ?= rm -f
RMDIR   ?= rm -rf
MKDIR_P ?= mkdir -p

# =============================================================================
# Verbosity
# =============================================================================

V ?= 0

ifeq ($(V),1)
Q :=
else
Q := @
endif

# =============================================================================
# Source discovery
# =============================================================================

SOURCES := $(sort $(shell find "$(THIS_DIR)" \
	-type f \
	-name '*.c' \
	-not -path '*/tests/*' \
	-not -path '*/test/*' \
	-not -path '*/tools/*' \
	-not -path '*/examples/*' \
	-print))

HEADERS := $(sort $(shell find "$(THIS_DIR)" \
	-type f \
	-name '*.h' \
	-not -path '*/tests/*' \
	-not -path '*/test/*' \
	-not -path '*/tools/*' \
	-not -path '*/examples/*' \
	-print))

# =============================================================================
# Objects
# =============================================================================

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

DEBUG_DEPS    := $(DEBUG_OBJECTS:.o=.d)
RELEASE_DEPS  := $(RELEASE_OBJECTS:.o=.d)
SANITIZE_DEPS := $(SANITIZE_OBJECTS:.o=.d)
ASAN_DEPS     := $(ASAN_OBJECTS:.o=.d)
UBSAN_DEPS    := $(UBSAN_OBJECTS:.o=.d)

# =============================================================================
# Include paths
# =============================================================================

CPPFLAGS_COMMON := \
	-I$(THIS_DIR) \
	-I$(SRC_ROOT) \
	-I$(SOURCE_ROOT) \
	-I$(PROJECT_ROOT) \
	-I$(ROOT_INCLUDE)

ifneq ($(wildcard $(SRC_INCLUDE)),)
CPPFLAGS_COMMON += -I$(SRC_INCLUDE)
endif

# =============================================================================
# C language
# =============================================================================

CSTD := -std=c17

# =============================================================================
# Strict warnings
# =============================================================================
#
# Intentional policy:
#
#   -Wswitch-enum
#   -Wcovered-switch-default   (Clang)
#
# Do NOT add -Wswitch-default.
#
# Exhaustive enum switches should explicitly handle INVALID/COUNT sentinels
# and should not need a redundant default branch.
#
# =============================================================================

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

# =============================================================================
# Compiler detection
# =============================================================================

CC_VERSION_TEXT := $(shell $(CC) --version 2>/dev/null | head -n 1)

CC_VERSION_LOWER := \
	$(shell printf '%s' "$(CC_VERSION_TEXT)" | tr '[:upper:]' '[:lower:]')

IS_CLANG := $(findstring clang,$(CC_VERSION_LOWER))
IS_GCC   := $(findstring gcc,$(CC_VERSION_LOWER))

WARNINGS_COMPILER :=

ifneq ($(IS_CLANG),)
WARNINGS_COMPILER += \
	-Wdocumentation \
	-Wnewline-eof \
	-Wcomma \
	-Wconditional-uninitialized \
	-Wcovered-switch-default
endif

ifneq ($(IS_GCC),)
WARNINGS_COMPILER += \
	-Wduplicated-cond \
	-Wduplicated-branches \
	-Wlogical-op \
	-Wjump-misses-init
endif

# =============================================================================
# Werror
# =============================================================================

WERROR ?= 0

ifeq ($(WERROR),1)
WARNINGS_WERROR := -Werror
else
WARNINGS_WERROR :=
endif

# =============================================================================
# Dependency generation
# =============================================================================

DEPFLAGS := -MMD -MP

# =============================================================================
# Reproducibility
# =============================================================================

REPRO_FLAGS := \
	-ffile-prefix-map=$(PROJECT_ROOT)=. \
	-fmacro-prefix-map=$(PROJECT_ROOT)=.

# =============================================================================
# Hardening
# =============================================================================

HARDEN ?= 0

ifeq ($(HARDEN),1)
HARDEN_FLAGS := \
	-fstack-protector-strong
else
HARDEN_FLAGS :=
endif

# =============================================================================
# LTO
# =============================================================================

LTO ?= 0

ifeq ($(LTO),1)
LTO_FLAGS := -flto
else
LTO_FLAGS :=
endif

# =============================================================================
# Common flags
# =============================================================================

CFLAGS_COMMON := \
	$(CSTD) \
	$(WARNINGS_COMMON) \
	$(WARNINGS_COMPILER) \
	$(WARNINGS_WERROR) \
	$(REPRO_FLAGS) \
	$(HARDEN_FLAGS)

# =============================================================================
# Profiles
# =============================================================================

CFLAGS_DEBUG := \
	$(CFLAGS_COMMON) \
	-O0 \
	-g3 \
	-fno-omit-frame-pointer \
	-DVITTE_DEBUG=1

CFLAGS_RELEASE := \
	$(CFLAGS_COMMON) \
	-O3 \
	-DNDEBUG \
	-DVITTE_RELEASE=1 \
	$(LTO_FLAGS)

CFLAGS_SANITIZE := \
	$(CFLAGS_COMMON) \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-fsanitize=address,undefined \
	-DVITTE_DEBUG=1 \
	-DVITTE_SANITIZE=1

CFLAGS_ASAN := \
	$(CFLAGS_COMMON) \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-fsanitize=address \
	-DVITTE_DEBUG=1 \
	-DVITTE_ASAN=1

CFLAGS_UBSAN := \
	$(CFLAGS_COMMON) \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-fsanitize=undefined \
	-DVITTE_DEBUG=1 \
	-DVITTE_UBSAN=1

# =============================================================================
# Default target
# =============================================================================

.DEFAULT_GOAL := debug

.PHONY: \
	all \
	debug \
	release \
	sanitize \
	asan \
	ubsan \
	profiles \
	syntax \
	check \
	check-header \
	check-string-header \
	preprocess \
	list \
	objects \
	info \
	compiler-info \
	archive-list \
	dirs \
	clean \
	clean-debug \
	clean-release \
	clean-sanitize \
	clean-asan \
	clean-ubsan \
	distclean \
	rebuild \
	rebuild-release \
	help

all: debug

# =============================================================================
# Debug
# =============================================================================

debug: $(DEBUG_LIB)

$(DEBUG_LIB): $(DEBUG_OBJECTS)
	@printf '  AR      %s\n' "$@"
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(AR) rcs "$@" $(DEBUG_OBJECTS)
	$(Q)$(RANLIB) "$@"

$(DEBUG_OBJ_DIR)/%.o: $(THIS_DIR)/%.c
	@printf '  CC      %s\n' "$<"
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CFLAGS_DEBUG) \
		$(DEPFLAGS) \
		-MF "$(@:.o=.d)" \
		-c "$<" \
		-o "$@"

# =============================================================================
# Release
# =============================================================================

release: $(RELEASE_LIB)

$(RELEASE_LIB): $(RELEASE_OBJECTS)
	@printf '  AR      %s\n' "$@"
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(AR) rcs "$@" $(RELEASE_OBJECTS)
	$(Q)$(RANLIB) "$@"

$(RELEASE_OBJ_DIR)/%.o: $(THIS_DIR)/%.c
	@printf '  CC[REL] %s\n' "$<"
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CFLAGS_RELEASE) \
		$(DEPFLAGS) \
		-MF "$(@:.o=.d)" \
		-c "$<" \
		-o "$@"

# =============================================================================
# ASan + UBSan
# =============================================================================

sanitize: $(SANITIZE_LIB)

$(SANITIZE_LIB): $(SANITIZE_OBJECTS)
	@printf '  AR      %s\n' "$@"
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(AR) rcs "$@" $(SANITIZE_OBJECTS)
	$(Q)$(RANLIB) "$@"

$(SANITIZE_OBJ_DIR)/%.o: $(THIS_DIR)/%.c
	@printf '  CC[SAN] %s\n' "$<"
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CFLAGS_SANITIZE) \
		$(DEPFLAGS) \
		-MF "$(@:.o=.d)" \
		-c "$<" \
		-o "$@"

# =============================================================================
# ASan
# =============================================================================

asan: $(ASAN_LIB)

$(ASAN_LIB): $(ASAN_OBJECTS)
	@printf '  AR      %s\n' "$@"
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(AR) rcs "$@" $(ASAN_OBJECTS)
	$(Q)$(RANLIB) "$@"

$(ASAN_OBJ_DIR)/%.o: $(THIS_DIR)/%.c
	@printf '  CC[ASAN] %s\n' "$<"
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CFLAGS_ASAN) \
		$(DEPFLAGS) \
		-MF "$(@:.o=.d)" \
		-c "$<" \
		-o "$@"

# =============================================================================
# UBSan
# =============================================================================

ubsan: $(UBSAN_LIB)

$(UBSAN_LIB): $(UBSAN_OBJECTS)
	@printf '  AR      %s\n' "$@"
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(AR) rcs "$@" $(UBSAN_OBJECTS)
	$(Q)$(RANLIB) "$@"

$(UBSAN_OBJ_DIR)/%.o: $(THIS_DIR)/%.c
	@printf '  CC[UB]  %s\n' "$<"
	$(Q)$(MKDIR_P) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CFLAGS_UBSAN) \
		$(DEPFLAGS) \
		-MF "$(@:.o=.d)" \
		-c "$<" \
		-o "$@"

# =============================================================================
# Build every profile
# =============================================================================

profiles: debug release sanitize asan ubsan

# =============================================================================
# Syntax validation
# =============================================================================

syntax:
	@set -e; \
	if [ -z "$(strip $(SOURCES))" ]; then \
		printf '%s\n' 'error: no C sources found'; \
		exit 1; \
	fi; \
	for source in $(SOURCES); do \
		printf '  CHECK   %s\n' "$$source"; \
		$(CC) \
			$(CPPFLAGS_COMMON) \
			$(CFLAGS_COMMON) \
			-fsyntax-only \
			"$$source"; \
	done

# =============================================================================
# Strict validation
# =============================================================================

check:
	$(Q)$(MAKE) \
		--no-print-directory \
		CC="$(CC)" \
		WERROR=1 \
		syntax
	$(Q)$(MAKE) \
		--no-print-directory \
		CC="$(CC)" \
		WERROR=1 \
		check-header

# =============================================================================
# Header self-containment
# =============================================================================

check-header:
	@printf '  HEADER  codegen.h\n'
	@printf '%s\n' \
		'#include "codegen.h"' \
		'int main(void) { return 0; }' \
		| $(CC) \
			$(CPPFLAGS_COMMON) \
			$(CFLAGS_COMMON) \
			-Werror \
			-x c \
			-fsyntax-only \
			-

# =============================================================================
# Verify system <string.h>
# =============================================================================

check-string-header:
	@printf '  HEADER  <string.h>\n'
	@printf '%s\n' \
		'#include <string.h>' \
		'int main(void) { return 0; }' \
		| $(CC) \
			$(CPPFLAGS_COMMON) \
			$(CSTD) \
			-H \
			-x c \
			-fsyntax-only \
			-

# =============================================================================
# Preprocessing
# =============================================================================

preprocess:
	@set -e; \
	for source in $(SOURCES); do \
		printf '\n===== %s =====\n' "$$source"; \
		$(CC) \
			$(CPPFLAGS_COMMON) \
			$(CSTD) \
			-E \
			"$$source"; \
	done

# =============================================================================
# Source/header listing
# =============================================================================

list:
	@printf '%s\n' 'Sources:'
	@for source in $(SOURCES); do \
		printf '  %s\n' "$$source"; \
	done
	@printf '\n%s\n' 'Headers:'
	@for header in $(HEADERS); do \
		printf '  %s\n' "$$header"; \
	done

objects:
	@printf '%s\n' 'Debug objects:'
	@for object in $(DEBUG_OBJECTS); do \
		printf '  %s\n' "$$object"; \
	done

# =============================================================================
# Configuration information
# =============================================================================

info:
	@printf '%s\n' \
		'Vitte codegen build configuration' \
		'----------------------------------' \
		"Makefile:        $(THIS_MAKEFILE)" \
		"Directory:       $(THIS_DIR)" \
		"Project root:    $(PROJECT_ROOT)" \
		"Source root:     $(SOURCE_ROOT)" \
		"src/src root:    $(SRC_ROOT)" \
		"Root include:    $(ROOT_INCLUDE)" \
		"src/include:     $(SRC_INCLUDE)" \
		"Build root:      $(BUILD_ROOT)" \
		"Build dir:       $(BUILD_DIR)" \
		"Compiler:        $(CC)" \
		"Compiler line:   $(CC_VERSION_TEXT)" \
		"Archiver:        $(AR)" \
		"Ranlib:          $(RANLIB)" \
		"WERROR:          $(WERROR)" \
		"HARDEN:          $(HARDEN)" \
		"LTO:             $(LTO)" \
		"Source count:    $(words $(SOURCES))" \
		"Header count:    $(words $(HEADERS))" \
		"Debug archive:   $(DEBUG_LIB)" \
		"Release archive: $(RELEASE_LIB)"

compiler-info:
	@printf 'Compiler command: %s\n' "$(CC)"
	@$(CC) --version || true
	@printf '\nPredefined compiler macros:\n'
	@printf '' \
		| $(CC) $(CSTD) -dM -E -x c - \
		| sort

# =============================================================================
# Archive inspection
# =============================================================================

archive-list: debug
	@printf 'Archive: %s\n' "$(DEBUG_LIB)"
	@$(AR) -t "$(DEBUG_LIB)"

# =============================================================================
# Directories
# =============================================================================

dirs:
	$(Q)$(MKDIR_P) \
		"$(DEBUG_OBJ_DIR)" \
		"$(RELEASE_OBJ_DIR)" \
		"$(SANITIZE_OBJ_DIR)" \
		"$(ASAN_OBJ_DIR)" \
		"$(UBSAN_OBJ_DIR)"

# =============================================================================
# Cleaning
# =============================================================================

clean-debug:
	$(Q)$(RMDIR) "$(DEBUG_DIR)"

clean-release:
	$(Q)$(RMDIR) "$(RELEASE_DIR)"

clean-sanitize:
	$(Q)$(RMDIR) "$(SANITIZE_DIR)"

clean-asan:
	$(Q)$(RMDIR) "$(ASAN_DIR)"

clean-ubsan:
	$(Q)$(RMDIR) "$(UBSAN_DIR)"

clean: \
	clean-debug \
	clean-release \
	clean-sanitize \
	clean-asan \
	clean-ubsan

distclean:
	$(Q)$(RMDIR) "$(BUILD_DIR)"

rebuild: clean-debug debug

rebuild-release: clean-release release

# =============================================================================
# Help
# =============================================================================

help:
	@printf '%s\n' \
		'Vitte codegen Makefile' \
		'' \
		'Build:' \
		'  make                  Build debug archive' \
		'  make debug            Build debug archive' \
		'  make release          Build optimized archive' \
		'  make sanitize         Build ASan + UBSan archive' \
		'  make asan             Build AddressSanitizer archive' \
		'  make ubsan            Build UndefinedBehaviorSanitizer archive' \
		'  make profiles         Build all profiles' \
		'' \
		'Validation:' \
		'  make syntax           Syntax-check every C source' \
		'  make check            Strict WERROR source/header validation' \
		'  make check-header     Validate codegen.h independently' \
		'  make check-string-header' \
		'                        Verify resolution of system <string.h>' \
		'' \
		'Inspection:' \
		'  make list             List source/header files' \
		'  make objects          List debug objects' \
		'  make info             Display build configuration' \
		'  make compiler-info    Display compiler information' \
		'  make archive-list     List archive members' \
		'  make preprocess       Preprocess all source files' \
		'' \
		'Configuration:' \
		'  CC=clang              Select Clang' \
		'  CC=gcc                Select GCC' \
		'  WERROR=1              Treat warnings as errors' \
		'  HARDEN=1              Enable stack protector' \
		'  LTO=1                 Enable release LTO' \
		'  V=1                   Display complete commands' \
		'' \
		'Cleaning:' \
		'  make clean' \
		'  make distclean' \
		'  make rebuild' \
		'  make rebuild-release'

# =============================================================================
# Generated dependencies
# =============================================================================

-include \
	$(DEBUG_DEPS) \
	$(RELEASE_DEPS) \
	$(SANITIZE_DEPS) \
	$(ASAN_DEPS) \
	$(UBSAN_DEPS)

# =============================================================================
# Make behavior
# =============================================================================

.SUFFIXES:

.SECONDARY:

.DELETE_ON_ERROR:
