# /Users/vincent/Documents/Github/vitte/std/bench/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/bench
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.bench as the micro/benchmark harness for Vitte stdlib.
# - Offer a registry + runners + stats reporting (json/text) and time sources.
# - Keep deps minimal: std-core, std-runtime, std-string; std-io optional.
# - Allow optional suites to be included as separate packages (std/bench/micro).
# - Include unit tests for stats/registry.
#
# Notes:
# - Align keys with your canonical Muffin EBNF if names differ.
# - Bench should be buildable standalone and as workspace member.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-bench"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: benchmark harness (registry, runners, stats)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "bench", "benchmark", "microbench", "stats"]
.end

workspace
  kind = "member"
.end

target
  name = "std-bench"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.bench"
  .end
  entry
    module = "std.bench.registry"
  .end
  entry
    module = "std.bench.runner"
  .end
  entry
    module = "std.bench.stats"
  .end
  entry
    module = "std.bench.time"
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
    name     = "std-collections"
    path     = "../collections"
    optional = true
  .end
.end

features
  feature
    name = "json"
    description = "Enable JSON output for benchmark results."
    default = true
  .end

  feature
    name = "text"
    description = "Enable human-readable text output (tables)."
    default = true
  .end

  feature
    name = "time"
    description = "Enable time sources and calibration."
    default = true
  .end

  feature
    name = "micro"
    description = "Enable microbench suites under std/bench/micro."
    default = true
  .end

  feature
    name = "alloc_stats"
    description = "Enable allocation statistics if runtime exposes hooks."
    default = false
  .end

  feature
    name = "cpu_affinity"
    description = "Pin threads/benchmarks to CPU cores when supported (requires std-sys)."
    default = false
  .end

  feature
    name = "strict"
    description = "Treat warnings as errors in bench harness (CI mode)."
    default = false
  .end
.end

profile
  opt_level = 3
  debug = true
  warnings_as_errors = false
.end

scripts
  script
    name = "test"
    run  = "muffin test std-bench"
  .end

  script
    name = "check"
    run  = "muffin check std-bench"
  .end

  script
    name = "run"
    run  = "muffin run std-bench"
  .end
.end
