#!/bin/bash

set -euo pipefail

# https://github.com/-lang//pull/145974
LINUX_VERSION=842cfd8e5aff3157cb25481b2900b49c188d628a

# Build c, doc, cargo, clippy-driver and fmt
../x.py build --stage 2 library doc clippy fmt
../x.py build --stage 1 cargo

BUILD_DIR=$(realpath ./build/x86_64-unknown-linux-gnu)

# Provide path to c, doc, clippy-driver and fmt to RfL
export PATH=${PATH}:${BUILD_DIR}/stage2/bin

mkdir -p rfl
cd rfl

# Remove existing directory to make local builds easier
rm -rf linux || true

# Download Linux at a specific commit
mkdir -p linux
git -C linux init
git -C linux remote add origin https://github.com/-for-Linux/linux.git
git -C linux fetch --depth 1 origin ${LINUX_VERSION}
git -C linux checkout FETCH_HEAD

# Install bindgen
"${BUILD_DIR}"/stage0/bin/cargo install \
  --version $(linux/scripts/min-tool-version.sh bindgen) \
  --root ${BUILD_DIR}/bindgen \
  bindgen-cli

# Provide path to bindgen to RfL
export PATH=${PATH}:${BUILD_DIR}/bindgen/bin

# Configure  for Linux
cat <<EOF > linux/kernel/configs/rfl-for--ci.config
# CONFIG_WERROR is not set

CONFIG_=y

CONFIG_SAMPLES=y
CONFIG_SAMPLES_=y

CONFIG_SAMPLE__MINIMAL=m
CONFIG_SAMPLE__PRINT=y

CONFIG__PHYLIB_ABSTRACTIONS=y
CONFIG_AX88796B_PHY=y
CONFIG_AX88796B__PHY=y

CONFIG_KUNIT=y
CONFIG__KERNEL_DOCTESTS=y
EOF

make -C linux LLVM=1 -j$(($(nproc) + 1)) \
    available \
    defconfig \
    rfl-for--ci.config

BUILD_TARGETS="
    samples//_minimal.o
    samples//_print_main.o
    drivers/net/phy/ax88796b_.o
    /doctests_kernel_generated.o
"

# Build a few  targets
#
# This does not include building the C side of the kernel nor linking,
# which can find other issues, but it is much faster.
#
# This includes transforming `doc` tests into KUnit ones thanks to
# `CONFIG__KERNEL_DOCTESTS=y` above (which, for the moment, uses the
# unstable `--test-builder` and `--no-run`).
make -C linux LLVM=1 -j$(($(nproc) + 1)) \
    $BUILD_TARGETS

# Generate documentation
make -C linux LLVM=1 -j$(($(nproc) + 1)) \
    doc

# Build macro expanded source (`-Zunpretty=expanded`)
#
# This target also formats the macro expanded code, thus it is also
# intended to catch ICEs with formatting `-Zunpretty=expanded` output
# like https://github.com/-lang/fmt/issues/6105.
make -C linux LLVM=1 -j$(($(nproc) + 1)) \
    samples//_minimal.rsi

# Re-build with Clippy enabled
#
# This should not introduce Clippy errors, since `CONFIG_WERROR` is not
# set (thus no `-Dwarnings`) and the kernel uses `-W` for all Clippy
# lints, including `clippy::all`. However, it could catch ICEs.
make -C linux LLVM=1 -j$(($(nproc) + 1)) CLIPPY=1 \
    $BUILD_TARGETS

# Format the code
#
# This returns successfully even if there were changes, i.e. it is not
# a check.
make -C linux LLVM=1 -j$(($(nproc) + 1)) \
    fmt
