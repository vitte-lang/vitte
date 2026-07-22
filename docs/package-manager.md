# Vitte Package Manager

Status: official surface for Vitte 0.1.0 package and workspace tooling.

The package manager supports four release-gated flows:

- `vitte package graph explain`: prints the resolved package graph, node count, edge count, selected root, and cycle status.
- `vitte workspace build --package NAME`: builds only selected workspace members and their dependencies.
- `vitte workspace test --all`: runs every package test command in deterministic workspace order.
- `vitte publish --dry-run`: validates package metadata and file inclusion without writing to any registry.

Every package artifact must be reproducible from the package manifest, lockfile, target triple, ABI, and selected features. Offline registry mode is the baseline for bootstrap and installer testing.

Required generated evidence:

- `target/package_manager/package_graph_explain.json`
- `target/package_manager/workspace_build_selective.json`
- `target/package_manager/workspace_test_all.json`
- `target/package_manager/workspace_publish_dry_run.json`
