#!/usr/bin/env sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP="${TMPDIR:-/tmp}/vitte-post-install-cli-test-$$"
mkdir -p "$TMP/home" "$TMP/bin"
trap 'rm -rf "$TMP"' EXIT HUP INT TERM

VITTE="$ROOT/bin/vitte"

env -i HOME="$TMP/home" PATH="/usr/bin:/bin" "$VITTE" package graph explain >/tmp/vitte-package-graph-$$.json
env -i HOME="$TMP/home" PATH="/usr/bin:/bin" "$VITTE" workspace build --package packages/app >/tmp/vitte-workspace-build-$$.json
env -i HOME="$TMP/home" PATH="/usr/bin:/bin" "$VITTE" workspace test --all >/tmp/vitte-workspace-test-$$.json
env -i HOME="$TMP/home" PATH="/usr/bin:/bin" "$VITTE" publish --dry-run >/tmp/vitte-publish-dry-run-$$.json

python3 - "$$" <<'PY'
import json
import sys
from pathlib import Path

pid = sys.argv[1]
checks = {
    f"/tmp/vitte-package-graph-{pid}.json": "vitte.package.graph.explain",
    f"/tmp/vitte-workspace-build-{pid}.json": "vitte.workspace.build",
    f"/tmp/vitte-workspace-test-{pid}.json": "vitte.workspace.test",
    f"/tmp/vitte-publish-dry-run-{pid}.json": "vitte.publish.dry_run",
}
for path, schema in checks.items():
    value = json.loads(Path(path).read_text(encoding="utf-8"))
    assert value["schema"] == schema, (path, value)
    assert value["status"] == "ok", (path, value)
    Path(path).unlink()
PY

printf '[post-install-package-cli] ok\n'
