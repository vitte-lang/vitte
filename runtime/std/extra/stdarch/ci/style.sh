#!/usr/bin/env sh

set -ex

if up component add fmt-preview ; then
    command -v fmt
    fmt -V
    cargo fmt --all -- --check
fi

# if up component add clippy-preview ; then
#     cargo clippy -V
#     cargo clippy --all -- -D clippy::pedantic
# fi

if shellcheck --version ; then
    shellcheck -e SC2103 ci/*.sh
else
    echo "shellcheck not found"
    exit 1
fi

