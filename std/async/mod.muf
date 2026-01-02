# /Users/vincent/Documents/Github/vitte/std/async/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/async
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.async as a standalone package (workspace-member compatible).
# - Expose async primitives and runtime facade:
#     * Future/Task
#     * Executor (single-thread + multi-thread)
#     * Waker/Context
#     * Channels (optional)
#     * Timers (optional, via std-time)
# - Keep deps minimal: std-core, std-runtime, std-string; std-time optional.
# - Feature-gate threading, io integration and heavier components.
# - Include unit tests.
#
# Notes:
# - std.async should be usable in non-IO contexts (pure executor).
# - IO reactors/select/poll backends can live in std-async-io (future subpkg).
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-async"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: async primitives (future/task/executor/waker)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "async", "future", "task", "executor", "concurrency"]
.end

workspace
  kind = "member"
.end

target
  name = "std-async"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.async"
  .end
  entry
    module = "std.async.future"
  .end
  entry
    module = "std.async.task"
  .end
  entry
    module = "std.async.waker"
  .end
  entry
    module = "std.async.executor"
  .end
  entry
    module = "std.async.join"
  .end
  entry
    module = "std.async.channel"
  .end
  entry
    module = "std.async.timer"
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
    name     = "std-time"
    path     = "../time"
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
    name = "single_thread"
    description = "Enable single-thread executor."
    default = true
  .end

  feature
    name = "multi_thread"
    description = "Enable multi-thread executor (requires runtime threads)."
    default = false
  .end

  feature
    name = "channels"
    description = "Enable async channels (mpsc/broadcast) helpers."
    default = true
  .end

  feature
    name = "timers"
    description = "Enable async timers (requires std-time)."
    default = true
    requires = ["std-time"]
  .end

  feature
    name = "alloc"
    description = "Allow heap allocations for tasks and scheduling."
    default = true
  .end

  feature
    name = "no_std"
    description = "Reduce dependencies for embedded contexts (no sys backend)."
    default = false
  .end

  feature
    name = "strict"
    description = "CI mode: tighter checks and warnings as errors."
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
    run  = "muffin test std-async"
  .end

  script
    name = "check"
    run  = "muffin check std-async"
  .end
.end
