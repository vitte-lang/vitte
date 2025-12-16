# WebAssembly Template

## Overview

This template creates a WebAssembly module for browser or WASI runtime.

## Building

```bash
vitte-build --target wasm32-unknown-unknown
```

## Running

```bash
# Browser
python -m http.server

# WASI
wasmtime main.wasm
```

## Performance

WebAssembly builds are optimized with `wasm-opt`:

```bash
wasm-opt -Oz main.wasm -o main.opt.wasm
```
