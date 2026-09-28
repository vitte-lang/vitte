# Release Engineering

Operational target:

- `make release-engineering`

This target industrializes Vitte releases with:

- debug/release profile exercise
- release artifact generation
- source archive
- binary archive
- SHA256 checksums
- changelog and release notes embedding
- optional signature (set `VITTE_GPG_KEY`)
- install test from archive
- verification via `vittec release verify`

Generated artifacts:

- `build/releases/vitte-0.1.0.tar.gz`
- `build/releases/vitte-0.1.0.tar.gz.sha256`
- `build/releases/vitte-0.1.0-src.tar.gz`
- `build/releases/vitte-0.1.0-src.tar.gz.sha256`
- `build/reports/release_engineering.txt`
