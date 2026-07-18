#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
MODE="${MODE:-build}"  # build | check
STRICT_EXAMPLES="${STRICT_EXAMPLES:-0}"
PARALLEL_JOBS="${PARALLEL_JOBS:-1}"
TOP_COUNT="${TOP_COUNT:-10}"
REGRESSION_THRESHOLD="${REGRESSION_THRESHOLD:-20}"
HISTORY_DIR="${HISTORY_DIR:-$ROOT_DIR/build/examples/history}"
mkdir -p "$HISTORY_DIR"

TIMEOUT_SECONDS="${TIMEOUT_SECONDS:-30}"
REPORT_DIR="${REPORT_DIR:-$ROOT_DIR/build/examples}"
JSON_REPORT="$REPORT_DIR/examples-matrix.json"
MARKDOWN_REPORT="$REPORT_DIR/examples-matrix.md"
HTML_REPORT="$REPORT_DIR/examples-matrix.html"
JUNIT_REPORT="$REPORT_DIR/examples-matrix.xml"
LOG_DIR="$REPORT_DIR/logs"
ARTIFACT_DIR="$REPORT_DIR/artifacts"

mkdir -p "$REPORT_DIR"
mkdir -p "$LOG_DIR"
mkdir -p "$ARTIFACT_DIR"

log() { printf "[examples-matrix] %s\n" "$*"; }
die() { printf "[examples-matrix][error] %s\n" "$*" >&2; exit 1; }
now_ms() {
  python3 -c 'import time; print(time.time_ns() // 1_000_000)'
}
expects_failure() {
  case "$1" in
    examples/bad_build_example.vit|examples/invalid_for_build.vit) return 0 ;;
    *) return 1 ;;
  esac
}

[ -x "$BIN" ] || die "missing binary: $BIN"
if [ ! -d "$ROOT_DIR/examples" ]; then
  if [ "$STRICT_EXAMPLES" = "1" ]; then
    die "missing examples dir"
  fi
  log "skip: missing examples dir"
  exit 0
fi

case "$MODE" in
  build|check) ;;
  *) die "invalid MODE=$MODE (expected build or check)" ;;
esac

total=0
ok=0
fail=0
total_elapsed_ms=0

slowest_time=0
slowest_file=""

json_entries=""
timeout_count=0
largest_log_size=0
largest_log_file=""
top_results=""
elapsed_values=""

junit_cases=""
category_stats=""
TOP_SLOWEST=""

AVERAGE_MS=0
MEDIAN_MS=0
P95_MS=0
P99_MS=0
PREVIOUS_AVERAGE_MS=0
AVERAGE_REGRESSION_PERCENT=0
REGRESSION_FAILURE=0

while IFS= read -r file; do
  [ -n "$file" ] || continue
  total=$((total + 1))
  rel="${file#$ROOT_DIR/}"
  category=$(printf "%s" "$rel" | cut -d/ -f2)
  category_stats="${category_stats}${category}\n"
  log "$MODE: $rel"
  log_file="$LOG_DIR/$(basename "$file").log"

  start_ts="$(now_ms)"

  set +e

  artifact="$ARTIFACT_DIR/$(printf '%s' "$rel" | tr '/' '_')"
  if [ "$MODE" = "build" ]; then
    command_args=("$BIN" build "$file" -o "$artifact")
  else
    command_args=("$BIN" check "$file")
  fi

  if command -v timeout >/dev/null 2>&1; then
    timeout "$TIMEOUT_SECONDS" "${command_args[@]}" >"$log_file" 2>&1
    result=$?
  else
    "${command_args[@]}" >"$log_file" 2>&1
    result=$?
  fi

  set -e

  end_ts="$(now_ms)"

  case "$start_ts" in ''|*[!0-9]*) die "invalid millisecond clock value: $start_ts" ;; esac
  case "$end_ts" in ''|*[!0-9]*) die "invalid millisecond clock value: $end_ts" ;; esac

  if [ "$result" -eq 124 ]; then
    timeout_count=$((timeout_count + 1))
  fi

  elapsed_ms=$((end_ts - start_ts))
  elapsed_values="${elapsed_values}${elapsed_ms}\n"
  total_elapsed_ms=$((total_elapsed_ms + elapsed_ms))

  log_size=$(wc -c < "$log_file" | tr -d ' ')

  if [ "$log_size" -gt "$largest_log_size" ]; then
    largest_log_size="$log_size"
    largest_log_file="$rel"
  fi

  if [ "$elapsed_ms" -gt "$slowest_time" ]; then
    slowest_time="$elapsed_ms"
    slowest_file="$rel"
  fi

  expected_failure=false
  if expects_failure "$rel"; then
    expected_failure=true
  fi

  if { [ "$expected_failure" = false ] && [ "$result" -eq 0 ]; } ||
     { [ "$expected_failure" = true ] && [ "$result" -ne 0 ] && [ "$result" -ne 124 ]; }; then
    ok=$((ok + 1))
    success=true
  else
    fail=$((fail + 1))
    success=false

    printf "[examples-matrix][fail] %s\n" "$rel" >&2
    sed -n '1,80p' "$log_file" >&2
  fi

  entry=$(printf '{"file":"%s","success":%s,"expected_failure":%s,"elapsed_ms":%s,"log_size":%s}' \
    "$rel" "$success" "$expected_failure" "$elapsed_ms" "$log_size")

  top_results="${top_results}${elapsed_ms}|${rel}|${success}\n"

  if [ "$success" = true ]; then
    junit_cases="${junit_cases}<testcase name=\"${rel}\" time=\"${elapsed_ms}\"/>"
  else
    junit_cases="${junit_cases}<testcase name=\"${rel}\" time=\"${elapsed_ms}\"><failure/></testcase>"
  fi

  if [ -z "$json_entries" ]; then
    json_entries="$entry"
  else
    json_entries="$json_entries,$entry"
  fi
