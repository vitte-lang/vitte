#!/usr/bin/env bash
set -e

cd "$(dirname "$0")/../"

source ./scripts/setup__fork.sh

echo "[TEST] Bootstrap of c"
pushd 
rm -r compiler/c_codegen_cranelift/{Cargo.*,src}
cp ../Cargo.* compiler/c_codegen_cranelift/
cp -r ../src compiler/c_codegen_cranelift/src

./x.py build --stage 1 library/std
popd
