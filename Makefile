# C:\Users\vince\Documents\GitHub\vitte\Makefile
#
# Vitte – build orchestration for the C toolchain (C99) + convenience targets.
#
# Layout assumptions:
#   compiler/            -> C compiler sources (vittec)
#   runtime/             -> optional C runtime sources
#   tests/               -> optional C tests
#   bench/               -> optional C benchmarks
#
# Usage:
#   make            # default release build (via CMake)
#   make debug      # debug build
#   make run ARGS="--help"
#   make test       # ctest
#   make clean
#   make format     # clang-format if present
#   make lint       # clang-tidy if present
#
# Notes:
# - Uses out-of-source builds in build/{release,debug}
# - Works on macOS/Linux; Windows via MSYS2/WSL.

SHELL := /bin/sh

PROJECT := vitte
BIN := vittec

BUILD_DIR := build
REL_DIR := $(BUILD_DIR)/release
DBG_DIR := $(BUILD_DIR)/debug

CMAKE ?= cmake
CTEST ?= ctest
NINJA ?= ninja

# Prefer Ninja if available, fallback to Makefiles generator.
GENERATOR :=
ifneq (,$(shell command -v $(NINJA) 2>/dev/null))
  GENERATOR := -G Ninja
endif

# Common CMake options
CMAKE_COMMON_OPTS := \
  -DVITTE_BUILD_TESTS=ON \
  -DVITTE_BUILD_BENCH=OFF \
  -DVITTE_STRICT_WERROR=OFF \
  -DVITTE_ENABLE_SANITIZERS=OFF

# ---- helpers -----------------------------------------------------------------

define cmake_configure
	$(CMAKE) -S . -B $(1) $(GENERATOR) -DCMAKE_BUILD_TYPE=$(2) $(CMAKE_COMMON_OPTS)
endef

define cmake_build
	$(CMAKE) --build $(1) --config $(2)
endef

# ---- targets -----------------------------------------------------------------

.PHONY: all
all: release

.PHONY: release
release:
	@mkdir -p "$(REL_DIR)"
	@$(call cmake_configure,$(REL_DIR),Release)
	@$(call cmake_build,$(REL_DIR),Release)

.PHONY: debug
debug:
	@mkdir -p "$(DBG_DIR)"
	@$(call cmake_configure,$(DBG_DIR),Debug)
	@$(call cmake_build,$(DBG_DIR),Debug)

.PHONY: relwithdebinfo
relwithdebinfo:
	@mkdir -p "$(BUILD_DIR)/relwithdebinfo"
	@$(call cmake_configure,$(BUILD_DIR)/relwithdebinfo,RelWithDebInfo)
	@$(call cmake_build,$(BUILD_DIR)/relwithdebinfo,RelWithDebInfo)

.PHONY: asan
asan:
	@mkdir -p "$(BUILD_DIR)/asan"
	@$(CMAKE) -S . -B "$(BUILD_DIR)/asan" $(GENERATOR) -DCMAKE_BUILD_TYPE=Debug \
	  -DVITTE_BUILD_TESTS=ON -DVITTE_ENABLE_SANITIZERS=ON
	@$(CMAKE) --build "$(BUILD_DIR)/asan" --config Debug

.PHONY: run
run: release
	@"$(REL_DIR)/$(BIN)" $(ARGS)

.PHONY: run-debug
run-debug: debug
	@"$(DBG_DIR)/$(BIN)" $(ARGS)

.PHONY: test
test: debug
	@cd "$(DBG_DIR)" && $(CTEST) --output-on-failure

.PHONY: test-release
test-release: release
	@cd "$(REL_DIR)" && $(CTEST) --output-on-failure

.PHONY: bench
bench:
	@mkdir -p "$(BUILD_DIR)/bench"
	@$(CMAKE) -S . -B "$(BUILD_DIR)/bench" $(GENERATOR) -DCMAKE_BUILD_TYPE=Release \
	  -DVITTE_BUILD_TESTS=OFF -DVITTE_BUILD_BENCH=ON
	@$(CMAKE) --build "$(BUILD_DIR)/bench" --config Release

.PHONY: install
install: release
	@$(CMAKE) --install "$(REL_DIR)"

.PHONY: clean
clean:
	@rm -rf "$(BUILD_DIR)"

.PHONY: re
re: clean all

# ---- tooling -----------------------------------------------------------------

.PHONY: format
format:
	@if command -v clang-format >/dev/null 2>&1; then \
	  echo "[format] clang-format"; \
	  find compiler runtime tests bench -type f \( -name '*.c' -o -name '*.h' \) -print 2>/dev/null | \
	    xargs clang-format -i; \
	else \
	  echo "[format] clang-format not found"; \
	fi

.PHONY: lint
lint:
	@if command -v clang-tidy >/dev/null 2>&1; then \
	  echo "[lint] clang-tidy (requires compile_commands.json)"; \
	  mkdir -p "$(DBG_DIR)"; \
	  $(CMAKE) -S . -B "$(DBG_DIR)" $(GENERATOR) -DCMAKE_BUILD_TYPE=Debug \
	    $(CMAKE_COMMON_OPTS) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON; \
	  clang-tidy -p "$(DBG_DIR)" $$(find compiler -type f -name '*.c' 2>/dev/null); \
	else \
	  echo "[lint] clang-tidy not found"; \
	fi

.PHONY: help
help:
	@echo "Targets:"
	@echo "  make | make release        Build Release (default)"
	@echo "  make debug                 Build Debug"
	@echo "  make asan                  Build Debug with ASan/UBSan"
	@echo "  make run ARGS='...'         Run vittec (release)"
	@echo "  make run-debug ARGS='...'   Run vittec (debug)"
	@echo "  make test                  Run tests (debug)"
	@echo "  make bench                 Build benchmarks (release)"
	@echo "  make install               Install (from release build dir)"
	@echo "  make format                clang-format (if available)"
	@echo "  make lint                  clang-tidy (if available)"
	@echo "  make clean                 Remove build/"
