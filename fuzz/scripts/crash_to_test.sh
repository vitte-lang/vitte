#!/usr/bin/env sh
# crash_to_test.sh — turn a minimized crash input into a standalone repro C file.
#
# Usage:
#   ./fuzz/scripts/crash_to_test.sh <target> <input> [--out DIR]
#
# The generated C file lives under fuzz/repro_tests/ by default and inlines the
# crash bytes. It includes the target TU directly and wraps it in a tiny main()
# so you can compile/run it with clang or your preferred toolchain.

set -eu

TARGET="${1:-}"
INPUT="${2:-}"
shift 2 || true

OUT_DIR="fuzz/repro_tests"

while [ $# -gt 0 ]; do
  case "$1" in
    --out) shift; OUT_DIR="${1:-}" ;;
    -h|--help)
      echo "usage: $(basename "$0") <target> <input> [--out DIR]" >&2
      exit 0 ;;
    *) echo "error: unknown arg: $1" >&2; exit 2 ;;
  esac
  shift || true
done

[ -n "$TARGET" ] || { echo "error: missing target" >&2; exit 2; }
[ -f "$INPUT" ] || { echo "error: input not found: $INPUT" >&2; exit 2; }

if [ ! -f "fuzz/targets/${TARGET}.c" ]; then
  echo "error: fuzz/targets/${TARGET}.c not found" >&2
  exit 2
fi

# Prefer shasum on macOS, fallback to sha1sum.
if command -v shasum >/dev/null 2>&1; then
  HASH=$(shasum -a 256 "$INPUT" | awk '{print $1}')
elif command -v sha1sum >/dev/null 2>&1; then
  HASH=$(sha1sum "$INPUT" | awk '{print $1}')
else
  echo "error: missing shasum/sha1sum" >&2
  exit 2
fi
SHORT=$(printf '%.8s' "$HASH")

mkdir -p "$OUT_DIR" "fuzz/corpora/${TARGET}"
OUT_FILE="$OUT_DIR/${TARGET}_${SHORT}.c"
CORPUS_COPY="fuzz/corpora/${TARGET}/repro_${SHORT}"
cp "$INPUT" "$CORPUS_COPY"

python3 - <<'PY' "$INPUT" "$OUT_FILE" "$TARGET"
import sys
from pathlib import Path
import textwrap

src = Path(sys.argv[1])
out = Path(sys.argv[2])
target = sys.argv[3]
data = src.read_bytes()

hex_bytes = ', '.join(f'0x{b:02x}' for b in data)
wrapped = textwrap.fill(hex_bytes, width=78)

content = f"""// Auto-generated repro for {target}
// Source: {src}

#include <stddef.h>
#include <stdint.h>

#define FUZZ_DRIVER_STANDALONE_MAIN 0
#define FUZZ_DISABLE_SANITIZER_TRACE 1
#include \"fuzz/targets/{target}.c\"

static const uint8_t kRepro[] = {{
{wrapped}
}};

int main(void) {{
  return FUZZ_DRIVER_TARGET(kRepro, sizeof(kRepro));
}}
"""

out.write_text(content)
print(f"wrote {out} ({len(data)} bytes embedded)")
PY

cat <<'EOT'
[repro] target=$TARGET
[repro] input=$INPUT
[repro] corpus copy -> $CORPUS_COPY
[repro] repro C file -> $OUT_FILE

Compile example:
  clang -std=c17 -I. -Ifuzz/include -DFUZZ_DISABLE_SANITIZER_TRACE=1 \
    "$OUT_FILE" -o fuzz/out/repro_${TARGET}
EOT
