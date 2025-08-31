#!/usr/bin/env bash
# ============================================================
# Vitte — scripts/fmt.sh
# Formatteur et vérif de style.
#
# Usage :
#   scripts/fmt.sh             # formate tout le workspace
#   scripts/fmt.sh --check     # vérifie sans modifier (CI)
#   scripts/fmt.sh --all       # inclut tests/examples/benches
#   scripts/fmt.sh --unstable  # active options nightly
#   scripts/fmt.sh --help      # affiche cette aide
#
# Variables :
#   FEATURES="..."             # activer des features cargo
#   NO_DEFAULT=1               # désactiver default-features
# ============================================================

set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# ---------- couleurs ----------
if [ -t 1 ]; then
  C_RED="\033[31m"; C_GRN="\033[32m"; C_YLW="\033[33m"; C_BLU="\033[34m"; C_RST="\033[0m"
else
  C_RED=""; C_GRN=""; C_YLW=""; C_BLU=""; C_RST=""
fi
log()  { echo -e "${C_BLU}[fmt]${C_RST} $*"; }
ok()   { echo -e "${C_GRN}[ok]${C_RST} $*"; }
warn() { echo -e "${C_YLW}[!]${C_RST} $*"; }
err()  { echo -e "${C_RED}[err]${C_RST} $*"; }

# ---------- options ----------
CHECK=false
ALL=false
UNSTABLE=false

while (( "$#" )); do
  case "$1" in
    --check) CHECK=true; shift;;
    --all) ALL=true; shift;;
    --unstable) UNSTABLE=true; shift;;
    --help)
      sed -n '1,/^set -euo pipefail/p' "$0" | sed 's/^# \{0,1\}//'
      exit 0;;
    *) err "Option inconnue: $1"; exit 1;;
  esac
done

# ---------- prérequis ----------
if ! command -v cargo >/dev/null 2>&1; then
  err "cargo introuvable. Installe rustup: https://rustup.rs"
fi
if ! cargo fmt -V >/dev/null 2>&1; then
  err "rustfmt non installé. Fais: rustup component add rustfmt"
fi

# ---------- cmd ----------
CMD=(cargo fmt)
$ALL && CMD+=(--all)
if $CHECK; then
  CMD+=(-- --check)
  log "Vérification du formatage (sans modifier)…"
else
  CMD+=(--all)
  log "Application du formatage…"
fi
$UNSTABLE && CMD+=(--unstable-features)

# ---------- exécution ----------
"${CMD[@]}" || { err "Échec rustfmt"; exit 1; }
ok "rustfmt terminé"

# ---------- autres fichiers (optionnel) ----------
# On peut ajouter stylisation pour Markdown / TOML si prettier ou taplo dispo
if command -v taplo >/dev/null 2>&1; then
  if $CHECK; then
    log "taplo check sur *.toml"
    taplo fmt --check
  else
    log "taplo fmt sur *.toml"
    taplo fmt
  fi
else
  warn "taplo (TOML formatter) non trouvé — skip"
fi

if command -v prettier >/dev/null 2>&1; then
  if $CHECK; then
    log "prettier check sur docs/*.md"
    prettier --check "docs/**/*.md" || true
  else
    log "prettier fmt sur docs/*.md"
    prettier --write "docs/**/*.md" || true
  fi
else
  warn "prettier non trouvé — skip Markdown"
fi

ok "Formatage global OK"
