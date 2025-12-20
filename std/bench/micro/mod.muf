# C:\Users\gogin\Documents\GitHub\vitte\std\bench\micro\mod.muf
# Muffin module manifest — std/bench/micro

[muf]
name = "std/bench/micro"
kind = "module"
version = "0.1.0"
license = "MIT"
edition = "2025"

[paths]
root = "."
src  = "src"
tests = "tests"

[exports]
modules = [
  "std/bench/micro/bench",

  "std/bench/micro/reporters/console",
  "std/bench/micro/reporters/json",

  "std/bench/micro/suites/alloc_suite",
  "std/bench/micro/suites/hash_suite",
  "std/bench/micro/suites/io_suite",
  "std/bench/micro/suites/regex_suite",
  "std/bench/micro/suites/sort_suite"
]

[deps]
"std/core"  = { path = "../../core" }
"std/alloc" = { path = "../../alloc" }
"std/time"  = { path = "../../time" }
"std/os"    = { path = "../../os" }

# Suite deps (optional, can be removed if you inline/remove stubs)
"std/regex" = { path = "../../regex", optional = true }
"std/hash"  = { path = "../../hash",  optional = true }

[features]
default = ["console", "json", "suites", "calibrate", "cli"]

cli       = { desc = "Enable main() entrypoint and argument parsing." }
console   = { desc = "Enable console reporter." }
json      = { desc = "Enable JSON reporter." }
calibrate = { desc = "Enable auto iters calibration (~1ms/sample target)." }
suites    = { desc = "Build all default suites (alloc/hash/io/regex/sort)." }

suite_alloc = { desc = "Enable alloc suite." }
suite_hash  = { desc = "Enable hash suite." }
suite_io    = { desc = "Enable io suite." }
suite_regex = { desc = "Enable regex suite." }
suite_sort  = { desc = "Enable sort suite." }

[build]
defines = ["STD_BENCH_MICRO=1"]

[test]
runner = "std/testing"
files = [
  # add when created:
  # "tests/bench_micro_tests.vitte"
]

[lint]
deny = ["unused", "shadowing"]
warn = ["style", "doc"]

[meta]
description = "Micro benchmark harness with suites and console/JSON reporters."
homepage = "https://github.com/vitte-lang/vitte"
repository = "https://github.com/vitte-lang/vitte"
