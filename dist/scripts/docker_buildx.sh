#!/usr/bin/env bash
set -euo pipefail
IMAGE=${1:-ghcr.io/vitte-lang/vitte:0.1.0}
docker buildx build --platform linux/amd64,linux/arm64 -t "$IMAGE" --push docker/
