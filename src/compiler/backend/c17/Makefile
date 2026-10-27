# Native Vitte C17 backend sources are compiled as part of the compiler.
# The C bootstrap implementation lives separately in src/bootstrap/backend/c17.

THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
PROJECT_ROOT := $(abspath $(THIS_DIR)/../../../..)

.PHONY: all smoke clean

all smoke clean:
	@$(MAKE) --no-print-directory -C "$(PROJECT_ROOT)" $@
