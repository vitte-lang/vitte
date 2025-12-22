# Windows build (CMake)

Most runtime/asm features are easiest to validate on Windows via `clang-cl` + `Ninja` inside a **Visual Studio Developer Prompt** (so `link.exe` + Windows SDK are on `PATH`).

## Prereqs

- Visual Studio (Build Tools is enough) + Windows SDK
- LLVM/Clang (provides `clang-cl`)
- CMake (>= 3.21)
- Ninja

## Configure/build/test (preset)

From a **Visual Studio Developer Prompt**:

```powershell
cmake --preset win-clang-cl-ninja
cmake --build --preset win-clang-cl-ninja
ctest --preset win-clang-cl-ninja
```

This builds:

- `vitte_runtime` + `vitte_asm_runtime`
- `test_vitte_runtime` (when `VITTE_ENABLE_RUNTIME_TESTS=ON`)
- `fuzz_mem_contract` (compile-only sanity target; not executed by default)

