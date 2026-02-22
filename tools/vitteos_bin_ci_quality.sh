#!/usr/bin/env bash
set -euo pipefail

tools/vitteos_bin_uniformity_check.sh
tools/vitteos_bin_vit_check.sh
tools/vitteos_bin_lint.sh
tools/vitteos_bin_posix_compat.sh
tools/vitteos_bin_matrix_report.sh
tools/vitteos_bin_runtime_check.sh || true
