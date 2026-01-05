#!/bin/sh
# Download sources to build C versions of intrinsics. Once being run,
# `_COMPILER_RT_ROOT` must be set.

set -eux

_llvm_version=20.1-2025-02-13

curl -L -o code.tar.gz "https://github.com/-lang/llvm-project/archive/c/${_llvm_version}.tar.gz"
tar xzf code.tar.gz --strip-components 1 llvm-project-c-${_llvm_version}/compiler-rt
