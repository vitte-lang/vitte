# Expressions

```
expr ::= unary_expr { WS? bin_op WS? unary_expr } ;

unary_expr ::= "not" WS1 unary_expr | "!" unary_expr | "-" unary_expr | "&" unary_expr | "*" unary_expr | primary ;

primary ::=
    literal
  | call_expr
  | ctor_expr
  | ident
  | "(" WS? expr WS? ")"
  | "if" WS1 expr WS? block [ WS? "else" WS? block ]
  | "proc" "(" WS? param_list? WS? ")" [ "->" type_expr ] WS? block
  ;

call_expr ::= primary "(" WS? arg_list? WS? ")" ;
ctor_expr ::= type_expr "(" WS? arg_list? WS? ")" ;
arg_list ::= expr { WS? "," WS? expr } ;

bin_op ::= "+" | "-" | "*" | "/" | "%" | "==" | "!=" | "<" | "<=" | ">" | ">=" | "and" | "or"
        | "&" | "|" | "^" | "<<" | ">>" | "=" ;
```

See `docs/grammar/precedence.md` for operator precedence.
