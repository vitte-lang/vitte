#!/usr/bin/env bash
set -euo pipefail
TAG=${1:-v0.1.0}
 echo gh release create "$TAG" ...
