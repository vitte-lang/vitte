# ==============================================================================
# Vitte Compiler — ISO C17 Backend
# src/backend/c17/Makefile
#
# Standalone build system for the Vitte ISO C17 backend.
#
# Goals:
#   - ISO C17
#   - deterministic source discovery
#   - recursive source discovery
#   - dependency tracking
#   - debug/release/sanitize profiles
#   - strict warnings
#   - static archive generation
#   - syntax-only validation
#   - no pollution of source directories
#
# IMPORTANT:
#
#   Never add:
#
#       -I$(PROJECT_ROOT)/include/vitte
#
#   because the Vitte public tree may contain:
#
#       include/vitte/string.h
#
#   which would shadow the system <string.h>.
#
#   Public Vitte headers must instead be included as:
#
#       #include <vitte/...>
#
#   with:
#
#       -I$(PROJECT_ROOT)/include
#
# ==============================================================================

.DEFAULT_GOAL := all

# ------------------------------------------------------------------------------
# Makefile location
# ------------------------------------------------------------------------------

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
THIS_DIR      := $(abspath $(dir $(THIS_MAKEFILE)))

# Repository layout:
#
#   <repo>/
#     include/
#     src/
#       include/
#       src/
#         backend/
#           c17/
#
PROJECT_ROOT := $(abspath $(THIS_DIR)/../../../..)

SOURCE_ROOT  := $(PROJECT_ROOT)/src
SRC_ROOT     := $(PROJECT_ROOT)/src/src
INCLUDE_ROOT := $(PROJECT_ROOT)/src/include
ROOT_INCLUDE := $(PROJECT_ROOT)/include

# ------------------------------------------------------------------------------
# Build directories
# ------------------------------------------------------------------------------

BUILD_ROOT ?= $(PROJECT_ROOT)/build
BUILD_DIR  ?= $(BUILD_ROOT)/backend/c17

DEBUG_DIR    := $(BUILD_DIR)/debug
RELEASE_DIR  := $(BUILD_DIR)/release
SANITIZE_DIR := $(BUILD_DIR)/sanitize
ASAN_DIR     := $(BUILD_DIR)/asan
UBSAN_DIR    := $(BUILD_DIR)/ubsan

# ------------------------------------------------------------------------------
# Toolchain
# ------------------------------------------------------------------------------

CC ?= cc
AR ?= ar
RANLIB ?= ranlib

RM    ?= rm -f
RMDIR ?= rm -rf
MKDIR ?= mkdir -p

# ------------------------------------------------------------------------------
# Archive
# ------------------------------------------------------------------------------

LIB_NAME := libvitte-c17.a

DEBUG_LIB    := $(DEBUG_DIR)/$(LIB_NAME)
RELEASE_LIB  := $(RELEASE_DIR)/$(LIB_NAME)
SANITIZE_LIB := $(SANITIZE_DIR)/$(LIB_NAME)
ASAN_LIB     := $(ASAN_DIR)/$(LIB_NAME)
UBSAN_LIB    := $(UBSAN_DIR)/$(LIB_NAME)

# ------------------------------------------------------------------------------
# Source discovery
# ------------------------------------------------------------------------------

#
# Recursive discovery allows backend subsystems to grow naturally:
#
#   c17/
#   c17/runtime/
#   c17/abi/
#   c17/internal/
#
# without editing this Makefile.
#
# Sort is important for deterministic archive construction.
#

SOURCES := $(sort $(shell find "$(THIS_DIR)" \
	-type f \
	-name '*.c' \
	-not -path '*/tests/*' \
	-not -path '*/test/*' \
	-not -path '*/tools/*' \
	-not -path '*/examples/*' \
	-print))

SOURCE_REL := $(patsubst $(THIS_DIR)/%,%,$(SOURCES))

# ------------------------------------------------------------------------------
# Object lists
# ------------------------------------------------------------------------------

DEBUG_OBJECTS := \
	$(patsubst %.c,$(DEBUG_DIR)/obj/%.o,$(SOURCE_REL))

RELEASE_OBJECTS := \
	$(patsubst %.c,$(RELEASE_DIR)/obj/%.o,$(SOURCE_REL))

SANITIZE_OBJECTS := \
	$(patsubst %.c,$(SANITIZE_DIR)/obj/%.o,$(SOURCE_REL))

ASAN_OBJECTS := \
	$(patsubst %.c,$(ASAN_DIR)/obj/%.o,$(SOURCE_REL))

