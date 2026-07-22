# Ownership Borrow Lifetimes Max

Fixtures for `make ownership-borrow-lifetimes-max`.

These files keep the max ownership contract expressed in Vitte syntax. The
compiler currently accepts the fixture corpus as syntax/check input; the deeper
ownership assertions are executed by `src/vitte/compiler/tests/borrowck_tests.vit`
and enforced by the max gate.
