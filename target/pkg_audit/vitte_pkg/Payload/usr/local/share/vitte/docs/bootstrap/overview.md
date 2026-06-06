# Bootstrap Overview

This documents the real rebuild path currently available in this repository.

Stages:
- stage0: seed shell compiler (`toolchain/seed/vittec0.seed`)
- stage1: compiler emitted by stage0
- stage2: compiler emitted by stage1

Primary checks:
- `vittec self-check --strict`
- `vittec self-host-check`
- `vittec doctor`
