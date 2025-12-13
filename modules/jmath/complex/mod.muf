# modules/jmath/complex/mod.muf
#
# Muffin manifest – module group: jmath.complex
# Build: one binary per module + "all" aggregator

[package]
name = "jmath-complex"
version = "0.1.0"
type = "module"
namespace = "jmath.complex"

[layout]
root = "modules/jmath/complex"
target = "target/jmath/complex"

[toolchain]
compiler = "steelc"

[build]
default = "all"

[tasks.all]
kind = "group"
deps = [
  "build:complex",
  "build:abs_arg",
  "build:exp_log",
  "build:roots",
  "build:trig",
]

# --------------------------------------------------------------------------
# One binary per module
# --------------------------------------------------------------------------

[tasks.build:complex]
kind = "compile"
input = "modules/jmath/complex/complex.vitte"
output = "target/jmath/complex/complex.bin"

[tasks.build:abs_arg]
kind = "compile"
input = "modules/jmath/complex/abs_arg.vitte"
output = "target/jmath/complex/abs_arg.bin"

[tasks.build:exp_log]
kind = "compile"
input = "modules/jmath/complex/exp_log.vitte"
output = "target/jmath/complex/exp_log.bin"

[tasks.build:roots]
kind = "compile"
input = "modules/jmath/complex/roots.vitte"
output = "target/jmath/complex/roots.bin"

[tasks.build:trig]
kind = "compile"
input = "modules/jmath/complex/trig.vitte"
output = "target/jmath/complex/trig.bin"
