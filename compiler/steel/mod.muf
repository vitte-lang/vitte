[package]
name = "rustc-main"
version = "0.0.0"
edition = "2024"

[dependencies]
# tidy-alphabetical-start

# Make sure rustc_codegen_ssa ends up in the sysroot, because this
# crate is intended to be used by codegen backends, which may not be in-tree.
rustc_codegen_ssa = { path = "../rustc_codegen_ssa" }

rustc_driver = { path = "../rustc_driver" }
rustc_driver_impl = { path = "../rustc_driver_impl" }

rustc_public = { path = "../rustc_public" }

# Make sure rustc_public_bridge ends up in the sysroot, because this
# crate is intended to be used by stable MIR consumers, which are not in-tree.
rustc_public_bridge = { path = "../rustc_public_bridge" }
# tidy-alphabetical-end

# Pin these to avoid pulling in a package with a binary blob
# <https://github.com/rust-lang/rust/pull/136395#issuecomment-2692769062>
[target.'cfg(target_os = "wasi")'.dependencies]
getrandom = "=0.3.3"
wasi = "=0.14.2"


[dependencies.tikv-jemalloc-sys]
version = "0.6.1"
optional = true
features = ['override_allocator_on_supported_platforms']

[features]
# tidy-alphabetical-start
check_only = ['rustc_driver_impl/check_only']
jemalloc = ['dep:tikv-jemalloc-sys']
llvm = ['rustc_driver_impl/llvm']
llvm_enzyme = ['rustc_driver_impl/llvm_enzyme']
llvm_offload = ['rustc_driver_impl/llvm_offload']
max_level_info = ['rustc_driver_impl/max_level_info']
rustc_randomized_layouts = ['rustc_driver_impl/rustc_randomized_layouts']
# tidy-alphabetical-end

[build-dependencies]
# tidy-alphabetical-start
rustc_windows_rc = { path = "../rustc_windows_rc" }
# tidy-alphabetical-end

# mod.muf (Muffin)
#
# Port of the original Cargo.toml for the `rustc-main` crate.
# Notes:
# - `edition = 2024` is preserved as metadata in `toolchain`.
# - Rust feature wiring is preserved in `features` as string paths.
# - The WASI pinning is expressed as a conditional `on` block.

muf 1

package
  name "rustc-main"
  version "0.0.0"
.end

toolchain
  # Rust metadata carried over (if you keep a Rust interop layer).
  rust_edition 2024
.end

# Main binary/tool entrypoint (Vitte port)
# `compiler/steel/src/main.vit` => module `steel.main`
# If your module path differs, adjust `entry` accordingly.
target
  kind tool
  name "rustc-main"
  entry steel.main
.end

# Dependencies (path deps preserved)
deps
  dep rustc_codegen_ssa
    path "../rustc_codegen_ssa"
  .end

  dep rustc_driver
    path "../rustc_driver"
  .end

  dep rustc_driver_impl
    path "../rustc_driver_impl"
  .end

  dep rustc_public
    path "../rustc_public"
  .end

  dep rustc_public_bridge
    path "../rustc_public_bridge"
  .end

  dep tikv-jemalloc-sys
    version "0.6.1"
    optional true
    features ["override_allocator_on_supported_platforms"]
  .end
.end

# Conditional deps (Cargo: target.'cfg(target_os = "wasi")'.dependencies)
on
  when cfg target_os == "wasi"
    deps
      dep getrandom
        version "=0.3.3"
      .end

      dep wasi
        version "=0.14.2"
      .end
    .end
  .end
.end

# Build dependencies (Cargo: [build-dependencies])
deps
  build_dep rustc_windows_rc
    path "../rustc_windows_rc"
  .end
.end

# Features (Cargo: [features])
features
  feature check_only
    enables ["rustc_driver_impl/check_only"]
  .end

  feature jemalloc
    enables ["dep:tikv-jemalloc-sys"]
  .end

  feature llvm
    enables ["rustc_driver_impl/llvm"]
  .end

  feature llvm_enzyme
    enables ["rustc_driver_impl/llvm_enzyme"]
  .end

  feature llvm_offload
    enables ["rustc_driver_impl/llvm_offload"]
  .end

  feature max_level_info
    enables ["rustc_driver_impl/max_level_info"]
  .end

  feature rustc_randomized_layouts
    enables ["rustc_driver_impl/rustc_randomized_layouts"]
  .end
.end