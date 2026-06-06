# Vitte Type System (Core Freeze 0.1)

Stable core typing rules in the professional seed pipeline:

- literals: `int`, `bool`, `string`.
- explicit variable typing required in strict paths.
- assignment type equality enforced (`E_TYPE_MISMATCH`).
- return type consistency enforced (`E_TYPE_RET_MISMATCH`).
- operator compatibility validated (`E_TYPE_OPERATOR`).
- dangerous implicit casts rejected (`E_TYPE_CAST_DANGEROUS`).
- `if` conditions must be `bool` (`E_TYPE_IF_COND`).

This freeze applies to the current compilable subset only.
