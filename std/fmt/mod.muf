# /Users/vincent/Documents/Github/vitte/std/fmt/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/fmt
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.fmt as a standalone package (workspace-member compatible).
# - Offer formatting utilities: fmt primitives, builders, number formatting,
#   and (optional) printf-like surface.
# - Keep deps minimal: std-core, std-runtime, std-string.
# - Include unit tests.
# - Feature-gate heavier subsystems (float formatting, locale, allocations).
#
# Notes:
# - Align keys with your canonical Muffin EBNF if names differ.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-fmt"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: formatting utilities (strings, numbers, builders)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "fmt", "format", "printing", "string"]
.end

workspace
  kind = "member"
.end

target
  name = "std-fmt"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.fmt"
  .end
  entry
    module = "std.fmt.builder"
  .end
  entry
    module = "std.fmt.num"
  .end
  entry
    module = "std.fmt.hex"
  .end
  entry
    module = "std.fmt.debug"
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

  dep
    name     = "std-collections"
    path     = "../collections"
    optional = true
  .end
.end

features
  feature
    name = "alloc"
    description = "Enable allocation-heavy formatting (builders, dynamic buffers)."
    default = true
  .end

  feature
    name = "float"
    description = "Enable float formatting (ryu/grisu-like or host backend)."
    default = true
  .end

  feature
    name = "printf"
    description = "Enable printf-like format strings (optional surface API)."
    default = false
  .end

  feature
    name = "locale"
    description = "Enable locale-aware formatting (grouping, decimal separators)."
    default = false
  .end

  feature
    name = "debug"
    description = "Enable debug/pretty formatting helpers."
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
    run  = "muffin test std-fmt"
  .end

  script
    name = "check"
    run  = "muffin check std-fmt"
  .end
.end
