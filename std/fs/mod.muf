# /Users/vincent/Documents/Github/vitte/std/fs/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/fs
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.fs as a standalone package (workspace-member compatible).
# - Expose path utilities + directory walking + basic file ops wrappers.
# - Keep deps minimal: std-core, std-runtime; std-sys for OS access.
# - Include unit tests.
# - Feature-gate heavy functionality (walk, glob, symlink following).
#
# Notes:
# - Align keys with your canonical Muffin EBNF if names differ.
# - std.fs typically depends on std.sys for platform calls.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-fs"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: filesystem facade (paths, walk, basic ops)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "fs", "filesystem", "path", "walk"]
.end

workspace
  kind = "member"
.end

target
  name = "std-fs"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.fs"
  .end
  entry
    module = "std.fs.path"
  .end
  entry
    module = "std.fs.walk"
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
    name = "std-sys"
    path = "../sys"
  .end

  dep
    name     = "std-collections"
    path     = "../collections"
    optional = true
  .end
.end

features
  feature
    name = "walk"
    description = "Enable directory walking utilities (std.fs.walk)."
    default = true
  .end

  feature
    name = "glob"
    description = "Enable glob matching helpers (optional)."
    default = false
  .end

  feature
    name = "follow_symlinks"
    description = "Allow walk/stat to follow symlinks (use with care)."
    default = false
  .end

  feature
    name = "permissions"
    description = "Expose chmod/chown-like wrappers when backend supports it."
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
    run  = "muffin test std-fs"
  .end

  script
    name = "check"
    run  = "muffin check std-fs"
  .end
.end
