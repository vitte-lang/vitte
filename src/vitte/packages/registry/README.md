# Vitte local package registry

This directory is the authoritative, machine-readable package registry used by
offline Vitte builds.

- `registry.json` indexes only complete, locally available packages.
- `lockfile.vitte.lock` pins package sources, dependencies, ABI and checksums.
- `checksums.sha256` authenticates each package source tree and both indexes.

Implicit downloads are forbidden. A missing package, version or checksum is a
build error. Update the registry with `make package-registry-update`; verify it
without changing files with `make package-registry-gate`.
