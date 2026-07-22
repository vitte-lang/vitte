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

# Keep the package command path deterministic for shells that source this file.
_vitte_env_bin="/usr/local/bin"
if [ -x "$_vitte_env_bin/vitte" ]; then
  export PATH="$_vitte_env_bin${PATH:+:$PATH}"
fi

unset _vitte_env_bin
unset _vitte_env_root
