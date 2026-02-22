#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT_DIR}"
MANIFEST="vitteos/domains.json"
INDEX_FILE="vitteos/modules.index"

usage() {
  cat >&2 <<'USAGE'
usage: tools/vitteos_new_module.sh <module_path>
example:
  tools/vitteos_new_module.sh vitteos/fs/cache2
USAGE
  exit 2
}

[[ $# -eq 1 ]] || usage

module_path="$1"
case "${module_path}" in
  vitteos/*) ;;
  *) echo "module_path must start with 'vitteos/'" >&2; exit 2 ;;
esac

module_dir="${module_path%/}"
space_name="${module_dir#vitteos/}"
module_root="${space_name%%/*}"

if [[ ! -f "${MANIFEST}" ]]; then
  echo "missing manifest: ${MANIFEST}" >&2
  exit 1
fi

if ! python3 - "$MANIFEST" "$module_root" <<'PY'
import json, sys
d = json.load(open(sys.argv[1], encoding="utf-8"))
root = sys.argv[2]
sys.exit(0 if root in d["domains"] else 1)
PY
then
  echo "module root '${module_root}' is not declared in vitteos/domains.json domains" >&2
  exit 1
fi

mkdir -p "${module_dir}"

create_file() {
  local target="$1"
  local content="$2"
  if [[ -e "${target}" ]]; then
    echo "skip existing: ${target}"
    return
  fi
  printf '%s\n' "${content}" > "${target}"
  echo "created: ${target}"
}

create_file "${module_dir}/mod.vit" "space ${space_name}/mod

<<< module wiring only (no business logic) >>>

use ${space_name}/api

share all"

create_file "${module_dir}/api.vit" "space ${space_name}/api

<<< public boundary: expose stable API only >>>
<<< avoid naked primitives in public signatures >>>"

create_file "${module_dir}/types.vit" "space ${space_name}/types

<<< domain model types >>>
<<< define explicit types for critical states >>>"

create_file "${module_dir}/consts.vit" "space ${space_name}/consts

<<< domain invariants and constants >>>"

create_file "${module_dir}/impl.vit" "space ${space_name}/impl

<<< internal execution only >>>
<<< no public exports here: API is ${space_name}/api >>>"

create_file "${module_dir}/tests.vit" "space ${space_name}/tests

<<< deterministic domain tests >>>"

create_file "${module_dir}/doc.vit" "space ${space_name}/doc

<<< role: TODO >>>
<<< allowed-dependencies: TODO >>>
<<< invariants: TODO >>>
<<< errors: TODO >>>
<<< contract: document each public API proc >>>"

touch "${INDEX_FILE}"
if ! grep -qx "${module_dir}" "${INDEX_FILE}"; then
  printf '%s\n' "${module_dir}" >> "${INDEX_FILE}"
  tmp="$(mktemp)"
  grep -v '^[[:space:]]*$' "${INDEX_FILE}" | awk '!seen[$0]++' | sort > "${tmp}"
  mv "${tmp}" "${INDEX_FILE}"
  echo "indexed: ${module_dir} -> ${INDEX_FILE}"
fi
