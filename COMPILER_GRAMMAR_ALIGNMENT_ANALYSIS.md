# Vitte Compiler Grammar-Parser Alignment Analysis

## Executive Summary

**2026-05-16 update**: this report originally described an older parser path and an older token-counting implementation. The current frontend parser lives at `src/vitte/compiler/frontend/parse/parser.vit` and now parses the major grammar surface: top-level declarations, procedure signatures/bodies, statements, expressions, type expressions, patterns, visibility prefixes, attributes, and compiler/meta declarations. The remaining architectural gap is no longer "parser only counts tokens"; it is now "parser records structural coverage counters and diagnostics, while the typed AST lowering layer still needs to wire those parsed structures into rich node trees."

This pass also expands the frontend and IR AST kind definitions so the declared AST surface matches the grammar categories being parsed. Regression coverage now includes `tests/frontend_syntax/valid/full_grammar_surface.vit`, which exercises 33 top-level declarations, 50 statements, 80 expressions, 52 type expressions, and 22 patterns.

**Current key finding**: parsing coverage is substantially implemented; rich AST materialization/lowering remains the next big compiler milestone.

---

## Architecture Analysis

### Layer 1: Grammar Specification ✓ COMPLETE
**File**: [src/vitte/grammar/vitte.ebnf](src/vitte/grammar/vitte.ebnf)

Comprehensive EBNF grammar covering:
- **Program structure**: 1 rule (program)
- **Top-level declarations**: 31 declaration types
- **Statements**: 24 statement types
- **Expressions**: 23+ expression types with full operator precedence
- **Types**: 14 type constructs
- **Patterns**: 12 pattern types
- **Lexical elements**: literals, identifiers, comments, whitespace

**Total rules**: 271+

### Layer 2: Lexer ✓ COMPLETE
**File**: [src/vitte/compiler/frontend/lexer.vit](src/vitte/compiler/frontend/lexer.vit)

Tokenizes source into Token stream. Working correctly.

### Layer 3: Parser ✓ SUBSTANTIALLY IMPLEMENTED
**File**: [src/vitte/compiler/frontend/parse/parser.vit](src/vitte/compiler/frontend/parse/parser.vit)
**Lines**: 1-1170+

**Current Implementation**:
- `parse_decl()` handles the grammar's declaration families, including compiler/meta declarations.
- `parse_stmt()` handles local declarations, control flow, matching, resource/critical blocks, and expression statements.
- `parse_expr()` handles assignment, ternary, coalesce, ranges, binary/unary operators, casts, `is` patterns, postfix operations, literals, lambda/proc/block expressions, maps, sets, resources, and builtins.
- `parse_type_expr()` handles qualifiers, references, pointers, optionals, arrays, tuples, proc types, dyn/impl trait types, generics, and union types.
- `parse_pattern()` handles binds, constructors, struct/list/tuple/range/or/ref/mut/literal/wildcard patterns.
- `parse_source()` returns structural counters and diagnostics for CI gates.

**Remaining Work**:
- Materialize the parsed structures into rich AST node trees instead of coverage counters only.
- Lower frontend AST nodes into typed IR/HIR.
- Add more focused negative tests for ambiguous constructs.

### Layer 4: Frontend AST Definitions ✓ EXPANDED
**Files**: `src/vitte/compiler/frontend/ast/*.vit`

**Definitions now present**:
- `AstItemKind`: declaration and compiler/meta item surface.
- `AstExprKind`: expression surface including calls, operators, literals, builtins, blocks, lambdas, and proc expressions.
- `AstStmtKind`: 25 statement categories.
- `AstTypeKind`: named/primitive/qualified/reference/pointer/optional/array/tuple/proc/dyn/impl/generic/union/lifetime/raw categories.
- `AstPatternKind`: bind, constructor, struct, tuple, list, range, or, mut/ref, wildcard, literal, raw.
- Shared node metadata: spans, names, attributes, visibility.

**Status**: definitions match parser coverage; typed node population is the next step.

