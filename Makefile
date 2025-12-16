.PHONY: all build clean run run-list run-all help

BUILD_DIR ?= build
CMAKE_FLAGS ?= -DCMAKE_BUILD_TYPE=Release

help:
	@echo "vitte-bench Makefile targets:"
	@echo "  make build         - Build benchmark executable"
	@echo "  make clean         - Clean build directory"
	@echo "  make run           - Build and run all benchmarks"
	@echo "  make run-list      - List available benchmarks"
	@echo "  make run-micro     - Run only micro benchmarks"
	@echo "  make run-macro     - Run only macro benchmarks"

build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake $(CMAKE_FLAGS) .. && cmake --build . -j

clean:
	rm -rf $(BUILD_DIR)

run: build
	$(BUILD_DIR)/benchc --all

run-list: build
	$(BUILD_DIR)/benchc --list-full

run-micro: build
	$(BUILD_DIR)/benchc --filter micro

run-macro: build
	$(BUILD_DIR)/benchc --filter macro

all: build
