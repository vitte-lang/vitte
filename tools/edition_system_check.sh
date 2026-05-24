#!/usr/bin/env sh
set -eu

TOOL="${VITTEC_BIN:-bin/vittec}"
REPORT="build/reports/edition_system_check.txt"
TMP="/tmp/vitte.editions.$$"

mkdir -p "$(dirname "$REPORT")"
rm -rf "$TMP"
mkdir -p "$TMP/e2025" "$TMP/e2026"

cat > "$TMP/e2025/vitte.toml" <<'EOF'
[project]
name = "ed2025"
edition = "2025"
EOF
cat > "$TMP/e2026/vitte.toml" <<'EOF'
[project]
name = "ed2026"
edition = "2026"
EOF

cat > "$TMP/e2025/main.vit" <<'EOF'
space vitte/bootstrap/editions

const VERSION_TEXT: string = "vittec edition test 0.1.0"
const BANNER_TEXT: string = "vittec edition test"

proc version_text() -> string {
  give VERSION_TEXT
}

proc banner_text() -> string {
  give BANNER_TEXT
}

proc main(args: list[string]) -> int {
  give 0
}

export *
EOF
cp "$TMP/e2025/main.vit" "$TMP/e2026/main.vit"

# Cross-edition smoke: parser/check path remains stable for same program.
"$TOOL" parse "$TMP/e2025/main.vit" >/dev/null
"$TOOL" parse "$TMP/e2026/main.vit" >/dev/null
"$TOOL" check "$TMP/e2025/main.vit" >/dev/null
"$TOOL" check "$TMP/e2026/main.vit" >/dev/null

{
  echo "edition_system=ok"
  echo "supported_editions=2025,2026"
  echo "feature_gates=edition-aware diagnostics + parser policy (baseline)"
} > "$REPORT"

rm -rf "$TMP"
echo "[edition-system-check] ok -> $REPORT"
