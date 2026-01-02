

# /Users/vincent/Documents/Github/vitte/std/cli/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/cli
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.cli as a standalone package (workspace-member compatible).
# - Offer CLI utilities: args parsing, ansi/colors, progress, prompt, term I/O.
# - Keep deps minimal: std-core, std-runtime, std-string; std-io/std-sys optional.
# - Include unit tests.
# - Feature-gate term raw-mode and platform backends.
#
# Notes:
# - Align keys with your canonical Muffin EBNF if names differ.
# - Term/raw_mode typically needs std-sys for TTY ioctls.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-cli"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: CLI utilities (args, ansi, progress, prompt, term)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "cli", "terminal", "ansi", "args", "prompt", "progress"]
.end

workspace
  kind = "member"
.end

target
  name = "std-cli"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.cli"
  .end
  entry
    module = "std.cli.ansi"
  .end
  entry
    module = "std.cli.args"
  .end
  entry
    module = "std.cli.colors"
  .end
  entry
    module = "std.cli.progress"
  .end
  entry
    module = "std.cli.prompt"
  .end
  entry
    module = "std.cli.term"
  .end
  entry
    module = "std.cli.term.input"
  .end
  entry
    module = "std.cli.term.raw_mode"
  .end
  entry
    module = "std.cli.term.size"
  .end
  entry
    module = "std.cli.term.tty"
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
    name     = "std-io"
    path     = "../io"
    optional = true
  .end

  dep
    name     = "std-sys"
    path     = "../sys"
    optional = true
  .end

  dep
    name     = "std-collections"
    path     = "../collections"
    optional = true
  .end
.end

features
  feature
    name = "ansi"
    description = "Enable ANSI utilities and color formatting."
    default = true
  .end

  feature
    name = "args"
    description = "Enable args parsing helpers."
    default = true
  .end

  feature
    name = "progress"
    description = "Enable progress bars/spinners."
    default = true
  .end

  feature
    name = "prompt"
    description = "Enable interactive prompts."
    default = true
  .end

  feature
    name = "term"
    description = "Enable terminal helpers (tty/size/input)."
    default = true
  .end

  feature
    name = "raw_mode"
    description = "Enable raw mode (requires std-sys)."
    default = true
    requires = ["std-sys"]
  .end

  feature
    name = "sys_backend"
    description = "Use OS backend for terminal queries (requires std-sys)."
    default = true
    requires = ["std-sys"]
  .end

  feature
    name = "no_alloc"
    description = "Prefer fixed buffers; reduce allocations when possible."
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
    run  = "muffin test std-cli"
  .end

  script
    name = "check"
    run  = "muffin check std-cli"
  .end
.end