# Compiler-Grammar Alignment - Implementation Roadmap

## Overview
This document tracks the alignment of `src/vitte/compiler` with `grammar/vitte.ebnf`. The goal is to ensure the parser can parse all language constructs defined in the grammar.

**Status**: Foundation phase complete, full implementation in progress

---

## What Was Created

### 1. Extended AST Module (`ast_extended.vit`)
- **Coverage**: All 31 declaration kinds, 25 statement kinds, 27+ expression kinds
- **Features**:
  - Complete type system representation (primitives, references, pointers, generics)
  - Pattern matching support (binding, constructor, struct, tuple, etc.)
  - Generic parameters and where clauses
  - Attributes and modifiers
  - Full statement and expression structure

### 2. Extended Parser Module (`parser_extended.vit`)
- **Declarations**: All 31 declaration types have parsing functions (though many are stubs)
- **Parser State**: Token navigation, lookahead, error collection
- **Functions**: 31 declaration parsers + utility functions
- **Status**: Ready for implementation phase

### 3. Grammar Alignment Checker (`grammar_alignment_checker.vit`)
- **Tracking**: Status for 55+ grammar rules across all categories
- **Statuses**: NotStarted, Partial, Complete, Tested
- **Reports**: Detailed alignment reports showing coverage gaps

---

## Implementation Roadmap - What's Next

### Phase 1: Complete Declaration Parsing ⚠️ (HIGH PRIORITY)
**Target Coverage**: 31 declaration types

| Declaration | Status | Priority | Blockers |
|---|---|---|---|
| space_decl | ✅ Partial | CRITICAL | None |
| use_decl | ✅ Partial | CRITICAL | use_group, use_glob, import_as |
| export_decl | ❌ Not Started | HIGH | All |
| const_decl | ✅ Partial | CRITICAL | Type parsing, expr parsing |
| static_decl | ✅ Partial | HIGH | Type parsing, expr parsing |
| global_decl | ✅ Partial | HIGH | Type parsing, expr parsing |
| type_alias_decl | ❌ Not Started | HIGH | Generic params, type parsing |
| **Aggregate Types** |
| form_decl | ✅ Partial | CRITICAL | Field parsing, generics, where |
| class_decl | ✅ Partial | CRITICAL | Field parsing, methods |
| union_decl | ✅ Partial | HIGH | Field parsing |
| bits_decl | ❌ Not Started | MEDIUM | Bitfield parsing |
| pick_decl | ❌ Not Started | MEDIUM | Case parsing, patterns |
| flags_decl | ❌ Not Started | MEDIUM | Flag list parsing |
| **Traits and Impl** |
| trait_decl | ✅ Partial | HIGH | Item parsing, signatures |
| impl_decl | ✅ Partial | HIGH | impl_head, trait parsing |
| **Procedures** |
| proc_decl | ✅ Partial | CRITICAL | Param parsing, body parsing |
| intrinsic_decl | ❌ Not Started | MEDIUM | Param parsing |
| **Compiler Surface** |
| compiler_decl | ✅ Partial | LOW | compiler_items |
| query_decl | ❌ Not Started | LOW | All |
| pass_decl | ❌ Not Started | LOW | All |
| backend_decl | ❌ Not Started | LOW | All |
| diagnostic_decl | ❌ Not Started | LOW | All |
| **Others** |
| extern_block | ❌ Not Started | MEDIUM | ABI, extern_item |
| macro_decl | ❌ Not Started | MEDIUM | Macro params |
| comptime_decl | ❌ Not Started | LOW | Block parsing |
| static_assert_decl | ❌ Not Started | LOW | Expr parsing |
| test_decl | ❌ Not Started | MEDIUM | Block parsing |
| bench_decl | ❌ Not Started | MEDIUM | Block parsing |
| entry_decl | ❌ Not Started | LOW | Module path, block |

### Phase 2: Statement Parsing (25 types)

**Currently**: 0% implemented (no statement parsing)

| Statement | Grammar Rule | Needed For |
|---|---|---|
| let_stmt | `let [mut] PATTERN: TYPE = EXPR` | Variable binding |
| set_stmt | `set TARGET OP EXPR` | Variable assignment |
| give_stmt | `give [EXPR]` | Return values |
| if_stmt | `if COND { } [elif] [else]` | Control flow |
| while_stmt | `while COND { }` | Loops |
| for_stmt | `for PATTERN in EXPR { }` | Iteration |
| loop_stmt | `loop { }` | Infinite loops |
| match_stmt | `match EXPR { case PATTERN => }` | Pattern matching |
| select_stmt | `select EXPR { when PATTERN { } }` | Channel ops |
| break_stmt | `break [EXPR]` | Loop control |
| continue_stmt | `continue` | Loop control |
| try_stmt | `try EXPR` | Error handling |
| defer_stmt | `defer { }` | Cleanup |
| unsafe_stmt | `unsafe { }` | Unsafe block |
| emit_stmt | `emit EXPR` | Signal emission |
| assert_stmt | `assert EXPR [, MSG]` | Assertions |
| panic_stmt | `panic EXPR` | Fatal errors |
| unreachable_stmt | `unreachable` | Impossible paths |
| asm_stmt | `asm(ARGS)` | Assembly |
| with_stmt | `with EXPR [as PATTERN] { }` | Resource |
| critical_stmt | `critical { }` | Critical section |
| when_match_stmt | `when EXPR is PATTERN { }` | Guard |
| local_const_stmt | `const NAME = EXPR` | Local constants |
| expr_stmt | `EXPR;` | Bare expression |

### Phase 3: Expression Parsing (27+ types)

**Currently**: ~10% implemented

