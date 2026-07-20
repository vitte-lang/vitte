# core.convert, core.default, and core.clone

These modules define explicit conversion, default-value, copy, and clone
contracts for Vitte core code.

## core.convert

`vitte/stdlib/core/convert` defines:

- `From`
- `Into`
- `TryFrom`
- `TryInto`
- `AsRef`
- `AsMut`
- `Borrow`
- `BorrowMut`

Numeric conversions are separated by intent:

- safe conversions preserve every value;
- checked conversions return a failure when the value is out of range;
- saturating conversions clamp to the target range;
- wrapping conversions keep low target bits;
- truncating conversions are explicit and may lose information.

Dangerous implicit conversions are forbidden. A conversion that can lose range,
precision, ownership, or representation must be written explicitly. Impossible
conversions use `conversion_impossible_error(from_type, to_type)` so diagnostics
name both sides and the reason no conversion family applies.

## core.default

`vitte/stdlib/core/default` defines `Default<T>`.

Primitive defaults are zero or false where that value has a clear neutral
meaning. Compatible structures may define defaults only through an explicit
builder and a non-empty reason.

The stdlib must not synthesize a default for a type that has no clear neutral
value. Such cases use `reject_meaningless_default(type_name)`.
A type with no clear neutral value must not implement `Default<T>`.

## core.clone

`vitte/stdlib/core/clone` defines `Clone<T>` and `Copy<T>`.

`Copy<T>` is reserved for trivial bitwise copies. `Clone<T>` is explicit and
carries a cost classification: trivial, cheap, expensive, or forbidden.

The compiler and stdlib must avoid implicit clones. Copy diagnostics use
`copy_forbidden_diagnostic(type_name, reason)` and clone diagnostics use
`clone_required_diagnostic(type_name)` or `reject_implicit_clone(type_name)`.
