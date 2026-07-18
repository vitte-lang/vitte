ROOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
OUT_DIR ?= $(ROOT_DIR)/pkgout
VERSION ?= $(shell tr -d ' \r\n' < $(ROOT_DIR)/toolchain/scripts/package/PACKAGE_VERSION)

.PHONY: help matrix matrix-test macos-all macos-arm64 macos-x86_64 macos-universal

help:
	@echo "Vitte installer matrix"
	@echo "  make -f scripts_build/Makefile matrix"
	@echo "  make -f scripts_build/Makefile matrix-test"
	@echo "  make -f scripts_build/Makefile macos-all"
	@echo "  make -f scripts_build/Makefile macos-arm64"
	@echo "  make -f scripts_build/Makefile macos-x86_64"
	@echo "  make -f scripts_build/Makefile macos-universal"

matrix:
	@$(ROOT_DIR)/scripts_build/package-matrix.sh list

matrix-test:
	@$(ROOT_DIR)/tools/package_platform_matrix_test.sh

macos-all:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=all $(ROOT_DIR)/scripts_build/build-macos-installers.sh

macos-arm64:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=arm64 $(ROOT_DIR)/scripts_build/build-macos-installers.sh

macos-x86_64:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=x86_64 $(ROOT_DIR)/scripts_build/build-macos-installers.sh

macos-universal:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=universal $(ROOT_DIR)/scripts_build/build-macos-installers.sh

.DEFAULT_GOAL := help
