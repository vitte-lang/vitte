#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
PKG_ROOT="$ROOT_DIR/src/vitte/packages"

die() { printf "[module-starter][error] %s\n" "$*" >&2; exit 1; }
log() { printf "[module-starter] %s\n" "$*"; }

usage() {
  cat <<'USAGE'
usage: tools/new_module_starter.sh --module <name|domain/name> --owner @team/name [--since <version>] [--stability <value>]

Creates:
  src/vitte/packages/<domain>/<name>/mod.vit
  src/vitte/packages/<domain>/<name>/info.vit
  src/vitte/packages/<domain>/<name>/OWNERS
USAGE
}

MODULE=""
OWNER=""
SINCE="3.0.0"
STABILITY="stable"

while [ $# -gt 0 ]; do
  case "$1" in
    --module) MODULE="${2:-}"; shift ;;
    --owner) OWNER="${2:-}"; shift ;;
    --since) SINCE="${2:-}"; shift ;;
    --stability) STABILITY="${2:-}"; shift ;;
    -h|--help) usage; exit 0 ;;
    *) die "unknown option: $1" ;;
  esac
  shift
done

[ -n "$MODULE" ] || die "missing --module"
[ -n "$OWNER" ] || die "missing --owner"
[[ "$OWNER" =~ ^@[a-z0-9_.-]+/[a-z0-9_.-]+$ ]] || die "--owner must match @team/name"

NAME="${MODULE##*/}"
SPACE="vitte/$MODULE"
DIR="$PKG_ROOT/$MODULE"

[ ! -e "$DIR" ] || die "module already exists: $DIR"
mkdir -p "$DIR"

cat > "$DIR/OWNERS" <<EOF
$OWNER
EOF

cat > "$DIR/info.vit" <<EOF
<<<
info.vit
package $SPACE
>>>
<<< PACKAGE-META
owner: $OWNER
stability: $STABILITY
since: $SINCE
deprecated_in: -
>>>

space $SPACE

proc package_name() -> string {
    give "$NAME"
}

proc package_tag() -> string {
    give "$SPACE"
}

proc package_ready() -> bool {
    give true
}
EOF

cat > "$DIR/mod.vit" <<EOF
<<<
mod.vit
package $SPACE
>>>

space $SPACE

<<< ROLE-CONTRACT
package: $SPACE
owner: $OWNER
stability: $STABILITY
since: $SINCE
deprecated_in: -
role: Module public $SPACE
input_contract: Entrees explicites et typables
output_contract: Sorties stables et predictibles
boundary: Aucun import legacy; aliases explicites uniquement
>>>

proc ready() -> bool {
  give true
}
EOF

log "created $DIR"
