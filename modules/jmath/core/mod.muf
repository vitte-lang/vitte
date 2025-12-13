# mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest — core numeric modules (batch)
# Commande toolchain: steelc
# -----------------------------------------------------------------------------

[package]
name = "core-math-numeric"
version = "0.1.0"
kind = "module-pack"
license = "MIT"
entry = "core/basic.vitte"

[toolchain]
compiler = "steelc"
language = "vitte"
edition = "2025"

[build]
opt_level = 3
debug = false
warnings = "all"
deny_warnings = false

[targets]
# Compile tout d'un coup
default = "all"

[target.all]
type = "group"
items = [
  "core/basic.vitte",
  "core/compare.vitte",
  "core/classify.vitte",
  "core/conversions.vitte",
  "core/fma.vitte",
  "core/nextafter.vitte",
  "core/remainder.vitte",
  "core/rounding.vitte",
  "core/safe.vitte",
  "core/scalb.vitte",
]

# Si tu veux aussi des binaires dédiés par module (optionnel)
# [target.basic]
# type = "bin"
# main = "core/basic.vitte"
#
# [target.compare]
# type = "bin"
# main = "core/compare.vitte"
#
# ...

[layout]
out_dir = "target"
cache_dir = "target/.cache"

[commands]
build = "steelc build -m mod.muf -t all"
check = "steelc check -m mod.muf -t all"
clean = "steelc clean -m mod.muf"

[deps]
# aucun ici (core pack)
