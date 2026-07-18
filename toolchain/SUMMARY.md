# Toolchain Summary

Vitte uses a manifest-pinned POSIX seed as its sole bootstrap trust root.

| Concern | Owner |
| --- | --- |
| Executable trust root | `toolchain/seed/vittec0.seed` |
| Reviewed seed identity | `toolchain/seed/src/main.vit` |
| Hash/version pin | `toolchain/seed/manifest.txt` |
| Machine configuration | `toolchain/bootstrap-config.json` |
| Installed command | `bin/vittec0` |
| Compiler source entry | `src/vitte/compiler/main.vit` |
| Hard gate | `tools/bootstrap_vitte_hard_gate.sh` |
| Self-host completion | `tools/selfhost_completion_audit.py` |

The bootstrap contract is deterministic and CI-blocking. The generated compiler
chain executes, but official autonomous self-hosting remains open while compiler
generations differ and a transition bridge is embedded.

Primary validation:

```sh
make bootstrap-source-of-truth
make bootstrap-vitte-hard-gate
toolchain/test_bootstrap_reproducibility.sh
python3 tools/selfhost_completion_audit.py --strict-complete
```
