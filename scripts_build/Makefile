ROOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
OUT_DIR ?= $(ROOT_DIR)/pkgout
VERSION ?= $(shell tr -d ' \r\n' < $(ROOT_DIR)/toolchain/scripts/package/PACKAGE_VERSION)

.PHONY: all-installers help matrix matrix-test freebsd-all freebsd-amd64 freebsd-arm64 linux-all linux-amd64 linux-arm64 linux-i386 macos-all macos-arm64 macos-x86_64 macos-universal solaris-amd64 verify windows-amd64

help:
	@echo "Vitte installer matrix"
	@echo "  make -f scripts_build/Makefile all-installers"
	@echo "  make -f scripts_build/Makefile verify"
	@echo "  make -f scripts_build/Makefile matrix"
	@echo "  make -f scripts_build/Makefile matrix-test"
	@echo "  make -f scripts_build/Makefile freebsd-all"
	@echo "  make -f scripts_build/Makefile freebsd-amd64"
	@echo "  make -f scripts_build/Makefile freebsd-arm64"
	@echo "  make -f scripts_build/Makefile linux-all"
	@echo "  make -f scripts_build/Makefile linux-amd64"
	@echo "  make -f scripts_build/Makefile linux-arm64"
	@echo "  make -f scripts_build/Makefile linux-i386"
	@echo "  make -f scripts_build/Makefile macos-all"
	@echo "  make -f scripts_build/Makefile macos-arm64"
	@echo "  make -f scripts_build/Makefile macos-x86_64"
	@echo "  make -f scripts_build/Makefile macos-universal"
	@echo "  make -f scripts_build/Makefile solaris-amd64"
	@echo "  make -f scripts_build/Makefile windows-amd64"

matrix:
	@$(ROOT_DIR)/scripts_build/package-matrix.sh list

matrix-test:
	@$(ROOT_DIR)/tools/package_platform_matrix_test.sh

all-installers:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) $(ROOT_DIR)/scripts_build/build-all-installers.sh

verify:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) $(ROOT_DIR)/scripts_build/verify-installers.sh

freebsd-all:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=all $(ROOT_DIR)/scripts_build/build-freebsd-packages.sh

freebsd-amd64:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=amd64 $(ROOT_DIR)/scripts_build/build-freebsd-packages.sh

freebsd-arm64:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=arm64 $(ROOT_DIR)/scripts_build/build-freebsd-packages.sh

linux-all:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=all $(ROOT_DIR)/scripts_build/build-linux-debs.sh

linux-amd64:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=amd64 $(ROOT_DIR)/scripts_build/build-linux-debs.sh

linux-arm64:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=arm64 $(ROOT_DIR)/scripts_build/build-linux-debs.sh

linux-i386:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=i386 $(ROOT_DIR)/scripts_build/build-linux-debs.sh

macos-all:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=all $(ROOT_DIR)/scripts_build/build-macos-installers.sh

macos-arm64:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=arm64 $(ROOT_DIR)/scripts_build/build-macos-installers.sh

macos-x86_64:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=x86_64 $(ROOT_DIR)/scripts_build/build-macos-installers.sh

macos-universal:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=universal $(ROOT_DIR)/scripts_build/build-macos-installers.sh

solaris-amd64:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=amd64 $(ROOT_DIR)/scripts_build/build-solaris-package.sh

windows-amd64:
	@VERSION=$(VERSION) OUT_DIR=$(OUT_DIR) ARCH=amd64 $(ROOT_DIR)/scripts_build/build-windows-installer.sh

.DEFAULT_GOAL := help
