# LLVM Backend Coverage

## Backend Status

| Feature | Status |
|----------|----------|
| LLVM IR emission | PASS |
| Object generation | PASS |
| DWARF debug info | PASS |
| Multi-target support | PASS |
| LTO | PASS |
| ThinLTO | PASS |
| PGO | PASS |

## Supported Targets

- x86_64-unknown-linux-gnu
- aarch64-unknown-linux-gnu
- riscv64-unknown-linux-gnu

## Optimization Levels

- O0
- O1
- O2
- O3
- Os
- Oz

## Compilation Pipeline

```text
Lexer
Parser
AST
HIR
Sema
Typeck
Borrowck
MIR
LLVM IR
Object
Linker
```

LLVM IR SHA256: `8b2cf2263ba703f7deeccbba13a6a580b9c6056b995d4f36b17f26b4a406c5e2`
