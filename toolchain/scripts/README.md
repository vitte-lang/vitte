# Vitte Toolchain Scripts

This folder contains helper scripts used to build, test, package, and install Vitte.

Goal: keep local and CI workflows predictable.

## Principles

- non-interactive by default
- explicit inputs through env vars/arguments
- reproducible outputs under `build/` or `target/`

## Layout

- `build/` configure/build helpers
- `test/` test entry scripts
- `ci/` CI wrappers
- `package/` artifact/package builders
- `install/` install/uninstall scripts
- `dev/` local debug helpers
- `steel/` wrappers for Steel bakes
- `targets/` host/arch target definitions
- `utils/` shared shell helpers
- `bootstrap/` bootstrap/stage scripts

## Common Usage

From repo root:

```sh
# configure/build helpers
./toolchain/scripts/build/configure.sh
./toolchain/scripts/build/build-debug.sh
./toolchain/scripts/build/build-release.sh

# tests
./toolchain/scripts/test/run.sh

# CI wrappers (local dry-run style)
./toolchain/scripts/ci/lint.sh
./toolchain/scripts/ci/format.sh

# packaging
./toolchain/scripts/package/make-debian-deb.sh
./toolchain/scripts/package/make-macos-pkg.sh

# local install/uninstall
./toolchain/scripts/install/install-local.sh
./toolchain/scripts/install/uninstall.sh
```

## Steel Wrappers

Use these when driving builds through `steelconf`:

```sh
./toolchain/scripts/steel/steel-env.sh
./toolchain/scripts/steel/steel-build.sh
./toolchain/scripts/steel/steel-test.sh
./toolchain/scripts/steel/steel-run.sh
```

## Targets

Target scripts define platform defaults (compiler/linker/triple):

- `targets/linux-x86_64.sh`
- `targets/linux-aarch64.sh`
- `targets/macos-x86_64.sh`
- `targets/macos-arm64.sh`
- `targets/freebsd-x86_64.sh`
- `targets/windows-x86_64.ps1`

## Packaging Notes

Most package outputs are written under `target/packages/`.

Completions are generated/checked by tools in `tools/`:

- `tools/generate_completions.py`
- `tools/completions/spec.json`
- `tools/completions_snapshots.sh`

## Troubleshooting

- run scripts from repo root unless script docs say otherwise
- verify executable bits if a script fails with permission errors
- check `SUPPORT.md` for report format when opening issues
