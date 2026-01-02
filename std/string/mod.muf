# /Users/vincent/Documents/Github/vitte/std/string/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/string
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide foundational string/UTF-8 utilities.
# - Keep the package usable in stage0: allow disabling heavy helpers.
# - Export a clean module surface:
#     std.string
#     std.string.ascii
#     std.string.replace
#     std.string.split
#     std.string.utf8
# - Keep deps minimal: std-core, std-runtime, std-mem, std-collections optional.
# - Include comprehensive tests.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-string"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: strings, UTF-8, ASCII helpers, split/replace."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "string", "utf8", "ascii", "split", "replace"]
.end

workspace
  kind = "member"
.end

target
  name = "std-string"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  entry
    module = "std.string"
  .end

  entry
    module = "std.string.ascii"
  .end

  entry
    module = "std.string.replace"
  .end

  entry
    module = "std.string.split"
  .end

  entry
    module = "std.string.utf8"
  .end
.end

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

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
    name     = "std-mem"
    path     = "../mem"
    optional = true
  .end

  dep
    name     = "std-collections"
    path     = "../collections"
    optional = true
  .end
.end

# -----------------------------------------------------------------------------
# Features
# -----------------------------------------------------------------------------

features
  feature
    name = "builder"
    description = "Enable string builder API (Builder + push_str/push_byte)."
    default = true
  .end

  feature
    name = "ascii"
    description = "Enable std.string.ascii module."
    default = true
  .end

  feature
    name = "replace"
    description = "Enable std.string.replace module."
    default = true
  .end

  feature
    name = "split"
    description = "Enable std.string.split module."
    default = true
  .end

  feature
    name = "utf8"
    description = "Enable std.string.utf8 module."
    default = true
  .end

  feature
    name = "collections"
    description = "Enable helpers that return Vec or depend on std-collections."
    default = false
    requires = ["std-collections"]
  .end

  feature
    name = "alloc"
    description = "Enable allocation-backed implementations (requires std-mem/runtime alloc)."
    default = true
    requires = ["std-mem"]
  .end

  feature
    name = "no_alloc"
    description = "Prefer caller-provided buffers; minimize allocations."
    default = false
  .end

  feature
    name = "test_vectors"
    description = "Enable extended test vectors."
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
    run  = "muffin test std-string"
  .end

  script
    name = "check"
    run  = "muffin check std-string"
  .end

  script
    name = "lint"
    run  = "sh ./scripts/lint_std.sh --ws --shell"
  .end
.end
