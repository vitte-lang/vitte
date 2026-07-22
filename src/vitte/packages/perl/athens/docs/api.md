# Athens API

Athens provides path normalization, temporary file planning, cleanup manifests,
permission probes, atomic text writes, and structured errors.

The public API is exported by `Vitte::City::Athens` and split internally across
the `Path`, `Temp`, `Cleanup`, `Manifest`, `Permissions`, `Atomic`, and `Error`
submodules.
