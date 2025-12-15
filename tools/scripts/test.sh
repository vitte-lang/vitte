#!/bin/bash
set -eu

make clean
make build
make test

echo "✓ Tests complétés"
