#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
FIXTURE="${1:-$ROOT_DIR/tests/modules/mod_graph/main.vit}"
COLD_MAX_MS="${COLD_MAX_MS:-10000}"
HOT_MAX_MS="${HOT_MAX_MS:-10000}"
HOT_RATIO_MAX="${HOT_RATIO_MAX:-2.00}"
RATIO_MIN_COLD_MS="${RATIO_MIN_COLD_MS:-20}"

log() { printf "[modules-cache-perf] %s\n" "$*"; }
die() { printf "[modules-cache-perf][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -f "$FIXTURE" ] || die "missing fixture: $FIXTURE"

run_once() {
  python3 - "$BIN" "$FIXTURE" <<'PY'
import subprocess
import sys
import time

bin_path = sys.argv[1]
fixture = sys.argv[2]
start = time.perf_counter()
proc = subprocess.run([bin_path, "check", "--lang=en", "--cache-report", fixture], capture_output=True, text=True)
elapsed_ms = int((time.perf_counter() - start) * 1000)
out = (proc.stdout or "") + (proc.stderr or "")
print(elapsed_ms)
print(proc.returncode)
print(out)
PY
}

cold_raw="$(run_once)"
cold_ms="$(printf '%s\n' "$cold_raw" | sed -n '1p')"
cold_rc="$(printf '%s\n' "$cold_raw" | sed -n '2p')"
cold_out="$(printf '%s\n' "$cold_raw" | sed -n '3,$p')"

[ "$cold_rc" -eq 0 ] || { printf "%s\n" "$cold_out"; die "cold run failed"; }
grep -Fq "[cache]" <<<"$cold_out" || { printf "%s\n" "$cold_out"; die "cold run missing cache-report line"; }

hot_raw="$(run_once)"
hot_ms="$(printf '%s\n' "$hot_raw" | sed -n '1p')"
hot_rc="$(printf '%s\n' "$hot_raw" | sed -n '2p')"
hot_out="$(printf '%s\n' "$hot_raw" | sed -n '3,$p')"

[ "$hot_rc" -eq 0 ] || { printf "%s\n" "$hot_out"; die "hot run failed"; }
grep -Fq "[cache]" <<<"$hot_out" || { printf "%s\n" "$hot_out"; die "hot run missing cache-report line"; }

log "cold_ms=$cold_ms hot_ms=$hot_ms"

[ "$cold_ms" -le "$COLD_MAX_MS" ] || die "cold run too slow: $cold_ms ms > $COLD_MAX_MS ms"
[ "$hot_ms" -le "$HOT_MAX_MS" ] || die "hot run too slow: $hot_ms ms > $HOT_MAX_MS ms"

if [ "$cold_ms" -lt "$RATIO_MIN_COLD_MS" ]; then
  log "skip ratio check (cold_ms=$cold_ms < min=$RATIO_MIN_COLD_MS ms)"
  log "OK"
  exit 0
fi

python3 - "$cold_ms" "$hot_ms" "$HOT_RATIO_MAX" <<'PY'
import sys
cold = float(sys.argv[1])
hot = float(sys.argv[2])
ratio_max = float(sys.argv[3])
if cold <= 0:
    sys.exit(0)
ratio = hot / cold
print(f"[modules-cache-perf] hot_cold_ratio={ratio:.3f}")
if ratio > ratio_max:
    print(f"[modules-cache-perf][error] ratio too high: {ratio:.3f} > {ratio_max:.3f}")
    sys.exit(1)
PY

log "OK"
