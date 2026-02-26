#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SNAP_DIR="${SNAP_DIR:-$ROOT_DIR/tests/modules/snapshots}"
REPORT_JSON="${REPORT_JSON:-$ROOT_DIR/target/reports/modules_snapshots.json}"
SNAP_FILTER="${SNAP_FILTER:-}"
SNAP_TIMEOUT_SEC="${SNAP_TIMEOUT_SEC:-30}"
SNAP_RETRIES="${SNAP_RETRIES:-0}"
SNAP_CACHE="${SNAP_CACHE:-0}"
SNAP_CACHE_DIR="${SNAP_CACHE_DIR:-$ROOT_DIR/.vitte-cache/modules-snapshots}"

UPDATE=0
BLESS=0
EXPLAIN_ON_FAIL=0

log() { printf "[modules-snapshots] %s\n" "$*"; }
die() { printf "[modules-snapshots][error] %s\n" "$*" >&2; exit 1; }

usage() {
  cat <<'USAGE'
usage: tools/modules_snapshots.sh [--update] [--bless] [--explain-on-fail] [--filter <glob|re:...>] [--timeout-sec N] [--retries N]

Options:
  --update           Rewrite .must/.diagjson.must/.codes.must/.fr.must + manifest from current outputs.
  --bless            Same as --update, but prints unified diff before writing.
  --explain-on-fail  Print reproducible command(s) on failure.
  --filter PATTERN   Run only matching snapshots. Use re:<regex> for regex, otherwise glob on rel path.
  --timeout-sec N    Per-command timeout in seconds (default: 30).
  --retries N        Retry count for flaky snapshots (default: 0).
USAGE
}

while [ $# -gt 0 ]; do
  case "$1" in
    --update) UPDATE=1 ;;
    --bless) BLESS=1; UPDATE=1 ;;
    --explain-on-fail) EXPLAIN_ON_FAIL=1 ;;
    --filter) SNAP_FILTER="${2:-}"; shift ;;
    --timeout-sec) SNAP_TIMEOUT_SEC="${2:-}"; shift ;;
    --retries) SNAP_RETRIES="${2:-}"; shift ;;
    -h|--help) usage; exit 0 ;;
    *) die "unknown option: $1" ;;
  esac
  shift
done

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -d "$SNAP_DIR" ] || die "missing snapshot dir: $SNAP_DIR"
mkdir -p "$(dirname "$REPORT_JSON")"
[ "$SNAP_CACHE" = "1" ] && mkdir -p "$SNAP_CACHE_DIR"

manifest_file="$SNAP_DIR/SNAPSHOTS.list"

match_filter() {
  local rel="$1"
  if [ -z "$SNAP_FILTER" ]; then
    return 0
  fi
  if [[ "$SNAP_FILTER" == re:* ]]; then
    local rx="${SNAP_FILTER#re:}"
    [[ "$rel" =~ $rx ]]
    return $?
  fi
  case "$rel" in
    $SNAP_FILTER) return 0 ;;
  esac
  return 1
}

owning_owners_file() {
  local dir="$1"
  while true; do
    if [ -f "$dir/OWNERS" ]; then
      printf "%s\n" "$dir/OWNERS"
      return 0
    fi
    [ "$dir" = "$SNAP_DIR" ] && break
    dir="$(dirname "$dir")"
  done
  return 1
}

lint_snapshot_ownership() {
  local failed=0
  while IFS= read -r cmd_file; do
    local dir
    dir="$(dirname "$cmd_file")"
    if ! owning_owners_file "$dir" >/dev/null; then
      printf "[modules-snapshots][error] %s: missing OWNERS in directory or ancestors\n" "${cmd_file#"$ROOT_DIR"/}" >&2
      failed=1
    fi
  done < <(find "$SNAP_DIR" -type f -name '*.cmd' | sort)
  [ "$failed" -eq 0 ] || die "snapshot ownership lint failed"
}

