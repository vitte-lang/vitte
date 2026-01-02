# /Users/vincent/Documents/Github/vitte/std/path/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/path
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.path as a standalone package (workspace-member compatible).
# - Expose a pure path manipulation API (no syscalls):
#     * Path / PathBuf style helpers
#     * join/split/normalize
#     * separators, roots, drive letters (platform-neutral surface)
#     * percent-encoding interop (optional)
# - Keep deps minimal: std-core, std-runtime, std-string, std-collections.
# - Include unit tests.
# - Feature-gate platform-specific semantics (windows/unix).
#
# Notes:
# - This package is PURE and should not depend on std-sys.
# - If you already have std.fs.path, std.path can be the underlying pure layer
#   that std.fs.path re-exports.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-path"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: pure path manipulation (join/split/normalize)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "path", "filesystem", "normalize", "join", "split"]
.end

workspace
  kind = "member"
.end

target
  name = "std-path"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.path"
  .end
  entry
    module = "std.path.segments"
  .end
  entry
    module = "std.path.normalize"
  .end
  entry
    module = "std.path.platform"
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
    name = "std-collections"
    path = "../collections"
  .end

  dep
    name     = "std-encoding"
    path     = "../encoding"
    optional = true
  .end
.end

features
  feature
    name = "normalize"
    description = "Enable normalization (dot-segment removal, separator cleanup)."
    default = true
  .end

  feature
    name = "platform_unix"
    description = "Unix path semantics helpers (/ root, no drive letters)."
    default = true
  .end

  feature
    name = "platform_windows"
    description = "Windows path semantics helpers (drives, UNC, backslashes)."
    default = false
  .end

  feature
    name = "encoding_glue"
    description = "Enable interop helpers with std-encoding (percent-encoding)."
    default = false
    requires = ["std-encoding"]
  .end

  feature
    name = "no_alloc"
    description = "Prefer fixed buffers; avoid allocations when possible."
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
    run  = "muffin test std-path"
  .end

  script
    name = "check"
    run  = "muffin check std-path"
  .end
.end
