.PHONY: all clean build debug release test help install

# Directories
BUILD_DIR ?= build
DEBUG_DIR ?= $(BUILD_DIR)/debug
RELEASE_DIR ?= $(BUILD_DIR)/release
INSTALL_DIR ?= /usr/local

# Compiler
CMAKE ?= cmake
MAKE_CMD ?= $(MAKE)
CTEST ?= ctest

help:
	@echo "Vitte Compiler Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make debug       - Build debug version"
	@echo "  make release     - Build release version"
	@echo "  make test        - Run unit tests"
	@echo "  make examples    - Test example programs"
	@echo "  make clean       - Clean all builds"
	@echo "  make install     - Install compiler to $(INSTALL_DIR)"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_DIR=...    - Custom build directory (default: build)"
	@echo "  INSTALL_DIR=...  - Custom install directory (default: /usr/local)"
	@echo ""
	@echo "Examples:"
	@echo "  make debug          # Debug build"
	@echo "  make release        # Release build"
	@echo "  make test           # Run all tests"
	@echo "  make debug test     # Debug build + run tests"

all: debug

debug:
	@mkdir -p $(DEBUG_DIR)
	@cd $(DEBUG_DIR) && $(CMAKE) -DCMAKE_BUILD_TYPE=Debug ../..
	@cd $(DEBUG_DIR) && $(MAKE_CMD)
	@echo "Debug build complete: $(DEBUG_DIR)/vittec"

release:
	@mkdir -p $(RELEASE_DIR)
	@cd $(RELEASE_DIR) && $(CMAKE) -DCMAKE_BUILD_TYPE=Release ../..
	@cd $(RELEASE_DIR) && $(MAKE_CMD)
	@echo "Release build complete: $(RELEASE_DIR)/vittec"

test: debug
	@echo "Running unit tests..."
	@cd $(DEBUG_DIR) && $(CTEST) --verbose
	@echo "All unit tests passed!"

examples: debug
	@echo "Testing example programs..."
	@./test_examples.sh $(BUILD_DIR)

clean:
	@echo "Cleaning build directories..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete"

install: release
	@mkdir -p $(INSTALL_DIR)/bin
	@mkdir -p $(INSTALL_DIR)/include
	@cp $(RELEASE_DIR)/vittec $(INSTALL_DIR)/bin/
	@cp -r include/* $(INSTALL_DIR)/include/
	@echo "Installed to $(INSTALL_DIR)"
	@echo "Run: $(INSTALL_DIR)/bin/vittec --help"

.PHONY: all clean debug release test examples help install
