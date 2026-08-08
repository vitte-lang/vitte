CC ?= cc
CFLAGS ?= -std=c17 -Wall -Wextra -Werror -pedantic -O2
ROOT_DIR := ..
OUT_DIR ?= $(ROOT_DIR)/target/bootstrap-c17
BIN ?= $(OUT_DIR)/vitte-bootstrap

.PHONY: all clean smoke install

all: $(BIN)

$(BIN): src/main.c
	@mkdir -p "$(OUT_DIR)"
	$(CC) $(CFLAGS) src/main.c -o "$(BIN)"

smoke: $(BIN)
	@"$(BIN)" --version
	@"$(BIN)" check "$(ROOT_DIR)/examples/hello.vit"
	@"$(BIN)" build "$(ROOT_DIR)/examples/hello.vit" -o "$(OUT_DIR)/hello"
	@"$(OUT_DIR)/hello"

install: $(BIN)
	@mkdir -p "$(ROOT_DIR)/bin"
	@cp "$(BIN)" "$(ROOT_DIR)/bin/vitte-bootstrap"
	@echo "Installed bootstrap binary: $(ROOT_DIR)/bin/vitte-bootstrap"

clean:
	@rm -rf "$(OUT_DIR)"
