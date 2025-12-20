# C:\Users\gogin\Documents\GitHub\vitte\std\async\time\mod.muf
# Muffin module manifest — std/async/time

[muf]
name = "std/async/time"
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
  "std/async/time/sleep",
  "std/async/time/interval"
]

[deps]
"std/core"  = { path = "../../core" }
"std/time"  = { path = "../../time" }
"std/os"    = { path = "../../os" }
"std/async" = { path = ".." }

[features]
default = ["stub"]
stub = { desc = "Polling-based async time primitives (no reactor timers)." }
reactor_timers = { desc = "Use reactor-backed timers when available (future)." }

[build]
defines = ["STD_ASYNC_TIME=1"]

[test]
runner = "std/testing"
files = [
  # add when created:
  # "tests/sleep_tests.vitte",
  # "tests/interval_tests.vitte"
]

[lint]
deny = ["unused", "shadowing"]
warn = ["style", "doc"]

[meta]
description = "Async time primitives: sleep/sleep_until and interval (stub polling)."
homepage = "https://github.com/vitte-lang/vitte"
repository = "https://github.com/vitte-lang/vitte"
