#!/bin/sh

scripts_build_die() {
  script_name=${SCRIPT_NAME:-scripts_build}
  printf '[%s][error] %s\n' "$script_name" "$*" >&2
  exit 1
}

scripts_build_require() {
  command -v "$1" >/dev/null 2>&1 ||
    scripts_build_die "missing required tool: $1"
}

scripts_build_parse_common_flags() {
  DRY_RUN=${DRY_RUN:-0}
  HELP=${HELP:-0}
  LIST_TARGETS=${LIST_TARGETS:-0}
  PRINT_ENV=${PRINT_ENV:-0}
  VERIFY_ONLY=${VERIFY_ONLY:-}
  CLEAN=${CLEAN:-0}

  while [ "$#" -gt 0 ]; do
    case "$1" in
      --dry-run)
        DRY_RUN=1
        ;;
      --help | -h)
        HELP=1
        ;;
      --list-targets)
        LIST_TARGETS=1
        ;;
      --print-env)
        PRINT_ENV=1
        ;;
      --verify-only)
        shift
        VERIFY_ONLY=${1:?missing OUT_DIR for --verify-only}
        ;;
      --clean)
        CLEAN=1
        ;;
      *)
        scripts_build_die "unsupported option: $1"
        ;;
    esac
    shift
  done
}

scripts_build_maybe_help() {
  usage=$1
  if [ "${HELP:-0}" -eq 1 ]; then
    printf '%s\n' "$usage"
    exit 0
  fi
}

scripts_build_maybe_dry_run() {
  description=$1
  if [ "${DRY_RUN:-0}" -eq 1 ]; then
    script_name=${SCRIPT_NAME:-scripts_build}
    printf '[%s][dry-run] %s\n' "$script_name" "$description"
    exit 0
  fi
}

scripts_build_sha256_write() {
  file=$1
  output=${2:-$file.sha256}
  output_name=$(basename "$output")

  [ -s "$file" ] ||
    scripts_build_die "checksum input missing or empty: $file"

  if [ "${SCRIPTS_BUILD_SHA256_BACKEND:-auto}" != python ] && command -v shasum >/dev/null 2>&1; then
    (
      cd "$(dirname "$file")"
      shasum -a 256 "$(basename "$file")" > "$output_name"
    )
    return 0
  fi

  if [ "${SCRIPTS_BUILD_SHA256_BACKEND:-auto}" != python ] && command -v sha256sum >/dev/null 2>&1; then
    (
      cd "$(dirname "$file")"
      sha256sum "$(basename "$file")" > "$output_name"
    )
    return 0
  fi

  scripts_build_require python3
  python3 - "$file" "$output" <<'PY'
import hashlib
import sys
from pathlib import Path

file = Path(sys.argv[1])
output = Path(sys.argv[2])
output.write_text(
    f"{hashlib.sha256(file.read_bytes()).hexdigest()}  {file.name}\n",
    encoding="utf-8",
)
PY
}

scripts_build_sha256_check() {
  file=$1
  sum_file=${2:-$file.sha256}

  [ -s "$file" ] ||
    scripts_build_die "missing artifact: $file"
  [ -s "$sum_file" ] ||
    scripts_build_die "missing checksum: $sum_file"

  if [ "${SCRIPTS_BUILD_SHA256_BACKEND:-auto}" != python ] && command -v shasum >/dev/null 2>&1; then
    (cd "$(dirname "$file")" && shasum -a 256 -c "$(basename "$sum_file")" >/dev/null)
    return 0
  fi

  if [ "${SCRIPTS_BUILD_SHA256_BACKEND:-auto}" != python ] && command -v sha256sum >/dev/null 2>&1; then
    (cd "$(dirname "$file")" && sha256sum -c "$(basename "$sum_file")" >/dev/null)
    return 0
  fi

  scripts_build_require python3
  python3 - "$file" "$sum_file" <<'PY'
import hashlib
import sys
from pathlib import Path

file = Path(sys.argv[1])
sum_file = Path(sys.argv[2])
expected = sum_file.read_text(encoding="utf-8").split()[0]
actual = hashlib.sha256(file.read_bytes()).hexdigest()
if actual != expected:
    raise SystemExit(f"checksum mismatch: {file}")
PY
}

scripts_build_tar_list_xz() {
  archive=$1
  if tar -tJf "$archive" >/dev/null 2>&1; then
    tar -tJf "$archive"
    return 0
  fi
  tar -tzf "$archive"
}

scripts_build_copy_tree() {
  source=$1
  destination=$2
  [ -e "$source" ] || return 0
  mkdir -p "$destination"
  COPYFILE_DISABLE=1 tar -cf - -C "$source" . | tar -xf - -C "$destination"
  find "$destination" \( -name '.DS_Store' -o -name '._*' -o -name '.vitte-cache' -o -name '__pycache__' -o -name 'node_modules' \) -prune -exec rm -rf {} \; 2>/dev/null || true
}

scripts_build_tar_gz() {
  output=$1
  base=$2
  shift 2
  COPYFILE_DISABLE=1 tar -czf "$output" -C "$base" "$@"
}

scripts_build_tar_xz() {
  output=$1
  base=$2
  shift 2
  COPYFILE_DISABLE=1 tar -cJf "$output" -C "$base" "$@"
}

scripts_build_write_artifact_manifest() {
  file=$1
  platform=$2
  arch=$3
  version=$4
  output=${5:-$file.MANIFEST.json}

  scripts_build_require python3
  python3 - "$file" "$platform" "$arch" "$version" "$output" <<'PY'
import hashlib
import json
import sys
from pathlib import Path

file = Path(sys.argv[1])
manifest = {
    "schema": "org.vitte.installer-artifact.v1",
    "name": file.name,
    "platform": sys.argv[2],
    "arch": sys.argv[3],
    "version": sys.argv[4],
    "size": file.stat().st_size,
    "sha256": hashlib.sha256(file.read_bytes()).hexdigest(),
}
Path(sys.argv[5]).write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY
}
