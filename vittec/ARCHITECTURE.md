# Vitte Compiler (vittec) Architecture

The vittec/ directory is organized into 5 logical domains, each handling a distinct phase of compilation:

## Frontend (Lexing, Parsing, AST)
- **vitte_lexer**: Tokenization and lexical analysis
- **vitte_ast**: Abstract Syntax Tree definitions  
- **vitte_ast_lowering**: Lowering AST to HIR
- **vitte_ast_ir**: AST Intermediate Representation
- **vitte_parse**: Parsing and parser infrastructure
- **vitte_expand**: Macro expansion
- **vitte_macros**: Built-in macro definitions
- **vitte_builtin_macros**: Macro implementations
- **vitte_attr_parsing**: Attribute parsing
- **vitte_passes**: AST passes and transformations
- **vitte_parse_format**: Format parsing utilities

## Middle (Type Checking, Analysis, HIR)
- **vitte_hir**: High-level Intermediate Representation
- **vitte_hir_id**: HIR node identification
- **vitte_hir_pretty**: Pretty-printing HIR
- **vitte_hir_analysis**: HIR analysis passes
- **vitte_hir_typeck**: Type checking
- **vitte_privacy**: Privacy checking
- **vitte_resolve**: Name resolution
- **vitte_middle**: Middle-level passes
- **vitte_borrowck**: Borrow checking
- **vitte_trait_selection**: Trait resolution
- **vitte_traits**: Trait definitions and manipulation
- **vitte_infer**: Type inference
- **vitte_next_trait_solver**: New trait solver implementation
- **vitte_type_ir**: Type system IR
- **vitte_type_ir_macros**: Type IR macros
- **vitte_ty_utils**: Type utilities
- **vitte_pattern_analysis**: Pattern analysis

## Backend (Code Generation, MIR, Optimization)
- **vitte_mir_build**: Building MIR from HIR
- **vitte_mir_dataflow**: Dataflow analysis on MIR
- **vitte_mir_transform**: MIR transformations
- **vitte_monomorphize**: Monomorphization
- **vitte_codegen_ssa**: SSA-based code generation
- **vitte_codegen_llvm**: LLVM backend
- **vitte_codegen_gcc**: GCC backend
- **vitte_codegen_cranelift**: Cranelift backend
- **vitte_symbol_mangling**: Symbol name mangling
- **vitte_transmute**: Transmute support
- **vitte_llvm**: LLVM integration
- **vitte_target**: Target platform specifications
- **vitte_sanitizers**: Sanitizer support

## Infrastructure (Utilities, Data Structures)
- **vitte_data_structures**: Core data structures
- **vitte_arena**: Arena allocators
- **vitte_hashes**: Hashing utilities
- **vitte_fs_util**: File system utilities
- **vitte_thread_pool**: Thread pool implementation
- **vitte_index**: Indexing data structures
- **vitte_index_macros**: Index macros
- **vitte_metadata**: Metadata handling
- **vitte_serialize**: Serialization utilities
- **vitte_graphviz**: Graphviz output support
- **vitte_baked_icu_data**: ICU data integration
- **vitte_proc_macro**: Procedural macro support
- **vitte_windows_rc**: Windows resource compilation
- **vitte_fluent_macro**: Fluent macros for localization

## Support (Error Handling, Diagnostics, Configuration)
- **vitte_errors**: Error handling infrastructure
- **vitte_error_messages**: Error message definitions
- **vitte_error_codes**: Error code registry
- **vitte_log**: Logging infrastructure
- **vitte_abi**: Application Binary Interface definitions
- **vitte_driver**: Main compiler driver
- **vitte_driver_impl**: Driver implementation details
- **vitte_session**: Compilation session management
- **vitte_span**: Source location tracking
- **vitte_feature**: Feature gate handling
- **vitte_interface**: Compiler interface
- **vitte_query_system**: Query system infrastructure
- **vitte_query_impl**: Query system implementation
- **vitte_incremental**: Incremental compilation support
- **vitte_lint**: Linting infrastructure
- **vitte_lint_defs**: Lint definitions
- **vitte_const_eval**: Const evaluation
- **vitte_public**: Public items tracking
- **vitte_public_bridge**: Public bridge interface

This organization makes the compiler more maintainable by grouping related functionality and clearly defining dependencies between phases.