lint_orphans_and_reasons() {
  local failed=0
  while IFS= read -r path; do
    local base="${path%.diagjson.must}"
    [ -f "$base.cmd" ] || { printf "[modules-snapshots][error] orphan diagjson snapshot: %s\n" "${path#"$ROOT_DIR"/}" >&2; failed=1; }
  done < <(find "$SNAP_DIR" -type f -name '*.diagjson.must' | sort)

  while IFS= read -r path; do
    local base="${path%.cmd}"
    if [ ! -f "$base.must" ] && [ ! -f "$base.no_must.reason" ]; then
      printf "[modules-snapshots][error] %s: missing .must (or .no_must.reason)\n" "${path#"$ROOT_DIR"/}" >&2
      failed=1
    fi
  done < <(find "$SNAP_DIR" -type f -name '*.cmd' | sort)

  [ "$failed" -eq 0 ] || die "snapshot orphan/reason lint failed"
}

lint_snapshot_format() {
  python3 - "$SNAP_DIR" <<'PY'
from pathlib import Path
import sys

root = Path(sys.argv[1])
failed = False
for p in sorted(root.rglob('*.must')):
    raw = p.read_bytes()
    try:
        text = raw.decode('utf-8')
    except UnicodeDecodeError:
        print(f"[modules-snapshots][error] {p}: not valid UTF-8", file=sys.stderr)
        failed = True
        continue
    if text and not text.endswith('\n'):
        print(f"[modules-snapshots][error] {p}: missing trailing newline", file=sys.stderr)
        failed = True
    for i, line in enumerate(text.splitlines(), start=1):
        if line.rstrip(' \t') != line:
            print(f"[modules-snapshots][error] {p}:{i}: trailing whitespace", file=sys.stderr)
            failed = True
        low = line.lower()
        if any(tok in low for tok in ('some error', 'failed somehow', 'thing', 'stuff')):
            print(f"[modules-snapshots][error] {p}:{i}: ambiguous wording token", file=sys.stderr)
            failed = True
if failed:
    raise SystemExit(1)
PY
}

write_snapshot_file() {
  local target="$1"
  local content="$2"
  local tmp
  tmp="$(mktemp "${TMPDIR:-/tmp}/vitte-mod-snap-write-XXXXXX")"
  if [ -n "$content" ]; then
    printf "%s\n" "$content" > "$tmp"
  else
    : > "$tmp"
    printf "\n" >> "$tmp"
  fi
  if [ "$BLESS" -eq 1 ] && [ -f "$target" ]; then
    if ! diff -u "$target" "$tmp" >/dev/null 2>&1; then
      diff -u "$target" "$tmp" || true
    fi
  fi
  mv "$tmp" "$target"
}

stable_needles() {
  local raw="$1"
  printf "%s\n" "$raw" | while IFS= read -r line; do
    [ -z "$line" ] && continue
    case "$line" in
      "[modules-cache-perf] cold_ms="*) continue ;;
      "[modules-cache-perf] skip ratio check ("*) continue ;;
      "[modules-cache-perf] hot_cold_ratio="*) continue ;;
      *"top10 slowest:"*) continue ;;
      "  "*": "*' ms') continue ;;
    esac
    printf "%s\n" "$line"
  done
}

extract_codes() {
  local out="$1"
  printf "%s\n" "$out" | grep -oE '(error|warning)\[E[0-9]{4}\]' | sed -E 's/.*\[(E[0-9]{4})\]/\1/' | sort -u || true
}

