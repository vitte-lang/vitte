

# /Users/vincent/Documents/Github/vitte/std/math/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/math
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.math as a standalone package (workspace member compatible).
# - Keep deps minimal (core/runtime/string; collections optional).
# - Export math modules (constants, basic ops, integer utils, float utils).
# - Include unit tests.
#
# Notes:
# - Align keys with your canonical Muffin EBNF if names differ.
# - This manifest avoids any build-system-specific files beyond Muffin.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-math"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: math utilities (ints, floats, trig, constants)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "math", "numeric", "float", "integer"]
.end

workspace
  kind = "member"
.end

target
  name = "std-math"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules (re-exported)
  entry
    module = "std.math"
  .end
  entry
    module = "std.math.const"
  .end
  entry
    module = "std.math.int"
  .end
  entry
    module = "std.math.float"
  .end
  entry
    module = "std.math.trig"
  .end
  entry
    module = "std.math.rand"
  .end
.end

deps
  dep
    name = "std-core"
    path = "../core"
  .end

  dep
    name = "std-runtime"
    path = "../runtime"
  .end

  dep
    name = "std-string"
    path = "../string"
  .end

  # Optional: if std.math exposes numeric containers / vectors
  dep
    name     = "std-collections"
    path     = "../collections"
    optional = true
  .end
.end

features
  feature
    name = "fast_math"
    description = "Enable fast-math approximations (may reduce precision)."
    default = false
  .end

  feature
    name = "libm"
    description = "Bind to host libm when available (sin/cos/exp/log)."
    default = true
  .end

  feature
    name = "pure"
    description = "Pure-Vitte fallbacks (no host libm)."
    default = false
    conflicts = ["libm"]
  .end

  feature
    name = "deterministic_rand"
    description = "Deterministic RNG for tests and reproducible builds."
    default = true
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
    run  = "muffin test std-math"
  .end

  script
    name = "lint"
    run  = "muffin check std-math"
  .end
.end