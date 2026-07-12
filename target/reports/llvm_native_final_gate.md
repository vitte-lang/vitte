# LLVM Native Final Gate

- Status: **PASS**
- Source: `tests/bootstrap_native/main_proc.vit`
- LLVM IR: `target/llvm_native_final/main.ll`
- Object: `target/llvm_native_final/main.o`
- Executable: `target/llvm_native_final/main`

## Steps

| Step | Status | Detail |
| --- | --- | --- |
| vitte-to-native-ir | PASS | target/llvm_native_final/main.native_ir |
| native-ir-to-llvm-ir | PASS | target/llvm_native_final/main.ll |
| llvm-as | SKIP | llvm-as not found; clang will consume .ll directly |
| clang-object | PASS | target/llvm_native_final/main.o |
| clang-link | PASS | target/llvm_native_final/main |
| run | PASS | exit=7 |
