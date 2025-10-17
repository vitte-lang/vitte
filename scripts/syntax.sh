#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if command -v tput >/dev/null 2>&1; then
  RED="$(tput setaf 1)"
  YELLOW="$(tput setaf 3)"
  GREEN="$(tput setaf 2)"
  CYAN="$(tput setaf 6)"
  BOLD="$(tput bold)"
  RESET="$(tput sgr0)"
else
  RED=""; YELLOW=""; GREEN=""; CYAN=""; BOLD=""; RESET=""
fi

msg() {
  local color="$1"; shift
  printf "%s%s%s\n" "$color" "$*" "$RESET"
}

if command -v vitte >/dev/null 2>&1; then
  VITTE_CMD=(vitte)
elif [[ -x "$ROOT/bin/bin/vitte" ]]; then
  VITTE_CMD=("$ROOT/bin/bin/vitte")
else
  VITTE_CMD=(cargo run --quiet --locked --package vitte-cli --)
fi

mapfile -t VIT_FILES < <(
  find . -type f \( -name "*.vit" -o -name "*.vitte" -o -name "*.vt" \) \
    -not -path "*/target/*" -not -path "*/.git/*" -print | sort
)

if [[ ${#VIT_FILES[@]} -eq 0 ]]; then
  msg "$YELLOW" "Aucun fichier .vit/.vitte trouvé — saute contrôle syntaxe."
else
  msg "$CYAN" "▶ Analyse syntaxique des sources Vitte (${#VIT_FILES[@]} fichiers)"
fi

failed=()
tmp_log="$(mktemp)"

for file in "${VIT_FILES[@]}"; do
  if ! "${VITTE_CMD[@]}" fmt --check "$file" >"$tmp_log" 2>&1; then
    failed+=("$file")
    msg "$RED" "✖ $file"
    sed -e "s/^/  /" "$tmp_log" | sed -E \
      -e "s/(error:)/${BOLD}${RED}\1${RESET}/Ig" \
      -e "s/(warning:)/${BOLD}${YELLOW}\1${RESET}/Ig"
  else
    msg "$GREEN" "✔ $file"
  fi
done

rm -f "$tmp_log"

if [[ ${#failed[@]} -gt 0 ]]; then
  msg "$RED" "Échec : ${#failed[@]} fichier(s) non conformes."
  exit 1
fi

msg "$GREEN" "Syntaxe Vitte valide."

msg "$CYAN" "▶ cargo check --workspace --all-features --locked --message-format=short"
log_file="$(mktemp)"
set +e
cargo check --workspace --all-features --locked --message-format=short >"$log_file" 2>&1
status=$?
set -e

if [[ $status -ne 0 ]]; then
  msg "$RED" "Échec compilation Rust."
  sed -E \
    -e "s/(error:)/${BOLD}${RED}\1${RESET}/Ig" \
    -e "s/(warning:)/${BOLD}${YELLOW}\1${RESET}/Ig" \
    "$log_file"
  rm -f "$log_file"
  exit $status
fi

msg "$GREEN" "Compilation Rust OK."
rm -f "$log_file"
