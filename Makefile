.PHONY: all build clean test install bootstrap help

# Variables
CC ?= gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
DEBUGFLAGS = -g -O0 -DDEBUG
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)/lib

TARGET = $(BIN_DIR)/steel
SOURCES = $(wildcard $(SRC_DIR)/**/*.c $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Cibles principales
all: build

build: $(TARGET)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

debug: CFLAGS += $(DEBUGFLAGS)
debug: clean build

test: build
	@echo "Exécution des tests..."
	@cd tests && ./run_tests.sh

clean:
	rm -rf $(BUILD_DIR)/obj $(BIN_DIR) $(LIB_DIR)

install: build
	install -m 755 $(TARGET) /usr/local/bin/

bootstrap:
	@bash build/scripts/steel_bootstrap.sh

help:
	@echo "Cibles disponibles:"
	@echo "  make build      - Compiler le projet"
	@echo "  make debug      - Compiler en mode debug"
	@echo "  make test       - Exécuter les tests"
	@echo "  make clean      - Nettoyer les fichiers compilés"
	@echo "  make install    - Installer le binaire"
	@echo "  make bootstrap  - Initialiser l'architecture"
