# Bootstrap Stages

- Stage0 role: trusted seed entrypoint and bootstrap contract.
- Stage1 role: first self-produced compiler artifact.
- Stage2 role: recompiled compiler for consistency checks.

System dependencies:
- POSIX shell
- `cc` toolchain
- linker (`ld`)
- optional `clang` for strict profile

## Exemple de bootstrap
```sh
make stage0
make stage1
make stage2
```
- `make stage0` construit le compilateur seed initial (`vittec0.seed`).
- `make stage1` produit le premier compilateur auto-hébergé et vérifie qu'il peut reconstruire le projet.
- `make stage2` reconstruit une seconde fois pour confirmer la cohérence des artefacts.
