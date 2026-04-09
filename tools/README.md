# Tools Index

This directory contains repository maintenance scripts, quality gates, generators,
and local developer helpers.

Use this file as the shortest map before opening individual scripts.

## Main Groups

- `tools/*_docsgen.sh`, `tools/generate_*_doc.py`, `tools/docs_*`: documentation generation and consistency checks
- `tools/lint_*`, `tools/check_*`, `tools/*_gate*`: policy, hygiene, and release checks
- `tools/*_smoke.sh`, `tools/*_bench.sh`, `tools/*_report*`: verification, benchmarking, and report helpers
- `tools/vitteos_*`: VitteOS-specific checks and generators
- `tools/completions/`, `tools/templates/`, `tools/hooks/`, `tools/kernel/`: grouped support assets

## Start Here

- `tools/LINTS.md`: overview of important lint families and package-governance checks
- `tools/repo_hygiene_check.sh`: root artifact hygiene check
- `tools/docs_paths_check.py`: validates local documentation links/paths
- `tools/generate_make_targets_doc.py`: regenerates `docs/MAKE_TARGETS.md`
- `tools/generate_package_index.py`: regenerates `docs/PACKAGE_INDEX.md`
- `tools/generate_vitteos_status.sh`: regenerates `docs/vitteos/STATUS.md`

## Rule

When adding a new high-traffic or user-facing script to `tools/`, link it here or
place it under an existing grouped subdirectory.
