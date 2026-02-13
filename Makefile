# ============================================================
# Makefile — Vitte Project
# Target: compiler / runtime / std / tools
# ============================================================

# ------------------------------------------------------------
# Global config
# ------------------------------------------------------------

PROJECT      := vitte
BUILD_DIR    := build
BIN_DIR      := bin
SRC_DIR      := src
STD_DIR      := src/vitte/std
TOOLS_DIR    := tools

CC           := clang
CXX          := clang++
AR           := ar
RM           := rm -rf
MKDIR        := mkdir -p

CFLAGS       := -std=c17 -Wall -Wextra -Werror -O2 -g
CXXFLAGS     := -std=c++20 -Wall -Wextra -Werror -O2 -g
LDFLAGS      :=

# Optional dependency roots (e.g. Homebrew)
ifdef OPENSSL_DIR
  CXXFLAGS += -I$(OPENSSL_DIR)/include
  LDFLAGS  += -L$(OPENSSL_DIR)/lib
endif
ifdef CURL_DIR
  CXXFLAGS += -I$(CURL_DIR)/include
  LDFLAGS  += -L$(CURL_DIR)/lib
endif

# Runtime deps
LDFLAGS += -lssl -lcrypto -lcurl

CLANG_TIDY   := clang-tidy
FORMAT       := clang-format

# ------------------------------------------------------------
# Files
# ------------------------------------------------------------

C_SOURCES    := $(shell find $(SRC_DIR) -name '*.c')
CPP_SOURCES  := $(shell find $(SRC_DIR) -name '*.cpp')
OBJECTS     := \
	$(C_SOURCES:%.c=$(BUILD_DIR)/%.o) \
	$(CPP_SOURCES:%.cpp=$(BUILD_DIR)/%.o)

# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------

.PHONY: all
all: build

# ------------------------------------------------------------
# Build
# ------------------------------------------------------------

.PHONY: build
build: dirs $(BIN_DIR)/$(PROJECT)

$(BIN_DIR)/$(PROJECT): $(OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ------------------------------------------------------------
# Directories
# ------------------------------------------------------------

.PHONY: dirs
dirs:
	@$(MKDIR) $(BUILD_DIR)
	@$(MKDIR) $(BIN_DIR)

# ------------------------------------------------------------
# Formatting
# ------------------------------------------------------------

.PHONY: format
format:
	$(FORMAT) -i $(C_SOURCES) $(CPP_SOURCES)

# ------------------------------------------------------------
# Static analysis
# ------------------------------------------------------------

.PHONY: tidy
tidy:
	$(CLANG_TIDY) \
		$(CPP_SOURCES) \
		-- \
		$(CXXFLAGS)

# ------------------------------------------------------------
# Tests (placeholder)
# ------------------------------------------------------------

.PHONY: test
test:
	@toolchain/scripts/test/run.sh

.PHONY: parse
parse:
	@tools/parse_tests.sh

.PHONY: parse-modules
parse-modules:
	@tools/parse_modules_tests.sh

.PHONY: parse-strict
parse-strict:
	@STRICT_ONLY=1 tools/parse_tests.sh

.PHONY: parse-watch
parse-watch:
	@tools/parse_watch.sh

.PHONY: hir-validate-test
hir-validate-test: dirs
	$(CXX) $(CXXFLAGS) -Isrc \
		tools/hir_validate_test.cpp \
		src/compiler/ir/validate.cpp \
		src/compiler/ir/hir.cpp \
		src/compiler/frontend/diagnostics.cpp \
		src/compiler/frontend/ast.cpp \
		-o $(BIN_DIR)/hir_validate_test

.PHONY: hir-validate-fixture
hir-validate-fixture: dirs
	$(CXX) $(CXXFLAGS) -Isrc \
		tools/hir_validate_fixture.cpp \
		src/compiler/frontend/lexer.cpp \
		src/compiler/frontend/parser.cpp \
		src/compiler/frontend/diagnostics.cpp \
		src/compiler/frontend/ast.cpp \
		src/compiler/frontend/disambiguate.cpp \
		src/compiler/frontend/lower_hir.cpp \
		src/compiler/ir/hir.cpp \
		src/compiler/ir/validate.cpp \
		-o $(BIN_DIR)/hir_validate_fixture

.PHONY: hir-validate
hir-validate: hir-validate-test hir-validate-fixture
	@$(BIN_DIR)/hir_validate_test
	@$(BIN_DIR)/hir_validate_fixture

.PHONY: check-tests
check-tests:
	@tools/check_tests.sh

.PHONY: stress-alloc
stress-alloc:
	@tools/stress_alloc_examples.sh

.PHONY: core-projects
core-projects:
	@tools/build_core_projects.sh

.PHONY: arduino-projects
arduino-projects:
	@tools/build_arduino_projects.sh

.PHONY: negative-tests
negative-tests:
	@tools/negative_tests.sh

.PHONY: update-diagnostics-ftl
update-diagnostics-ftl:
	@tools/update_diagnostics_ftl.py

.PHONY: repro
repro:
	@tools/repro_compare.sh

.PHONY: repro-generate
repro-generate:
	@tools/repro_generate_cpp.sh

# ------------------------------------------------------------
# Stdlib checks
# ------------------------------------------------------------

.PHONY: std-check
std-check:
	@echo "Checking std layout…"
	@test -d $(STD_DIR)/core
	@test -d $(STD_DIR)/io
	@test -d $(STD_DIR)/math
	@test -d $(STD_DIR)/test
	@echo "std layout OK"

# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------

.PHONY: clean
clean:
	$(RM) $(BUILD_DIR) $(BIN_DIR)

# ------------------------------------------------------------
# Distclean
# ------------------------------------------------------------

.PHONY: distclean
distclean: clean
	$(RM) .cache .clangd

# ------------------------------------------------------------
# Help
# ------------------------------------------------------------

.PHONY: help
help:
	@echo ""
	@echo "Vitte Makefile targets:"
	@echo ""
	@echo "  make            build everything"
	@echo "  make format     run clang-format"
	@echo "  make tidy       run clang-tidy"
	@echo "  make test       run tests (std/test)"
	@echo "  make std-check  verify std layout"
	@echo "  make clean      remove build artifacts"
	@echo "  make distclean  full cleanup"
	@echo ""
