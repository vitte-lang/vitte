# =============================================================================
# VITTE TETRIS — MAKEFILE
# =============================================================================
#
# Project:
#   Vitte Tetris
#
# Sources:
#   main.vitte
#   game.vitte
#   board.vitte
#   piece.vitte
#   terminal.vitte
#
# Commands:
#   make
#   make build
#   make debug
#   make release
#   make run
#   make test
#   make check
#   make clean
#   make rebuild
#   make install
#   make uninstall
#   make info
#   make help
#
# =============================================================================

PROJECT       := vitte-tetris
VERSION       := 0.1.0

SHELL         := /bin/sh

# =============================================================================
# DIRECTORIES
# =============================================================================

ROOT_DIR      := $(abspath .)
BUILD_DIR     := $(ROOT_DIR)/build
DEBUG_DIR     := $(BUILD_DIR)/debug
RELEASE_DIR   := $(BUILD_DIR)/release

# =============================================================================
# COMPILER
# =============================================================================

REPOSITORY_DIR := $(abspath $(ROOT_DIR)/../..)

VITTEC        ?= $(REPOSITORY_DIR)/build/bin/vitte
VITTE_MODULE_PATH ?= $(REPOSITORY_DIR)/modules/core/src

VITTE_FLAGS   ?= --backend=c17
DEBUG_FLAGS   ?= --profile=debug --emit-debug-info
RELEASE_FLAGS ?= --profile=release --opt=2

# =============================================================================
# SOURCE FILES
# =============================================================================

MAIN_SOURCE   := main.vitte

MODULES       := \
	board.vitte \
	piece.vitte \
	game.vitte \
	terminal.vitte

SOURCES       := $(MAIN_SOURCE) $(MODULES)

# =============================================================================
# OUTPUT
# =============================================================================

DEBUG_TARGET   := $(DEBUG_DIR)/$(PROJECT)
RELEASE_TARGET := $(RELEASE_DIR)/$(PROJECT)

# =============================================================================
# INSTALLATION
# =============================================================================

PREFIX        ?= /usr/local
BINDIR        ?= $(PREFIX)/bin

INSTALL       ?= install

# =============================================================================
# DEFAULT TARGET
# =============================================================================

.DEFAULT_GOAL := all

.PHONY: \
	all \
	build \
	debug \
	release \
	run \
	run-debug \
	run-release \
	test \
	check \
	clean \
	distclean \
	rebuild \
	install \
	uninstall \
	info \
	help \
	verify \
	dirs

# =============================================================================
# ALL
# =============================================================================

all: release

build: release

# =============================================================================
# VERIFY COMPILER
# =============================================================================

verify:
	@command -v "$(VITTEC)" >/dev/null 2>&1 || { \
		echo "Error: Vitte compiler not found: $(VITTEC)"; \
		echo "Build the repository compiler or set VITTEC=/path/to/vitte"; \
		exit 1; \
	}
	@echo "[vitte] Compiler found: $(VITTEC)"

# =============================================================================
# DIRECTORIES
# =============================================================================

dirs:
	@mkdir -p "$(DEBUG_DIR)"
	@mkdir -p "$(RELEASE_DIR)"

# =============================================================================
# DEBUG BUILD
# =============================================================================

debug: verify dirs
	@echo "========================================"
	@echo " Building Vitte Tetris — DEBUG"
	@echo "========================================"
	cd "$(REPOSITORY_DIR)" && \
	VITTE_MODULE_PATH="$(VITTE_MODULE_PATH)" "$(VITTEC)" compile \
		$(VITTE_FLAGS) $(DEBUG_FLAGS) \
		games/tetris/$(MAIN_SOURCE) \
		-o "$(DEBUG_TARGET)"
	@echo "[vitte] Debug build completed."
	@echo "[vitte] Binary: $(DEBUG_TARGET)"

# =============================================================================
# RELEASE BUILD
# =============================================================================