done <<EOF_FILES
$(find "$ROOT_DIR/examples" -type f -name '*.vit' \
  ! -name '*.reduced.vit' \
  ! -name '*.reduce.tmp.vit' | sort)
EOF_FILES

[ "$total" -gt 0 ] || {
  if [ "$STRICT_EXAMPLES" = "1" ]; then
    die "no .vit files in examples/"
  fi
  log "skip: no .vit files in examples/"
  exit 0
}

if [ "$total" -gt 0 ]; then
  AVERAGE_MS=$((total_elapsed_ms / total))
fi

SORTED_TIMES=$(printf "%b" "$elapsed_values" | grep -v '^$' | sort -n)

if [ "$total" -gt 0 ]; then
  MEDIAN_INDEX=$(( (total + 1) / 2 ))
  MEDIAN_MS=$(printf "%s\n" "$SORTED_TIMES" | sed -n "${MEDIAN_INDEX}p")

  P95_INDEX=$(( (total * 95 + 99) / 100 ))
  P99_INDEX=$(( (total * 99 + 99) / 100 ))

  P95_MS=$(printf "%s\n" "$SORTED_TIMES" | sed -n "${P95_INDEX}p")
  P99_MS=$(printf "%s\n" "$SORTED_TIMES" | sed -n "${P99_INDEX}p")
fi

TOP_SLOWEST=$(printf "%b" "$top_results" | sort -t'|' -k1,1rn | head -n "$TOP_COUNT")

BUILD_TIMESTAMP=$(date -u +%Y%m%dT%H%M%SZ)
HISTORY_JSON="$HISTORY_DIR/$BUILD_TIMESTAMP.json"

cp /dev/null "$HISTORY_JSON"

UNIQUE_CATEGORIES=$(printf "%b" "$category_stats" | sort -u | grep -v '^$' | wc -l | tr -d ' ')

PREVIOUS_HISTORY="$(ls -1 "$HISTORY_DIR"/*.json 2>/dev/null | sort | tail -n 2 | head -n 1 || true)"
REGRESSION_INFO="none"
PREVIOUS_TOTAL=0
PREVIOUS_FAILED=0
PREVIOUS_TIMEOUTS=0
PREVIOUS_TOTAL_ELAPSED_MS=0
PERF_REGRESSION_PERCENT=0
REGRESSION_PERCENT=0

