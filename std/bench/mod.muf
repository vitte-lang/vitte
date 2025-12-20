# C:\Users\gogin\Documents\GitHub\vitte\std\bench\mod.muf
# Muffin module manifest — std/bench (umbrella)

[muf]
name = "std/bench"
kind = "module"
version = "0.1.0"
license = "MIT"
edition = "2025"

[paths]
root = "."
src  = "src"

[exports]
modules = [
  "std/bench/macro/bench_macro",
  "std/bench/micro/bench"
]

[submodules]
"std/bench/macro" = { path = "macro" }
"std/bench/micro" = { path = "micro" }

[deps]
"std/core"  = { path = "../core" }
"std/alloc" = { path = "../alloc" }
"std/time"  = { path = "../time" }
"std/os"    = { path = "../os" }

[features]
default = ["macro", "micro"]

macro = { desc = "Enable macro benchmark harness." }
micro = { desc = "Enable micro benchmark harness." }

[build]
defines = ["STD_BENCH=1"]

[meta]
description = "Benchmark utilities (macro + micro harness)."
homepage = "https://github.com/vitte-lang/vitte"
repository = "https://github.com/vitte-lang/vitte"
