# Vitte Type System v1

This document is the normative typing contract for the canonical HIR checker. The machine-readable rule index is `schemas/type_system/rules.json`; CI verifies that every rule below names an existing implementation procedure, an executed production test entry, and registered diagnostics.

## Judgments

- `Gamma |- e : T`: expression `e` has type `T` in value environment `Gamma`.
- `Gamma |- s => Gamma'`: statement `s` is valid and produces environment `Gamma'`.
- `Gamma |- A <= E`: actual type `A` may be used where expected type `E` is required.
- `Impls |- T : Trait`: the visible coherent impl set proves that `T` satisfies `Trait`.
- `Variants |- match exhaustive`: a match covers its finite scrutinee domain.

`Gamma` contains local bindings, procedure signatures, nominal types and visible members. Generic substitution is capture-free and replaces complete type identifiers only. An expression or statement is accepted only when all premises of its rule hold.

## T-LITERAL

If a literal kind has canonical primitive type `T`, then `Gamma |- literal : T`. Integer, float, boolean, string and character literals are typed before contextual coercion.

## T-NAME

If `Gamma(x) = T`, `x` is initialized and `x` has not been moved, then `Gamma |- x : T`. Otherwise name, initialization or move diagnostics are emitted.

## T-DECLARED-TYPE

An annotation is well formed when each nested identifier denotes a primitive, a generic parameter, `Self`, or a visible nominal type. The rule recurses through references, pointers, arrays, tuples, functions and generic applications.

## T-ASSIGN

If `Gamma(x) = E`, `Gamma |- value : A`, `Gamma |- A <= E`, and `x` is mutable, then `Gamma |- set x = value => Gamma`. Mutation does not change the declared binding type.

## T-RETURN

If procedure `p` declares return type `E`, `Gamma |- value : A`, and `Gamma |- A <= E`, then `give value` is valid in `p`. Every mismatch records both the signature origin and expression origin.

## T-UNARY

If `Gamma |- e : A` and unary operator `op` is defined for `A`, then `Gamma |- op e : result(op, A)`. Dereference is restricted to references and supported pointer types.

## T-BINARY

If both operands type-check and the operator admits their common type, then the binary expression has the operator result type. Arithmetic uses numeric unification; comparisons produce `bool`; unsupported pairs are rejected.

## T-CONDITION

Control-flow conditions must have a truthy type. The current truthy domain is defined by `type_is_truthy`; values outside that domain are rejected before branch unification.

## T-BRANCH

If every value-producing branch type unifies, an `if` expression has the unified type. Incompatible branch types are rejected rather than silently reduced to `opaque`.

## T-CALL

For `callee(args)`, the callee must have type `fn(P1, ..., Pn) -> R`, arity must match its variadic contract, and each argument type must coerce to its corresponding parameter type. The call then has type `R`.

## T-GENERIC-INFER

Generic calls collect constraints from explicit type arguments and parameter/argument type patterns. A unique consistent substitution `S` must resolve every generic parameter; the result is `S(R)`. Unconstrained or contradictory parameters are rejected before MIR lowering.

## T-GENERIC-BOUND

For each bound `P: Trait`, substitution `S(P) = T` is valid only if `Impls |- T : Trait`. Built-in and visible user impl evidence are both considered; missing evidence rejects the call.

## T-MEMBER

If `Gamma |- receiver : Owner` and `Owner` exposes member `m: T`, then `Gamma |- receiver.m : T`. Field and variant lookup use the nominal member table produced from HIR.

## T-METHOD

A method access is valid when exactly one visible provider applies to the receiver type. `Self` is removed from the bound call signature. Multiple equally applicable providers are an ambiguity error.

## T-CAST

If `Gamma |- e : A` and the explicit conversion `A -> T` is supported, then `Gamma |- e as T : T`. Explicit casts may permit numeric narrowing; unsupported representation changes remain errors.

## T-COERCE

`Gamma |- A <= E` holds for identity, allowed numeric widening, compatible pointer/reference forms, and shared reborrows from mutable references. Numeric narrowing and unrelated nominal conversions require an explicit valid cast.

## T-IMPL

An impl proves `T: Trait` only when the trait exists, every required method is present, and parameter and return types match the trait declaration after `Self` binding.

## T-COHERENCE

The visible impl set is coherent only when no two impls have the same normalized trait and target type. Conflicting impls are rejected with labels for both declarations.

## T-CONSTRAINT-ACYCLIC

The directed graph formed by generic bounds and default-type dependencies must be acyclic. Any path from a generic parameter back to itself makes the declaration invalid.

## T-MATCH-EXHAUSTIVE

A match is exhaustive when it has a wildcard/fallback arm or covers every member of a known finite domain. The current finite domains are `bool` and visible `pick` variants; diagnostics name the first missing case.

## Failure Contract

Type errors return `TypeckResult.valid = false` and retain typed HIR plus structured diagnostics. DIAG-0062 through DIAG-0068 require cause chains. Type checking must not panic on user source; malformed HIR is reported as an internal compiler contract failure.
