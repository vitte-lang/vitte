# Intermediate Representation (IR) Design

How Vitte's compiler represents and transforms code internally.

## IR Levels

### HIR (High-level IR)

Closest to source code, preserves high-level structures:

```
fn factorial(n: i32) -> i32 {
    match n {
        0 => 1,
        _ => n * factorial(n - 1),
    }
}
```

**Characteristics**:
- Pattern matching constructs
- Original variable names
- Structured control flow
- Type annotations

### MIR (Mid-level IR)

Intermediate form for analysis and optimization:

- Explicitized control flow
- Explicit borrow tracking
- Reduced instruction set

### LIR (Low-level IR)

Close to final representation:

- Basic blocks with jumps
- Explicit memory operations
- Register allocation hints

### C17 IR

Target representation:

```c
int32_t factorial(int32_t n) {
    if (n == 0) return 1;
    return n * factorial(n - 1);
}
```

## IR Transformations

The compiler applies passes to each IR level:

1. **Validation** - Ensure well-formedness
2. **Type Propagation** - Infer and propagate types
3. **Borrow Checking** - Validate ownership rules
4. **Dead Code Elimination** - Remove unused code
5. **Constant Folding** - Evaluate constants at compile time
6. **Inlining** - Expand small functions
7. **Vectorization** - Expose parallelism

## Example: Type Inference

```vitte
let x = 42
let y = x + 1

fn process(items: Vec<i32>) -> i32 {
    items.iter().map(|x| x * 2).sum()
}
```

The compiler infers:
- `x: i32` (from literal 42)
- `y: i32` (from `x + 1`)
- Return type of `process` is `i32`
- Lambda parameter `x: i32` (from Vec element type)

## Memory Representation

Variables are tracked through their lifetimes:

```
let s1 = String::from("hello")  // Allocated
let s2 = s1                      // s1 moved to s2, s1 invalid
drop(s2)                         // Deallocated
```

At IR level:

```
s1 = allocate(String)
s1 = init_string("hello")
s2 = s1              // Move
invalidate(s1)
deallocate(s2)
```

## Optimization Example

**Input**:
```vitte
let x = 5 + 3
let y = x * 2
let z = if true { y } else { 0 }
print(z)
```

**After constant folding**:
```
let x = 8
let y = 16
let z = 16
print(16)
```

**After dead code elimination**:
```
print(16)
```

## Current Implementation Status

| Component | Status | Notes |
|-----------|--------|-------|
| HIR Builder | ✓ Designed | From AST to HIR |
| Type Inference | ✓ Designed | Hindley-Milner style |
| Borrow Checker | 🟡 In Progress | Validates ownership rules |
| Optimizer | 🟡 Basic | Dead code, constant folding |
| Code Generator | 🟡 In Progress | HIR to C17 |

## See Also

- [Compiler Architecture](./compiler.md) - Overall pipeline
- [Type System](./type-system.md) - Type inference details
