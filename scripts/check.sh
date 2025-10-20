#!/usr/bin/env bash
set -euo pipefail

append_unique_flag() {
  local var_name="$1"
  local flag="$2"
  local current="${!var_name-}"
  if [[ -n "$current" ]]; then
    case " $current " in
      *" $flag "*) return ;;
      *) export "$var_name"="$current $flag" ;;
    esac
  else
    export "$var_name"="$flag"
  fi
}

STRICT_WARNINGS=1
if [[ -n "${VITTE_STRICT_WARNINGS:-}" ]]; then
  case "${VITTE_STRICT_WARNINGS,,}" in
    0|false|no|off)
      STRICT_WARNINGS=0
      ;;
  esac
fi

if (( STRICT_WARNINGS )); then
  append_unique_flag RUSTFLAGS "-Dwarnings"
  append_unique_flag RUSTDOCFLAGS "-Dwarnings"
  if [[ -z "${VITTE_STRICT_WARNINGS_EMITTED:-}" ]]; then
    echo "▶ mode strict (warnings → erreurs)"
  fi
else
  if [[ -z "${VITTE_STRICT_WARNINGS_EMITTED:-}" ]]; then
    echo "⚠ mode strict désactivé (VITTE_STRICT_WARNINGS)"
  fi
fi

echo "▶ rustfmt --check"
FMT_OUT=$(mktemp)
set +e
cargo fmt --all -- --check >"$FMT_OUT" 2>&1
FMT_CODE=$?
set -e
if (( FMT_CODE != 0 )); then
  if command -v rustup >/dev/null 2>&1 && rustup toolchain list | grep -q "nightly"; then
    echo "⚠ rustfmt stable a échoué; tentative avec nightly"
    cargo +nightly fmt --all -- --check || { cat "$FMT_OUT"; exit 1; }
  else
    echo "⚠ rustfmt stable a échoué et nightly introuvable. Installe-le avec 'rustup toolchain install nightly' ou 'INSTALL=1 make bootstrap'."
    cat "$FMT_OUT"
    exit 1
  fi
fi

echo "▶ clippy (all features)"
cargo clippy --workspace --all-features -- -D warnings

echo "▶ tests (all features)"
cargo test --workspace --all-features

if command -v cargo-deny >/dev/null 2>&1; then
  echo "▶ cargo-deny"
  cargo deny check advisories bans licenses sources
else
  echo "⚠ cargo-deny non installé — skip (install: `cargo install cargo-deny`)"
fi

echo "▶ docs (no-deps)"
cargo doc --workspace --all-features --no-deps

echo "▶ architecture lint"
./scripts/pro/arch-lint.py

echo "✅ check OK"
