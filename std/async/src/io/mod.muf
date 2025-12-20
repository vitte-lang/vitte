# C:\Users\gogin\Documents\GitHub\vitte\std\async\mod.muf
# Muffin module manifest — std/async

[muf]
name = "std/async"
kind = "module"
version = "0.1.0"
license = "MIT"
edition = "2025"

[paths]
root = "."
src  = "src"
tests = "tests"
include = "include"

[exports]
modules = [
  "std/async/future",

  "std/async/executor/mod",
  "std/async/executor/waker",
  "std/async/executor/local",
  "std/async/executor/multithread",

  "std/async/io/mod",
  "std/async/io/async_read_write",
  "std/async/io/reactor_stub"
]

[deps]
"std/core"  = { path = "../core" }
"std/alloc" = { path = "../alloc" }
"std/sync"  = { path = "../sync" }
"std/time"  = { path = "../time" }
"std/os"    = { path = "../os" }

[features]
default = ["executor_local", "executor_mt", "io_stub", "stats"]

executor_local = { desc = "Enable single-thread cooperative executor." }
executor_mt    = { desc = "Enable multi-thread thread-pool executor." }

io_stub        = { desc = "Enable stub reactor (no OS polling), for tests and fallback." }
io_posix_epoll = { desc = "Enable Linux epoll reactor (future)." }
io_posix_kqueue = { desc = "Enable BSD/macOS kqueue reactor (future)." }
io_win_iocp    = { desc = "Enable Windows IOCP reactor (future)." }

stats          = { desc = "Collect executor/reactor statistics." }

[build]
link = []
cflags = []
defines = ["STD_ASYNC=1"]

[test]
runner = "std/testing"
files = [
  # add when you create them:
  # "tests/executor_local_tests.vitte",
  # "tests/executor_mt_tests.vitte",
  # "tests/reactor_stub_tests.vitte"
]

[lint]
deny = ["unused", "shadowing"]
warn = ["style", "doc"]

[meta]
description = "Async primitives: Future/Poll, executors (local + multithread), async IO traits, and reactor stub."
homepage = "https://github.com/vitte-lang/vitte"
repository = "https://github.com/vitte-lang/vitte"
