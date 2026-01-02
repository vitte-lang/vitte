# /Users/vincent/Documents/Github/vitte/std/runtime/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/runtime
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.runtime as the foundational runtime facade for Vitte.
# - Expose ABI-stable primitives used by stdlib and generated code:
#     * allocation + memory intrinsics
#     * panic/assert
#     * basic time + rng hooks
#     * thin OS/syscall bridge (optional, via std-sys)
# - Keep deps minimal: std-core mandatory; std-string optional for messages.
# - Provide tests for allocation/intrinsics where backend exists.
# - Feature-gate heavy/OS-dependent capabilities.
#
# Notes:
# - std.runtime is typically implemented by the host backend (C/VM).
# - This package should compile even in stage0 with many features disabled.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-runtime"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: runtime facade (alloc/mem/panic/assert/intrinsics)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "runtime", "alloc", "intrinsics", "panic", "assert"]
.end

workspace
  kind = "member"
.end

target
  name = "std-runtime"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.runtime"
  .end
  entry
    module = "std.runtime.panic"
  .end
  entry
    module = "std.runtime.assert"
  .end
  entry
    module = "std.runtime.intrinsics"
  .end
.end

deps
  dep
    name = "std-core"
    path = "../core"
  .end

  dep
    name     = "std-string"
    path     = "../string"
    optional = true
  .end

  dep
    name     = "std-sys"
    path     = "../sys"
    optional = true
  .end

  dep
    name     = "std-mem"
    path     = "../mem"
    optional = true
  .end
.end

features
  feature
    name = "panic"
    description = "Enable panic surface (std.runtime.panic)."
    default = true
  .end

  feature
    name = "assert"
    description = "Enable assertions (std.runtime.assert)."
    default = true
  .end

  feature
    name = "intrinsics"
    description = "Enable intrinsics facade (memory/alloc/pointer IO/time/rng)."
    default = true
  .end

  feature
    name = "alloc"
    description = "Expose allocator hooks (malloc/free/realloc)."
    default = true
  .end

  feature
    name = "mem"
    description = "Expose memory intrinsics (memcpy/memmove/memset/memcmp/memswap/memzero_secure)."
    default = true
  .end

  feature
    name = "ptr_io"
    description = "Expose pointer load/store and byte IO intrinsics."
    default = true
  .end

  feature
    name = "time"
    description = "Expose time hooks (monotonic/system clocks, sleep)."
    default = true
  .end

  feature
    name = "rng"
    description = "Expose RNG hooks (fill_bytes)."
    default = false
  .end

  feature
    name = "sys_backend"
    description = "Use OS-backed implementations via std-sys."
    default = true
    requires = ["std-sys"]
  .end

  feature
    name = "stage0_min"
    description = "Minimal stage0 runtime surface (disable sys/time/rng)."
    default = false
  .end

  feature
    name = "strict"
    description = "CI mode: warnings as errors / stronger checks."
    default = false
  .end
.end

profile
  opt_level = 2
  debug = true
  warnings_as_errors = false
.end

scripts
  script
    name = "test"
    run  = "muffin test std-runtime"
  .end

  script
    name = "check"
    run  = "muffin check std-runtime"
  .end
.end
