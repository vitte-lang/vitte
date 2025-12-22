## Vitte Fuzz Harnesses

This directory keeps the shared fuzz infra (`include/`, `src/`, scripts) plus
individual harnesses under `targets/`.

### Building harness binaries

Use `fuzz/scripts/build_targets.sh` to compile every target that has moved beyond
placeholder status (i.e. the file defines `FUZZ_DRIVER_TARGET`). It emits a
standalone binary under `fuzz/out/<target>` by compiling the target TU directly
with `FUZZ_DRIVER_STANDALONE_MAIN=1` plus the `FUZZ_DISABLE_SANITIZER_TRACE`
stub. For asm fastpaths, it links against the CMake-built `vitte_asm_runtime`
target (no relinking globs of asm sources).

Environment variables (optional):

- `VITTE_FUZZ_CMAKE_BUILD_DIR`: where to configure/build the CMake tree (default: `./build-fuzz`)
- `VITTE_FUZZ_CMAKE_GENERATOR`: override generator (e.g. `Ninja`, `Ninja Multi-Config`)
- `VITTE_FUZZ_CMAKE_CONFIG`: multi-config configuration name (e.g. `Debug`)

```sh
./fuzz/scripts/build_targets.sh            # build everything ready
./fuzz/scripts/build_targets.sh --only fuze_vm_decode
CC=clang-17 FUZZ_CFLAGS="-O1 -g" ./fuzz/scripts/build_targets.sh
```

On Windows, use the PowerShell equivalent:

```powershell
$env:VITTE_FUZZ_CMAKE_GENERATOR="Ninja Multi-Config"
$env:VITTE_FUZZ_CMAKE_CONFIG="Debug"
powershell -ExecutionPolicy Bypass -File fuzz/scripts/build_targets.ps1
powershell -ExecutionPolicy Bypass -File fuzz/scripts/build_targets.ps1 -Only fuze_vm_decode
```

(cmd.exe: `set VITTE_FUZZ_CMAKE_GENERATOR=Ninja Multi-Config` + `set VITTE_FUZZ_CMAKE_CONFIG=Debug`)

Targets that are still placeholders (files with comments/TODOs only) are skipped,
so once you flesh out a harness you automatically start building the binary and
get compile coverage on its dependency tree.
