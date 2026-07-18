# Bootstrap Design

## Invariant

Vitte bootstrap execution starts only from `toolchain/seed/vittec0.seed`.
`toolchain/bootstrap-config.json` declares exactly one bootstrap item, named
`seed`, whose output is `bin/vittec0`. Missing or invalid seed state is fatal;
there is no compiler fallback.

## Dependency Graph

```text
toolchain/seed/src/main.vit (reviewed intent)
             |
             v
toolchain/seed/manifest.txt --pins--> toolchain/seed/vittec0.seed
                                           |
                                           v
                               scripts/seed/install_seed.sh
                                           |
                                           v
                                      bin/vittec0
                                           |
                    +----------------------+----------------------+
                    |                      |                      |
                    v                      v                      v
          native snapshots       compiler-entry gates       user fixtures
                    |                      |
                    +-----------+----------+
                                v
                    src/vitte/compiler/main.vit
```

## Real Execution

1. `seed-verify` checks the manifest hash, executable version, and readable
   source identity.
2. `bootstrap-seed` installs an exact copy at `bin/vittec0`.
3. `bootstrap-stage-chain-check --artifacts` verifies configuration and copy
   hashes.
4. `bootstrap-native-snapshots` exercises parser, diagnostics, IR, shell
   emission, type checks, and native user programs.
5. `bootstrap-vitte-hard-gate` composes those checks and writes the hard-gate
   report.

Every command checks return codes. Temporary outputs use isolated directories
and are removed on exit. CI runs the hard gate as a blocking job.

## Self-Hosting Boundary

The trust-root bootstrap and complete self-hosting are separate claims. The
seed currently emits an executable compiler payload from
`src/vitte/compiler/main.vit`; that payload can rebuild the same entry. The
self-host completion audit records generation hashes, executable state, shell
payload markers, sidecars, and embedded bridge markers. Completion requires:

- the entire generation chain succeeds;
- two successive generated compilers are byte-identical;
- no shell, sidecar, or embedded transition bridge remains.

Until all three are true, the project is seed-rooted but not officially fully
self-hosted.

## Portability

The seed is POSIX shell and requires the tools listed in
`bootstrap-config.json`. Native output availability depends on the selected
backend and host toolchain. Platform, object-format, archive, and linker claims
must be validated by their dedicated gates; the bootstrap contract alone does
not imply ELF, Mach-O, PE, or cross-target completeness.
