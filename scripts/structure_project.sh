#!/usr/bin/env bash
# Script de structuration complète du projet Vitte

set -e

VITTE_ROOT="${1:-.}"
echo "📁 Structuring Vitte project at: $VITTE_ROOT"

# Fonction utilitaire
mkdir_safe() {
    [ -d "$1" ] || mkdir -p "$1"
}

# === Compiler - Phase Lexer ===
mkdir_safe "$VITTE_ROOT/src/compiler/lexer"
cat > "$VITTE_ROOT/src/compiler/lexer/.placeholder" << 'EOF'
# Lexer
## Responsibility
- Convert source code (streams of UTF-8 bytes) into a linear token sequence
- Handle number/string literals parsing
- Manage line/column tracking for diagnostics

## Files
- mod.rs/h           : Public lexer interface
- scanner.rs/c       : State machine (DFA)
- token_type.rs/h    : Token enumeration
- literal.rs/c       : Literal parsing (0x, 0b, e notation, escape sequences)
- tests/             : Lexer unit tests

## Key exports
- fn lex(source: &str) -> Vec<Token>
- struct Token { kind, text, span }
- enum TokenKind { Ident, IntLit, ... }
EOF

# === Compiler - Phase Parser ===
mkdir_safe "$VITTE_ROOT/src/compiler/parser"
cat > "$VITTE_ROOT/src/compiler/parser/.placeholder" << 'EOF'
# Parser
## Responsibility
- Convert token stream into Abstract Syntax Tree (AST)
- Implement operator precedence (Pratt parsing for expressions)
- Provide error recovery for better diagnostics
- Each parser module handles one grammar domain

## Files
- mod.rs          : Public parser interface
- expr.rs         : Expression parsing (Pratt algorithm)
- stmt.rs         : Statement/declaration parsing
- decl.rs         : Function/type/const declarations
- recovery.rs     : Error recovery (skip tokens until sync point)
- tests/          : Parser unit & integration tests

## Key exports
- fn parse(tokens: Vec<Token>) -> (Program, Vec<Diagnostic>)
- struct Program { items: Vec<Item> }
- enum Item { FnDecl, StructDecl, ... }
EOF

# === Compiler - AST ===
mkdir_safe "$VITTE_ROOT/src/compiler/ast"
cat > "$VITTE_ROOT/src/compiler/ast/.placeholder" << 'EOF'
# Abstract Syntax Tree
## Responsibility
- Define all AST node types for the language
- Provide visitors for traversing the tree
- Implement pretty-printing for debugging

## Files
- mod.rs           : AST node definitions
- expr.rs          : Expression nodes (BinOp, Call, Literal, etc.)
- stmt.rs          : Statement nodes (If, Loop, Assign, etc.)
- decl.rs          : Declaration nodes (Fn, Struct, Const, etc.)
- types.rs         : Type annotation nodes
- visitor.rs       : Generic visitor pattern trait
- printer.rs       : Pretty-printing for AST dump
- tests/           : AST construction/printing tests

## Key pattern
pub trait Visitor<T> {
    fn visit_expr(&mut self, e: &Expr) -> T;
    fn visit_stmt(&mut self, s: &Stmt) -> T;
    ...
}
EOF

# === Compiler - Resolver ===
mkdir_safe "$VITTE_ROOT/src/compiler/resolver"
cat > "$VITTE_ROOT/src/compiler/resolver/.placeholder" << 'EOF'
# Name Resolution & Symbol Table
## Responsibility
- Map identifier references to their definitions
- Manage scoping (blocks, functions, modules)
- Check visibility rules
- Resolve imports/module paths

## Files
- mod.rs       : Resolver entry point
- scopes.rs    : Scope stack & symbol table
- symbol.rs    : Symbol metadata
- imports.rs   : Module resolution & imports
- visitor.rs   : AST visitor to resolve names

## Output
Annotated AST where each identifier has a resolved symbol reference
EOF

# === Compiler - Type Checker ===
mkdir_safe "$VITTE_ROOT/src/compiler/typer"
cat > "$VITTE_ROOT/src/compiler/typer/.placeholder" << 'EOF'
# Type Checker
## Responsibility
- Perform type inference (Hindley-Milner style)
- Verify type correctness of all expressions/statements
- Generate constraint equations and solve via unification
- Provide helpful type error messages

## Files
- mod.rs           : Type checker entry
- constraints.rs   : Constraint generation & representation
- unify.rs         : Unification algorithm (Robinson)
- inference.rs     : Type inference algorithm
- builtin.rs       : Built-in types & type operations
- diagnostic.rs    : Type error formatting

## Algorithm
1. Generate type constraints from AST
2. Unify constraints via Robinson's algorithm
3. Apply substitutions to infer types
4. Report conflicts as diagnostics
EOF

# === Compiler - HIR ===
mkdir_safe "$VITTE_ROOT/src/compiler/hir"
cat > "$VITTE_ROOT/src/compiler/hir/.placeholder" << 'EOF'
# High-Level Intermediate Representation
## Responsibility
- Simplified, normalized form of typed AST
- Remove syntactic sugar (desugar)
- Prepare for IR optimization passes
- More regular structure for analysis

## Files
- mod.rs       : HIR node definitions
- builder.rs   : AST → HIR lowering (desugaring)
- visitor.rs   : HIR visitor pattern
- printer.rs   : HIR debug output