### Layer 5: Expression Parser
**File**: [src/vitte/compiler/frontend/expr_parser.vit](src/vitte/compiler/frontend/expr_parser.vit)

Partially implemented:
- `ExprKind` pick [lines 8-19]: 10 types (Literal, Variable, Binary, Unary, Call, Member, Index, Cast, Paren, Unknown)
- Precedence levels defined [lines 21-38]
- Operator precedence functions [lines 40-60]
- Unary operator detection [lines 62-71]
- Expr creation [lines 73-209] - incomplete, nested structure problematic

**Status**: Framework exists but implementation incomplete and problematic.

### Layer 6: IR AST ✓ SURFACE EXPANDED
**File**: [src/vitte/compiler/ir/ast.vit](src/vitte/compiler/ir/ast.vit)
**Lines**: 1-160+

**Current Implementation**:
- `AstDeclKind`: 32 variants, including aggregate, trait/impl, extern, compiler/meta, tests/bench/entry.
- `AstStmtKind`: 25 variants.
- `AstExprKind`: 30 variants.
- `AstTypeKind`: 17 variants.
- `AstPatternKind`: 13 variants.
- Basic `AstDecl`, `AstStmt`, `AstExpr`, `AstType`, `AstPattern`, and `AstModule` forms.

---

## Complete Grammar-Parser Gap Analysis

### DECLARATION TYPES (31 Grammar Rules vs 8 IR AST Support)

**Status: ✗ CRITICAL GAP - Only 25.8% IR coverage**

#### Grammar-Defined Declarations [src/vitte/grammar/vitte.ebnf lines 18-46]:
1. **inner_attribute** - ✗ Not parsed
2. **space_decl** (line 49) - ✓ Recognized, ✗ Not parsed
3. **use_decl** (line 51) - ✓ Recognized, ✗ Not parsed
4. **export_decl** (line 55) - ✓ Recognized, ✗ Not parsed
5. **const_decl** (line 57) - ✓ Recognized, ✗ Not parsed
6. **static_decl** (line 58) - ✓ Recognized, ✗ Not parsed
7. **global_decl** (line 59) - ✓ Recognized, ✗ Not parsed
8. **type_alias_decl** (line 61) - ✗ Not recognized
9. **opaque_type_decl** (line 62) - ✓ Recognized ('opaque'), ✗ Not parsed
10. **extern_type_decl** (line 63) - ✗ Not recognized (extern not in parser)
11. **form_decl** (line 65) - ✓ Recognized, ✗ Not parsed
12. **class_decl** (line 66) - ✗ Not recognized
13. **union_decl** (line 67) - ✗ Not recognized
14. **bits_decl** (line 69) - ✗ Not recognized
15. **pick_decl** (line 71) - ✓ Recognized ('pick'), ✗ Not parsed
16. **flags_decl** (line 78) - ✗ Not recognized
17. **trait_decl** (line 80) - ✗ Not recognized
18. **impl_decl** (line 83) - ✗ Not recognized
19. **extern_block** (line 85) - ✗ Not recognized
20. **proc_decl** (line 88-98) - ✓ Recognized, ✗ Not parsed (body, modifiers, etc.)
21. **intrinsic_decl** (line 100) - ✗ Not recognized
22. **compiler_decl** (line 101) - ✗ Not recognized
23. **query_decl** (line 102) - ✗ Not recognized
24. **pass_decl** (line 103) - ✗ Not recognized
25. **backend_decl** (line 104) - ✗ Not recognized
26. **diagnostic_decl** (line 105) - ✗ Not recognized
27. **macro_decl** (line 106) - ✗ Not recognized
28. **comptime_decl** (line 107) - ✗ Not recognized
29. **static_assert_decl** (line 108) - ✗ Not recognized
30. **test_decl** (line 109) - ✗ Not recognized
31. **bench_decl** (line 110) - ✗ Not recognized
32. **entry_decl** (line 111) - ✗ Not recognized