UBSAN_OBJECTS := \
	$(patsubst %.c,$(UBSAN_DIR)/obj/%.o,$(SOURCE_REL))

# ------------------------------------------------------------------------------
# Dependency files
# ------------------------------------------------------------------------------

DEBUG_DEPS := $(DEBUG_OBJECTS:.o=.d)
RELEASE_DEPS := $(RELEASE_OBJECTS:.o=.d)
SANITIZE_DEPS := $(SANITIZE_OBJECTS:.o=.d)
ASAN_DEPS := $(ASAN_OBJECTS:.o=.d)
UBSAN_DEPS := $(UBSAN_OBJECTS:.o=.d)

# ------------------------------------------------------------------------------
# Preprocessor flags
# ------------------------------------------------------------------------------

CPPFLAGS_COMMON := \
	-I$(THIS_DIR) \
	-I$(SRC_ROOT) \
	-I$(SOURCE_ROOT) \
	-I$(PROJECT_ROOT) \
	-I$(ROOT_INCLUDE)

#
# INCLUDE_ROOT is intentionally added only if it exists.
#
# This remains:
#
#     .../src/include
#
# not:
#
#     .../include/vitte
#
ifneq ($(wildcard $(INCLUDE_ROOT)),)
CPPFLAGS_COMMON += -I$(INCLUDE_ROOT)
endif

CPPFLAGS ?=

# ------------------------------------------------------------------------------
# Language flags
# ------------------------------------------------------------------------------

CSTD := -std=c17

# ------------------------------------------------------------------------------
# Warning policy
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

#
# These warnings are useful but not uniformly available on every compiler.
# Compiler-family-specific additions are selected below.
#

CC_VERSION_TEXT := $(shell $(CC) --version 2>/dev/null | head -n 1)

IS_CLANG := $(findstring clang,$(shell printf '%s' "$(CC_VERSION_TEXT)" | tr A-Z a-z))
IS_GCC   := $(findstring gcc,$(shell printf '%s' "$(CC_VERSION_TEXT)" | tr A-Z a-z))

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

WARNINGS := $(WARNINGS_COMMON) $(WARNINGS_COMPILER)

# ------------------------------------------------------------------------------
# Dependency generation
# ------------------------------------------------------------------------------

DEPFLAGS := -MMD -MP

# ------------------------------------------------------------------------------
# Reproducibility / diagnostics
# ------------------------------------------------------------------------------

#
# Avoid embedding the absolute repository path in diagnostics/debug metadata
# where supported.
#

REPRO_FLAGS := \
	-ffile-prefix-map=$(PROJECT_ROOT)=. \
	-fmacro-prefix-map=$(PROJECT_ROOT)=.

# ------------------------------------------------------------------------------
# Common compiler flags
# ------------------------------------------------------------------------------

CFLAGS_COMMON := \
	$(CSTD) \
	$(WARNINGS) \
	$(DEPFLAGS) \
	$(REPRO_FLAGS)

CFLAGS ?=

# ------------------------------------------------------------------------------
# Build profiles
# ------------------------------------------------------------------------------

CFLAGS_DEBUG := \
	-O0 \
	-g3 \
	-fno-omit-frame-pointer \
	-DVITTE_DEBUG=1

CFLAGS_RELEASE := \
	-O3 \
	-DNDEBUG \
	-DVITTE_RELEASE=1

CFLAGS_SANITIZE := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-fsanitize=address,undefined \
	-DVITTE_DEBUG=1 \
	-DVITTE_SANITIZE=1

CFLAGS_ASAN := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-fsanitize=address \
	-DVITTE_DEBUG=1 \
	-DVITTE_ASAN=1

CFLAGS_UBSAN := \
	-O1 \
	-g3 \
	-fno-omit-frame-pointer \
	-fno-optimize-sibling-calls \
	-fsanitize=undefined \
	-DVITTE_DEBUG=1 \
	-DVITTE_UBSAN=1

# ------------------------------------------------------------------------------
# Optional hardening
# ------------------------------------------------------------------------------

HARDEN ?= 0

ifeq ($(HARDEN),1)
CFLAGS_RELEASE += \
	-fstack-protector-strong
endif

# ------------------------------------------------------------------------------
# Optional warnings-as-errors
# ------------------------------------------------------------------------------

WERROR ?= 0

ifeq ($(WERROR),1)
CFLAGS_COMMON += -Werror
endif

# ------------------------------------------------------------------------------
# Optional LTO
# ------------------------------------------------------------------------------

