#!/usr/bin/env bash
set -euo pipefail
PHASE="${1:-${DOCS_MAX_PHASE:-phase3}}"
tools/docs_maximal.sh validate "$PHASE"
