#!/usr/bin/env bash
# =============================================================================
# Runner de tests goldens (parse/AST)
# -----------------------------------------------------------------------------
# Exécute le parser sur les cas situés dans tests/goldens/parse et vérifie que
# les diagnostics attendus apparaissent dans la sortie.
# Chaque fichier .golden contient une liste de chaînes à retrouver (une par
# ligne). Les lignes vides ou débutant par '#' sont ignorées.
# =============================================================================

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GOLDEN_DIR="$ROOT/tests/goldens/parse"

PYTHON_BIN="${PYTHON:-python3}"
PARSER_WRAPPER="$ROOT/tools/run_parser_with_diags.py"

if [[ ! -f "$PARSER_WRAPPER" ]]; then
  echo "[goldens] ERROR: wrapper parser manquant: $PARSER_WRAPPER"
  exit 1
fi

if [[ ! -d "$GOLDEN_DIR" ]]; then
  echo "[goldens] WARNING: pas de dossier $GOLDEN_DIR, rien à faire."
  exit 0
fi

status=0
shopt -s nullglob
goldens=("$GOLDEN_DIR"/*.golden)
if [[ ${#goldens[@]} -eq 0 ]]; then
  echo "[goldens] WARNING: aucun fichier .golden dans $GOLDEN_DIR"
  exit 0
fi

cd "$ROOT"

for golden in "${goldens[@]}"; do
  name="$(basename "$golden" .golden)"
  rel_input="tests/goldens/parse/$name.vitte"
  input="$ROOT/$rel_input"

  if [[ ! -f "$input" ]]; then
    echo "[goldens][FAIL] $name: fichier d'entrée manquant ($input)"
    status=1
    continue
  fi

  expected_exit=""
  expected_strings=()
  while IFS= read -r line; do
    [[ -z "$line" || "${line:0:1}" == "#" ]] && continue
    if [[ "$line" =~ ^exit:([0-9]+)$ ]]; then
      expected_exit="${BASH_REMATCH[1]}"
      continue
    fi
    expected_strings+=("$line")
  done <"$golden"

  tmp="$(mktemp)"
  run_rc=0
  "$PYTHON_BIN" "$PARSER_WRAPPER" "$rel_input" >"$tmp" 2>&1 || run_rc=$?

  fail=0
  if [[ -n "$expected_exit" && "$run_rc" -ne "$expected_exit" ]]; then
    echo "[goldens][FAIL] $name: exit $run_rc (attendu $expected_exit)"
    fail=1
  fi

  for expected in "${expected_strings[@]:-}"; do
    if ! grep -Fq "$expected" "$tmp"; then
      echo "[goldens][FAIL] $name: manquant => $expected"
      fail=1
    fi
  done

  if [[ $fail -eq 0 ]]; then
    echo "[goldens][OK] $name"
  else
    echo "[goldens][OUTPUT] $name:"
    sed 's/^/    /' "$tmp"
    status=1
  fi

  rm -f "$tmp"
done

exit $status
