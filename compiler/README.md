# Compiler - Vitte Compilation Engine

The heart of Vitte: from parsed code to machine instructions.

## Structure

### ir/
Intermediate representations and optimization
- **vitte_mir_**: Middle Intermediate Representation  
- **vitte_optimize**: MIR optimization passes
- **vitte_monomorphize**: Code specialization

### backends/
Code generation targets
- **vitte_codegen_llvm**: LLVM backend
- **vitte_codegen_gcc**: GCC backend  
- **vitte_codegen_cranelift**: Cranelift backend
- **vitte_codegen_ssa**: SSA infrastructure

## Purpose

The compiler layer transforms the semantic AST through increasingly low-level representations until machine code is generated.
