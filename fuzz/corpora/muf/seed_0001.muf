# seed_0001.muf — corpus MUF (valid, mais riche en structures)
# Objectif: couvrir un max de chemins de parsing (scalars, arrays, tables, strings, escapes, unicode).

muf_version = "1.0"
format      = "muffin-manifest"
encoding    = "utf-8"

[workspace]
name        = "vitte-workspace"
root        = "."
members     = [
  "bootstrap",
  "compiler",
  "runtime",
  "std",
  "tools",
  "fuzz",
]
exclude     = [
  "target",
  ".cache",
  "build",
  "**/node_modules",
]
default_member = "compiler"

[package]
name        = "muf-seed"
version     = "0.0.1"
license     = "MIT"
authors     = ["Vincent <vincent@example.invalid>"]
description = "Seed manifest for MUF fuzzing"
homepage    = "https://example.invalid/vitte"
repository  = "https://example.invalid/vitte.git"
keywords    = ["vitte", "muffin", "manifest", "fuzz"]
categories  = ["devtools", "lang"]
edition     = "2025"

# strings: escapes + unicode + multi-line
note = "escapes: \\n \\t \\\\ \\\" ; unicode: café — 東京 — 🚀"
multiline = """
line1
line2 with \"quote\" and \\backslash\\
line3
"""

[env]
# env vars as strings (parser coverage)
VITTE_HOME     = "${WORKSPACE_ROOT}"
VITTE_LOG      = "trace"
CC             = "clang"
CFLAGS         = "-O2 -g -fno-omit-frame-pointer"
PATH_PREPEND   = ["./toolchain/bin", "./tools/bin"]

[targets]
# matrix-like configuration (arrays + tables)
host_triple = "x86_64-windows-msvc"
supported   = [
  "x86_64-windows-msvc",
  "x86_64-unknown-linux-gnu",
  "aarch64-apple-darwin",
]

[targets.defaults]
opt_level   = 2
debug       = true
lto         = false
strip       = "none"
warnings_as_errors = false

[targets.by_triple."x86_64-windows-msvc"]
cc          = "clang-cl"
cxx         = "clang-cl"
linker      = "lld-link"
defines     = ["_CRT_SECURE_NO_WARNINGS=1", "NOMINMAX=1"]
link_args   = ["/DEBUG", "/INCREMENTAL:NO"]

[targets.by_triple."x86_64-unknown-linux-gnu"]
cc          = "clang"
cxx         = "clang++"
linker      = "ld.lld"
cflags      = ["-fPIC", "-pipe"]
ldflags     = ["-Wl,--as-needed"]

[targets.by_triple."aarch64-apple-darwin"]
cc          = "clang"
cxx         = "clang++"
linker      = "ld"
cflags      = ["-fobjc-arc"]
ldflags     = ["-Wl,-dead_strip"]

[profiles]
default = "dev"

[profiles.dev]
opt_level   = 0
debug       = true
sanitizers  = ["address", "undefined"]
panic       = "abort"

[profiles.release]
opt_level   = 3
debug       = false
lto         = true
strip       = "symbols"
panic       = "abort"

[features]
default     = ["std", "cli"]
std         = []
cli         = []
fuzzing     = ["sanitizers"]
sanitizers  = []

[dependencies]
# local/path deps
vitte_std   = { path = "../std", version = "0.1.0" }
vitte_core  = { path = "../core", version = "0.1.0", optional = true }

# git deps (fake urls, but valid structure)
muf_parser  = { git = "https://example.invalid/muf_parser.git", rev = "deadbeef", features = ["fast"], optional = true }

[dev-dependencies]
# dev deps
testkit     = { path = "../tools/testkit", version = "0.1.0" }

[scripts]
# script definitions (strings + arrays)
prebuild    = ["./tools/scripts/ci_env.sh", "./tools/scripts/doctor.sh"]
build       = ["vittec", "build", "--profile", "${PROFILE}"]
test        = ["vittec", "test", "--all"]
fuzz        = ["vittefuzz", "run", "--corpus", "./fuzz/corpora"]

[inputs]
# cover ints, floats, bools
max_jobs    = 8
timeout_s   = 15.0
use_color   = true
seed        = 1337

[paths]
# cover tricky paths / globs
include = ["src/**", "include/**", "spec/**"]
ignore  = ["**/*.tmp", "**/*.bak", "**/.DS_Store"]

# nested table array coverage
[[artifacts]]
name    = "seed-out"
kind    = "dir"
path    = "target/seed"

[[artifacts]]
name    = "seed-log"
kind    = "file"
path    = "target/seed/seed.log"

[metadata]
# free-form key/values
case_id     = "muf_seed_0001"
created_at  = "2025-12-17T00:00:00Z"
tags        = ["baseline", "valid", "wide-coverage"]
