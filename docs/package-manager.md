# Vitte Package Manager

Status: official offline surface for Vitte 0.1.0 package and workspace tooling.

The package manager supports these release-gated flows:

- `vitte package init NAME`: creates a manifest, source entry and executable test.
- `vitte package check`: validates metadata and offline dependencies, then invokes the compiler.
- `vitte package build`: compiles binary packages and emits a checksummed package artifact.
- `vitte package install`: atomically installs built artifacts into a local prefix.
- `vitte package graph`: prints the resolved workspace graph and deterministic build order.
- `vitte package graph explain`: prints the resolved package graph, node count, edge count, selected root, and cycle status.
- `vitte package publish --dry-run`: validates package metadata and files without writing to the registry.
- `vitte workspace build --package NAME`: builds only selected workspace members and their dependencies.
- `vitte workspace test --all`: compiles and executes every package test in deterministic workspace order.

All compiler calls use the absolute repository compiler path. The local registry checksum and lockfile are verified before every command. Network access and implicit dependency downloads are forbidden.

Required generated evidence:

- `target/package_manager/package_graph_explain.json`
- `target/package_manager/workspace_build_selective.json`
- `target/package_manager/workspace_test_all.json`
- `target/package_manager/workspace_publish_dry_run.json`
