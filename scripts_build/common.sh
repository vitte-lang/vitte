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

scripts_build_sha256_write() {
  file=$1
  output=${2:-$file.sha256}

  [ -s "$file" ] ||
    scripts_build_die "checksum input missing or empty: $file"

  if command -v shasum >/dev/null 2>&1; then
    (
      cd "$(dirname "$file")"
      shasum -a 256 "$(basename "$file")" > "$output"
    )
    return 0
  fi

  if command -v sha256sum >/dev/null 2>&1; then
    (
      cd "$(dirname "$file")"
      sha256sum "$(basename "$file")" > "$output"
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

  if command -v shasum >/dev/null 2>&1; then
    (cd "$(dirname "$file")" && shasum -a 256 -c "$(basename "$sum_file")" >/dev/null)
    return 0
  fi

  if command -v sha256sum >/dev/null 2>&1; then
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
