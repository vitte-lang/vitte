CC ?= gcc
CFLAGS = -Wall -Wextra -Wpedantic -O2 -Iinclude
DEBUGFLAGS = -g -O0 -DDEBUG
LDFLAGS = -lm

BUILD_DIR = build
SRC_DIR = src
TEST_DIR = tests
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)/lib

TARGET = $(BIN_DIR)/steel
VERSION = 0.1.0
