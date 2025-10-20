SHELL := /bin/bash
.DEFAULT_GOAL := help

BUILD_OPTS ?=
CHECK_OPTS ?=
LINT_OPTS ?=
TEST_OPTS ?=
DOC_OPTS ?=
PIPELINE_OPTS ?=
BENCH_OPTS ?=
WASM_OPTS ?=
ARCH_OPTS ?=
COVERAGE_OPTS ?=

.PHONY: help bootstrap build build-release build-analysis check lint fmt fmt-check test doc pipeline bench wasm arch coverage clean ci

help:
	@echo "Available targets (set VARIABLE_OPTS=\"--flag\" to pass extra options):"
	@printf "  %-18s %s\n" "bootstrap" "Validate toolchain; use INSTALL=1 for auto-install."
	@printf "  %-18s %s\n" "build" "Strict build via scripts/build.sh."
	@printf "  %-18s %s\n" "build-release" "Release build with strict checks."
	@printf "  %-18s %s\n" "build-analysis" "Run analysis-only preflight."
	@printf "  %-18s %s\n" "check" "Full fmt+lint+tests+deny+docs pipeline."
	@printf "  %-18s %s\n" "lint" "Formatting + clippy (warnings denied)."
	@printf "  %-18s %s\n" "fmt" "cargo fmt --all."
	@printf "  %-18s %s\n" "fmt-check" "cargo fmt --all -- --check."
	@printf "  %-18s %s\n" "test" "Workspace tests (supports TEST_OPTS)."
	@printf "  %-18s %s\n" "doc" "Generate docs with warnings as errors."
	@printf "  %-18s %s\n" "pipeline" "scripts/pipeline.sh orchestrated flow."
	@printf "  %-18s %s\n" "bench" "Run benchmarks via scripts/bench.sh."
	@printf "  %-18s %s\n" "wasm" "Build the wasm target helper."
	@printf "  %-18s %s\n" "arch" "Architecture lint (layered dependencies)."
	@printf "  %-18s %s\n" "coverage" "Generate coverage report via scripts/pro/coverage.sh."
	@printf "  %-18s %s\n" "clean" "cargo clean."
	@printf "  %-18s %s\n" "ci" "Alias for pipeline target."

bootstrap:
	@set -euo pipefail; \
	ARGS=(); \
	if [[ "$${INSTALL:-0}" == "1" ]]; then \
		ARGS+=("--install"); \
	fi; \
	if [[ "$${SKIP_NODE:-0}" == "1" ]]; then \
		ARGS+=("--skip-node"); \
	fi; \
	if [[ "$${SKIP_WASM:-0}" == "1" ]]; then \
		ARGS+=("--skip-wasm"); \
	fi; \
	./scripts/bootstrap.sh "$${ARGS[@]}"

build:
	@set -euo pipefail; ./scripts/build.sh $(BUILD_OPTS)

build-release:
	@set -euo pipefail; ./scripts/build.sh --release $(BUILD_OPTS)

build-analysis:
	@set -euo pipefail; ./scripts/build.sh --analysis-only $(BUILD_OPTS)

check:
	@set -euo pipefail; ./scripts/check.sh $(CHECK_OPTS)

lint:
	@set -euo pipefail; ./scripts/lint.sh $(LINT_OPTS)

fmt:
	@set -euo pipefail; cargo fmt --all

fmt-check:
	@set -euo pipefail; cargo fmt --all -- --check

test:
	@set -euo pipefail; ./scripts/test.sh --workspace $(TEST_OPTS)

doc:
	@set -euo pipefail; cargo doc --workspace --all-features --no-deps $(DOC_OPTS)

pipeline:
	@set -euo pipefail; ./scripts/pipeline.sh $(PIPELINE_OPTS)

bench:
	@set -euo pipefail; ./scripts/bench.sh $(BENCH_OPTS)

wasm:
	@set -euo pipefail; ./scripts/wasm-build.sh $(WASM_OPTS)

arch:
	@set -euo pipefail; ./scripts/pro/arch-lint.py $(ARCH_OPTS)

coverage:
	@set -euo pipefail; ./scripts/pro/coverage.sh $(COVERAGE_OPTS)

clean:
	@set -euo pipefail; cargo clean

ci: pipeline
