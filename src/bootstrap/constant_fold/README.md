# Bootstrap constant folding

`constant.c` and `constant.h` provide the bootstrap implementation and public
C ABI for primitive constant values and checked folding operations. The
matching native Vitte module lives at
`src/compiler/constant_fold/constant.vitte`.

The API works on already-known values. It does not traverse AST expressions or
resolve identifiers. It supports fixed-width signed and unsigned integers,
32/64-bit floats, booleans, Unicode scalar values, borrowed byte strings,
null, and poison values. Strings are not copied, so their backing storage must
remain alive while a constant uses it.

The native Vitte module currently implements value representation, names,
validation and classification, zero/one queries, equality, hashing,
constructors, and result helpers. The bootstrap C file remains the
implementation of unary/binary folds, casts, statistics, and the
translation-unit anchor while those operations are being ported.

The module-specific Makefile in this directory builds the bootstrap C
implementation. It does not validate or compile the native Vitte module.
