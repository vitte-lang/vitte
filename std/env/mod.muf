

# /Users/vincent/Documents/Github/vitte/std/env/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/env
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.env as a standalone package (workspace-member compatible).
# - Offer environment + args + process context helpers (pure facade).
# - Prefer runtime/sys backends; keep pure fallbacks for tests.
# - Keep deps minimal: std-core, std-runtime; std-sys optional.
# - Include unit tests.
#
# Notes:
# - Align keys with your canonical Muffin EBNF if names differ.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-env"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: environment + args helpers (process context facade)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "env", "args", "process", "context"]
.end

workspace
  kind = "member"
.end

target
  name = "std-env"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.env"
  .end
  entry
    module = "std.env.args"
  .end
  entry
    module = "std.env.vars"
  .end
  entry
    module = "std.env.home"
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
    name     = "std-sys"
    path     = "../sys"
    optional = true
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
    name = "sys"
    description = "Use OS sys backend when available (std-sys)."
    default = true
    requires = ["std-sys"]
  .end

  feature
    name = "runtime"
    description = "Use runtime-provided env/args facades (std-runtime)."
    default = true
  .end

  feature
    name = "home"
    description = "Enable home directory resolution helpers."
    default = true
  .end

  feature
    name = "tests_fake_backend"
    description = "Enable deterministic fake env/args backend for tests."
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
    run  = "muffin test std-env"
  .end

  script
    name = "check"
    run  = "muffin check std-env"
  .end
.end