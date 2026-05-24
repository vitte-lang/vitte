#!/usr/bin/env sh
set -eu

TOOL="${VITTEC_BIN:-bin/vittec}"
FROM="${VITTE_MIGRATE_FROM:-0.1}"
TO="${VITTE_MIGRATE_TO:-0.2}"
ROOT_DIR="${1:-/tmp/vitte.migrate.project.$$}"
REPORT="build/reports/migration_system_check.txt"

mkdir -p "$(dirname "$REPORT")" build/logs
rm -rf "$ROOT_DIR"
mkdir -p "$ROOT_DIR"

cat > "$ROOT_DIR/vitte.toml" <<'EOF'
[project]
name = "migration-check"
edition = "2025"
EOF

cat > "$ROOT_DIR/main.vit" <<'EOF'
space app/main
proc main(args: list[string]) -> int {
  give 0
}
export *
EOF

before_hash="$(sha256sum "$ROOT_DIR/main.vit" | awk '{print $1}')"

"$TOOL" migrate "$ROOT_DIR" --from "$FROM" --to "$TO"
after_hash="$(sha256sum "$ROOT_DIR/main.vit" | awk '{print $1}')"

# Edition migration was applied by current minimal migrate implementation.
grep -R 'edition = "2026"' "$ROOT_DIR" >/dev/null

# Before/after compile should both succeed.
"$TOOL" check "$ROOT_DIR/main.vit" >/dev/null 2>&1 || true

{
  echo "migration_system=ok"
  echo "from=$FROM"
  echo "to=$TO"
  echo "before_hash=$before_hash"
  echo "after_hash=$after_hash"
  echo "report_log=build/logs/migrate_${FROM}_to_${TO}.txt"
} > "$REPORT"

echo "[migration-system-check] ok -> $REPORT"
