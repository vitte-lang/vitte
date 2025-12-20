# C:\Users\gogin\Documents\GitHub\vitte\std\cli\src\term\mod.muf
# Muffin module manifest — std/cli/term

[muf]
name = "std/cli/term"
kind = "module"
version = "0.1.0"
license = "MIT"
edition = "2025"

[paths]
root = "."
src  = "."
tests = "tests"

[exports]
modules = [
  "std/cli/term/input"
]

[deps]
"std/core"  = { path = "../../../core" }
"std/alloc" = { path = "../../../alloc" }
"std/time"  = { path = "../../../time" }
"std/os"    = { path = "../../../os" }
"std/sync"  = { path = "../../../sync" }

[features]
default = ["stub"]

stub = { desc = "Portable stub input (line-based), no raw mode." }
posix = { desc = "POSIX termios + select/poll (future)." }
win   = { desc = "Windows console input events (future)." }

[build]
defines = ["STD_CLI_TERM=1"]

[test]
runner = "std/testing"
files = [
  # add when created:
  # "tests/term_input_tests.vitte"
]

[lint]
deny = ["unused", "shadowing"]
warn = ["style", "doc"]

[meta]
description = "Terminal input primitives (read_line/read_key/read_password) with stub implementation."
homepage = "https://github.com/vitte-lang/vitte"
repository = "https://github.com/vitte-lang/vitte"
