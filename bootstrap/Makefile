CC ?= cc
CFLAGS ?= -std=c17 -Wall -Wextra -Werror -pedantic -O2
ROOT_DIR := ..
OUT_DIR ?= $(ROOT_DIR)/target/bootstrap-c17
BIN ?= $(OUT_DIR)/vitte-bootstrap
SOURCES := $(shell find src -type f -name '*.c' | sort)
HEADERS := $(shell find . -type f -name '*.h' | sort)
CHECKED_FILES := $(shell find . -type f \( -name '*.c' -o -name '*.h' -o -name 'Makefile' -o -name 'CMakeLists.txt' -o -name 'README.md' \) | sort)

.PHONY: all alignment clean corpus smoke install verify

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

alignment:
	@python3 "$(ROOT_DIR)/tools/bootstrap_src_alignment_gate.py"

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
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/multi_decl_main.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/module_alias_rich_main.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/sema_calls/local_param_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/sema_calls/import_param_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/frontend_expr_stmt/expr_stmt_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/sema_operators/logical_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/grammar_alignment/let_mut_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/assignment_ok.vit"
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
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_collision/glob_conflict_main.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_IMPORT_CONFLICT" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/module_path_unsupported.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_DRIVER_E_IMPORT_USE_CONTRACT" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	grep -q "use 'use multi_decl_lib::{first}' to import symbol 'first'" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
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
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/sema_operators/logical_type_mismatch.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_OPERATOR" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	grep -q "invalid operand types for operator" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
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
	@"$(BIN)" build "$(ROOT_DIR)/bootstrap/tests/frontend_expr_stmt/expr_stmt_ok.vit" -o "$(OUT_DIR)/expr_stmt_ok"
	@tmp="$$(mktemp)"; \
	"$(OUT_DIR)/expr_stmt_ok" >"$$tmp"; \
	grep -q "^expr-stmt-ok$$" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@"$(BIN)" build "$(ROOT_DIR)/bootstrap/tests/sema_operators/logical_ok.vit" -o "$(OUT_DIR)/logical_ok"
	@"$(OUT_DIR)/logical_ok"
	@"$(BIN)" build "$(ROOT_DIR)/bootstrap/tests/grammar_alignment/let_mut_ok.vit" -o "$(OUT_DIR)/let_mut_ok"
	@"$(OUT_DIR)/let_mut_ok"
	@"$(BIN)" build "$(ROOT_DIR)/bootstrap/tests/assignment_ok.vit" -o "$(OUT_DIR)/assignment_ok"
	@"$(OUT_DIR)/assignment_ok"; status=$$?; test "$$status" -eq 2

corpus: verify $(BIN)
	@mkdir -p "$(OUT_DIR)/corpus"
	@"$(BIN)" check "$(ROOT_DIR)/examples/hello.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/frontend_expr_stmt/expr_stmt_ok.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/multi_decl_main.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/module_alias_rich_main.vit"
	@"$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/coverage_chain_main.vit"
	@"$(BIN)" emit-c "$(ROOT_DIR)/examples/hello.vit" -o "$(OUT_DIR)/corpus/hello.c"
	@"$(BIN)" emit-c "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/multi_decl_main.vit" -o "$(OUT_DIR)/corpus/multi_decl_main.c"
	@"$(BIN)" emit-c "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/module_alias_rich_main.vit" -o "$(OUT_DIR)/corpus/module_alias_rich_main.c"
	@"$(BIN)" emit-c "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/coverage_chain_main.vit" -o "$(OUT_DIR)/corpus/coverage_chain_main.c"
	@"$(BIN)" emit-c "$(ROOT_DIR)/bootstrap/tests/pick_parse.vit" -o "$(OUT_DIR)/corpus/pick_parse.c"
	@"$(BIN)" emit-c "$(ROOT_DIR)/bootstrap/tests/form_parse.vit" -o "$(OUT_DIR)/corpus/form_parse.c"
	@"$(BIN)" build "$(ROOT_DIR)/examples/hello.vit" -o "$(OUT_DIR)/corpus/hello"
	@"$(BIN)" build "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/multi_decl_main.vit" -o "$(OUT_DIR)/corpus/multi_decl_main"
	@"$(BIN)" build "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/module_alias_rich_main.vit" -o "$(OUT_DIR)/corpus/module_alias_rich_main"
	@"$(BIN)" build "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/coverage_chain_main.vit" -o "$(OUT_DIR)/corpus/coverage_chain_main"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_collision/glob_conflict_main.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_IMPORT_CONFLICT" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/import_export_coverage/module_path_unsupported.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_DRIVER_E_IMPORT_USE_CONTRACT" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"
	@tmp="$$(mktemp)"; \
	if "$(BIN)" check "$(ROOT_DIR)/bootstrap/tests/sema_calls/import_param_mismatch.vit" >"$$tmp" 2>&1; then \
		cat "$$tmp"; rm -f "$$tmp"; exit 1; \
	fi; \
	grep -q "VITTE_SEMA_E_CALL" "$$tmp" || { cat "$$tmp"; rm -f "$$tmp"; exit 1; }; \
	rm -f "$$tmp"

install: $(BIN)
	@mkdir -p "$(ROOT_DIR)/bin"
	@cp "$(BIN)" "$(ROOT_DIR)/bin/vitte-bootstrap"
	@echo "Installed bootstrap binary: $(ROOT_DIR)/bin/vitte-bootstrap"

clean:
	@rm -rf "$(OUT_DIR)"