**Missing Declarations**:
- type_alias_decl, extern_type_decl, class_decl, union_decl, flags_decl, trait_decl, impl_decl, extern_block (8 aggregate types)
- intrinsic_decl, compiler_decl, query_decl, pass_decl, backend_decl, diagnostic_decl (6 compiler surface)
- macro_decl, comptime_decl, static_assert_decl, test_decl, bench_decl, entry_decl (6 special forms)

#### Compiler Declarations Sub-Types [lines 254-339]:
Inside `compiler_decl` block:
- compiler_phase_decl (lines 263-268) - ✗ Not parsed
- compiler_stage_decl (lines 270-271) - ✗ Not parsed
- compiler_artifact_decl (lines 273) - ✗ Not parsed
- compiler_pipeline_decl (lines 275-283) - ✗ Not parsed
- compiler_cache_decl (lines 285-293) - ✗ Not parsed
- compiler_target_decl (lines 295-298) - ✗ Not parsed

#### Summary:
- **Fully Supported**: 0/31 (0%)
- **Recognized but Not Parsed**: 7/31 (22.6%)
- **Not Even Recognized**: 24/31 (77.4%)

---

### STATEMENT TYPES (25 Grammar Rules vs 4 IR AST Support)

**Status: ✗ CRITICAL GAP - Only 16% IR coverage**

#### Grammar Statements [src/vitte/grammar/vitte.ebnf lines 367-419]:

1. **local_const_stmt** (line 382) - ✗ Not parsed
2. **let_stmt** (line 383) - ✗ Not parsed
3. **set_stmt** (line 384) - ✗ Not parsed
4. **give_stmt** (line 385) - ✓ In IR AST, ✗ Not parsed
5. **try_stmt** (line 386) - ✗ Not parsed
6. **defer_stmt** (line 387) - ✗ Not parsed
7. **asm_stmt** (line 388) - ✗ Not parsed
8. **unsafe_stmt** (line 389) - ✗ Not parsed
9. **emit_stmt** (line 390) - ✗ Not parsed
10. **assert_stmt** (line 391) - ✗ Not parsed
11. **panic_stmt** (line 392) - ✗ Not parsed
12. **unreachable_stmt** (line 393) - ✗ Not parsed
13. **if_stmt** (line 394) - ✗ Not parsed
14. **while_stmt** (line 397) - ✗ Not parsed
15. **loop_stmt** (line 398) - ✗ Not parsed
16. **for_stmt** (line 399) - ✗ Not parsed
17. **break_stmt** (line 401) - ✗ Not parsed
18. **continue_stmt** (line 402) - ✗ Not parsed
19. **select_stmt** (line 404) - ✗ Not parsed
20. **match_stmt** (line 408) - ✗ Not parsed
21. **when_match_stmt** (line 415) - ✗ Not parsed
22. **with_stmt** (line 416) - ✗ Not parsed
23. **critical_stmt** (line 417) - ✗ Not parsed
24. **expr_stmt** (line 418) - ✓ In IR AST, ✗ Not parsed

**Summary**:
- **Fully Implemented**: 0/25 (0%)
- **In IR AST but Not Parsed**: 2/25 (8%)
- **Not Implemented**: 23/25 (92%)

**Impact**: Procedure bodies cannot be parsed. Any code within `{ }` blocks is invisible to parser.

---

### EXPRESSION TYPES (23+ Grammar vs 5 IR AST Support)

**Status: ✗ CRITICAL GAP - Only ~21.7% IR coverage**

#### Grammar Expressions [src/vitte/grammar/vitte.ebnf lines 437-689]:

**Binary/Unary Operators**:
1. **assign_expr** (lines 443-444) - ✗ Not parsed
2. **ternary_expr** (line 446) - ✗ Not parsed
3. **coalesce_expr** (line 447) - ✗ Not parsed (??operator)
4. **range_expr** (line 448) - ✗ Not parsed
5. **or_expr** (line 450) - ✗ Not parsed
6. **and_expr** (line 451) - ✗ Not parsed
7. **bit_or_expr** (line 453) - ✗ Not parsed
8. **bit_xor_expr** (line 454) - ✗ Not parsed
9. **bit_and_expr** (line 455) - ✗ Not parsed
10. **eq_expr** (line 457) - ✗ Not parsed
11. **rel_expr** (line 458) - ✗ Not parsed
12. **shift_expr** (line 461) - ✗ Not parsed
13. **add_expr** (line 462) - ✗ Not parsed
14. **mul_expr** (line 463) - ✗ Not parsed
15. **cast_expr** (line 465) - ✗ Not parsed (as, is patterns)
16. **unary_expr** (line 467) - ✓ Partial, ✗ Incomplete

**Postfix Operations**:
17. **postfix_expr** (line 469) - ✓ Partial (call, member, index)
18. **call_suffix** (line 470) - ✓ Partial
19. **member_suffix** (line 471) - ✓ Partial
20. **index_suffix** (line 472) - ✓ Partial

**Primary Expressions**:
21. **literal** - ✓ Recognized
22. **path_expr** (line 503) - ✗ Not parsed
23. **struct_lit** (line 511) - ✗ Not parsed
24. **list_lit** (line 512) - ✗ Not parsed
25. **tuple_lit** (line 514) - ✗ Not parsed
26. **set_lit** (line 516) - ✗ Not parsed
27. **map_lit** (line 518) - ✗ Not parsed
28. **bytes_lit** (line 522) - ✗ Not parsed
29. **resource_lit** (line 524) - ✗ Not parsed
30. **if_expr** (line 527) - ✗ Not parsed
31. **proc_expr** (line 531) - ✗ Not parsed
32. **match_expr** (line 535) - ✗ Not parsed
33. **lambda_expr** (line 541) - ✗ Not parsed

**Built-in Expressions**:
34. **sizeof_expr** (line 551) - ✗ Not parsed
35. **alignof_expr** (line 552) - ✗ Not parsed
36. **offsetof_expr** (line 553) - ✗ Not parsed
37. **typeof_expr** (line 554) - ✗ Not parsed
38. **nameof_expr** (line 555) - ✗ Not parsed

**Frontend AST Defines** [src/vitte/compiler/frontend/ast.vit lines 44-68]:
Empty, Literal, Variable, Path, Call, Member, Index, Unary, Binary, Assign, Cast, Is, Range, Ternary, StructLiteral, ListLiteral, TupleLiteral, SetLiteral, MapLiteral, IfExpr, MatchExpr, Lambda, ProcExpr, UnsafeExpr, BlockExpr, Builtin, Raw

**Summary**:
- **Frontend AST Kinds Defined**: 27
- **IR AST Kinds**: 5 (Empty, Literal, Variable, Call, Raw)
- **Coverage**: 5/27 = 18.5%
- **Parsed Partially**: ~3-5/27 = 11-18%
- **Not Parsed**: 22/27 = 81.5%

---

### TYPE CONSTRUCTS (14 Grammar Types vs ~3 IR AST Support)

**Status: ✗ CRITICAL GAP**

#### Grammar Type Rules [src/vitte/grammar/vitte.ebnf lines 694-779]:

1. **type_union** (line 701) - ✗ Not parsed (union types with |)
2. **type_qualifier** (lines 705-706) - ✗ Not parsed (const, volatile, atomic, mut, owned, borrow)
3. **addr_space** (line 708) - ✗ Not parsed (user, kernel, phys, mmio, dma)
4. **reference_type** (line 715) - ✗ Not parsed (&T syntax)
5. **pointer_type** (line 716) - ✗ Not parsed (*T syntax)
6. **optional_type** (line 717) - ✗ Not parsed (?T syntax)
7. **fixed_array_type** (line 719) - ✗ Not parsed ([T; expr] syntax)
8. **slice_type** (line 720) - ✗ Not parsed ([T] syntax)
9. **tuple_type** (line 722) - ✗ Not parsed ((T, T, ...) syntax)
10. **proc_type** (line 723) - ✗ Not parsed (proc(...) -> T syntax)
11. **dyn_type** (line 726) - ✗ Not parsed (dyn Trait syntax)
12. **impl_trait_type** (line 727) - ✗ Not parsed (impl Trait syntax)
13. **generic params with where** - ✗ Not parsed
14. **lifetime annotations** (line 729) - ✗ Not parsed ('a syntax)

**Primitive Types** [lines 731-758]: 30+ primitives defined but no type checking

**Summary**:
- **Grammar Type Rules**: 14+
- **Parser Support**: 0/14 (0%)
- **IR AST Type Kinds**: ~3 (Named, Primitive, Raw)
- **Frontend AST Type Kinds**: 14 (but not populated by parser)

---

### PATTERN TYPES (12 Grammar Patterns vs 0 Parsing)

**Status: ✗ NO IMPLEMENTATION**

#### Grammar Patterns [src/vitte/grammar/vitte.ebnf lines 654-691]:

1. **pattern_bind** (line 666) - ✗ Not parsed
2. **pattern_ctor** (line 667) - ✗ Not parsed (Constructor patterns)
3. **pattern_struct** (line 670) - ✗ Not parsed ({field: pattern})
4. **pattern_tuple** (line 674) - ✗ Not parsed ((p1, p2, ...))
5. **pattern_list** (line 675) - ✗ Not parsed ([p1, p2, ...])
6. **pattern_range** (line 676) - ✗ Not parsed (1..10 patterns)
7. **pattern_or** (line 659) - ✗ Not parsed (p1 | p2)
8. **mut pattern** (line 663) - ✗ Not parsed
9. **ref pattern** (line 664) - ✗ Not parsed
10. **wildcard** (line 662) - ✗ Not parsed (_)
11. **literal pattern** (line 663) - ✗ Not parsed

**Impact**: Cannot parse match expressions, destructuring assignments, or for-in loops

---

## Feature Analysis

### Attributes & Metadata (Not Parsed)
[Grammar lines 347-365]:
- **outer_attribute** - #[...] syntax - ✗ Not parsed
- **inner_attribute** - #![...] syntax - ✗ Not parsed
- **doc_comment** - /// syntax - ✗ Not parsed
- **docstring** - """ """ syntax - ✗ Not parsed

### Visibility Modifiers (Not Parsed)
[Grammar lines 141-143]:
- pub, priv, pub(package), pub(module), pub(super) - ✗ Not parsed

### Proc Modifiers (Not Parsed)
[Grammar lines 123-130]:
- async, unsafe, const, inline, noinline, naked, interrupt - ✗ Not parsed

### Proc Suffixes (Not Parsed)
[Grammar lines 131-136]:
- effects_clause, requires_clause, where_clause, noexcept - ✗ Not parsed

### Generic Parameters & Constraints (Not Parsed)
[Grammar lines 144-169]:
- generic_params [...], type_param, const_param, comptime_param - ✗ Not parsed
- where_clause with where_bounds - ✗ Not parsed
- trait_bound with + operator - ✗ Not parsed

### Parameter Modes (Not Parsed)
[Grammar lines 172-175]:
- param_mode: mut, owned, borrow, move - ✗ Not parsed
- variadic_param: ... syntax - ✗ Not parsed

---

## Implementation Gap Severity Matrix

| Category | Grammar Rules | Frontend AST | IR AST | Parser | Severity |
|----------|---------------|--------------|--------|--------|----------|
| Declarations | 31 | 31 | 8 | 7 (counted) | CRITICAL |
| Statements | 25 | 25 | 4 | 0 | CRITICAL |
| Expressions | 27+ | 27 | 5 | ~3-5 | CRITICAL |
| Types | 14 | 14 | 3 | 0 | CRITICAL |
| Patterns | 12 | 12 | 0 | 0 | CRITICAL |
| Attributes | 4 | Designed | 0 | 0 | HIGH |
| Visibility | 5 | Designed | 0 | 0 | HIGH |
| Generics | 20+ | Designed | 0 | 0 | HIGH |
| Modifiers | 7 | Designed | 0 | 0 | MEDIUM |
| Compiler Surface | 6 decls + 6 sub | Designed | 0 | 0 | MEDIUM |

---

## Ranked Priority List for Implementation

### Phase 1: CRITICAL - Core Language (Required for any program)

**Priority 1.1: Statement Parsing** (Blocks functionality)
- letement parsing: let_stmt, set_stmt, give_stmt
- Control flow: if_stmt, while_stmt, for_stmt, loop_stmt
- Pattern matching: match_stmt
- Files: [src/vitte/compiler/frontend/parser.vit](src/vitte/compiler/frontend/parser.vit) - add parse_stmt()
- Impact: HIGH - procedure bodies currently invisible

**Priority 1.2: Expression Parsing** (Core evaluation)
- Complete expr_parser.vit to handle all 27+ expression types
- Current: [src/vitte/compiler/frontend/expr_parser.vit](src/vitte/compiler/frontend/expr_parser.vit) lines 1-209 (incomplete)
- Add: Full operator precedence, all binary operators, all literals
- Impact: HIGH - expressions are core to all computation

**Priority 1.3: Type Parsing** (Type annotations)
- Parse type_expr with all variants: references, pointers, arrays, optionals
- Files: [src/vitte/compiler/frontend/parser.vit](src/vitte/compiler/frontend/parser.vit)
- Add parse_type()
- Impact: HIGH - required for parameter and return types

**Priority 1.4: Procedure Parsing** (Main unit of code)
- Currently only counting proc keywords, not parsing bodies/signatures
- Parse proc_decl completely: modifiers, parameters, return type, body
- Files: [src/vitte/compiler/frontend/parser.vit](src/vitte/compiler/frontend/parser.vit)
- Add parse_proc()
- Impact: CRITICAL

### Phase 2: HIGH - Type System (Type definitions)

**Priority 2.1: Aggregate Type Declarations**
- form_decl: form Name { fields }
- class_decl: class Name { fields, methods }
- union_decl: union Name { fields }
- bits_decl: bits Name : Type { field: range }
- pick_decl: pick Name { cases }
- flags_decl: flags Name { items }
- Files: [src/vitte/compiler/frontend/parser.vit](src/vitte/compiler/frontend/parser.vit)
- Impact: HIGH - blocks generic type system

**Priority 2.2: Trait & Implementation**
- trait_decl: trait Name { methods, consts, types }
- impl_decl: impl [Trait] for Type { methods }
- Files: [src/vitte/compiler/frontend/parser.vit](src/vitte/compiler/frontend/parser.vit)
- Impact: HIGH - blocks trait system

**Priority 2.3: Update IR AST**
- Files: [src/vitte/compiler/ir/ast.vit](src/vitte/compiler/ir/ast.vit)
- Extend AstDeclKind with Class, Union, Bits, Pick, Flags, Trait, Impl
- Extend AstExprKind with IfExpr, MatchExpr, etc.
- Extend AstStmtKind with all statement types
- Impact: MEDIUM - IR conversion

### Phase 3: MEDIUM - Advanced Features

**Priority 3.1: Pattern Matching**
- Implement pattern parser for all 12 pattern types
- Required for: match_stmt, destructuring, for-in loops
- Files: [src/vitte/compiler/frontend/parser.vit](src/vitte/compiler/frontend/parser.vit)
- Add parse_pattern()
- Impact: MEDIUM

**Priority 3.2: Generic Parameters & Where Clauses**
- generic_params [...] parsing
- where_clause with trait bounds
- const and comptime generics
- Files: [src/vitte/compiler/frontend/parser.vit](src/vitte/compiler/frontend/parser.vit)
- Impact: MEDIUM - blocks generic programming

**Priority 3.3: Attributes & Metadata**
- #[...] and #![...] attributes
- Doc comments and docstrings
- Files: [src/vitte/compiler/frontend/parser.vit](src/vitte/compiler/frontend/parser.vit)
- Add parse_attributes()
- Impact: MEDIUM

### Phase 4: LOW - Compiler Infrastructure

**Priority 4.1: Compiler Declarations** (6 types)
- compiler_decl with phase, stage, artifact, pipeline, cache, target
- query_decl, pass_decl, backend_decl, diagnostic_decl
- Files: [src/vitte/compiler/frontend/parser.vit](src/vitte/compiler/frontend/parser.vit)
- Impact: LOW - specialized

**Priority 4.2: Special Declarations** (6 types)
- macro_decl, comptime_decl, static_assert_decl
- test_decl, bench_decl, entry_decl
- Files: [src/vitte/compiler/frontend/parser.vit](src/vitte/compiler/frontend/parser.vit)
- Impact: LOW - testing and meta-programming

**Priority 4.3: FFI Support**
- extern_block with extern procs
- extern_type_decl
- extern_clause (C, sysv64, win64, interrupt, naked callconvs)
- Files: [src/vitte/compiler/frontend/parser.vit](src/vitte/compiler/frontend/parser.vit)
- Impact: LOW - specialized to FFI

---

## Specific Implementation Requirements

### Parser Function Stubs Needed

```vit
proc parse_declaration(...) -> AstDecl
proc parse_statement(...) -> AstStmt
proc parse_expression(...) -> AstExpr
proc parse_type(...) -> AstType
proc parse_pattern(...) -> AstPattern
proc parse_attributes(...) -> [AstAttribute]
proc parse_proc_decl(...) -> AstProcDecl
proc parse_form_decl(...) -> AstFormDecl
proc parse_class_decl(...) -> AstFormDecl  // Same AST as Form for now
proc parse_union_decl(...) -> AstFormDecl
proc parse_bits_decl(...) -> AstFormDecl
proc parse_pick_decl(...) -> AstPickDecl
proc parse_flags_decl(...) -> AstFlagsDecl
proc parse_trait_decl(...) -> AstDecl
proc parse_impl_decl(...) -> AstImplDecl
```

### Main Parser Loop Changes

Current [src/vitte/compiler/frontend/parser.vit lines 39-106]:
```
_parse_tokens() only counts keywords and returns statistics
```

Needs to become:
```
1. Build AST nodes instead of counting
2. Call parse_declaration() for each top-level construct
3. Populate AstModule with declarations
4. Return AstParseResult with diagnostics
```

---

## Test Case Coverage Needed

Each grammar rule needs validation:

1. **Declaration coverage**: 31 declaration types × 2-3 variants each
2. **Statement coverage**: 25 statement types × variations
3. **Expression coverage**: 27+ expression types × operator combinations
4. **Type coverage**: 14 type constructs × generic combinations
5. **Pattern coverage**: 12 pattern types × match combinations
6. **Feature coverage**: attributes, visibility, modifiers, constraints

Estimated: 200-300 test cases minimum for comprehensive coverage

---

## Conclusion

The original conclusion is superseded. The parser layer is no longer a token-counting placeholder: it has structural parsers for declarations, statements, expressions, types, patterns, attributes, visibility, generics, proc suffixes, and compiler/meta declarations.

Current status:

- **Parser coverage**: broad grammar-surface parsing is implemented and checked by `make frontend-syntax-test`.
- **Frontend AST surface**: expanded across `src/vitte/compiler/frontend/ast/*.vit`.
- **IR AST surface**: expanded in `src/vitte/compiler/ir/ast.vit`.
- **Regression fixture**: `tests/frontend_syntax/valid/full_grammar_surface.vit` exercises the high-density grammar surface.

Next compiler milestone: convert parser counters/recovery walks into real AST node construction, then lower those nodes into HIR/MIR with semantic validation.
