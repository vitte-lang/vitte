#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT="$ROOT_DIR/target/reports/network_process_profile_matrix.txt"
mkdir -p "$(dirname "$OUT")"
{
  echo "profile,http,http_client,process"
  echo "core,0,0,0"
  echo "desktop,1,1,1"
  echo "system,1,1,1"
} > "$OUT"
echo "[network-process-profile-matrix] wrote $OUT"
