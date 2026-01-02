

# /Users/vincent/Documents/Github/vitte/std/core/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/core
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.core as the foundational package for the Vitte stdlib.
# - Define canonical core types and utilities: Option/Result, cmp/hash, prelude.
# - Keep deps minimal: std-runtime + std-string (optional) + std-ffi (optional).
# - Include unit tests.
# - Avoid platform dependencies; sys calls are handled by std-sys.
#
# Notes:
# - Align keys with your canonical Muffin EBNF if names differ.
# - std.core should be usable in stage0/stage1 compiler contexts.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-core"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: core primitives (Option/Result/prelude/cmp/hash)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "core", "result", "option", "prelude"]
.end

workspace
  kind = "member"
.end

target
  name = "std-core"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.core.prelude"
  .end
  entry
    module = "std.core.option"
  .end
  entry
    module = "std.core.result"
  .end
  entry
    module = "std.core.cmp"
  .end
  entry
    module = "std.core.hash"
  .end
.end

deps
  dep
    name = "std-runtime"
    path = "../runtime"
  .end

  dep
    name     = "std-string"
    path     = "../string"
    optional = true
  .end

  dep
    name     = "std-ffi"
    path     = "../ffi"
    optional = true
  .end
.end

features
  feature
    name = "prelude"
    description = "Export the std.core prelude module."
    default = true
  .end

  feature
    name = "error_strings"
    description = "Include human-readable error messages (requires std-string)."
    default = true
    requires = ["std-string"]
  .end

  feature
    name = "ffi"
    description = "Expose core FFI-facing types where needed (requires std-ffi)."
    default = false
    requires = ["std-ffi"]
  .end

  feature
    name = "panic_on_bug"
    description = "Panic on internal invariants instead of returning errors."
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
    run  = "muffin test std-core"
  .end

  script
    name = "check"
    run  = "muffin check std-core"
  .end
.end