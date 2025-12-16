[package]
name = "vitte-bench"
version = "0.1.0"
edition = "2024"
description = "Benchmark harness for Vitte compiler and runtime"
authors = ["Vitte Contributors"]
license = "MIT OR Apache-2.0"

[lib]
name = "benchlib"
path = "src/bench/bench_time.c"

[[bin]]
name = "benchc"
path = "src/bench/bench_main.c"

[dependencies]

[dev-dependencies]

[profile.dev]
opt-level = 0
debug = true

[profile.release]
opt-level = 3
debug = false
lto = true
