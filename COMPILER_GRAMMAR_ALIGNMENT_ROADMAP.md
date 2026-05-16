# Compiler-Grammar Alignment - Implementation Roadmap

## Current Status

The alignment work now targets the real compiler surface, not detached `*_extended.vit` placeholder modules.

Real implementation paths:

- `src/vitte/compiler/frontend/ast/*.vit` - frontend AST surface definitions.
- `src/vitte/compiler/frontend/parse/parser.vit` - grammar-surface parser and recovery logic.
- `src/vitte/compiler/frontend/grammar_alignment_checker.vit` - Vitte-side alignment metadata.
- `src/vitte/compiler/ir/ast.vit` - IR AST surface definitions.
- `tools/grammar_alignment_checker.py` - executable CI checker.
- `tests/frontend_syntax/valid/full_grammar_surface.vit` - high-density grammar fixture.

Validation command:

```sh
make grammar-alignment-test
```

Current checker result:

| Category | Complete | Tested |
|---|---:|---:|
| Declarations | 32/32 | 32/32 |
| Statements | 24/24 | 24/24 |
| Expressions | 24/24 | 24/24 |
| Types | 14/14 | 14/14 |
| Patterns | 11/11 | 11/11 |

The old roadmap said statement/type/pattern parsing was not started. That is now obsolete: the parser has structural coverage for declarations, statements, expressions, types, patterns, attributes, visibility, generics, proc suffixes, and compiler/meta declarations.

## Implemented Surface

### 1. AST Surface

Implemented in `src/vitte/compiler/frontend/ast/*.vit` and mirrored in `src/vitte/compiler/ir/ast.vit`.

Coverage:

- Declaration kinds: space, use, export, const/static/global, type/opaque/extern, form/class/union/bits/pick/flags, trait/impl, proc/intrinsic/query, compiler/pass/backend/diagnostic/macro, comptime/static_assert/test/bench/entry.
- Statement kinds: local const, let, set, give, try, defer, asm, unsafe, emit, assert, panic, unreachable, if, while, loop, for, break, continue, select, match, when-match, with, critical, expr.
- Expression kinds: literals, variables/paths, calls, members, indexes, unary/binary/assign/cast/is/range/ternary, struct/list/tuple/set/map/bytes/resource literals, if/match/lambda/proc/unsafe/block/builtin/raw.
- Type kinds: named, primitive, qualified, reference, pointer, optional, fixed array, slice, tuple, proc, dyn, impl trait, generic, union, lifetime, raw.
- Pattern kinds: bind, constructor, struct, tuple, list, range, or, mut, ref, wildcard, literal, raw.

### 2. Parser Surface

Implemented in `src/vitte/compiler/frontend/parse/parser.vit`.

Parser coverage includes:

- `parse_decl`
- `parse_stmt`
- `parse_expr`
- `parse_type_expr`
- `parse_pattern`
- `parse_attrs`
- `parse_proc_decl`
- container/meta declaration parsing
- module paths, generics, where/proc suffixes, parameter lists

Recent parser fixes:

- Top-level visibility prefixes before non-proc declarations.
- `when expr is pattern` no longer lets expression parsing consume `is`.
- Attribute parsing for `#[...]` and `#![...]`.
- `select expr { when pattern { ... } else { ... } }`.
- Byte literals like `b"abc"`.

### 3. Alignment Checker

Implemented as:

- `src/vitte/compiler/frontend/grammar_alignment_checker.vit` for in-tree Vitte metadata.
- `tools/grammar_alignment_checker.py` for executable CI enforcement.

The executable checker inspects:

- Parser evidence in `frontend/parse/parser.vit`.
- AST evidence in frontend AST and IR AST files.
- Fixture evidence in `tests/frontend_syntax/valid/*.vit`.

The checker fails on `NotStarted` or `Partial` status.

## Next Work

### Phase 1: Rich AST Materialization

Current parser validates and counts structural coverage. Next step: return typed AST nodes, not only counters/diagnostics.

Targets:

- Build `AstItem` nodes from `parse_decl`.
- Build `AstStmt` nodes from `parse_stmt`.
- Build `AstExpr`, `AstTypeExpr`, and `AstPattern` nodes from their parse families.
- Preserve spans, names, attributes, visibility, generic params, and where clauses.

### Phase 2: AST to HIR Lowering

Targets:

- Convert frontend AST nodes into HIR/MIR-ready compiler structures.
- Add semantic diagnostics for declaration bodies, type arity, unresolved names, and unsupported forms.
- Keep the current parser recovery behavior intact.

### Phase 3: Focused Negative Fixtures

Add targeted invalid fixtures for:

- malformed attributes
- malformed `select`
- malformed byte literals
- malformed proc suffixes
- malformed generic params and where clauses
- ambiguous `when expr is pattern`

### Phase 4: CI Expansion

`compiler-gate` now includes `grammar-alignment-test`.

Recommended next CI additions:

- Add `make frontend-syntax-test` to the same GitHub workflow that runs driver runtime checks.
- Archive `tools/grammar_alignment_checker.py --json` as an artifact for PR review.
- Add a minimum tested-rule threshold if the grammar grows.

## Success Criteria

For each grammar rule:

- Parser evidence exists.
- AST kind evidence exists.
- At least one valid fixture exercises the construct, or the rule is explicitly marked complete-only because it is structural/non-lexical.
- `make grammar-alignment-test` passes.
- `make frontend-syntax-test` passes.
- `make compiler-gate` passes.
