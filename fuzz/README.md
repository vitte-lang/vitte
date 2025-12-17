## Vitte Fuzz Harnesses

This directory keeps the shared fuzz infra (`include/`, `src/`, scripts) plus
individual harnesses under `targets/`.

### Building harness binaries

Use `fuzz/scripts/build_targets.sh` to compile every target that has moved beyond
placeholder status (i.e. the file defines `FUZZ_DRIVER_TARGET`). It emits a
standalone binary under `fuzz/out/<target>` by compiling the target TU directly
with `FUZZ_DRIVER_STANDALONE_MAIN=1` plus the `FUZZ_DISABLE_SANITIZER_TRACE`
stub, which guarantees the `fuzz_util.h` include stack is validated without
needing helper translation units or sanitizer runtimes.

```sh
./fuzz/scripts/build_targets.sh            # build everything ready
./fuzz/scripts/build_targets.sh --only fuze_vm_decode
CC=clang-17 FUZZ_CFLAGS="-O1 -g" ./fuzz/scripts/build_targets.sh
```

Targets that are still placeholders (files with comments/TODOs only) are skipped,
so once you flesh out a harness you automatically start building the binary and
get compile coverage on its dependency tree.