## vs AST
- No operators like += (desugared to x = x + y)
- Simplified control flow (no loop/while sugar)
- All calls explicit
- More uniform structure
EOF

# === Compiler - IR ===
mkdir_safe "$VITTE_ROOT/src/compiler/ir"
cat > "$VITTE_ROOT/src/compiler/ir/.placeholder" << 'EOF'
# Low-Level Intermediate Representation
## Responsibility
- Machine-like representation (linear, explicit control flow)
- Foundation for optimization passes
- Platform-independent but close to native execution

## Files
- mod.rs            : IR node definitions
- builder.rs        : HIR → IR lowering
- const_fold.rs     : Constant folding optimization
- dce.rs            : Dead code elimination
- inlining.rs       : Function inlining
- tail_call.rs      : Tail call optimization
- visitor.rs        : IR visitor

## Structure
- Functions decomposed into basic blocks
- Values explicitly assigned to temporaries
- Control flow graph explicit
EOF

# === Compiler - Backend C ===
mkdir_safe "$VITTE_ROOT/src/compiler/backend_c"
cat > "$VITTE_ROOT/src/compiler/backend_c/.placeholder" << 'EOF'
# C17 Code Generation Backend
## Responsibility
- Emit C17-compatible code from IR
- Map runtime operations to C function calls
- Allocate C variables for values
- Manage includes/declarations

## Files
- mod.rs                 : Backend entry point
- emitter.rs            : Main code generator
- cgen_expr.rs          : Expression → C code
- cgen_stmt.rs          : Statement → C code
- names.rs              : Name mangling / C identifier mapping
- runtime_calls.rs      : Runtime function interface
- header.rs             : Generated #include's and forward decls
- c_builtins.rs         : C type representations

## Key function
fn emit_ir(ir: &Program) -> String // returns C17 code
EOF

# === Common ===
mkdir_safe "$VITTE_ROOT/src/common"
cat > "$VITTE_ROOT/src/common/.placeholder" << 'EOF'
# Common Utilities
- arena.rs      : Memory arena allocator
- hashmap.rs    : Hash table implementation
- vec.rs        : Dynamic vector / growable array
- str.rs        : String utilities
- utf8.rs       : UTF-8 validation & indexing
- fs.rs         : File system operations
- log.rs        : Logging & verbosity control
- os.rs         : OS abstractions
- path.rs       : Path manipulation (cross-platform)
EOF

# === Diagnostics ===
mkdir_safe "$VITTE_ROOT/src/diag"
cat > "$VITTE_ROOT/src/diag/.placeholder" << 'EOF'
# Diagnostics & Error Reporting
- codes.rs      : Error code enumeration
- diag.rs       : Diagnostic message structure
- source_map.rs : Source location tracking
- span.rs       : Code span (start/end)
EOF

# === Runtime ===
mkdir_safe "$VITTE_ROOT/src/runtime"
cat > "$VITTE_ROOT/src/runtime/.placeholder" << 'EOF'
# Runtime Support
- rt_alloc.c    : Memory allocation
- rt_panic.c    : Panic/exit handling
- rt_handles.c  : Pointer/handle management
- rt_slice.c    : Slice operations
- rt_string.c   : String runtime
- rt_atomics.c  : Atomic operations (if needed)
EOF

# === PAL ===
mkdir_safe "$VITTE_ROOT/src/pal/posix"
mkdir_safe "$VITTE_ROOT/src/pal/win32"
cat > "$VITTE_ROOT/src/pal/.placeholder" << 'EOF'
# Platform Abstraction Layer
- pal.rs        : PAL public interface
- fs.rs         : File system
- os.rs         : OS primitives
- mem.rs        : Memory operations
- thread.rs     : Threading (if needed)
- time.rs       : Time/clocks

posix/          : POSIX implementations
win32/          : Windows implementations
EOF

# === Tests ===
mkdir_safe "$VITTE_ROOT/tests/unit/lexer"
mkdir_safe "$VITTE_ROOT/tests/unit/parser"
mkdir_safe "$VITTE_ROOT/tests/unit/typer"
mkdir_safe "$VITTE_ROOT/tests/unit/ir_gen"
mkdir_safe "$VITTE_ROOT/tests/unit/backend_c"
mkdir_safe "$VITTE_ROOT/tests/integration/simple"
mkdir_safe "$VITTE_ROOT/tests/fixtures/valid"
mkdir_safe "$VITTE_ROOT/tests/fixtures/invalid"
mkdir_safe "$VITTE_ROOT/tests/golden"

# === Spec ===
mkdir_safe "$VITTE_ROOT/spec/type-system"
mkdir_safe "$VITTE_ROOT/spec/runtime"

# === Docs ===
mkdir_safe "$VITTE_ROOT/docs/tutorial"
mkdir_safe "$VITTE_ROOT/docs/reference"
mkdir_safe "$VITTE_ROOT/docs/examples"

echo "✅ Structure créée avec succès !"
echo ""
echo "📋 Prochaines étapes :"
echo "  1. Écrire spec/01-syntax-lexical.md (lexique)"
echo "  2. Écrire spec/02-grammar.md (grammaire EBNF)"
echo "  3. Implémenter src/compiler/lexer"
echo "  4. Implémenter src/compiler/parser"
echo "  5. Implémenter src/compiler/ast"
echo "  ... et ainsi de suite"
echo ""
echo "📖 Documentation : voir ARCHITECTURE.md"
