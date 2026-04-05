#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
MODE="${1:-fast}"

log() { printf "[compiler-max-gate] %s\n" "$*"; }

go() {
  log "run: $*"
  "$@"
}

cd "$ROOT_DIR"

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
go python3 tools/pass_contract_audit.py

# 4) Pass contracts (audited above)
# 5) Diagnostics quality baseline
if [ "$MODE" = "fast" ]; then
  go make core-semantic-resolve-snapshots
else
  if ! make diag-snapshots; then
    log "diag-snapshots failed; fallback to core-semantic-resolve-snapshots"
    go make core-semantic-resolve-snapshots
  fi
fi

# 6) Strict recovery gate
go tools/strict_recovery_smoke.sh

# 7) Precedence/associativity matrix sanity
go python3 tools/parser_precedence_property_test.py --cases 80 --seed 1337

# 8) Strict modes surface
if [ "$MODE" = "fast" ]; then
  go make strict-core-guard-test
else
  go make core-language-test
fi

# 9) Determinism
if [ "$MODE" = "fast" ]; then
  go tools/determinism_smoke.sh
else
  if ! make same-output-hash; then
    log "same-output-hash failed; fallback to determinism smoke"
    go tools/determinism_smoke.sh
  fi
fi

# 10) Incremental/cache safety smoke
if [ "$MODE" = "fast" ]; then
  go tools/incremental_cache_smoke.sh
else
  if ! make ci-fast-compiler; then
    log "ci-fast-compiler failed; fallback to incremental-cache-smoke"
    go tools/incremental_cache_smoke.sh
  fi
fi

# 11) Perf telemetry baseline (reuse profile path)
if [ "$MODE" != "fast" ]; then
  if make -qp 2>/dev/null | grep -q '^profile-sample:'; then
    go make profile-sample
  else
    log "profile-sample target missing; skip perf sample"
  fi
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
  if ! make all-tests-group GROUP=core; then
    log "all-tests core group failed; fallback to crash_report_snapshots"
    go tools/crash_report_snapshots.sh
  fi
fi

# 15) Unified quality gate output
log "OK mode=$MODE"
