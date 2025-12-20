# C:\Users\gogin\Documents\GitHub\vitte\std\alloc\mod.muf
# Muffin module manifest — std/alloc

[muf]
name = "std/alloc"
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
# Public surface of this module.
modules = [
  "std/alloc/alloc",
  "std/alloc/arena",
  "std/alloc/box",
  "std/alloc/rc",
  "std/alloc/string",
  "std/alloc/vec"
]

[deps]
# std/core provides Option/Result, basic traits, etc.
"std/core" = { path = "../core" }

[features]
default = ["checked", "oom_abort"]
checked = { desc = "Enable bounds/overflow checks and debug assertions in alloc structures." }
oom_abort = { desc = "Abort on OOM instead of returning Result." }
oom_result = { desc = "Return Result on allocation failure (requires std/core::Result)." }

[build]
# If you have a C PAL/runtime allocator, wire it here; otherwise leave empty.
link = []
cflags = []
defines = ["STD_ALLOC=1"]

[test]
runner = "std/testing"
files = [
  "tests/string_tests.vitte",
  "tests/vec_tests.vitte"
]

[lint]
deny = ["unused", "shadowing"]
warn = ["style", "doc"]

[meta]
description = "Allocation primitives and containers backed by the global allocator (String, Vec, Arena, Rc, Box)."
homepage = "https://github.com/vitte-lang/vitte"
repository = "https://github.com/vitte-lang/vitte"
