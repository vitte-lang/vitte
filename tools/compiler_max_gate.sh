#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
MODE="${1:-fast}"
STRICT=0
if [ "$MODE" = "strict" ]; then
  STRICT=1
fi

log() { printf "[compiler-max-gate] %s\n" "$*"; }

go() {
  log "run: $*"
  "$@"
}

cd "$ROOT_DIR"

# 0) Hard contract checks first
go make bootstrap-source-of-truth
go make compiler-entry-lock
go make compiler-path-typos
go make compiler-src-critical
if [ "$STRICT" -eq 1 ]; then
  go make diagnostics-migration-gate
  go make compiler-reachability-audit
  if [ "${REAL_PIPELINE_GATE:-0}" = "1" ]; then
    go make compiler-real-pipeline-audit
  fi
fi

# 0b) Generated compiler chain and active native JSON contracts.
if [ "$STRICT" -eq 1 ]; then
  go python3 tools/selfhost_completion_audit.py
  go tools/native_json_schema_contract_test.sh
fi

# 1) Golden frontend snapshots
if [ "$MODE" = "fast" ]; then
  go make test-golden-critical
else
  go make test-golden
fi

# 2) Parser/lexer fuzz smoke
if [ "$MODE" = "fast" ]; then
  go python3 tools/parser_lexer_fuzz_smoke.py --cases 40 --seed 1337
else
  go make parser-lexer-fuzz-smoke
fi

# 3) Invariants and validation chain
if [ "$STRICT" -eq 1 ]; then
  go make seed-contract-check
  go make selfhost-audit
else
  go python3 tools/compiler_effective_gate/check.py
fi

# 4) Pass contracts (audited above)
# 5) Diagnostics quality baseline
if [ "$MODE" = "fast" ]; then
  go make core-semantic-resolve-snapshots
else
  go make diag-snapshots
fi

# 6) Strict recovery gate
go tools/strict_recovery_smoke.sh

# 7) Precedence/associativity matrix sanity
if [ "$MODE" = "fast" ]; then
  go python3 tools/parser_precedence_property_test.py --cases 80 --seed 1337
else
  go python3 tools/parser_bootstrap_surface_test.py
fi

# 8) Strict modes surface
if [ "$MODE" = "fast" ]; then
  go make strict-core-guard-test
else
  if [ "$STRICT" -eq 1 ]; then
    go make core-semantic-success
  else
    go make core-language-test
  fi
fi

# 9) Determinism
if [ "$MODE" = "fast" ]; then
  go tools/determinism_smoke.sh
elif [ "$STRICT" -eq 1 ]; then
  go tools/determinism_smoke.sh
  go make bootstrap-selfhost-repro
else
  go make same-output-hash
fi

# 10) Incremental/cache safety smoke
if [ "$MODE" = "fast" ]; then
  go tools/incremental_cache_smoke.sh
elif [ "$STRICT" -eq 1 ]; then
  go tools/incremental_cache_smoke.sh
else
  go make ci-fast-compiler
fi

# 11) Perf telemetry baseline (reuse profile path)
if [ "$MODE" != "fast" ]; then
  go make profile-sample
fi

# 12) ABI/interop baseline
if [ "$MODE" = "fast" ]; then
  go make extern-abi-host
else
  go make extern-abi-all
fi

# 13) Differential stability (repro hash + snapshots)
# covered by same-output-hash + test-golden

# 14) Crash regression snapshots
if [ "$MODE" = "fast" ]; then
  go tools/crash_report_snapshots.sh
else
  go make all-tests-group GROUP=core
fi

# 15) Unified quality gate output
log "OK mode=$MODE"
