#!/usr/bin/env sh
# tools/scripts/checksums.sh
#
# Compute and verify SHA-256 checksums for the repo (build-critical files).
# Produces: checksums.sha256
#
# Usage:
#   sh tools/scripts/checksums.sh            # generate/update checksums.sha256
#   sh tools/scripts/checksums.sh --verify   # verify against checksums.sha256
#   sh tools/scripts/checksums.sh --all      # include more file types
#
# Exit codes:
#   0 ok
#   1 usage / missing files
#   2 checksum mismatch / verification failed

set -eu

OUT="checksums.sha256"
MODE="write"     # write|verify
INCLUDE_ALL=0

usage() {
  cat <<EOF
Usage: sh tools/scripts/checksums.sh [options]
  --verify        verify checksums against $OUT
  --write         generate/update $OUT (default)
  --out FILE      output file (default: $OUT)
  --all           include extra file types (docs, scripts, etc.)
  -h, --help
EOF
}

die(){ printf "%s\n" "$*" 1>&2; exit 1; }

has(){ command -v "$1" >/dev/null 2>&1; }

sha256_line() {
  # prints: "<hash>  <path>"
  f="$1"
  if has sha256sum; then
    sha256sum "$f" | awk '{print $1"  "substr($0, index($0,$2))}'
  elif has shasum; then
    # macOS
    shasum -a 256 "$f" | awk '{print $1"  "substr($0, index($0,$2))}'
  else
    die "No sha256 tool found (need sha256sum or shasum)."
  fi
}

# Normalize path to forward slashes for deterministic files, keep relative path.
norm_path() {
  printf "%s" "$1" | sed 's|\\|/|g'
}

list_files() {
  # Keep deterministic, repo-root relative paths.
  # "build/" excluded by default.
  # Minimal set: project meta + manifests + scripts + compiler/runtime sources + headers.
  if [ "$INCLUDE_ALL" -eq 0 ]; then
    find . \
      -type f \
      \( \
        -name ".editorconfig" -o \
        -name "CMakeLists.txt" -o \
        -name "Makefile" -o \
        -name "LICENSE" -o \
        -name "VERSION" -o \
        -name "muffin.muf" -o \
        -name "muffin.lock" -o \
        -path "./tools/scripts/*" -o \
        -path "./compiler/*" -o \
        -path "./runtime/*" -o \
        -path "./std/*" \
      \) \
      ! -path "./build/*" \
      ! -path "./.git/*" \
      | sed 's|^\./||' \
      | sort
  else
    find . \
      -type f \
      ! -path "./build/*" \
      ! -path "./.git/*" \
      | sed 's|^\./||' \
      | sort
  fi
}

verify() {
  [ -f "$OUT" ] || die "Missing $OUT (run without --verify first)."

  fail=0
  # Format is: "<hash>  <path>"
  while IFS= read -r line; do
    [ -n "$line" ] || continue
    case "$line" in
      \#*) continue ;;
    esac
    hash="$(printf "%s" "$line" | awk '{print $1}')"
    path="$(printf "%s" "$line" | cut -d' ' -f3-)"
    [ -n "$hash" ] || continue
    [ -n "$path" ] || continue

    if [ ! -f "$path" ]; then
      printf "[missing] %s\n" "$path" 1>&2
      fail=1
      continue
    fi

    got="$(sha256_line "$path" | awk '{print $1}')"
    if [ "$got" != "$hash" ]; then
      printf "[mismatch] %s\n  expected=%s\n  got     =%s\n" "$path" "$hash" "$got" 1>&2
      fail=1
    fi
  done < "$OUT"

  [ "$fail" -eq 0 ] || exit 2
  printf "[verify] OK\n"
}

write() {
  tmp="${OUT}.tmp.$$"
  : > "$tmp"
  {
    printf "# SHA-256 checksums (generated)\n"
    printf "# format: <sha256>␠␠<path>\n"
  } >> "$tmp"

  list_files | while IFS= read -r f; do
    [ -f "$f" ] || continue
    hline="$(sha256_line "$f")"
    # Ensure path is relative and normalized
    h="$(printf "%s" "$hline" | awk '{print $1}')"
    p="$(norm_path "$f")"
    printf "%s  %s\n" "$h" "$p" >> "$tmp"
  done

  mv "$tmp" "$OUT"
  printf "[write] %s\n" "$OUT"
}

# args
while [ $# -gt 0 ]; do
  case "$1" in
    --verify) MODE="verify" ;;
    --write) MODE="write" ;;
    --out) shift; [ $# -gt 0 ] || die "--out requires a value"; OUT="$1" ;;
    --all) INCLUDE_ALL=1 ;;
    -h|--help) usage; exit 0 ;;
    *) die "Unknown arg: $1" ;;
  esac
  shift
done

case "$MODE" in
  write) write ;;
  verify) verify ;;
  *) die "bad mode" ;;
esac
