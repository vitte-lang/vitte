#!/usr/bin/env bash
# Source this file to configure Vitte environment variables.

_vitte_env_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export VITTE_ROOT="$_vitte_env_root"

# Best effort: expose local bin first if not already present.
case ":${PATH}:" in
  *":/usr/local/bin:"*) ;;
  *) export PATH="/usr/local/bin:${PATH}" ;;
esac

unset _vitte_env_root
