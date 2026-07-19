Central diagnostic catalog test manifests.

The `*.catalog.json` files map every registered diagnostic code to a stable test case.
The `sources/` and `snapshots/` trees contain real negative Vitte cases used to grow
phase-specific snapshot coverage without weakening the catalog-level invariant that
every diagnostic has an associated test entry.
