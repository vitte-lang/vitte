# C:\Users\gogin\Documents\GitHub\vitte\std\async\tests\mod.muf
# Muffin module manifest — std/async/tests

[muf]
name = "std/async/tests"
kind = "tests"
version = "0.1.0"
license = "MIT"
edition = "2025"

[paths]
root = "."
tests = "."

[deps]
"std/testing" = { path = "../../testing" }
"std/core"    = { path = "../../core" }
"std/alloc"   = { path = "../../alloc" }
"std/sync"    = { path = "../../sync" }
"std/time"    = { path = "../../time" }
"std/os"      = { path = "../../os" }
"std/async"   = { path = ".." }

[test]
runner = "std/testing"
files = [
  "future_tests.vitte",
  "executor_tests.vitte"
]

[lint]
deny = ["unused", "shadowing"]
warn = ["style", "doc"]

[meta]
description = "Unit tests for std/async (future + executors)."
homepage = "https://github.com/vitte-lang/vitte"
repository = "https://github.com/vitte-lang/vitte"