run_variant() {
  local cmd="$1"
  local lang="$2"
  local diag="$3"
  local rel="$4"

  local cache_key=""
  local cache_dir=""
  if [ "$SNAP_CACHE" = "1" ]; then
    cache_key="$(printf '%s\n' "$cmd|$lang|$diag|$BIN|$(stat -c %Y "$BIN" 2>/dev/null || echo 0)" | sha256sum | awk '{print $1}')"
    cache_dir="$SNAP_CACHE_DIR/$cache_key"
    if [ -f "$cache_dir/rc" ] && [ -f "$cache_dir/out" ] && [ -f "$cache_dir/ms" ]; then
      printf "%s\n%s\n%s\n" "$(cat "$cache_dir/rc")" "$(cat "$cache_dir/ms")" "$(cat "$cache_dir/out")"
      return 0
    fi
  fi

  local attempt=0
  local rc=1
  local out=""
  local ms=0

  while [ "$attempt" -le "$SNAP_RETRIES" ]; do
    local start
    start="$(python3 - <<'PY'
import time
print(int(time.time() * 1000))
PY
)"

    if [[ "$cmd" == shell:* ]]; then
      local shell_cmd="${cmd#shell:}"
      set +e
      if command -v timeout >/dev/null 2>&1; then
        out="$(timeout "$SNAP_TIMEOUT_SEC" bash -lc "$shell_cmd" 2>&1)"
      else
        out="$(bash -lc "$shell_cmd" 2>&1)"
      fi
      rc=$?
      set -e
    else
      local args=("$BIN" "--lang=$lang")
      [ "$diag" = "1" ] && args+=("--diag-json")
      set +e
      if command -v timeout >/dev/null 2>&1; then
        out="$(timeout "$SNAP_TIMEOUT_SEC" ${args[@]} $cmd 2>&1)"
      else
        out="$(${args[@]} $cmd 2>&1)"
      fi
      rc=$?
      set -e
    fi

    local end
    end="$(python3 - <<'PY'
import time
print(int(time.time() * 1000))
PY
)"
    ms=$((end - start))

    if [ "$rc" -eq 0 ] || [ "$attempt" -eq "$SNAP_RETRIES" ]; then
      break
    fi
    attempt=$((attempt + 1))
    log "retry ${attempt}/${SNAP_RETRIES} for ${rel} (lang=$lang diag=$diag)"
  done

  if [ "$SNAP_CACHE" = "1" ]; then
    mkdir -p "$cache_dir"
    printf "%s\n" "$rc" > "$cache_dir/rc"
    printf "%s\n" "$ms" > "$cache_dir/ms"
    printf "%s\n" "$out" > "$cache_dir/out"
  fi

  printf "%s\n%s\n%s\n" "$rc" "$ms" "$out"
}

assert_needles() {
  local out="$1"
  local must_file="$2"
  while IFS= read -r needle; do
    [[ -z "$needle" ]] && continue
    if ! grep -Fq "$needle" <<<"$out"; then
      printf "%s\n" "$out"
      die "missing snapshot needle '$needle' for $must_file"
    fi
  done < "$must_file"
}

ensure_manifest() {
  local tmp
  tmp="$(mktemp "${TMPDIR:-/tmp}/vitte-mod-snap-manifest-XXXXXX")"
  find "$SNAP_DIR" -type f -name '*.cmd' | sed "s#^$ROOT_DIR/##" | sort > "$tmp"
  if [ "$UPDATE" -eq 1 ]; then
    write_snapshot_file "$manifest_file" "$(cat "$tmp")$([ -s "$tmp" ] && printf '\n')"
  else
    [ -f "$manifest_file" ] || die "missing manifest: $manifest_file (run --update)"
    if ! diff -u "$manifest_file" "$tmp" >/dev/null 2>&1; then
      diff -u "$manifest_file" "$tmp" || true
      die "snapshot manifest mismatch (deterministic order check failed)"
    fi
  fi
  rm -f "$tmp"
}

lint_snapshot_ownership
lint_orphans_and_reasons
ensure_manifest

pass=0
fail=0
skipped=0
updated=0

metrics_tmp="$(mktemp "${TMPDIR:-/tmp}/vitte-mod-snap-metrics-XXXXXX")"
json_tmp="$(mktemp "${TMPDIR:-/tmp}/vitte-mod-snap-json-XXXXXX")"
trap 'rm -f "$metrics_tmp" "$json_tmp"' EXIT

