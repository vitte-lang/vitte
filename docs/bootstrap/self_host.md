# Self-Hosting Status

The compiler is not yet fully self-hosted. Stage0 C under `src/bootstrap/`
still supplies the executable shell and host toolchain integration, while
ordinary `compile`, `run`, and `--emit-c` inputs use the linked Vitte frontend
under `src/compiler/`. See the active [compiler pipeline](../compiler/pipeline.md).

The former target `src/vitte/compiler/main.vit` is not present in the current
repository. The native parser and semantic surface are partial; frontend
failures still fall back to bootstrap analysis, HIR/IR do not drive the
ordinary route, and object/link/run still cross the C host boundary.

Previous documentation named a generation audit at `tools/selfhost_completion_audit.py`, but that script is not present in this checkout. The current source map is in [the compiler reference](../MODULE_REFERENCE.md); migration status is in [the native frontend port notes](frontend_native_port.md).
