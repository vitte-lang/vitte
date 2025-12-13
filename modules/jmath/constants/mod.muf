# modules/jmath/constants/mod.muf
#
# Muffin manifest – module group: jmath.constants
# Build: one binary per module + "all" aggregator

[package]
name = "jmath-constants"
version = "0.1.0"
type = "module"
namespace = "jmath.constants"

[layout]
root = "modules/jmath/constants"
target = "target/jmath/constants"

[toolchain]
compiler = "steelc"

[build]
default = "all"

[tasks.all]
kind = "group"
deps = [
  "build:angles",
  "build:ieee754",
  "build:physical",
  "build:real",
  "build:tolerance",
]

# --------------------------------------------------------------------------
# One binary per module
# --------------------------------------------------------------------------

[tasks.build:angles]
kind = "compile"
input = "modules/jmath/constants/angles.vitte"
output = "target/jmath/constants/angles.bin"

[tasks.build:ieee754]
kind = "compile"
input = "modules/jmath/constants/ieee754.vitte"
output = "target/jmath/constants/ieee754.bin"

[tasks.build:physical]
kind = "compile"
input = "modules/jmath/constants/physical.vitte"
output = "target/jmath/constants/physical.bin"

[tasks.build:real]
kind = "compile"
input = "modules/jmath/constants/real.vitte"
output = "target/jmath/constants/real.bin"

[tasks.build:tolerance]
kind = "compile"
input = "modules/jmath/constants/tolerance.vitte"
output = "target/jmath/constants/tolerance.bin"
