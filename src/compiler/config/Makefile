PROJECT_ROOT := $(abspath ../../../)

.PHONY: all native-frontend help

all: native-frontend

native-frontend:
	$(MAKE) -C "$(PROJECT_ROOT)" native-frontend

help:
	@printf '%s\n' \
		'Native compiler configuration is built as part of the compiler module graph.' \
		'Run make -C $(PROJECT_ROOT) native-frontend to compile the native frontend.'