release: verify dirs
	@echo "========================================"
	@echo " Building Vitte Tetris — RELEASE"
	@echo "========================================"
	cd "$(REPOSITORY_DIR)" && \
	VITTE_MODULE_PATH="$(VITTE_MODULE_PATH)" "$(VITTEC)" compile \
		$(VITTE_FLAGS) $(RELEASE_FLAGS) \
		games/tetris/$(MAIN_SOURCE) \
		-o "$(RELEASE_TARGET)"
	@echo "[vitte] Release build completed."
	@echo "[vitte] Binary: $(RELEASE_TARGET)"

# =============================================================================
# RUN
# =============================================================================

run: release
	@echo "[vitte] Starting Tetris..."
	@"$(RELEASE_TARGET)"

run-debug: debug
	@echo "[vitte] Starting Tetris (Debug)..."
	@"$(DEBUG_TARGET)"

run-release: release
	@"$(RELEASE_TARGET)"

# =============================================================================
# CHECK
# =============================================================================

check: verify
	@echo "[vitte] Checking source files..."
	@for file in $(SOURCES); do \
		if [ ! -f "$$file" ]; then \
			echo "[error] Missing source: $$file"; \
			exit 1; \
		fi; \
		echo "[ok] $$file"; \
	done
	@echo "[vitte] Running semantic check..."
	cd "$(REPOSITORY_DIR)" && \
	VITTE_MODULE_PATH="$(VITTE_MODULE_PATH)" "$(VITTEC)" check \
		$(VITTE_FLAGS) games/tetris/$(MAIN_SOURCE)
	@echo "[vitte] Source and semantic checks completed."

# =============================================================================
# TEST
# =============================================================================

test: check
	@echo "[warning] The current compiler CLI does not run inline test blocks."

# =============================================================================
# CLEAN
# =============================================================================

clean:
	@echo "[vitte] Cleaning build artifacts..."
	@rm -rf "$(BUILD_DIR)"
	@echo "[vitte] Clean completed."

distclean: clean

# =============================================================================
# REBUILD
# =============================================================================

rebuild: clean release

# =============================================================================
# INSTALL
# =============================================================================

install: release
	@echo "[vitte] Installing $(PROJECT)..."
	@mkdir -p "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -m 755 \
		"$(RELEASE_TARGET)" \
		"$(DESTDIR)$(BINDIR)/$(PROJECT)"
	@echo "[vitte] Installation completed."

# =============================================================================
# UNINSTALL
# =============================================================================

uninstall:
	@echo "[vitte] Uninstalling $(PROJECT)..."
	@rm -f "$(DESTDIR)$(BINDIR)/$(PROJECT)"
	@echo "[vitte] Uninstallation completed."

# =============================================================================
# INFORMATION
# =============================================================================

info:
	@echo "========================================"
	@echo " VITTE TETRIS"
	@echo "========================================"
	@echo "Project       : $(PROJECT)"
	@echo "Version       : $(VERSION)"
	@echo "Root          : $(ROOT_DIR)"
	@echo "Compiler      : $(VITTEC)"
	@echo "Build dir     : $(BUILD_DIR)"
	@echo "Debug target  : $(DEBUG_TARGET)"
	@echo "Release target: $(RELEASE_TARGET)"
	@echo "Prefix        : $(PREFIX)"
	@echo "Sources       : $(SOURCES)"
	@echo "========================================"

# =============================================================================
# HELP
# =============================================================================

help:
	@echo "Vitte Tetris — available targets:"
	@echo ""
	@echo "  make             Build release version"
	@echo "  make build       Build release version"
	@echo "  make debug       Build debug version"
	@echo "  make release     Build release version"
	@echo "  make run         Build and run Tetris"
	@echo "  make run-debug   Build and run debug version"
	@echo "  make test        Run source checks (inline tests need a test runner)"
	@echo "  make check       Run source and semantic checks"
	@echo "  make clean       Remove build artifacts"
	@echo "  make distclean   Remove all build artifacts"
	@echo "  make rebuild     Clean and rebuild"
	@echo "  make install     Install executable"
	@echo "  make uninstall   Remove installed executable"
	@echo "  make info        Show build configuration"
	@echo "  make help        Show this help"
