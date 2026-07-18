# Native C Runtime

`runtime_c` is the native host boundary used by generated Vitte programs and
compiler tools. It is an approved native component, not a compiler frontend or
an alternative implementation of the standard library.

## Architecture

The boundary has four layers:

1. `vitte_runtime.h` defines the versioned C ABI, value layouts, and exported
   functions.
2. `vitte_runtime.c` implements allocation helpers, panic boundaries, host I/O,
   process execution, and native object/link integration.
3. `src/vitte/stdlib/io/host_runtime.vitl` declares private `vitte_host_*`
   intrinsics and exposes public `host_*` wrappers.
4. Higher-level stdlib modules own user-facing file, path, process, and
   collection semantics.

The runtime must not absorb policy that belongs to higher-level stdlib modules.

## ABI Contract

`toolchain/scripts/interop/vitte_c_abi_v1.json` is the canonical ABI manifest.
The public header exposes `VITTE_C_ABI_VERSION_*` and
`VITTE_C_ABI_VERSION`. ABI v1 permits additive exports; changing an existing
layout, signature, ownership rule, or symbol meaning requires a new ABI major
version.

`VitteString`, `VitteSliceI32`, and `VitteSliceString` use pointer-plus-length
layouts defended by compile-time offset and size assertions. The runtime ABI
contract gate compares the manifest, header, C definitions, Vitte intrinsics,
and public wrappers.

## Ownership

Functions returning newly allocated strings or slices transfer ownership to the
caller. Use `vitte_string_release`, `vitte_slice_i32_release`, or
`vitte_owned_slice_string_release` as appropriate. `cli_args` returns a newly
allocated slice whose string elements are borrowed; release only its array with
`vitte_slice_string_release`.

`vitte_host_list_directory` returns an owned slice with owned string elements.
Its result is sorted lexically for deterministic builds.

## Filesystem Guarantees

- Invalid pointer/length pairs, oversized values, and embedded NUL paths fail
  before entering host APIs.
- Failed and self-referential copies do not truncate an existing destination.
- Successful copies publish atomically and preserve source permission bits.
- File deletion uses `unlink`; directory deletion uses `rmdir`.

## Platform Scope

The current implementation is POSIX-native and uses `dirent`, `fork`, `exec`,
`waitpid`, and ELF object inspection. Linux and macOS are active host profiles.
Windows requires a separate platform implementation behind the same versioned
ABI; POSIX emulation must not be reported as native Windows support.

## Verification

Run:

```sh
make runtime-abi-contract
make runtime-stdlib-real
GROUP=runtime-stdlib make all-tests-group
```

The real-runtime probe compiles `vitte_runtime.c` with warnings as errors and
executes ABI, ownership, panic, filesystem, and deterministic-listing checks.
