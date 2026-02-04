## ============================================================
## vitte_const_eval — diagnostic messages
## ============================================================

## ------------------------------------------------------------
## Structural / dispatcher
## ------------------------------------------------------------

const-eval-unsupported-const-expr =
    unsupported constant expression

const-eval-unsupported-unary-op =
    unsupported unary operator `{ $op }` in constant expression

const-eval-unsupported-binary-op =
    unsupported binary operator `{ $op }` in constant expression


## ------------------------------------------------------------
## Type system
## ------------------------------------------------------------

const-eval-type-mismatch-unary =
    type mismatch for unary operator `{ $op }` on value `{ $value }`

const-eval-type-mismatch-binary =
    type mismatch for binary operator `{ $op }` between `{ $lhs }` and `{ $rhs }`

const-eval-invalid-unary-operation =
    invalid unary operation `{ $op }` on value `{ $value }`

const-eval-invalid-binary-operation =
    invalid binary operation `{ $op }` between `{ $lhs }` and `{ $rhs }`


## ------------------------------------------------------------
## Arithmetic
## ------------------------------------------------------------

const-eval-division-by-zero =
    division by zero in constant expression

const-eval-remainder-by-zero =
    remainder by zero in constant expression

const-eval-integer-overflow =
    integer overflow during constant evaluation

const-eval-integer-underflow =
    integer underflow during constant evaluation

const-eval-float-overflow =
    floating-point overflow during constant evaluation

const-eval-float-nan =
    floating-point NaN produced during constant evaluation

const-eval-float-infinite =
    floating-point infinity produced during constant evaluation


## ------------------------------------------------------------
## Policy / context limits
## ------------------------------------------------------------

const-eval-step-limit-exceeded =
    constant evaluation step limit exceeded

const-eval-depth-limit-exceeded =
    constant evaluation recursion depth exceeded

const-eval-expr-size-limit-exceeded =
    constant expression too large to evaluate at compile time


## ------------------------------------------------------------
## Policy restrictions
## ------------------------------------------------------------

const-eval-float-not-allowed =
    floating-point operations are not allowed in this constant context

const-eval-bitwise-not-allowed =
    bitwise operations are not allowed in this constant context

const-eval-pointer-not-allowed =
    pointer operations are not allowed in constant expressions

const-eval-division-by-zero-not-allowed =
    division by zero is forbidden in this constant context


## ------------------------------------------------------------
## Values / literals
## ------------------------------------------------------------

const-eval-invalid-const-value =
    invalid constant value

const-eval-invalid-literal =
    invalid literal in constant expression

const-eval-non-const-value =
    non-constant value used in constant expression


## ------------------------------------------------------------
## Const intrinsics / casts / functions
## ------------------------------------------------------------

const-eval-unsupported-intrinsic =
    unsupported constant intrinsic `{ $name }`

const-eval-unsupported-cast =
    unsupported constant cast `{ $desc }`

const-eval-unsupported-const-fn =
    unsupported constant function `{ $name }`


## ------------------------------------------------------------
## Memory / pointer (future)
## ------------------------------------------------------------

const-eval-memory-access-not-allowed =
    memory access is not allowed during constant evaluation

const-eval-pointer-arithmetic-not-allowed =
    pointer arithmetic is not allowed during constant evaluation


## ------------------------------------------------------------
## Notes / help (optional extensions)
## ------------------------------------------------------------

const-eval-note-strict-mode =
    note: this evaluation runs in strict constant-evaluation mode

const-eval-note-relaxed-mode =
    note: this evaluation runs in relaxed constant-evaluation mode

const-eval-help-const-fn =
    help: only functions explicitly marked as `const` may be evaluated at compile time

const-eval-help-split-expression =
    help: consider simplifying this expression or splitting it into smaller parts
