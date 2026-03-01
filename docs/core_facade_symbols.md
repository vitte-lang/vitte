# vitte/core Facade Symbols

## some
Usage: `some[T](value)` creates `Option.Some`.
Counter-example: do not encode absence with magic values.

## none
Usage: `none[T]()` creates `Option.None`.
Counter-example: do not mix with arbitrary sentinel strings.

## ok / err
Usage: `ok[T,E](value)` and `err[T,E](error)` for explicit success/failure.
Counter-example: do not throw hidden side-effect errors at import time.

## assert_or_err
Usage: convert boolean invariant into `Result[bool,string]`.
Counter-example: do not crash directly in facade-level helpers.

## panic_guard
Usage: fallback extraction from `Result[T,string]`.
Counter-example: do not ignore error branches in strict profile checks.
