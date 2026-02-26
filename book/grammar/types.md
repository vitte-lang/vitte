# Types

```
type_expr ::=
    ident
  | ident "[" WS? type_expr { WS? "," WS? type_expr } WS? "]"
  | "bool"
  | "string"
  | "int"
  | "*" type_expr
  | "[" type_expr "]"
  | "proc" "(" WS? type_expr_list? WS? ")" [ "->" type_expr ]
  ;

type_expr_list ::= type_expr { WS? "," WS? type_expr } ;
```

Notes:
- `*T` is pointer type, `[T]` is slice type.
- `proc(...) -> T` is a procedure type.
