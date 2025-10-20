#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/bootstrap.sh [options]

Environment / tooling bootstrap for the Vitte workspace. Checks the Rust
toolchain, required components, optional cargo tools, and supporting binaries.

Options:
  --install         Attempt to install missing components/tools automatically.
  --skip-node       Skip Node.js/NPM presence checks.
  --skip-wasm       Skip wasm-pack availability checks.
  -h, --help        Show this message.

You can run via 'make bootstrap' and set INSTALL=1 to enable auto-install.
EOF
}

log()  { printf '▶ %s\n' "$*"; }
warn() { printf '⚠ %s\n' "$*" >&2; }
ok()   { printf '✓ %s\n' "$*"; }

INSTALL_MISSING=0
CHECK_NODE=1
CHECK_WASM=1

if [[ "${INSTALL:-0}" =~ ^(1|true|yes|on)$ ]]; then
  INSTALL_MISSING=1
fi
if [[ "${SKIP_NODE:-0}" =~ ^(1|true|yes|on)$ ]]; then
  CHECK_NODE=0
fi
if [[ "${SKIP_WASM:-0}" =~ ^(1|true|yes|on)$ ]]; then
  CHECK_WASM=0
fi

while [[ $# -gt 0 ]]; do
  case "$1" in
    --install)
      INSTALL_MISSING=1
      shift
      ;;
    --skip-node)
      CHECK_NODE=0
      shift
      ;;
    --skip-wasm)
      CHECK_WASM=0
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      warn "Unknown option: $1"
      usage
      exit 1
      ;;
  esac
done

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

ensure_cmd() {
  local cmd="$1"
  local label="${2:-$1}"
  if command -v "$cmd" >/dev/null 2>&1; then
    ok "$label présent"
    return 0
  fi
  warn "$label manquant"
  return 1
}

CRITICAL_MISSING=0
WARNINGS=0

if ! ensure_cmd rustup "rustup"; then
  warn "Installe rustup depuis https://rustup.rs/"
  CRITICAL_MISSING=1
fi

if ! ensure_cmd cargo "cargo"; then
  warn "Installe Rust (voir rustup) pour obtenir cargo."
  CRITICAL_MISSING=1
fi

if ! ensure_cmd rustc "rustc"; then
  warn "Installe Rust (voir rustup) pour obtenir rustc."
  CRITICAL_MISSING=1
fi

TOOLCHAIN_FILE="rust-toolchain.toml"
TOOLCHAIN_NAME=""

if command -v rustup >/dev/null 2>&1; then
  TOOLCHAIN_NAME=$(rustup show active-toolchain 2>/dev/null | head -n1)
  TOOLCHAIN_NAME=${TOOLCHAIN_NAME%% *}
  if [[ -z "$TOOLCHAIN_NAME" ]]; then
    TOOLCHAIN_NAME="stable"
  fi
  log "Toolchain active: $TOOLCHAIN_NAME"
  # Assure la présence de nightly pour rustfmt avancé si demandé
  if (( INSTALL_MISSING )); then
    if ! rustup toolchain list | grep -q "^nightly"; then
      log "Installation de la toolchain nightly (pour rustfmt avancé)"
      if rustup toolchain install nightly -c rustfmt; then
        ok "nightly installé (avec rustfmt)"
      else
        warn "Impossible d'installer nightly. Les options rustfmt nightly resteront indisponibles."
        WARNINGS=1
      fi
    fi
  fi
fi

extract_list() {
  local key="$1"
  if [[ ! -f "$TOOLCHAIN_FILE" ]]; then
    return
  fi
  sed -n "/${key}[[:space:]]*=/,/]/p" "$TOOLCHAIN_FILE" \
    | grep -oE '"[^"]+"' \
    | tr -d '"'
}

