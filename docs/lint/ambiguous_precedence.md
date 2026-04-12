# ambiguous-precedence

Rule id: `ambiguous-precedence`

When mixing operators with different precedence, add explicit parentheses.

Examples:

```vit
let x = (a + b) * c
let ok = (is_ready and has_token) or is_admin
```
