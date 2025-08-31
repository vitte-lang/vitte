#!/usr/bin/env bash
# ============================================================
# Vitte — scripts/lint.sh
# Orchestration des lints (Rust + annexes).
#
# Usage :
#   scripts/lint.sh              # rustfmt + clippy
#   scripts/lint.sh --fix        # corrige rustfmt, suggestions clippy
#   scripts/lint.sh --all        # inclut TOML/Markdown/Shell
#   scripts/lint.sh --strict     # clippy warnings => errors
#   scripts/lint.sh --help
#
# Prérequis :
#   rustfmt, clippy (via rustup component add)
# Optionnels :
#   taplo (cargo install taplo-cli)
#   prettier (npm install -g prettier)
#   shellcheck (paquet distro)
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
log()  { echo -e "${C_BLU}[lint]${C_RST} $*"; }
ok()   { echo -e "${C_GRN}[ok]${C_RST} $*"; }
warn() { echo -e "${C_YLW}[!]${C_RST} $*"; }
err()  { echo -e "${C_RED}[err]${C_RST} $*"; }

# ---------- options ----------
FIX=false
ALL=false
STRICT=false

while (( "$#" )); do
  case "$1" in
    --fix) FIX=true; shift;;
    --all) ALL=true; shift;;
    --strict) STRICT=true; shift;;
    --help)
      sed -n '1,/^set -euo pipefail/p' "$0" | sed 's/^# \{0,1\}//'
      exit 0;;
    *) err "Option inconnue: $1"; exit 1;;
  esac
done

# ---------- Rustfmt ----------
if cargo fmt -V >/dev/null 2>&1; then
  if $FIX; then
    log "rustfmt (correction)…"
    cargo fmt --all
  else
    log "rustfmt (check)…"
    cargo fmt --all -- --check
  fi
else
  warn "rustfmt non installé. Fais: rustup component add rustfmt"
fi

# ---------- Clippy ----------
if cargo clippy -V >/dev/null 2>&1; then
  log "clippy…"
  CLIPPY_ARGS=(--workspace --all-targets)
  if $STRICT; then
    CLIPPY_ARGS+=(-- -D warnings)
  elif $FIX; then
    CLIPPY_ARGS+=(--fix --allow-dirty --allow-staged)
  fi
  cargo clippy "${CLIPPY_ARGS[@]}"
else
  warn "clippy non installé. Fais: rustup component add clippy"
fi

# ---------- Annexes (ALL) ----------
if $ALL; then
  # TOML
  if command -v taplo >/dev/null 2>&1; then
    if $FIX; then
      log "taplo fmt sur TOML"
      taplo fmt
    else
      log "taplo check sur TOML"
      taplo fmt --check
    fi
  else
    warn "taplo non trouvé (cargo install taplo-cli)"
  fi

  # Markdown / JSON / YAML
  if command -v prettier >/dev/null 2>&1; then
    if $FIX; then
      log "prettier fmt sur docs/"
      prettier --write "docs/**/*.{md,json,yml,yaml}" || true
    else
      log "prettier check sur docs/"
      prettier --check "docs/**/*.{md,json,yml,yaml}" || true
    fi
  else
    warn "prettier non trouvé (npm install -g prettier)"
  fi

  # Shell scripts
  if command -v shellcheck >/dev/null 2>&1; then
    log "shellcheck sur scripts/*.sh"
    shellcheck scripts/*.sh || true
  else
    warn "shellcheck non trouvé (apt/yum/pacman install shellcheck)"
  fi
fi

ok "Lints terminés."
