# modules/jmath/algorithms/mod.muf
#
# Muffin manifest – module group: jmath.algorithms
# Build: one binary per module + optional "all" aggregator

[package]
name = "jmath-algorithms"
version = "0.1.0"
type = "module"
namespace = "jmath.algorithms"

[layout]
root = "modules/jmath/algorithms"
target = "target/jmath/algorithms"

[toolchain]
compiler = "steelc"

[build]
# Build everything in one command
default = "all"

[tasks.all]
kind = "group"
deps = [
  "build:cordic",
  "build:fast_math",
  "build:polynomial_approx",
  "build:precise_math",
]

# --------------------------------------------------------------------------
# One binary per module
# --------------------------------------------------------------------------

[tasks.build:cordic]
kind = "compile"
input = "modules/jmath/algorithms/cordic.vitte"
output = "target/jmath/algorithms/cordic.bin"

[tasks.build:fast_math]
kind = "compile"
input = "modules/jmath/algorithms/fast_math.vitte"
output = "target/jmath/algorithms/fast_math.bin"

[tasks.build:polynomial_approx]
kind = "compile"
input = "modules/jmath/algorithms/polynomial_approx.vitte"
output = "target/jmath/algorithms/polynomial_approx.bin"

[tasks.build:precise_math]
kind = "compile"
input = "modules/jmath/algorithms/precise_math.vitte"
output = "target/jmath/algorithms/precise_math.bin"
