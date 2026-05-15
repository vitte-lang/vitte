# Bootstrap Vitte File Mapping

Source reference: `reference-bootstrap-vitte`

## Top-level

- `src/bootstrap/src/lib.rs` -> `toolchain/src/bootstrap_vitte/mod.vit`
- `src/bootstrap/src/bin/main.rs` -> `toolchain/src/bootstrap_vitte/bin/main.vit`
- `src/bootstrap/src/bin/compiler_frontend.rs` -> `toolchain/src/bootstrap_vitte/bin/compiler_frontend.vit`
- `src/bootstrap/src/bin/docgen.rs` -> `toolchain/src/bootstrap_vitte/bin/docgen.vit`
- `src/bootstrap/src/bin/cache-plus-cl.rs` -> `toolchain/src/bootstrap_vitte/bin/cache_bridge.vit`

## core/

- `core/builder/mod.rs` -> `core/builder/mod.vit`
- `core/builder/cargo.rs` -> `core/builder/cargo.vit`
- `core/builder/tests.rs` -> `core/builder/tests.vit`

- `core/config/mod.rs` -> `core/config/mod.vit`
- `core/config/flags.rs` -> `core/config/flags.vit`
- `core/config/manifest/mod.rs` -> `core/config/manifest/mod.vit`
- `core/config/manifest/compiler_profile.rs` -> `core/config/manifest/compiler_profile.vit`

- `core/build_steps/mod.rs` -> `core/build_steps/mod.vit`
- `core/build_steps/compile.rs` -> `core/build_steps/compile.vit`
- `core/build_steps/test.rs` -> `core/build_steps/test.vit`
- `core/build_steps/doc.rs` -> `core/build_steps/doc.vit`
- `core/build_steps/dist.rs` -> `core/build_steps/dist.vit`
- `core/build_steps/clean.rs` -> `core/build_steps/clean.vit`
- `core/build_steps/install.rs` -> `core/build_steps/install.vit`
- `core/build_steps/check.rs` -> `core/build_steps/check.vit`
- `core/build_steps/clippy.rs` -> `core/build_steps/clippy.vit`
- `core/build_steps/format.rs` -> `core/build_steps/format.vit`
- `core/build_steps/llvm.rs` -> `core/build_steps/llvm.vit`
- `core/build_steps/run.rs` -> `core/build_steps/run.vit`
- `core/build_steps/setup.rs` -> `core/build_steps/setup.vit`
- `core/build_steps/suggest.rs` -> `core/build_steps/suggest.vit`
- `core/build_steps/tool.rs` -> `core/build_steps/tool.vit`
- `core/build_steps/vendor.rs` -> `core/build_steps/vendor.vit`

- `core/download.rs` -> `core/download/mod.vit`
- `core/metadata.rs` -> `core/metadata/mod.vit`
- `core/sanity.rs` -> `core/sanity/mod.vit`

## utils/

- `utils/mod.rs` -> `utils/mod.vit`
- `utils/build_stamp.rs` -> `utils/build_stamp.vit`
- `utils/cache.rs` -> `utils/cache.vit`
- `utils/channel.rs` -> `utils/channel.vit`
- `utils/change_tracker.rs` -> `utils/change_tracker.vit`
- `utils/exec.rs` -> `utils/exec.vit`
- `utils/helpers.rs` -> `utils/helpers.vit`
- `utils/job.rs` -> `utils/job.vit`
- `utils/metrics.rs` -> `utils/metrics.vit`
- `utils/render_tests.rs` -> `utils/render_tests.vit`
- `utils/tarball.rs` -> `utils/tarball.vit`

## Status

- `mapped`: path exists
- `stub`: created placeholder API, implementation pending
- `implemented`: non-trivial logic ported
