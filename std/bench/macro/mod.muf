# C:\Users\gogin\Documents\GitHub\vitte\std\bench\macro\mod.muf
# Muffin module manifest — std/bench/macro

[muf]
name = "std/bench/macro"
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
  "std/bench/macro/bench_macro"
]

[deps]
"std/core"  = { path = "../../core" }
"std/alloc" = { path = "../../alloc" }
"std/time"  = { path = "../../time" }
"std/os"    = { path = "../../os" }

[features]
default = ["cli", "stats"]
cli   = { desc = "Enable main() and argument parsing for macro bench runner." }
stats = { desc = "Collect and report percentile statistics." }
json  = { desc = "Enable JSON output mode." }

[build]
defines = ["STD_BENCH_MACRO=1"]

[test]
runner = "std/testing"
files = [
  # add when created:
  # "tests/bench_macro_tests.vitte"
]

[lint]
deny = ["unused", "shadowing"]
warn = ["style", "doc"]

[meta]
description = "Macro benchmark harness for stdlib and tools (Runner/Bencher + stats + optional JSON)."
homepage = "https://github.com/vitte-lang/vitte"
repository = "https://github.com/vitte-lang/vitte"