while IFS= read -r cmd_file; do
  rel="${cmd_file#"$ROOT_DIR"/}"
  if ! match_filter "$rel"; then
    skipped=$((skipped + 1))
    continue
  fi

  base="${cmd_file%.cmd}"
  must="$base.must"
  must_diagjson="$base.diagjson.must"
  must_codes="$base.codes.must"
  must_fr="$base.fr.must"
  no_must_reason="$base.no_must.reason"
  no_diag_reason="$base.no_diagjson.reason"
  no_fr_reason="$base.no_fr.reason"
  exit_file="$base.exit"

  log "$rel"
  cmd="$(cat "$cmd_file")"
  expected_exit=0
  [ -f "$exit_file" ] && expected_exit="$(tr -d '\n' < "$exit_file")"

  set +e
  run_txt="$(run_variant "$cmd" "en" "0" "$rel")"
  rc_run=$?
  set -e
  [ "$rc_run" -eq 0 ] || die "internal runner error for $rel"

  rc_txt="$(printf '%s\n' "$run_txt" | sed -n '1p')"
  ms_txt="$(printf '%s\n' "$run_txt" | sed -n '2p')"
  out_txt="$(printf '%s\n' "$run_txt" | sed -n '3,$p')"

  if [ "$rc_txt" -ne "$expected_exit" ]; then
    printf "%s\n" "$out_txt"
    if [ "$EXPLAIN_ON_FAIL" -eq 1 ]; then
      printf "[modules-snapshots][repro] %s --lang=en %s\n" "$BIN" "$cmd" >&2
    fi
    fail=$((fail + 1))
    die "unexpected exit ($rc_txt != $expected_exit): $cmd_file"
  fi

  if [ -f "$must" ]; then
    if [ "$UPDATE" -eq 1 ]; then
      write_snapshot_file "$must" "$(stable_needles "$out_txt")"
      updated=$((updated + 1))
    else
      assert_needles "$out_txt" "$must"
    fi
  else
    [ -f "$no_must_reason" ] || die "missing must for $rel (or add $no_must_reason)"
  fi

  set +e
  run_diag="$(run_variant "$cmd" "en" "1" "$rel")"
  rc_run=$?
  set -e
  [ "$rc_run" -eq 0 ] || die "internal diag runner error for $rel"
  rc_diag="$(printf '%s\n' "$run_diag" | sed -n '1p')"
  ms_diag="$(printf '%s\n' "$run_diag" | sed -n '2p')"
  out_diag="$(printf '%s\n' "$run_diag" | sed -n '3,$p')"

  if [ "$rc_diag" -ne "$expected_exit" ]; then
    printf "%s\n" "$out_diag"
    if [ "$EXPLAIN_ON_FAIL" -eq 1 ]; then
      printf "[modules-snapshots][repro] %s --lang=en --diag-json %s\n" "$BIN" "$cmd" >&2
    fi
    fail=$((fail + 1))
    die "unexpected diag-json exit ($rc_diag != $expected_exit): $cmd_file"
  fi

  if [ "$UPDATE" -eq 1 ]; then
    write_snapshot_file "$must_diagjson" "$(stable_needles "$out_diag")"
    updated=$((updated + 1))
  else
    [ -f "$must_diagjson" ] || { [ -f "$no_diag_reason" ] || die "missing diagjson snapshot for $rel (or add $no_diag_reason)"; }
    [ ! -f "$must_diagjson" ] || assert_needles "$out_diag" "$must_diagjson"
  fi

  codes="$(extract_codes "$out_txt")"
  if [ "$UPDATE" -eq 1 ]; then
    write_snapshot_file "$must_codes" "$(printf '%s\n' "$codes")"
    updated=$((updated + 1))
  else
    [ -f "$must_codes" ] || die "missing codes snapshot: $must_codes"
    tmp_codes="$(mktemp "${TMPDIR:-/tmp}/vitte-mod-snap-codes-XXXXXX")"
    printf "%s\n" "$codes" > "$tmp_codes"
    if ! diff -u "$must_codes" "$tmp_codes" >/dev/null 2>&1; then
      diff -u "$must_codes" "$tmp_codes" || true
      rm -f "$tmp_codes"
      die "diagnostic code regression for $rel"
    fi
    rm -f "$tmp_codes"
  fi

  is_critical=0
  [ -n "$codes" ] && is_critical=1
  if [ "$is_critical" -eq 1 ]; then
    set +e
    run_fr="$(run_variant "$cmd" "fr" "0" "$rel")"
    rc_run=$?
    set -e
    [ "$rc_run" -eq 0 ] || die "internal fr runner error for $rel"
    rc_fr="$(printf '%s\n' "$run_fr" | sed -n '1p')"
    ms_fr="$(printf '%s\n' "$run_fr" | sed -n '2p')"
    out_fr="$(printf '%s\n' "$run_fr" | sed -n '3,$p')"
    [ "$rc_fr" -eq "$expected_exit" ] || die "unexpected fr exit ($rc_fr != $expected_exit): $cmd_file"

    if [ "$UPDATE" -eq 1 ]; then
      write_snapshot_file "$must_fr" "$(stable_needles "$out_fr")"
      updated=$((updated + 1))
    else
      [ -f "$must_fr" ] || { [ -f "$no_fr_reason" ] || die "missing fr snapshot for critical diagnostics: $rel"; }
      [ ! -f "$must_fr" ] || assert_needles "$out_fr" "$must_fr"
    fi
  fi

  total_ms=$((ms_txt + ms_diag))
  printf "%s\t%s\n" "$total_ms" "$rel" >> "$metrics_tmp"
  python3 - "$rel" "$expected_exit" "$rc_txt" "$rc_diag" "$ms_txt" "$ms_diag" "$is_critical" <<'PY' >> "$json_tmp"
