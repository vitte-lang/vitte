# Explicit Generic Call Syntax

## Purpose

This note defines the current parser and IR behavior for explicit generic calls in Vitte.

It is intentionally narrow.
It exists to document behavior that is implemented and tested, but not yet part of the protected core language contract.

Related documents:

- `docs/LANGUAGE_CORE_SURFACE.md`
- `docs/LANGUAGE_CORE_GUARANTEES.md`
- `docs/LANGUAGE_CORE_TEST_PLAN.md`

## Current Status

Explicit generic call syntax is currently `experimental`.

That means:

- the implementation exists
- targeted IR snapshots protect the current behavior
- the syntax is not yet part of the protected core gate
- ambiguity rules may still tighten before promotion

## Currently Supported Forms

The following forms are implemented today:

- explicit generic constructors such as `Public[int](1)`
- qualified explicit generic constructors such as `facade_mod.Public[int](1)`
- explicit generic proc calls such as `id[int](1)` when the bracket content is parsed as unambiguous type arguments

## Current Ambiguity Rule

The parser only treats `foo[...](...)` as an explicit generic call when the bracket content is an unambiguous type list.

Current consequences:

- `id[int](1)` is treated as an explicit generic proc call
- `id[Public](1)` is treated as an explicit generic proc call
- `id[i](1)` stays an index expression followed by a normal call
- `id[](1)` and `Public[](1)` are rejected with `E1006`

This boundary is intentional.
It avoids silently changing indexed-call code while the language still decides how broad generic-call syntax should be.

## Examples

Examples that currently parse as explicit generic calls:

```vitte
form Public[T] {
  value: T
}

proc id[T](x: T) -> T {
  give x
}

proc main() -> int {
  let a = Public[int](1)
  let b = id[int](1)
  give 0
}
```

Example that intentionally does not become a generic proc call:

```vitte
proc main() -> int {
  let i = 0
  let f = arr[i](1)
  give 0
}
```

## Current Test Coverage

The current implementation is locked by focused IR snapshots:

- `tests/diag_snapshots/ir/generic_ctor_type_args_preserved.vit`
- `tests/diag_snapshots/ir/qualified_generic_ctor_type_args_preserved/main.vit`
- `tests/diag_snapshots/ir/generic_proc_type_args_preserved.vit`
- `tests/diag_snapshots/ir/generic_proc_call_still_indexed_variable.vit`

These snapshots prove the current parser and HIR lowering behavior.
They do not promote the syntax into the protected core by themselves.

Invalid empty generic-call suffixes are also diagnosed explicitly in the grammar corpus:

- `tests/grammar/invalid/generic-call-empty-proc-01.vit`
- `tests/grammar/invalid/generic-call-empty-ctor-01.vit`

## Promotion Rule

This syntax should only move from `experimental` toward `stable` when all of the following are true:

1. The ambiguity rule is documented and intentionally accepted.
2. Beginner-facing docs use the syntax consistently.
3. The parser behavior is covered by a dedicated long-lived gate, not only ad hoc IR snapshots.
4. The frontend and backend story for generic calls is coherent enough to avoid misleading examples.
