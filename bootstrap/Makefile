CC ?= cc
CFLAGS ?= -std=c17 -Wall -Wextra -Werror -pedantic -O2
ROOT_DIR := ..
OUT_DIR ?= $(ROOT_DIR)/target/bootstrap-c17
BIN ?= $(OUT_DIR)/vitte-bootstrap
SOURCES := $(shell find src -type f -name '*.c' | sort)
HEADERS := $(shell find . -type f -name '*.h' | sort)
CHECKED_FILES := $(shell find . -type f \( -name '*.c' -o -name '*.h' -o -name 'Makefile' -o -name 'CMakeLists.txt' -o -name 'README.md' \) | sort)

.PHONY: all clean smoke install verify

all: verify $(BIN)

$(BIN): $(SOURCES)
	@mkdir -p "$(OUT_DIR)"
	$(CC) $(CFLAGS) $(SOURCES) -o "$(BIN)"

verify:
	@test -n "$(SOURCES)"
	@test -n "$(HEADERS)"
	@test -n "$(CHECKED_FILES)"
	@empty="$$(find . -type f -size 0 -print)"; \
	if [ -n "$$empty" ]; then \
		printf '%s\n' "$$empty"; \
		exit 1; \
	fi
	@for src in $(SOURCES); do \
		$(CC) $(CFLAGS) -fsyntax-only "$$src"; \
	done
	@for header in $(HEADERS); do \
		printf '#include "%s"\nint main(void) { return 0; }\n' "$$header" | \
			$(CC) $(CFLAGS) -I. -x c -fsyntax-only -; \
	done

smoke: verify $(BIN)
	@"$(BIN)" --version
	@"$(BIN)" check "$(ROOT_DIR)/examples/hello.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_visibility/direct_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_visibility/module_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/export_forms/direct_star_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/export_forms/module_star_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/export_forms/direct_alias_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/export_forms/module_alias_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/coverage_explicit_main.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/coverage_star_main.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/coverage_chain_main.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/sema_calls/local_param_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/sema_calls/import_param_ok.vit"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_visibility/ambiguous_symbol_form_main.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_DRIVER_E_IMPORT_USE_CONTRACT" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	grep -q "use 'use contract_lib::{value}' to import symbol 'value'" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_visibility/direct_private.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_IMPORT_PRIVATE" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_visibility/glob_private.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_IMPORT_PRIVATE" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_visibility/module_private.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_IMPORT_PRIVATE_MEMBER" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_collision/local_main.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_collision/modules_main.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_collision/symbol_alias_main.vit"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/sema_calls/local_param_mismatch.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_CALL" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	grep -q "procedure argument type mismatch" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/sema_calls/import_param_mismatch.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_CALL" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	grep -q "procedure argument type mismatch" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/export_forms/missing_export.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_EXPORT" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/export_forms/reexport_group_unsupported.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_PARSER_E_REEXPORT" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/export_forms/reexport_module_alias_unsupported.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_REEXPORT" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/export_forms/reexport_symbol_unsupported.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_REEXPORT" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@"$(BIN)" build "$(ROOT_DIR)/examples/hello.vit" -o "$(OUT_DIR)/hello"
	@"$(OUT_DIR)/hello"

install: $(BIN)
	@mkdir -p "$(ROOT_DIR)/bin"
	@cp "$(BIN)" "$(ROOT_DIR)/bin/vitte-bootstrap"
	@echo "Installed bootstrap binary: $(ROOT_DIR)/bin/vitte-bootstrap"

clean:
	@rm -rf "$(OUT_DIR)"