LTO ?= 0

ifeq ($(LTO),1)
CFLAGS_RELEASE += -flto
endif

# ------------------------------------------------------------------------------
# Verbosity
# ------------------------------------------------------------------------------

V ?= 0

ifeq ($(V),1)
Q :=
else
Q := @
endif

# ------------------------------------------------------------------------------
# Utility output
# ------------------------------------------------------------------------------

define PRINT_CC
	@printf '  CC      %s\n' "$(1)"
endef

define PRINT_AR
	@printf '  AR      %s\n' "$(1)"
endef

define PRINT_CHECK
	@printf '  CHECK   %s\n' "$(1)"
endef

# ==============================================================================
# Primary targets
# ==============================================================================

.PHONY: all
all: debug

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

# ==============================================================================
# Archive generation
# ==============================================================================

$(DEBUG_LIB): $(DEBUG_OBJECTS)
	$(call PRINT_AR,$@)
	$(Q)$(MKDIR) "$(dir $@)"
	$(Q)$(RM) "$@"
	$(Q)$(AR) rcs "$@" $(DEBUG_OBJECTS)
	$(Q)$(RANLIB) "$@" 2>/dev/null || true

$(RELEASE_LIB): $(RELEASE_OBJECTS)
	$(call PRINT_AR,$@)
	$(Q)$(MKDIR) "$(dir $@)"
	$(Q)$(RM) "$@"
	$(Q)$(AR) rcs "$@" $(RELEASE_OBJECTS)
	$(Q)$(RANLIB) "$@" 2>/dev/null || true

$(SANITIZE_LIB): $(SANITIZE_OBJECTS)
	$(call PRINT_AR,$@)
	$(Q)$(MKDIR) "$(dir $@)"
	$(Q)$(RM) "$@"
	$(Q)$(AR) rcs "$@" $(SANITIZE_OBJECTS)
	$(Q)$(RANLIB) "$@" 2>/dev/null || true

$(ASAN_LIB): $(ASAN_OBJECTS)
	$(call PRINT_AR,$@)
	$(Q)$(MKDIR) "$(dir $@)"
	$(Q)$(RM) "$@"
	$(Q)$(AR) rcs "$@" $(ASAN_OBJECTS)
	$(Q)$(RANLIB) "$@" 2>/dev/null || true

$(UBSAN_LIB): $(UBSAN_OBJECTS)
	$(call PRINT_AR,$@)
	$(Q)$(MKDIR) "$(dir $@)"
	$(Q)$(RM) "$@"
	$(Q)$(AR) rcs "$@" $(UBSAN_OBJECTS)
	$(Q)$(RANLIB) "$@" 2>/dev/null || true

# ==============================================================================
# Compilation rules
# ==============================================================================

$(DEBUG_DIR)/obj/%.o: $(THIS_DIR)/%.c
	$(call PRINT_CC,$<)
	$(Q)$(MKDIR) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CPPFLAGS) \
		$(CFLAGS_COMMON) \
		$(CFLAGS_DEBUG) \
		$(CFLAGS) \
		-c "$<" \
		-o "$@"

$(RELEASE_DIR)/obj/%.o: $(THIS_DIR)/%.c
	$(call PRINT_CC,$<)
	$(Q)$(MKDIR) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CPPFLAGS) \
		$(CFLAGS_COMMON) \
		$(CFLAGS_RELEASE) \
		$(CFLAGS) \
		-c "$<" \
		-o "$@"

$(SANITIZE_DIR)/obj/%.o: $(THIS_DIR)/%.c
	$(call PRINT_CC,$<)
	$(Q)$(MKDIR) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CPPFLAGS) \
		$(CFLAGS_COMMON) \
		$(CFLAGS_SANITIZE) \
		$(CFLAGS) \
		-c "$<" \
		-o "$@"

$(ASAN_DIR)/obj/%.o: $(THIS_DIR)/%.c
	$(call PRINT_CC,$<)
	$(Q)$(MKDIR) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CPPFLAGS) \
		$(CFLAGS_COMMON) \
		$(CFLAGS_ASAN) \
		$(CFLAGS) \
		-c "$<" \
		-o "$@"

$(UBSAN_DIR)/obj/%.o: $(THIS_DIR)/%.c
	$(call PRINT_CC,$<)
	$(Q)$(MKDIR) "$(dir $@)"
	$(Q)$(CC) \
		$(CPPFLAGS_COMMON) \
		$(CPPFLAGS) \
		$(CFLAGS_COMMON) \
		$(CFLAGS_UBSAN) \
		$(CFLAGS) \
		-c "$<" \
		-o "$@"

