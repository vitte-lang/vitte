

# /Users/vincent/Documents/Github/vitte/std/cli/term/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/cli/term
# -----------------------------------------------------------------------------
# MAX goals:
# - Package the terminal submodule as an optional standalone component.
# - Expose terminal capabilities: tty detection, size query, raw mode,
#   line/input handling.
# - Keep deps minimal: std-core, std-runtime, std-string; std-sys optional.
# - Include unit tests.
# - Feature-gate platform backends (raw mode / ioctl) and interactive input.
#
# Notes:
# - This is a subpackage under std-cli; it can be built standalone or as part
#   of std-cli workspace.
# - Align keys with your canonical Muffin EBNF if names differ.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-cli-term"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte stdlib: terminal utilities (tty/size/raw mode/input)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "cli", "terminal", "tty", "raw-mode", "input"]
.end

workspace
  kind = "member"
.end

target
  name = "std-cli-term"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.cli.term"
  .end
  entry
    module = "std.cli.term.tty"
  .end
  entry
    module = "std.cli.term.size"
  .end
  entry
    module = "std.cli.term.raw_mode"
  .end
  entry
    module = "std.cli.term.input"
  .end
.end

deps
  dep
    name = "std-core"
    path = "../../core"
  .end

  dep
    name = "std-runtime"
    path = "../../runtime"
  .end

  dep
    name = "std-string"
    path = "../../string"
  .end

  dep
    name     = "std-sys"
    path     = "../../sys"
    optional = true
  .end

  dep
    name     = "std-io"
    path     = "../../io"
    optional = true
  .end

  dep
    name     = "std-collections"
    path     = "../../collections"
    optional = true
  .end
.end

features
  feature
    name = "tty"
    description = "Enable tty detection helpers."
    default = true
  .end

  feature
    name = "size"
    description = "Enable terminal size querying."
    default = true
  .end

  feature
    name = "input"
    description = "Enable terminal input helpers (line reading, key events)."
    default = true
  .end

  feature
    name = "raw_mode"
    description = "Enable raw mode support (requires std-sys)."
    default = true
    requires = ["std-sys"]
  .end

  feature
    name = "sys_backend"
    description = "Use OS backend for tty/size/raw mode (requires std-sys)."
    default = true
    requires = ["std-sys"]
  .end

  feature
    name = "fallbacks"
    description = "Enable best-effort fallbacks when sys backend is absent."
    default = true
  .end

  feature
    name = "no_alloc"
    description = "Prefer fixed buffers and avoid allocations when possible."
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
    run  = "muffin test std-cli-term"
  .end

  script
    name = "check"
    run  = "muffin check std-cli-term"
  .end
.end