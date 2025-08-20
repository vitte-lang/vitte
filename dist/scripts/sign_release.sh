#!/usr/bin/env bash
set -euo pipefail
gpg --armor --detach-sign --output manifests/checksums.txt.asc manifests/checksums.txt || true