Needed:
- Binary operators (arithmetic, logical, bitwise)
- Unary operators (-, !, ~, *, &, not, move, await)
- Assignments and all compound assignments
- Ternary, coalesce, range operators
- Function calls, method calls, member access
- Indexing and slicing
- Type casts, pattern matching in expressions
- Collection literals (tuple, list, set, map, struct)
- Control flow expressions (if, match, blocks)
- Lambda and closure expressions
- Builtin functions (sizeof, alignof, typeof, etc.)

### Phase 4: Type Parsing (14 types)

**Currently**: 0% implemented

Needed:
- Primitive types (int, bool, string, etc.)
- Named types (user-defined)
- References `&Type`
- Pointers `*Type`
- Arrays `[Type; size]`
- Slices `[Type]`
- Tuples `(Type, Type, ...)`
- Proc types `proc(...) -> Type`
- Dynamic/impl trait types `dyn Trait`, `impl Trait`
- Generic instantiation `Type[T, U]`
- Lifetime parsing `'lifetime`
- Type qualifiers (const, volatile, mut, owned, borrow)

### Phase 5: Pattern Parsing (12 types)

**Currently**: 0% implemented

Needed:
- Literal patterns
- Binding patterns (identifiers)
- Wildcard `_`
- Constructor patterns `Variant(...)`
- Struct patterns `Struct {field: pattern}`
- Tuple patterns `(p1, p2)`
- List patterns `[p1, p2]`
- Range patterns `1..10`
- Or patterns `p1 | p2`
- Reference patterns `ref pattern`
- Mutable patterns `mut pattern`

### Phase 6: Supporting Structures

Needed:
- Generic parameter parsing with bounds
- Where clause parsing
- Attribute and visibility parsing
- Procedure modifiers and suffixes
- Assignment operators
- Field declarations with defaults
- Trait bounds and implementations

---

## Implementation Strategy

### Approach
1. **Bottom-up**: Parse primitives first (literals, identifiers, operators)
2. **Build up**: Combine into expressions, then statements
3. **Top-down**: Declarations use the lower levels
4. **Iterative**: Each phase has tests before moving to the next

### Integration Points

1. **Main Parser** (`src/vitte/compiler/frontend/parser.vit`)
   - Update to use `parser_extended` functions
   - Replace token counting with real parsing
   - Integrate diagnostics

2. **IR Pipeline** (`src/vitte/compiler/ir/mod.vit`)
   - Accept extended AST from parser
   - Lower to HIR
   - Continue to MIR and backend

3. **Tests** (`src/vitte/compiler/frontend/tests/`)
   - Add tests for each parser function
   - Add integration tests
   - Add grammar compliance tests

### Success Criteria

For each grammar rule:
- ✅ Parser function exists
- ✅ Builds correct AST node
- ✅ Handles all sub-rules
- ✅ Recovers from errors
- ✅ Has passing tests

---

## Quick Start - Next Immediate Action

```vit
// Priority 1: Implement full statement parsing
proc _parse_let_stmt(state: ParserState) -> (ParserState, Stmt, bool)
proc _parse_set_stmt(state: ParserState) -> (ParserState, Stmt, bool)
proc _parse_give_stmt(state: ParserState) -> (ParserState, Stmt, bool)
proc _parse_if_stmt(state: ParserState) -> (ParserState, Stmt, bool)
// ... continue for all 25 statement types

// Priority 2: Implement expression parsing
proc _parse_expr(state: ParserState) -> (ParserState, Expr, bool)
proc _parse_binary_expr(state: ParserState, min_prec: int) -> (ParserState, Expr, bool)
proc _parse_unary_expr(state: ParserState) -> (ParserState, Expr, bool)
proc _parse_primary_expr(state: ParserState) -> (ParserState, Expr, bool)
// ... postfix operators, calls, etc.

// Priority 3: Implement type parsing
proc _parse_type_expr(state: ParserState) -> (ParserState, TypeExpr, bool)
proc _parse_type_primary(state: ParserState) -> (ParserState, TypeExpr, bool)
proc _parse_reference_type(state: ParserState) -> (ParserState, TypeExpr, bool)
// ... continue for all type kinds

// Priority 4: Implement pattern parsing
proc _parse_pattern(state: ParserState) -> (ParserState, Pattern, bool)
proc _parse_pattern_atom(state: ParserState) -> (ParserState, Pattern, bool)
// ... continue for all pattern types
```

---

## Files to Update

1. **Main Parser** (`parser.vit`)
   - Integrate `parser_extended` module
   - Replace token counting with AST building
   - Add comprehensive error handling

2. **Frontend** (`src/vitte/compiler/frontend/mod.vit`)
   - Export extended AST and parser
   - Update pipeline to use new parser

3. **Tests** (`tests/core_semantic/parser/`)
   - Add new test files for each phase
   - Coverage for happy path and error cases

4. **Documentation** (`docs/grammar/`)
   - Update alignment tracker
   - Document implemented vs. missing features

---

## Estimated Timeline

- **Phase 1** (Declarations): 4-6 hours
- **Phase 2** (Statements): 6-8 hours
- **Phase 3** (Expressions): 8-10 hours
- **Phase 4** (Types): 4-5 hours
- **Phase 5** (Patterns): 4-5 hours
- **Phase 6** (Support): 2-3 hours
- **Testing & Integration**: 5-8 hours

**Total**: 33-45 hours of implementation work

---

## Related Documents

- `src/vitte/grammar/vitte.ebnf` - Grammar definition
- `src/vitte/compiler/frontend/ast_extended.vit` - Extended AST types
- `src/vitte/compiler/frontend/parser_extended.vit` - Parser skeleton
- `src/vitte/compiler/frontend/grammar_alignment_checker.vit` - Alignment tracking
