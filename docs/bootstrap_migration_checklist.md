# Bootstrap Migration Checklist

The numbered-stage bootstrap has been retired. This checklist records the
remaining path from a seed-rooted bootstrap to autonomous self-hosting.

## Overview

Bootstrap trust, compiler generation, and official self-host completion are
separate contracts. The seed path is complete; autonomous compiler generation
is still in transition.

## Responsibilities

- Keep the seed manifest and executable synchronized.
- Reject fallback compilers and retired bootstrap paths.
- Report self-hosting gaps instead of treating transition output as complete.

## Invariants

- Only the manifest-pinned seed may initialize bootstrap execution.
- All bootstrap gates are blocking and deterministic.
- Full self-hosting requires parity and removal of every transition bridge.

## Data Flow

`vittec0.seed` installs `bin/vittec0`, which validates and builds the canonical
compiler entry. Generated compilers rebuild that same entry only inside the
self-host completion audit.

## Completed

- [x] Pin `toolchain/seed/vittec0.seed` with a manifest and reviewed source.
- [x] Declare a single seed item in `toolchain/bootstrap-config.json`.
- [x] Remove numbered stage sources and bootstrap scripts.
- [x] Remove active build, test, CI, release, install, and package references to
  numbered stages.
- [x] Make missing or invalid seed state fatal with no compiler fallback.
- [x] Run bootstrap CI gates as blocking jobs.
- [x] Verify deterministic seed installation, IR, and native fixture emission.
- [x] Keep the repository host-source audit free of bootstrap exceptions.

## Remaining

- [ ] Remove the transition bridge from generated compiler payloads.
- [ ] Obtain byte-identical successive compiler generations.
- [ ] Compile the full compiler, runtime, and standard library with the
  generated compiler without delegating to the seed or a host compiler.
- [ ] Validate every supported object, archive, executable, and cross-target
  format through dedicated platform gates.
- [ ] Make `python3 tools/selfhost_completion_audit.py --strict-complete` pass.

## Required Evidence

```sh
make seed-verify
make bootstrap-source-of-truth
make bootstrap-vitte-hard-gate
make bootstrap-verify
toolchain/test_bootstrap_reproducibility.sh
python3 tools/selfhost_completion_audit.py --strict-complete
```

The first four commands prove the current seed-rooted bootstrap contract. Only
the strict self-host completion audit can close the final migration item.
