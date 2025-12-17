# muffin.muf — Vitte workspace manifest
#
# Format: Muffin (TOML-like)
# Purpose:
#   - describe workspace packages (bench/toolchain/fuzz)
#   - define toolchains, targets, profiles, and common flags
#
# SPDX-License-Identifier: MIT

muffin = "1"

[workspace]
name = "vitte"
version = "0.1.0"
description = "Vitte C17 tooling: toolchain + bench + fuzz"
license = "MIT"
repository = "https://github.com/vitte-lang/vitte"

# Optional: keep these aligned with your repo conventions.
[paths]
root = "."
build = "build"
dist = "dist"
cache = ".cache"

# Workspace members (relative directories).
[members]
packages = [
  "bench",
  "toolchain",
  "fuzz",
]

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------

# Canonical triples used across CI and local builds.
# Extend freely (BSD/RISC-V/etc.) when your toolchain layer supports it.
[targets]
default = "native"
list = [
  "native",
  "x86_64-apple-darwin",
  "aarch64-apple-darwin",
  "x86_64-unknown-linux-gnu",
  "aarch64-unknown-linux-gnu",
  "x86_64-pc-windows-msvc",
  "aarch64-pc-windows-msvc",
]

# -----------------------------------------------------------------------------
# Toolchains
# -----------------------------------------------------------------------------

# Toolchain selection priority:
#   1) --toolchain <name>
#   2) MUF_TOOLCHAIN env var
#   3) platform default
[toolchains]
default_unix = "clang"
default_windows = "msvc"

[toolchain.clang]
cc = "clang"
cxx = "clang++"
ar = "llvm-ar"
ranlib = "llvm-ranlib"
ld = "lld"
strip = "llvm-strip"

# Common flags (can be extended per package/profile/target)
[toolchain.clang.flags]
c = ["-std=c17"]
warn = ["-Wall", "-Wextra", "-Wpedantic"]

[toolchain.gcc]
cc = "gcc"
cxx = "g++"
ar = "ar"
ranlib = "ranlib"
ld = "ld"
strip = "strip"

[toolchain.gcc.flags]
c = ["-std=c17"]
warn = ["-Wall", "-Wextra", "-Wpedantic"]

[toolchain.msvc]
cc = "cl"
link = "link"
lib = "lib"

[toolchain.msvc.flags]
c = ["/std:c17"]
warn = ["/W4"]

# -----------------------------------------------------------------------------
# Profiles
# -----------------------------------------------------------------------------

# Profile merge order (low → high priority):
#   toolchain.flags -> profile.flags -> package.flags -> target.flags

[profiles.debug]
opt = 0
debug = true
lto = false
sanitize = "none"

[profiles.debug.flags]
c = ["-O0", "-g"]

[profiles.release]
opt = 3
debug = false
lto = true
sanitize = "none"

[profiles.release.flags]
c = ["-O3"]

# Sanitizers (best-effort; availability depends on compiler/OS)
[profiles.asan]
opt = 1
debug = true
lto = false
sanitize = "address"

[profiles.asan.flags]
# clang/gcc
c = ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=address"]
ld = ["-fsanitize=address"]

[profiles.ubsan]
opt = 1
debug = true
lto = false
sanitize = "undefined"

[profiles.ubsan.flags]
c = ["-O1", "-g", "-fno-omit-frame-pointer", "-fsanitize=undefined"]
ld = ["-fsanitize=undefined"]

[profiles.coverage]
opt = 0
debug = true
lto = false
sanitize = "none"

[profiles.coverage.flags]
# clang/gcc coverage (adjust for llvm-cov vs gcov workflows)
c = ["-O0", "-g", "--coverage"]
ld = ["--coverage"]

# -----------------------------------------------------------------------------
# Packages
# -----------------------------------------------------------------------------

# Notes:
# - `kind` is informational for Muffin; actual build graph is driven by src/include.
# - `defines` are preprocessor definitions applied to C compilation.
# - `public_includes` are exported to dependents.

[package.bench]
kind = "c"

# bench module root
root = "bench"

# compilation units
src = [
  "bench/src/bench",
]

# exported includes (for other packages)
public_includes = [
  "bench/src",
]

# local-only includes
private_includes = [
  "bench/src",
]

defines = [
  # Example: keep empty by default.
]

# Suggested output binary for the demo harness.
[[package.bench.bin]]
name = "bench"
entry = "bench/src/bench/sample.c"

# Optional: build as library too (for embedding).
[[package.bench.lib]]
name = "bench"

[package.bench.lib.sources]
# For a static library build you usually include everything except the demo main.
# Keep the list explicit for deterministic builds.
files = [
  "bench/src/bench/registry.c",
  "bench/src/bench/runner.c",
  "bench/src/bench/output.c",
  "bench/src/bench/options.c",
  "bench/src/bench/strutil.c",
  "bench/src/bench/timing.c",
]

[package.toolchain]
kind = "c"
root = "toolchain"

src = [
  "toolchain/src",
  "toolchain/config/src",
]

public_includes = [
  "toolchain/include",
  "toolchain/config/include",
]

private_includes = [
  "toolchain/src",
  "toolchain/config/src",
]

[[package.toolchain.lib]]
name = "vitte_toolchain"

[package.toolchain.lib.sources]
# Keep this list aligned with your actual files.
# If you generate headers/sources, add them here.
files = [
  "toolchain/config/src/clang_archive.c",
  "toolchain/config/src/clang_compile.c",
  "toolchain/config/src/clang_env.c",
  "toolchain/config/src/clang_error.c",
  "toolchain/config/src/clean_error.c",
  "toolchain/config/src/clang_flags.c",
  "toolchain/config/src/clang_link.c",
]

[package.fuzz]
kind = "c"
root = "fuzz"

src = [
  "fuzz/src",
]

public_includes = [
  "fuzz/include",
]

private_includes = [
  "fuzz/src",
]

defines = [
  # Example hooks for fuzz builds.
  # "VITTE_FUZZ=1",
]

# Fuzz drivers are often multiple binaries; declare stubs here.
# Adjust entries to match your repo.
[[package.fuzz.bin]]
name = "vitte_fuzz_lexer"
entry = "fuzz/src/fuzz_lexer_main.c"

[[package.fuzz.bin]]
name = "vitte_fuzz_ast"
entry = "fuzz/src/fuzz_ast_main.c"

# -----------------------------------------------------------------------------
# Scripts (optional convenience layer)
# -----------------------------------------------------------------------------

[scripts]
# Convention: scripts are executed from repo root.
# Keep them stable for CI.

# Build everything (default toolchain/profile).
build = "tools/scripts/build_all.sh"

# Build once (single target).
build_once = "tools/scripts/build_once.sh"

# Run bench demo.
bench = "./build/bin/bench --list"

# Run fuzz (CI entry).
fuzz = "tools/scripts/fuzz.sh"

# Clean.
clean = "tools/scripts/clean.sh"

# Doctor / environment diagnostics.
doctor = "tools/scripts/doctor.sh"

# -----------------------------------------------------------------------------
# Exported constants (optional)
# -----------------------------------------------------------------------------

[constants]
VITTE_NAME = "vitte"
VITTE_VERSION = "0.1.0"
VITTE_LICENSE = "MIT"
