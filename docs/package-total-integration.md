# Vitte Package Total Integration

Status: blocking contract for Vitte 0.1.0 package integration.

Vitte may claim total package integration only when all of these gates pass:

- `make package-format-gate`
- `make package-registry-gate`
- `make package-compile-gate`
- `make package-import-gate`
- `make package-lockfile-gate`
- `make script-build-install-gate`
- `make installer-doctor-gate`
- `make post-install-build-run-gate`
- `make vitte-total-integration-gate`
- `make release-check`

The required evidence is:

- every package is compiled or represented by an installed typed package cache entry;
- every package is importable from the offline language package graph;
- every export is typed or represented by stable typed export metadata;
- every dependency is locked by deterministic checksum-bearing lockfiles;
- every scripts build path resolves Vitte through an absolute executable without user shell configuration;
- every installer passes install, `--version`, `--help`, `check`, `build`, and post-build execution;
- every platform artifact carries a manifest, SHA-256 checksums, SBOM files, local registry, stdlib cache, package cache, install script, uninstall script, and installer doctor;
- release validation is blocked when any one of those gates fails.