if [ -n "$PREVIOUS_HISTORY" ] && [ -f "$PREVIOUS_HISTORY" ]; then
  REGRESSION_INFO="baseline=$(basename "$PREVIOUS_HISTORY")"

  PREVIOUS_TOTAL=$(grep -m1 '"total"' "$PREVIOUS_HISTORY" | sed 's/[^0-9]//g' || echo 0)
  PREVIOUS_TOTAL_ELAPSED_MS=$(grep -m1 '"total_elapsed_ms"' "$PREVIOUS_HISTORY" | sed 's/[^0-9]//g' || echo 0)
  PREVIOUS_FAILED=$(grep -m1 '"failed"' "$PREVIOUS_HISTORY" | sed 's/[^0-9]//g' || echo 0)
  PREVIOUS_TIMEOUTS=$(grep -m1 '"timeouts"' "$PREVIOUS_HISTORY" | sed 's/[^0-9]//g' || echo 0)
  PREVIOUS_AVERAGE_MS=$(grep -m1 '"average_ms"' "$PREVIOUS_HISTORY" | sed 's/[^0-9]//g' || echo 0)

  if [ "$PREVIOUS_TOTAL" -gt 0 ]; then
    REGRESSION_PERCENT=$(( ((total - PREVIOUS_TOTAL) * 100) / PREVIOUS_TOTAL ))
  fi

  if [ "$PREVIOUS_FAILED" -eq 0 ] && [ "$PREVIOUS_TOTAL_ELAPSED_MS" -gt 0 ]; then
    PERF_REGRESSION_PERCENT=$(( ((total_elapsed_ms - PREVIOUS_TOTAL_ELAPSED_MS) * 100) / PREVIOUS_TOTAL_ELAPSED_MS ))
  fi

  if [ "$PREVIOUS_FAILED" -eq 0 ] && [ "$PREVIOUS_AVERAGE_MS" -gt 0 ]; then
    AVERAGE_REGRESSION_PERCENT=$(( ((AVERAGE_MS - PREVIOUS_AVERAGE_MS) * 100) / PREVIOUS_AVERAGE_MS ))
  fi
fi

cat > "$JSON_REPORT" <<EOF_JSON
{
  "total": $total,
  "passed": $ok,
  "failed": $fail,
  "timeouts": $timeout_count,
  "total_elapsed_ms": $total_elapsed_ms,
  "average_ms": $AVERAGE_MS,
  "median_ms": $MEDIAN_MS,
  "p95_ms": $P95_MS,
  "p99_ms": $P99_MS,
  "slowest_file": "${slowest_file}",
  "slowest_ms": $slowest_time,
  "largest_log_bytes": $largest_log_size,
  "largest_log_file": "${largest_log_file}",
  "unique_categories": $UNIQUE_CATEGORIES,
  "previous_total": $PREVIOUS_TOTAL,
  "previous_failed": $PREVIOUS_FAILED,
  "previous_timeouts": $PREVIOUS_TIMEOUTS,
  "previous_total_elapsed_ms": $PREVIOUS_TOTAL_ELAPSED_MS,
  "previous_average_ms": $PREVIOUS_AVERAGE_MS,
  "regression_percent": $REGRESSION_PERCENT,
  "performance_regression_percent": $PERF_REGRESSION_PERCENT,
  "average_regression_percent": $AVERAGE_REGRESSION_PERCENT,
  "results": [
    $json_entries
  ]
}
EOF_JSON

cp "$JSON_REPORT" "$HISTORY_JSON"

cat > "$MARKDOWN_REPORT" <<EOF_MD
# Vitte Examples Matrix

| Metric | Value |
|----------|----------|
| Total | $total |
| Passed | $ok |
| Failed | $fail |
| Timeouts | $timeout_count |
| Total Elapsed (ms) | $total_elapsed_ms |
| Average Time (ms) | $AVERAGE_MS |
| Median Time (ms) | $MEDIAN_MS |
| P95 Time (ms) | $P95_MS |
| P99 Time (ms) | $P99_MS |
| Slowest Example | $slowest_file |
| Slowest Time (ms) | $slowest_time |
| Largest Log | $largest_log_file |
| Largest Log Size | $largest_log_size |

## Top Slowest Examples

