#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BASE_REF="${BASE_REF:-HEAD~1}"

if ! git -C "$ROOT_DIR" rev-parse --verify "$BASE_REF" >/dev/null 2>&1; then
  echo "[changed-packages-ci][warn] BASE_REF=$BASE_REF not found, running packages-only-ci"
  exec make -C "$ROOT_DIR" -s packages-only-ci
fi

changed=$(git -C "$ROOT_DIR" diff --name-only "$BASE_REF"...HEAD)
if [ -z "$changed" ]; then
  echo "[changed-packages-ci] no changes detected; running facade gates only"
  exec make -C "$ROOT_DIR" -s facade-role-contracts-lint facade-thin-lint diag-namespace-lint
fi

pkgs=()
while IFS= read -r f; do
  case "$f" in
    src/vitte/packages/*)
      p=$(echo "$f" | cut -d/ -f4)
      pkgs+=("$p")
      ;;
    tests/modules/contracts/*)
      p=$(echo "$f" | cut -d/ -f4)
      pkgs+=("$p")
      ;;
    tools/lint_*_*)
      p=$(basename "$f" | sed -E 's/^lint_([a-z_]+)_.*/\1/')
      pkgs+=("$p")
      ;;
  esac
done <<< "$changed"

if [ ${#pkgs[@]} -eq 0 ]; then
  echo "[changed-packages-ci] no package-local changes; running packages-only-ci"
  exec make -C "$ROOT_DIR" -s packages-only-ci
fi

# unique
uniq_pkgs=$(printf '%s\n' "${pkgs[@]}" | sort -u)

targets=(facade-role-contracts-lint facade-thin-lint diag-namespace-lint)
while IFS= read -r p; do
  [ -n "$p" ] || continue
  case "$p" in
    core|std|log|fs|db|http|process|json|yaml|test|lint)
      targets+=("$p-only-ci")
      ;;
    http_client)
      targets+=("http-client-only-ci")
      ;;
  esac
done <<< "$uniq_pkgs"

echo "[changed-packages-ci] targets: ${targets[*]}"
make -C "$ROOT_DIR" -s "${targets[@]}"
