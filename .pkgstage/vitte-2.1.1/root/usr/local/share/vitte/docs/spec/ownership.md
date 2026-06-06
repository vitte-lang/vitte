# Vitte Ownership and Borrow (Core Freeze 0.1)

Stable borrow semantics checked by the mandatory borrowck phase:

- move invalidates previous owner uses (`E_BORROW_USE_AFTER_MOVE`).
- borrowed value cannot outlive owner (`E_BORROW_OUTLIVES_VALUE`).
- mutable and shared borrows cannot alias simultaneously (`E_BORROW_MUT_AND_SHARED`).
- borrowck is critical: skipped borrowck fails the build gates.

Advanced borrow features (NLL, reborrow graph precision, partial moves) remain experimental.
