# compiler-enterprise

Squelette “enterprise-grade” pour un compilateur (toolchain hermétique, reproducible builds,
supply-chain (SBOM+provenance), compat ABI/API/diag, fuzzing, bench, observabilité).

Notes:
- Le build system est volontairement abstrait (placeholders) : vous pouvez l’implémenter via Muffin/CMake/Ninja/Bazel.
- Les formats JSON (schemas) servent de contrats entre CI, outils, et consommateurs.
