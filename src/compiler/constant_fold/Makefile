ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)
BOOTSTRAP_DIR := $(ROOT)/src/bootstrap/constant_fold

.DEFAULT_GOAL := help
.PHONY: help bootstrap native-frontend

help:
	@printf '%s\n' \
		'Native Vitte constant module' \
		'' \
		'constant.vitte is compiled through the root compiler source graph.' \
		'It is not imported by the frontend pipeline yet.' \
		'' \
		'Targets:' \
		'  bootstrap       Build the bootstrap C implementation' \
		'  native-frontend Build the current native frontend pipeline'

bootstrap:
	$(MAKE) -C $(BOOTSTRAP_DIR)

native-frontend:
	$(MAKE) -C $(ROOT) native-frontend