if [[ -f "$TOOLCHAIN_FILE" && -n "$TOOLCHAIN_NAME" ]]; then
  COMPONENTS=($(extract_list "components"))
  if [[ ${#COMPONENTS[@]} -gt 0 ]]; then
    log "Vérification des composants rustup requis…"
    for component in "${COMPONENTS[@]}"; do
      if rustup component list --toolchain "$TOOLCHAIN_NAME" --installed | grep -Fq "$component"; then
        ok "component ${component}"
      else
        warn "component ${component} manquant"
        CRITICAL_MISSING=1
        if (( INSTALL_MISSING )); then
          log "Installation du composant ${component}"
          if rustup component add --toolchain "$TOOLCHAIN_NAME" "$component"; then
            ok "component ${component} installé"
          else
            warn "Impossible d'installer ${component} (voir logs)."
            CRITICAL_MISSING=1
            WARNINGS=1
          fi
        fi
      fi
    done
  fi

  TARGETS=($(extract_list "targets"))
  if [[ ${#TARGETS[@]} -gt 0 ]]; then
    log "Vérification des cibles rustup déclarées…"
    missing_targets=()
    installed_targets=$(rustup target list --toolchain "$TOOLCHAIN_NAME" --installed | awk '{print $1}')
    for target in "${TARGETS[@]}"; do
      if echo "$installed_targets" | grep -Fxq "$target"; then
        ok "target ${target}"
      else
        warn "target ${target} manquante"
        missing_targets+=("$target")
        WARNINGS=1
      fi
    done
    if (( ${#missing_targets[@]} > 0 )); then
      if (( INSTALL_MISSING )); then
        log "Installation des cibles manquantes (${missing_targets[*]})"
        if rustup target add --toolchain "$TOOLCHAIN_NAME" "${missing_targets[@]}"; then
          ok "Cibles installées"
        else
          warn "Certaines cibles n'ont pas pu être installées."
          WARNINGS=1
        fi
      else
        warn "Ajoute les cibles avec: rustup target add ${missing_targets[*]}"
      fi
    fi
  fi
fi

ensure_optional_tool() {
  local bin="$1"
  local label="$2"
  local install_cmd="$3"
  local critical="${4:-0}"
  if command -v "$bin" >/dev/null 2>&1; then
    ok "$label présent"
    return 0
  fi
  warn "$label manquant"
  local missing=1
  if (( INSTALL_MISSING )) && [[ -n "$install_cmd" ]]; then
    log "Installation de $label via: $install_cmd"
    if bash -lc "$install_cmd"; then
      ok "$label installé"
      missing=0
    else
      warn "Échec de l'installation pour $label."
      missing=1
    fi
  fi
  if (( missing )); then
    WARNINGS=1
    if (( critical )); then
      CRITICAL_MISSING=1
    fi
    return 1
  fi
  return 0
}

log "Vérification des outils cargo optionnels…"
ensure_optional_tool cargo-deny "cargo-deny" "cargo install cargo-deny --locked"
ensure_optional_tool cargo-nextest "cargo-nextest" "cargo install cargo-nextest --locked"
if (( CHECK_WASM )); then
  ensure_optional_tool wasm-pack "wasm-pack" "cargo install wasm-pack"
else
  log "wasm-pack ignoré (--skip-wasm)"
fi
ensure_optional_tool just "just" "cargo install just --locked"

if (( CHECK_NODE )); then
  log "Vérification Node.js / npm…"
  if ! ensure_cmd node "node"; then
    warn "Node.js est requis pour les outils web (npm install nodejs)."
    WARNINGS=1
  fi
  if ! ensure_cmd npm "npm"; then
    warn "npm est requis pour les outils web."
    WARNINGS=1
  fi
fi

if (( CRITICAL_MISSING )); then
  warn "Bootstrap terminé avec erreurs critiques. Corrige les éléments listés ci-dessus."
  exit 1
fi

if (( WARNINGS )); then
  warn "Bootstrap terminé avec avertissements (voir ci-dessus)."
else
  ok "Environnement prêt."
fi
