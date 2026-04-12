# for-stability

Rule id: `for-stability`

`for ... in` is still considered a constrained feature until lowering behavior is uniformly stable across all backends.
Prefer explicit `while` loops in critical/shared code.
