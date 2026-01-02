# /Users/vincent/Documents/Github/vitte/std/log/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/log
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.log as a standalone package (workspace-member compatible).
# - Expose a stable logging facade (levels, filters, sinks, formatting).
# - Keep deps minimal: std.core + std.runtime; std.sys optional.
# - Include unit tests.
# - Support feature-gated timestamping and syslog/eventlog backends.
#
# Notes:
# - Align keys with your canonical Muffin EBNF if names differ.
# - Avoid build-system specific metadata beyond Muffin.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-log"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: logging facade (levels, filters, sinks)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "log", "logging", "diagnostics"]
.end

workspace
  kind = "member"
.end

target
  name = "std-log"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Canonical entry modules (re-exported)
  entry
    module = "std.log"
  .end
  entry
    module = "std.log.level"
  .end
  entry
    module = "std.log.filter"
  .end
  entry
    module = "std.log.sink"
  .end
  entry
    module = "std.log.format"
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
    name     = "std-time"
    path     = "../time"
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
    name = "timestamps"
    description = "Include timestamps in log records (requires std-time)."
    default = true
    requires = ["std-time"]
  .end

  feature
    name = "color"
    description = "ANSI colored output for console sink (requires std-cli/ansi if used)."
    default = false
  .end

  feature
    name = "sys_backend"
    description = "Enable OS-backed sinks (syslog/eventlog) when std-sys is available."
    default = false
    requires = ["std-sys"]
  .end

  feature
    name = "memory_sink"
    description = "Enable in-memory sink for tests and embeddings."
    default = true
  .end

  feature
    name = "async"
    description = "Enable async logging queue (requires runtime threads)."
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
    run  = "muffin test std-log"
  .end

  script
    name = "check"
    run  = "muffin check std-log"
  .end
.end