# ==============================================================================
# Syntax validation
# ==============================================================================

.PHONY: syntax
syntax:
	@set -e; \
	for src in $(SOURCES); do \
		printf '  CHECK   %s\n' "$$src"; \
		$(CC) \
			$(CPPFLAGS_COMMON) \
			$(CPPFLAGS) \
			$(CFLAGS_COMMON) \
			$(CFLAGS_DEBUG) \
			$(CFLAGS) \
			-fsyntax-only \
			"$$src"; \
	done

# ==============================================================================
# Strict validation
# ==============================================================================

.PHONY: check
check:
	@$(MAKE) --no-print-directory \
		WERROR=1 \
		syntax

# ==============================================================================
# Preprocessor validation
# ==============================================================================

.PHONY: preprocess
preprocess:
	@set -e; \
	for src in $(SOURCES); do \
		printf '  CPP     %s\n' "$$src"; \
		$(CC) \
			$(CPPFLAGS_COMMON) \
			$(CPPFLAGS) \
			$(CSTD) \
			-E \
			"$$src" \
			-o /dev/null; \
	done

# ==============================================================================
# List discovered sources
# ==============================================================================

.PHONY: list
list:
	@printf '%s\n' $(SOURCE_REL)

# ==============================================================================
# List expected objects
# ==============================================================================

.PHONY: objects
objects:
	@printf '%s\n' $(DEBUG_OBJECTS)

# ==============================================================================
# Build information
# ==============================================================================

.PHONY: info
info:
	@printf '%-24s %s\n' "Makefile:" "$(THIS_MAKEFILE)"
	@printf '%-24s %s\n' "Backend directory:" "$(THIS_DIR)"
	@printf '%-24s %s\n' "Project root:" "$(PROJECT_ROOT)"
	@printf '%-24s %s\n' "Source root:" "$(SOURCE_ROOT)"
	@printf '%-24s %s\n' "Compiler source root:" "$(SRC_ROOT)"
	@printf '%-24s %s\n' "Public include:" "$(ROOT_INCLUDE)"
	@printf '%-24s %s\n' "Internal include:" "$(INCLUDE_ROOT)"
	@printf '%-24s %s\n' "Build root:" "$(BUILD_ROOT)"
	@printf '%-24s %s\n' "Build directory:" "$(BUILD_DIR)"
	@printf '%-24s %s\n' "Compiler:" "$(CC)"
	@printf '%-24s %s\n' "Compiler version:" "$(CC_VERSION_TEXT)"
	@printf '%-24s %s\n' "Archiver:" "$(AR)"
	@printf '%-24s %s\n' "Ranlib:" "$(RANLIB)"
	@printf '%-24s %s\n' "C standard:" "$(CSTD)"
	@printf '%-24s %s\n' "Sources:" "$(words $(SOURCES))"
	@printf '%-24s %s\n' "Werror:" "$(WERROR)"
	@printf '%-24s %s\n' "Hardening:" "$(HARDEN)"
	@printf '%-24s %s\n' "LTO:" "$(LTO)"
	@printf '\nCPPFLAGS:\n  %s\n' "$(CPPFLAGS_COMMON) $(CPPFLAGS)"
	@printf '\nCFLAGS common:\n  %s\n' "$(CFLAGS_COMMON) $(CFLAGS)"
	@printf '\nDebug flags:\n  %s\n' "$(CFLAGS_DEBUG)"
	@printf '\nRelease flags:\n  %s\n' "$(CFLAGS_RELEASE)"
	@printf '\nSanitize flags:\n  %s\n' "$(CFLAGS_SANITIZE)"

# ==============================================================================
# Archive inspection
# ==============================================================================

.PHONY: archive-list
archive-list: debug
	@$(AR) t "$(DEBUG_LIB)"

# ==============================================================================
# Compiler macros / include diagnostics
# ==============================================================================

.PHONY: compiler-info
compiler-info:
	@printf 'Compiler:\n'
	@$(CC) --version 2>/dev/null | head -n 3 || true
	@printf '\nInclude search paths:\n'
	@printf '' | \
		$(CC) \
			$(CPPFLAGS_COMMON) \
			$(CPPFLAGS) \
			$(CSTD) \
			-E \
			-v \
			-x c - \
			-o /dev/null \
			2>&1 || true

