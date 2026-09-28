# CloneVitte — Root Makefile

PROJECT := CloneVitte
TARGET  := vitte

CC ?= cc

SRC_ROOT := src/src
INC_ROOT := src/include
TEST_DIR := src/tests

BUILD   := build
OBJ_DIR := $(BUILD)/obj
BIN_DIR := $(BUILD)/bin
DEP_DIR := $(BUILD)/dep

TARGET_BIN := $(BIN_DIR)/$(TARGET)

SOURCES := $(shell find $(SRC_ROOT) -type f -name '*.c' | sort)
OBJECTS := $(patsubst $(SRC_ROOT)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))
DEPS    := $(patsubst $(SRC_ROOT)/%.c,$(DEP_DIR)/%.d,$(SOURCES))

CPPFLAGS := \
	-I$(INC_ROOT) \
	-I$(SRC_ROOT)

CFLAGS_COMMON := \
	-std=c17 \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Wshadow \
	-Wconversion \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wformat=2

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

LDFLAGS :=
LDLIBS   :=

MODE ?= debug

ifeq ($(MODE),release)
	CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_RELEASE)
else ifeq ($(MODE),asan)
	CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_ASAN)
	LDFLAGS += -fsanitize=address,undefined
else
	CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_DEBUG)
endif

.PHONY: all
all: $(TARGET_BIN)

$(TARGET_BIN): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	@printf '\nBuilt: %s\n' "$@"

$(OBJ_DIR)/%.o: $(SRC_ROOT)/%.c
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEP_DIR)/$*.d)
	$(CC) \
		$(CPPFLAGS) \
		$(CFLAGS) \
		-MMD \
		-MP \
		-MF $(DEP_DIR)/$*.d \
		-c $< \
		-o $@

-include $(DEPS)

.PHONY: debug
debug:
	$(MAKE) MODE=debug all

.PHONY: release
release:
	$(MAKE) MODE=release all

.PHONY: asan
asan:
	$(MAKE) MODE=asan all

.PHONY: run
run: $(TARGET_BIN)
	$(TARGET_BIN)

.PHONY: compile
compile: $(TARGET_BIN)
ifndef FILE
	$(error FILE is required: make compile FILE=program.vit)
endif
	$(TARGET_BIN) $(FILE)

TEST_FILES := $(shell find $(TEST_DIR) -type f -name '*.vit' | sort)

.PHONY: test
test: $(TARGET_BIN)
	@set -e; \
	count=0; \
	failed=0; \
	for file in $(TEST_FILES); do \
		count=$$((count + 1)); \
		printf '[TEST] %s\n' "$$file"; \
		if ! $(TARGET_BIN) check "$$file"; then \
			printf '[FAIL] %s\n' "$$file"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	printf '\nTests: %d  Failed: %d\n' "$$count" "$$failed"; \
	test "$$failed" -eq 0

.PHONY: check
check:
	@printf 'Checking C sources...\n'
	@set -e; \
	for file in $(SOURCES); do \
		printf '[CC] %s\n' "$$file"; \
		$(CC) \
			$(CPPFLAGS) \
			$(CFLAGS_COMMON) \
			-fsyntax-only \
			"$$file"; \
	done

.PHONY: info
info:
	@printf 'Project:      %s\n' "$(PROJECT)"
	@printf 'Target:       %s\n' "$(TARGET_BIN)"
	@printf 'Compiler:     %s\n' "$(CC)"
	@printf 'Mode:         %s\n' "$(MODE)"
	@printf 'Source root:  %s\n' "$(SRC_ROOT)"
	@printf 'Include root: %s\n' "$(INC_ROOT)"
	@printf 'C sources:    %s\n' "$(words $(SOURCES))"
	@printf 'Objects:      %s\n' "$(words $(OBJECTS))"

.PHONY: clean
clean:
	rm -rf $(BUILD)

.PHONY: distclean
distclean: clean

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

.PHONY: install
install: $(TARGET_BIN)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET_BIN) $(DESTDIR)$(BINDIR)/$(TARGET)

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

.PHONY: help
help:
	@printf '%s\n' \
		'CloneVitte build system' \
		'' \
		'Targets:' \
		'  make                       Build debug compiler' \
		'  make debug                 Build debug compiler' \
		'  make release               Build optimized compiler' \
		'  make asan                  Build with ASan + UBSan' \
		'  make check                 Check all C sources' \
		'  make test                  Run Vitte tests' \
		'  make run                   Run compiler' \
		'  make compile FILE=x.vit    Compile a Vitte source' \
		'  make info                  Show build information' \
		'  make install               Install compiler' \
		'  make uninstall             Uninstall compiler' \
		'  make clean                 Remove build files'

# -----------------------------------------------------------------------------
# macOS builds
# -----------------------------------------------------------------------------

MACOS_ARM64_DIR := target/macos-arm64
MACOS_X86_64_DIR := target/macos-x86_64
MACOS_UNIVERSAL_DIR := target/universal

MACOS_ARM64_BIN := $(MACOS_ARM64_DIR)/vitte
MACOS_X86_64_BIN := $(MACOS_X86_64_DIR)/vitte
MACOS_UNIVERSAL_BIN := $(MACOS_UNIVERSAL_DIR)/vitte

MACOS_CC ?= clang

.PHONY: macos-arm64-bin
macos-arm64-bin:
	@printf 'Building Vitte for macOS arm64...\n'
	@mkdir -p $(MACOS_ARM64_DIR)
	$(MACOS_CC) $(CPPFLAGS) $(CFLAGS_COMMON) -O2 -DNDEBUG \
		-arch arm64 -mmacosx-version-min=11.0 \
		$(SOURCES) $(LDFLAGS) $(LDLIBS) \
		-o $(MACOS_ARM64_BIN)
	@lipo -archs $(MACOS_ARM64_BIN)
	@printf 'Built: %s\n' "$(MACOS_ARM64_BIN)"

.PHONY: macos-x86_64-bin
macos-x86_64-bin:
	@printf 'Building Vitte for macOS x86_64...\n'
	@mkdir -p $(MACOS_X86_64_DIR)
	$(MACOS_CC) $(CPPFLAGS) $(CFLAGS_COMMON) -O2 -DNDEBUG \
		-arch x86_64 -mmacosx-version-min=10.13 \
		$(SOURCES) $(LDFLAGS) $(LDLIBS) \
		-o $(MACOS_X86_64_BIN)
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
	@printf 'Built: %s\n' "$(MACOS_UNIVERSAL_BIN)"
