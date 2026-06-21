#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VITTE="$ROOT_DIR/bin/vitte"
EXAMPLES_DIR="${EXAMPLES_DIR:-$ROOT_DIR/examples}"
REPORT_DIR="${REPORT_DIR:-$ROOT_DIR/target/reports/arduino}"

START_TIME=$(date +%s)

FAILED_FILES=()
SUCCESS_FILES=()
FILE_DURATIONS=()
BUILD_TARGET="arduino-uno"
FASTEST_TIME=999999
SLOWEST_TIME=0
TOTAL_BUILD_TIME=0

HISTORY_FILE="$REPORT_DIR/arduino-build-history.jsonl"
BADGE_FILE="$REPORT_DIR/arduino-build-badge.svg"
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

COLOR_RED='\033[31m'
COLOR_GREEN='\033[32m'
COLOR_YELLOW='\033[33m'
COLOR_RESET='\033[0m'

detect_target() {
  case "$1" in
    *mega2560*) echo "arduino-mega2560" ;;
    *nano*) echo "arduino-nano" ;;
    *esp32*) echo "esp32" ;;
    *) echo "arduino-uno" ;;
  esac
}

on_exit() {
  END_TIME=$(date +%s)
  DURATION=$((END_TIME - START_TIME))

  log "duration=${DURATION}s"
  log "summary: total=$total ok=$ok fail=$fail"
  if [ "$total" -gt 0 ]; then
    AVG_TIME=$((TOTAL_BUILD_TIME / total))
    log "avg=${AVG_TIME}s fastest=${FASTEST_TIME}s slowest=${SLOWEST_TIME}s"
    if [ "$fail" -eq 0 ]; then
      printf "${COLOR_GREEN}BUILD SUCCESS${COLOR_RESET}\n"
    else
      printf "${COLOR_RED}BUILD FAILED${COLOR_RESET}\n"
    fi
  fi
  if [ ${#SUCCESS_FILES[@]} -ne 0 ]; then
    log "successful files: ${#SUCCESS_FILES[@]}"
  fi

  if [ ${#FILE_DURATIONS[@]} -ne 0 ]; then
    log "timings:"
    for t in "${FILE_DURATIONS[@]}"; do
      printf '  - %s\n' "$t"
    done
  fi

  if [ ${#FAILED_FILES[@]} -ne 0 ]; then
    log "failed files:"
    for f in "${FAILED_FILES[@]}"; do
      printf '  - %s\n' "$f"
    done
  fi
}

log() {
  printf "${COLOR_GREEN}[arduino-build]${COLOR_RESET} %s\n" "$*"
}
die() {
  printf "${COLOR_RED}[arduino-build][error]${COLOR_RESET} %s\n" "$*" >&2
  exit 1
}

[ -x "$VITTE" ] || die "missing compiler: $VITTE"
mkdir -p "$REPORT_DIR"

total=0
ok=0
fail=0
interrupted=0

on_interrupt() {
  interrupted=1
  log "interrupted"
}
trap on_interrupt INT TERM
trap on_exit EXIT

while IFS= read -r -d '' file; do
  case "$file" in
    *.reduced.vit|*.reduce.tmp.vit)
      continue
      ;;
  esac

  total=$((total + 1))

  if [ "$interrupted" -ne 0 ]; then
    break
  fi

  BUILD_TARGET=$(detect_target "$file")
  FILE_START=$(date +%s)
  log "emit: $file"

  if "$VITTE" emit --target "$BUILD_TARGET" "$file"; then
    ok=$((ok + 1))
    SUCCESS_FILES+=("$file")
    FILE_END=$(date +%s)
    ELAPSED=$((FILE_END - FILE_START))
    FILE_DURATIONS+=("$file:$ELAPSED")
    TOTAL_BUILD_TIME=$((TOTAL_BUILD_TIME + ELAPSED))

    if [ "$ELAPSED" -lt "$FASTEST_TIME" ]; then
      FASTEST_TIME="$ELAPSED"
    fi

    if [ "$ELAPSED" -gt "$SLOWEST_TIME" ]; then
      SLOWEST_TIME="$ELAPSED"
    fi
  else
    fail=$((fail + 1))
    FAILED_FILES+=("$file")
    FILE_END=$(date +%s)
    ELAPSED=$((FILE_END - FILE_START))
    FILE_DURATIONS+=("$file:$ELAPSED")
    TOTAL_BUILD_TIME=$((TOTAL_BUILD_TIME + ELAPSED))

    if [ "$ELAPSED" -lt "$FASTEST_TIME" ]; then
      FASTEST_TIME="$ELAPSED"
    fi

    if [ "$ELAPSED" -gt "$SLOWEST_TIME" ]; then
      SLOWEST_TIME="$ELAPSED"
    fi
  fi

done < <(
  if [ -d "$EXAMPLES_DIR" ]; then
    find "$EXAMPLES_DIR" \
    -type f \
    -name 'arduino_*.vit' \
    -print0
  fi
)

AVG_TIME=0
if [ "$total" -gt 0 ]; then
  AVG_TIME=$((TOTAL_BUILD_TIME / total))
fi

JSON_REPORT="$REPORT_DIR/arduino-build-report.json"

{
  printf '{\n'
  printf '  "target": "%s",\n' "$BUILD_TARGET"
  printf '  "total": %s,\n' "$total"
  printf '  "ok": %s,\n' "$ok"
  printf '  "fail": %s,\n' "$fail"
  printf '  "total_build_time": %s,\n' "$TOTAL_BUILD_TIME"
  printf '  "fastest_time": %s,\n' "$FASTEST_TIME"
  printf '  "slowest_time": %s,\n' "$SLOWEST_TIME"
  printf '  "average_time": %s,\n' "$AVG_TIME"

  printf '  "success_files": [\n'
  for i in "${!SUCCESS_FILES[@]}"; do
    printf '    "%s"' "${SUCCESS_FILES[$i]}"
    [ "$i" -lt $((${#SUCCESS_FILES[@]} - 1)) ] && printf ','
    printf '\n'
  done
  printf '  ],\n'

  printf '  "failed_files": [\n'
  for i in "${!FAILED_FILES[@]}"; do
    printf '    "%s"' "${FAILED_FILES[$i]}"
    [ "$i" -lt $((${#FAILED_FILES[@]} - 1)) ] && printf ','
    printf '\n'
  done
  printf '  ]\n'
  printf '  ,\n'
  printf '  "durations": [\n'
  for i in "${!FILE_DURATIONS[@]}"; do
    printf '    "%s"' "${FILE_DURATIONS[$i]}"
    [ "$i" -lt $((${#FILE_DURATIONS[@]} - 1)) ] && printf ','
    printf '\n'
  done
  printf '  ]\n'
  printf '}\n'
} > "$JSON_REPORT"

printf '{"timestamp":"%s","total":%s,"ok":%s,"fail":%s,"duration":%s}\n' \
  "$TIMESTAMP" "$total" "$ok" "$fail" "$TOTAL_BUILD_TIME" \
  >> "$HISTORY_FILE"

BUILD_STATUS="PASS"
if [ "$fail" -ne 0 ]; then
  BUILD_STATUS="FAIL"
fi

HTML_REPORT="$REPORT_DIR/arduino-build-report.html"

{
  printf '<!DOCTYPE html>\n'
  printf '<html lang="en">\n'
  printf '<head>\n'
  printf '  <meta charset="UTF-8">\n'
  printf '  <meta name="viewport" content="width=device-width, initial-scale=1.0">\n'
  printf '  <title>Arduino Build Report</title>\n'
  printf '</head>\n'
  printf '<body>\n'
  printf '  <h1>Arduino Build Report</h1>\n'
  printf '  <p><strong>Target:</strong> %s</p>\n' "$BUILD_TARGET"
  printf '  <p><strong>Total:</strong> %s</p>\n' "$total"
  printf '  <p><strong>Successful:</strong> %s</p>\n' "$ok"
  printf '  <p><strong>Failed:</strong> %s</p>\n' "$fail"
  printf '  <p><strong>Total Build Time:</strong> %ss</p>\n' "$TOTAL_BUILD_TIME"
  printf '  <p><strong>Fastest File:</strong> %ss</p>\n' "$FASTEST_TIME"
  printf '  <p><strong>Slowest File:</strong> %ss</p>\n' "$SLOWEST_TIME"
  printf '  <p><strong>Average File Time:</strong> %ss</p>\n' "$AVG_TIME"

  printf '  <h2>Successful Files</h2>\n'
  if [ ${#SUCCESS_FILES[@]} -eq 0 ]; then
    printf '  <p>None</p>\n'
  else
    printf '  <ul>\n'
    for f in "${SUCCESS_FILES[@]}"; do
      printf '    <li>%s</li>\n' "$f"
    done
    printf '  </ul>\n'
  fi

  printf '  <h2>Failed Files</h2>\n'
  if [ ${#FAILED_FILES[@]} -eq 0 ]; then
    printf '  <p>None</p>\n'
  else
    printf '  <ul>\n'
    for f in "${FAILED_FILES[@]}"; do
      printf '    <li>%s</li>\n' "$f"
    done
    printf '  </ul>\n'
  fi

  printf '  <h2>Build Timings</h2>\n'
  if [ ${#FILE_DURATIONS[@]} -eq 0 ]; then
    printf '  <p>None</p>\n'
  else
    printf '  <ul>\n'
    for t in "${FILE_DURATIONS[@]}"; do
      printf '    <li>%s</li>\n' "$t"
    done
    printf '  </ul>\n'
  fi

  printf '  <h2>Badge</h2>\n'
  printf '  <p>%s</p>\n' "$BUILD_STATUS"

  printf '  <h2>Summary</h2>\n'
  printf '  <p>Build completed with %s successful compilations and %s failures.</p>\n' "$ok" "$fail"

  printf '</body>\n'
  printf '</html>\n'
} > "$HTML_REPORT"

{
  printf '<svg xmlns="http://www.w3.org/2000/svg" width="220" height="20">\n'
  printf '<rect width="120" height="20" fill="#555"/>\n'
  if [ "$fail" -eq 0 ]; then
    printf '<rect x="120" width="100" height="20" fill="#4c1"/>\n'
  else
    printf '<rect x="120" width="100" height="20" fill="#e05d44"/>\n'
  fi
  printf '<text x="60" y="14" fill="white" text-anchor="middle">Arduino Build</text>\n'
  printf '<text x="170" y="14" fill="white" text-anchor="middle">%s %s/%s</text>\n' "$BUILD_STATUS" "$ok" "$total"
  printf '</svg>\n'
} > "$BADGE_FILE"

log "report: $JSON_REPORT"
log "report: $HTML_REPORT"
log "badge: $BADGE_FILE"
log "history: $HISTORY_FILE"
[ "$interrupted" -eq 0 ] || exit 130
[ "$fail" -eq 0 ]
