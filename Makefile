.PHONY: all bootstrap stage1 stage2 self-host debug release test smoke dist clean

ROOT        := $(CURDIR)
TARGET_DIR  := $(ROOT)/target
BOOT_DIR    := $(TARGET_DIR)/bootstrap
BIN_DIR     := $(ROOT)/target/release
VITTEC      := $(BIN_DIR)/vittec
PYTHON      ?= python3

all: release

bootstrap:
	@$(ROOT)/scripts/bootstrap_stage0.sh

stage1:
	@$(ROOT)/scripts/self_host_stage1.sh

stage2 self-host:
	@$(ROOT)/scripts/hooks/build_vittec_stage2.sh

debug:
	@$(ROOT)/scripts/steelc build-debug

release:
	@$(ROOT)/scripts/steelc build-release

smoke:
	@$(ROOT)/scripts/run_goldens.sh smoke

test:
	@$(PYTHON) $(ROOT)/tools/vitte_tests.py

dist:
	@$(ROOT)/scripts/dist.sh

clean:
	@$(ROOT)/scripts/clean.sh
