# /Users/vincent/Documents/Github/vitte/std/mem/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/mem
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.mem as a standalone package (workspace-member compatible).
# - Low-level memory façade: alignment, ptr/slice/buffer views, memcpy/memmove,
#   memset/memcmp/memeq, (secure) zeroing, allocation wrappers.
# - Keep deps minimal: std-core + std-runtime; std-string optional.
# - Include unit tests.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-mem"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: memory utilities (ptr/slice/buffer, memcpy/memset, alloc wrappers)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "mem", "memory", "alloc", "buffer", "alignment", "ptr", "slice"]
.end

workspace
  kind = "member"
.end

target
  name = "std-mem"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.mem"
  .end
  entry
    module = "std.mem.ptr"
  .end
  entry
    module = "std.mem.slice"
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
    name     = "std-string"
    path     = "../string"
    optional = true
  .end
.end

features
  feature
    name = "ptr"
    description = "Enable pointer façade (std.mem.ptr)."
    default = true
  .end

  feature
    name = "slice"
    description = "Enable slice façade (std.mem.slice)."
    default = true
  .end

  feature
    name = "buffers"
    description = "Enable Buffer view helpers (ptr+len, slicing, compare)."
    default = true
  .end

  feature
    name = "secure_zero"
    description = "Enable secure zeroing (backend must not elide)."
    default = true
  .end

  feature
    name = "alloc"
    description = "Enable allocation wrappers (malloc/free via runtime)."
    default = true
  .end

  feature
    name = "strict"
    description = "CI mode: tighter asserts and invariants."
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
    run  = "muffin test std-mem"
  .end

  script
    name = "check"
    run  = "muffin check std-mem"
  .end
.end
