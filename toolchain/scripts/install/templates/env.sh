#!/usr/bin/env bash
# Source this file to configure Vitte environment variables.

_vitte_env_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export VITTE_ROOT="$_vitte_env_root"
export VITTE_FLUENT_DIR="${VITTE_FLUENT_DIR:-$VITTE_ROOT/locales}"

if [ -z "${VITTE_LANG:-}" ]; then
  case "${LC_ALL:-${LC_MESSAGES:-${LANG:-}}}" in
    fr*) export VITTE_LANG="fr" ;;
    es*) export VITTE_LANG="es" ;;
    de*) export VITTE_LANG="de" ;;
    it*) export VITTE_LANG="it" ;;
    pt*) export VITTE_LANG="pt-BR" ;;
    nl*) export VITTE_LANG="nl" ;;
    pl*) export VITTE_LANG="pl" ;;
    ru*) export VITTE_LANG="ru" ;;
    uk*) export VITTE_LANG="uk" ;;
    zh*) export VITTE_LANG="zh-CN" ;;
    ja*) export VITTE_LANG="ja" ;;
    ko*) export VITTE_LANG="ko" ;;
    tr*) export VITTE_LANG="tr" ;;
    ar*) export VITTE_LANG="ar" ;;
    *) export VITTE_LANG="en" ;;
  esac
fi

# Best effort: expose local bin first if not already present.
case ":${PATH}:" in
  *":/usr/local/bin:"*) ;;
  *) export PATH="/usr/local/bin:${PATH}" ;;
esac

unset _vitte_env_root