\`\`\`text
$TOP_SLOWEST
\`\`\`

## Build Artifacts

- JSON: $JSON_REPORT
- HTML: $HTML_REPORT
- JUnit: $JUNIT_REPORT
- History Snapshot: $HISTORY_JSON

## Regression Analysis

- Baseline: $REGRESSION_INFO
- Previous Total: $PREVIOUS_TOTAL
- Previous Failed: $PREVIOUS_FAILED
- Previous Timeouts: $PREVIOUS_TIMEOUTS
- Previous Total Elapsed (ms): $PREVIOUS_TOTAL_ELAPSED_MS
- Previous Average (ms): $PREVIOUS_AVERAGE_MS
- Regression (%): $REGRESSION_PERCENT
- Performance Regression (%): $PERF_REGRESSION_PERCENT
- Average Regression (%): $AVERAGE_REGRESSION_PERCENT
EOF_MD

{
  printf '<!doctype html>\n'
  printf '<html><head><meta charset="utf-8">\n'
  printf '<title>Vitte Examples Matrix</title></head><body>\n'
  printf '<h1>Vitte Examples Matrix</h1>\n'
  printf '<ul>\n'
  printf '<li>Total: %s</li>\n' "$total"
  printf '<li>Passed: %s</li>\n' "$ok"
  printf '<li>Failed: %s</li>\n' "$fail"
  printf '<li>Timeouts: %s</li>\n' "$timeout_count"
  printf '<li>Total elapsed: %sms</li>\n' "$total_elapsed_ms"
  printf '<li>Average: %sms</li>\n' "$AVERAGE_MS"
  printf '<li>Median: %sms</li>\n' "$MEDIAN_MS"
  printf '<li>P95: %sms</li>\n' "$P95_MS"
  printf '<li>P99: %sms</li>\n' "$P99_MS"
  printf '<li>Slowest: %s (%sms)</li>\n' "$slowest_file" "$slowest_time"
  printf '<li>Largest log: %s (%s bytes)</li>\n' "$largest_log_file" "$largest_log_size"
  printf '</ul>\n'
  printf '<h2>Top Slowest Examples</h2>\n'
  printf '<pre>%s</pre>\n' "$TOP_SLOWEST"
  printf '<h2>Artifacts</h2>\n'
  printf '<ul>\n'
  printf '<li>JSON: %s</li>\n' "$JSON_REPORT"
  printf '<li>JUnit: %s</li>\n' "$JUNIT_REPORT"
  printf '<li>History: %s</li>\n' "$HISTORY_JSON"
  printf '</ul>\n'
  printf '<h2>Regression Analysis</h2>\n'
  printf '<ul>\n'
  printf '<li>Baseline: %s</li>\n' "$REGRESSION_INFO"
  printf '<li>Previous Total: %s</li>\n' "$PREVIOUS_TOTAL"
  printf '<li>Previous Failed: %s</li>\n' "$PREVIOUS_FAILED"
  printf '<li>Previous Timeouts: %s</li>\n' "$PREVIOUS_TIMEOUTS"
  printf '<li>Previous Total Elapsed (ms): %s</li>\n' "$PREVIOUS_TOTAL_ELAPSED_MS"
  printf '<li>Previous Average (ms): %s</li>\n' "$PREVIOUS_AVERAGE_MS"
  printf '<li>Regression %%: %s</li>\n' "$REGRESSION_PERCENT"
  printf '<li>Performance Regression %%: %s</li>\n' "$PERF_REGRESSION_PERCENT"
  printf '<li>Average Regression %%: %s</li>\n' "$AVERAGE_REGRESSION_PERCENT"
  printf '</ul>\n'
  printf '</body></html>\n'
} > "$HTML_REPORT"

cat > "$JUNIT_REPORT" <<EOF_XML
<?xml version="1.0" encoding="UTF-8"?>
<testsuite tests="$total" failures="$fail" name="vitte-examples">
$junit_cases
</testsuite>
EOF_XML

log "json report: $JSON_REPORT"
log "markdown report: $MARKDOWN_REPORT"
log "largest log: $largest_log_file (${largest_log_size} bytes)"
log "slowest example: $slowest_file (${slowest_time}ms)"
log "timeouts: $timeout_count"
log "html report: $HTML_REPORT"
log "junit report: $JUNIT_REPORT"
log "history snapshot: $HISTORY_JSON"
log "regression baseline: $REGRESSION_INFO"
log "regression percent: $REGRESSION_PERCENT%"
log "performance regression: $PERF_REGRESSION_PERCENT%"

if [ "$PERF_REGRESSION_PERCENT" -gt "$REGRESSION_THRESHOLD" ]; then
  REGRESSION_FAILURE=1
fi

log "average time: ${AVERAGE_MS}ms"
log "median time: ${MEDIAN_MS}ms"
log "p95 time: ${P95_MS}ms"
log "p99 time: ${P99_MS}ms"
log "average regression: $AVERAGE_REGRESSION_PERCENT%"
log "regression threshold: $REGRESSION_THRESHOLD%"

log "total elapsed: ${total_elapsed_ms}ms"
log "summary: total=$total ok=$ok fail=$fail"
log "parallel jobs: $PARALLEL_JOBS"
log "top slowest tracked: $TOP_COUNT"
log "categories detected: $UNIQUE_CATEGORIES"
if [ -f "$JSON_REPORT" ]; then
  log "artifacts generated successfully"
fi

if [ "$fail" -ne 0 ]; then
  exit 1
fi

if [ "$REGRESSION_FAILURE" -ne 0 ]; then
  log "performance regression exceeded threshold"
  exit 1
fi

exit 0
