#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT_DIR"

BIN="${BIN:-$ROOT_DIR/bin/vitte}"
OUT_DIR="$ROOT_DIR/build/repro"
COMPARE_BIN="${COMPARE_BIN:-0}"
DIAG_BIN="${DIAG_BIN:-0}"
COMPARE_MANUAL_CPP="${COMPARE_MANUAL_CPP:-0}"

log() { printf "[repro] %s\n" "$*"; }
die() { printf "[repro][error] %s\n" "$*" >&2; exit 1; }

strip_codesig_hash() {
  python3 - "$1" <<'PY'
import sys, hashlib
path = sys.argv[1]
with open(path, 'rb') as f:
    data = f.read()
magic = b'\xfa\xde\x0c\xc0'
i = data.find(magic)
if i != -1:
    length = int.from_bytes(data[i+4:i+8], 'big')
    data = data[:i] + b'\x00' * length + data[i+length:]
print(hashlib.sha256(data).hexdigest())
PY
}

[ -x "$BIN" ] || die "missing binary: $BIN (run make build)"
mkdir -p "$OUT_DIR"

BASE_FLAGS=(
  "-O0"
  "-g0"
  "-fno-ident"
  "-fno-asynchronous-unwind-tables"
  "-fno-builtin"
  "-fno-stack-protector"
  "-ffile-prefix-map=$ROOT_DIR=."
  "-fdebug-prefix-map=$ROOT_DIR=."
)

OBJ_FLAGS=(
  "${BASE_FLAGS[@]}"
  "-fno-exceptions"
  "-fno-rtti"
)

BIN_FLAGS=(
  "${BASE_FLAGS[@]}"
)

INCLUDES=(
  "-I$ROOT_DIR/src/compiler/backends/runtime"
)

LIB_DIRS=()
if [ -n "${OPENSSL_DIR:-}" ]; then
  INCLUDES+=("-I$OPENSSL_DIR/include")
  LIB_DIRS+=("-L$OPENSSL_DIR/lib")
fi

LD_FLAGS=()
case "$(uname -s)" in
  Darwin)
    LD_FLAGS+=("-Wl,-no_uuid")
    ;;
  Linux|FreeBSD)
    LD_FLAGS+=("-Wl,--build-id=none")
    ;;
  MINGW*|MSYS*|CYGWIN*)
    LD_FLAGS+=("-Wl,/Brepro")
    ;;
esac

cases=(min branch)
for name in "${cases[@]}"; do
  VITTE_SRC="$ROOT_DIR/tests/repro/$name.vit"
  CPP_SRC="$ROOT_DIR/tests/repro/$name.cpp"
  EMIT_CPP="$OUT_DIR/$name.vitte.cpp"
  VITTE_OBJ="$OUT_DIR/$name.vitte.o"
  CPP_OBJ="$OUT_DIR/$name.cpp.o"
  EMIT_OBJ="$OUT_DIR/$name.emit.o"

  [ -f "$VITTE_SRC" ] || die "missing vitte source: $VITTE_SRC"
  if [ "$COMPARE_MANUAL_CPP" = "1" ]; then
    [ -f "$CPP_SRC" ] || die "missing c++ source: $CPP_SRC"
  fi

  log "$name: vitte -> object"
  "$BIN" build --repro --emit-obj -o "$VITTE_OBJ" "$VITTE_SRC"

  log "$name: emit c++"
  "$BIN" --emit-cpp --stdout --repro "$VITTE_SRC" > "$EMIT_CPP"

  log "$name: emitted c++ -> object"
  clang++ -std=c++20 -c "$EMIT_CPP" -o "$EMIT_OBJ" "${OBJ_FLAGS[@]}" "${INCLUDES[@]}"

  log "$name: compare objects"
  if ! cmp -s "$EMIT_OBJ" "$VITTE_OBJ"; then
    log "$name: mismatch"
    cmp -l "$EMIT_OBJ" "$VITTE_OBJ" | head -n 20 || true
    exit 1
  fi
  log "$name: OK (bit-identical)"

  if [ "$COMPARE_MANUAL_CPP" = "1" ]; then
    log "$name: manual c++ -> object"
    clang++ -std=c++20 -c "$CPP_SRC" -o "$CPP_OBJ" "${FLAGS[@]}" "${INCLUDES[@]}"
    log "$name: compare manual c++"
    if cmp -s "$CPP_OBJ" "$VITTE_OBJ"; then
      log "$name: OK (manual c++ matches)"
    else
      log "$name: manual c++ mismatch"
      cmp -l "$CPP_OBJ" "$VITTE_OBJ" | head -n 20 || true
      exit 1
    fi
  fi

  if [ "$COMPARE_BIN" = "1" ]; then
    VITTE_BIN="$OUT_DIR/$name.vitte.bin"
    EMIT_BIN="$OUT_DIR/$name.emit.bin"
    RUNTIME_CPP="$ROOT_DIR/src/compiler/backends/runtime/vitte_runtime.cpp"

    log "$name: vitte -> binary"
    "$BIN" build --repro -o "$VITTE_BIN" "$VITTE_SRC"

    log "$name: emitted c++ -> binary"
    clang++ -std=c++20 "$EMIT_CPP" "$RUNTIME_CPP" -o "$EMIT_BIN" \
      "${BIN_FLAGS[@]}" "${INCLUDES[@]}" "${LIB_DIRS[@]}" -lstdc++ -lssl -lcrypto -lcurl "${LD_FLAGS[@]}"

    log "$name: compare binaries"
    if cmp -s "$EMIT_BIN" "$VITTE_BIN"; then
      log "$name: OK (bit-identical binaries)"
    else
      if [ "$DIAG_BIN" = "1" ]; then
        diag_dir="$OUT_DIR/diag/$name"
        mkdir -p "$diag_dir"
        log "$name: writing diagnostics to $diag_dir"
        otool -hv "$EMIT_BIN" > "$diag_dir/emit.headers.txt" || true
        otool -hv "$VITTE_BIN" > "$diag_dir/vitte.headers.txt" || true
        otool -l "$EMIT_BIN" > "$diag_dir/emit.loadcmds.txt" || true
        otool -l "$VITTE_BIN" > "$diag_dir/vitte.loadcmds.txt" || true
        nm -an "$EMIT_BIN" > "$diag_dir/emit.symbols.txt" || true
        nm -an "$VITTE_BIN" > "$diag_dir/vitte.symbols.txt" || true
      fi
      if [ "${STRIP_CODESIG:-0}" = "1" ]; then
        log "$name: binary mismatch (checking after code signature strip)"
        h1="$(strip_codesig_hash "$EMIT_BIN")"
        h2="$(strip_codesig_hash "$VITTE_BIN")"
        if [ "$h1" = "$h2" ]; then
          log "$name: code signature only: OK"
          continue
        fi
        log "$name: code signature only: still different"
      fi
      log "$name: binary mismatch"
      cmp -l "$EMIT_BIN" "$VITTE_BIN" | head -n 20 || true
      exit 1
    fi
  fi
done

log "all cases OK"
exit 0
