#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

log()  { printf '▶ %s\n' "$*"; }
warn() { printf '⚠ %s\n' "$*" >&2; }
ok()   { printf '✓ %s\n' "$*"; }

# Checks summary
STATUS=0

check_cmd() {
  local cmd="$1"; shift
  local desc="${*:-$cmd}"
  if command -v "$cmd" >/dev/null 2>&1; then
    ok "$desc";
  else
    warn "$desc manquant";
    STATUS=1
  fi
}

log "Système"
uname -a || warn "uname indisponible"

log "Toolchains"
check_cmd rustup "rustup"
check_cmd cargo "cargo"
check_cmd rustc "rustc"
if command -v rustup >/dev/null 2>&1; then
  rustup show active-toolchain | sed 's/^/  /'
  if rustup toolchain list | grep -q '^nightly'; then
    ok "toolchain nightly installée"
  else
    warn "toolchain nightly absente (recommandé pour rustfmt avancé)."
  fi
fi

log "Composants rustup"
if command -v rustup >/dev/null 2>&1; then
  for component in rustfmt clippy rust-src; do
    if rustup component list --installed | grep -Fq "$component"; then
      ok "component $component"
    else
      warn "component $component manquant"
      STATUS=1
    fi
  done
fi

log "Outils cargo optionnels"
for tool in cargo-deny cargo-nextest cargo-tarpaulin wasm-pack just; do
  if command -v "$tool" >/dev/null 2>&1; then
    ok "$tool"
  else
    warn "$tool manquant (optionnel)"
  fi
done

log "Node / npm"
if command -v node >/dev/null 2>&1; then
  ok "node"
else
  warn "node manquant (utile pour outils web)"
fi
if command -v npm >/dev/null 2>&1; then
  ok "npm"
else
  warn "npm manquant"
fi

log "Caches cargo"
if [[ -d "$HOME/.cargo/bin" ]]; then
  ok "~/.cargo/bin présent"
else
  warn "~/.cargo/bin introuvable"
fi

if [[ -d target ]]; then
  du -sh target 2>/dev/null || true
fi

log "Scripts clés"
for script in scripts/check.sh scripts/lint.sh scripts/test.sh scripts/pro/arch-lint.py; do
  if [[ -x "$script" ]]; then
    ok "$script exécutable"
  else
    warn "$script non exécutable"
    STATUS=1
  fi
done

if (( STATUS == 0 )); then
  log "Doctor OK"
else
  warn "Doctor a signalé des éléments manquants"
fi

exit "$STATUS"
