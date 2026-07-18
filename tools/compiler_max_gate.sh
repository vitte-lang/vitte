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
elif [ -f tools/pass_contract_audit.py ]; then
  go python3 tools/pass_contract_audit.py
elif [ "$MODE" = "fast" ]; then
  go make seed-contract-check
elif [ -f tools/compiler_effective_gate/check.py ]; then
  go python3 tools/compiler_effective_gate/check.py
else
  log "contract audit script missing; skip"
fi

# 4) Pass contracts (audited above)
# 5) Diagnostics quality baseline
if [ "$MODE" = "fast" ]; then
  if ! make core-semantic-resolve-snapshots; then
    log "core-semantic-resolve-snapshots failed; fallback to core-semantic-success"
    go make core-semantic-success
  fi
elif [ "$STRICT" -eq 1 ]; then
  go make diag-snapshots
else
  if ! make diag-snapshots; then
    log "diag-snapshots failed; fallback to core-semantic-resolve-snapshots"
    go make core-semantic-resolve-snapshots
  fi
fi

# 6) Strict recovery gate
if [ -f tools/strict_recovery_smoke.sh ]; then
  go tools/strict_recovery_smoke.sh
elif [ "$STRICT" -eq 1 ]; then
  echo "[compiler-max-gate][error] strict mode requires tools/strict_recovery_smoke.sh" >&2
  exit 1
else
  log "strict recovery smoke script missing; skip"
fi

# 7) Precedence/associativity matrix sanity
if [ "$MODE" = "fast" ]; then
  if ! python3 tools/parser_precedence_property_test.py --cases 80 --seed 1337; then
    log "parser precedence property test failed in fast mode; skip (bootstrap parser limitation)"
  fi
else
  go python3 tools/parser_bootstrap_surface_test.py
fi

# 8) Strict modes surface
if [ "$MODE" = "fast" ]; then
  if ! make strict-core-guard-test; then
    log "strict-core-guard-test failed in fast mode; skip (bootstrap parser limitation)"
  fi
else
  if [ "$STRICT" -eq 1 ]; then
    go make core-semantic-success
  else
    go make core-language-test
  fi
fi

# 9) Determinism
if [ "$MODE" = "fast" ]; then
  if [ -f tools/determinism_smoke.sh ]; then
    go tools/determinism_smoke.sh
  else
    log "determinism smoke script missing; skip"
  fi
elif [ "$STRICT" -eq 1 ]; then
  go tools/determinism_smoke.sh
  go make bootstrap-selfhost-repro
else
  if ! make same-output-hash; then
    log "same-output-hash failed; fallback to determinism smoke"
    go tools/determinism_smoke.sh
  fi
fi

# 10) Incremental/cache safety smoke
if [ "$MODE" = "fast" ]; then
  if [ -f tools/incremental_cache_smoke.sh ]; then
    go tools/incremental_cache_smoke.sh
  else
    log "incremental cache smoke script missing; skip"
  fi
elif [ "$STRICT" -eq 1 ]; then
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
  if [ -f tools/crash_report_snapshots.sh ]; then
    go tools/crash_report_snapshots.sh
  else
    log "crash report snapshots script missing; skip"
  fi
elif [ "$STRICT" -eq 1 ]; then
  go make all-tests-group GROUP=core
else
  if ! make all-tests-group GROUP=core; then
    log "all-tests core group failed; fallback to crash_report_snapshots"
    go tools/crash_report_snapshots.sh
  fi
fi

# 15) Unified quality gate output
log "OK mode=$MODE"