import json
import sys
obj = {
  "snapshot": sys.argv[1],
  "expected_exit": int(sys.argv[2]),
  "rc_text": int(sys.argv[3]),
  "rc_diag_json": int(sys.argv[4]),
  "ms_text": int(sys.argv[5]),
  "ms_diag_json": int(sys.argv[6]),
  "critical_diag": bool(int(sys.argv[7])),
}
print(json.dumps(obj, ensure_ascii=True))
PY

  pass=$((pass + 1))
done < <(find "$SNAP_DIR" -type f -name '*.cmd' | sort)

lint_snapshot_format

python3 - "$REPORT_JSON" "$json_tmp" "$metrics_tmp" "$pass" "$fail" "$skipped" "$updated" <<'PY'
import json
import sys
from pathlib import Path

report_path = Path(sys.argv[1])
json_lines = Path(sys.argv[2]).read_text(encoding='utf-8').splitlines()
metric_lines = Path(sys.argv[3]).read_text(encoding='utf-8').splitlines()

items = [json.loads(line) for line in json_lines if line.strip()]
slow = []
for line in metric_lines:
    if not line.strip():
        continue
    ms, name = line.split('\t', 1)
    slow.append((int(ms), name))
slow.sort(reverse=True)

data = {
    "summary": {
        "pass": int(sys.argv[4]),
        "fail": int(sys.argv[5]),
        "skipped": int(sys.argv[6]),
        "updated": int(sys.argv[7]),
    },
    "slowest": [{"snapshot": name, "ms_total": ms} for ms, name in slow[:10]],
    "items": items,
}
report_path.parent.mkdir(parents=True, exist_ok=True)
report_path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding='utf-8')
print(f"[modules-snapshots] wrote {report_path}")
if slow:
    print("[modules-snapshots] top10 slowest:")
    for ms, name in slow[:10]:
        print(f"  {name}: {ms} ms")
PY

log "summary: pass=$pass fail=$fail skipped=$skipped updated=$updated"
log "OK"
