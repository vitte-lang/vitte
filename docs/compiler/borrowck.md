# Borrow Checker (Current Seed)

Current enforced failures:
- use-after-move
- mutable + shared aliasing
- borrow outlives value marker

This is a minimal safety pass, not full NLL.