# ==============================================================================
# Check system-header shadowing
# ==============================================================================

.PHONY: check-string-header
check-string-header:
	@printf '#include <string.h>\n' | \
		$(CC) \
			$(CPPFLAGS_COMMON) \
			$(CPPFLAGS) \
			$(CSTD) \
			-H \
			-E \
			-x c - \
			-o /dev/null \
			2>&1

# ==============================================================================
# Directory creation
# ==============================================================================

.PHONY: dirs
dirs:
	@$(MKDIR) \
		"$(DEBUG_DIR)" \
		"$(RELEASE_DIR)" \
		"$(SANITIZE_DIR)" \
		"$(ASAN_DIR)" \
		"$(UBSAN_DIR)"

# ==============================================================================
# Clean
# ==============================================================================

.PHONY: clean
clean:
	@printf '  CLEAN   %s\n' "$(BUILD_DIR)"
	$(Q)$(RMDIR) "$(BUILD_DIR)"

.PHONY: clean-debug
clean-debug:
	@printf '  CLEAN   %s\n' "$(DEBUG_DIR)"
	$(Q)$(RMDIR) "$(DEBUG_DIR)"

.PHONY: clean-release
clean-release:
	@printf '  CLEAN   %s\n' "$(RELEASE_DIR)"
	$(Q)$(RMDIR) "$(RELEASE_DIR)"

.PHONY: clean-sanitize
clean-sanitize:
	@printf '  CLEAN   %s\n' "$(SANITIZE_DIR)"
	$(Q)$(RMDIR) "$(SANITIZE_DIR)"

# ==============================================================================
# Distclean
# ==============================================================================

.PHONY: distclean
distclean:
	@printf '  CLEAN   %s\n' "$(BUILD_ROOT)"
	$(Q)$(RMDIR) "$(BUILD_ROOT)"

# ==============================================================================
# Rebuild
# ==============================================================================

.PHONY: rebuild
rebuild:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory all

.PHONY: rebuild-release
rebuild-release:
	@$(MAKE) --no-print-directory clean-release
	@$(MAKE) --no-print-directory release

# ==============================================================================
# Help
# ==============================================================================

.PHONY: help
help:
	@printf '%s\n' \
		'Vitte ISO C17 backend build system' \
		'' \
		'Targets:' \
		'  all              Build default debug archive' \
		'  debug            Build debug archive' \
		'  release          Build optimized release archive' \
		'  sanitize         Build ASan + UBSan archive' \
		'  asan             Build AddressSanitizer archive' \
		'  ubsan            Build UndefinedBehaviorSanitizer archive' \
		'  profiles         Build all profiles' \
		'  syntax           Syntax-check every backend C source' \
		'  check            Strict syntax check with -Werror' \
		'  preprocess       Preprocess every source' \
		'  list             List discovered C sources' \
		'  objects          List debug object paths' \
		'  info             Display build configuration' \
		'  compiler-info    Display compiler/include information' \
		'  check-string-header' \
		'                   Show which <string.h> is selected' \
		'  archive-list     List objects in debug archive' \
		'  clean            Remove backend build directory' \
		'  distclean        Remove complete build root' \
		'  rebuild          Clean and rebuild debug' \
		'  rebuild-release  Clean and rebuild release' \
		'  help             Display this help' \
		'' \
		'Variables:' \
		'  CC=clang         Select C compiler' \
		'  AR=ar            Select archiver' \
		'  WERROR=1         Treat warnings as errors' \
		'  HARDEN=1         Enable release hardening' \
		'  LTO=1            Enable release LTO' \
		'  V=1              Verbose commands' \
		'' \
		'Examples:' \
		'  make' \
		'  make check' \
		'  make release' \
		'  make sanitize' \
		'  make WERROR=1 release' \
		'  make CC=clang WERROR=1 check' \
		'  make info'

# ==============================================================================
# Dependency inclusion
# ==============================================================================

-include $(DEBUG_DEPS)
-include $(RELEASE_DEPS)
-include $(SANITIZE_DEPS)
-include $(ASAN_DEPS)
-include $(UBSAN_DEPS)

# ==============================================================================
# Safety
# ==============================================================================

#
# Disable implicit suffix rules. Every compilation path above is explicit.
#
.SUFFIXES:

#
# Do not delete intermediate object files on interrupted dependency chains.
#
.SECONDARY:

#
# Delete partially generated targets if a recipe fails.
#
.DELETE_ON_ERROR:
