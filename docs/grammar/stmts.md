# Statements

```
block ::= "{" WS? { stmt WS? } "}" ;

stmt ::=
    let_stmt | make_stmt | set_stmt | give_stmt | emit_stmt
  | if_stmt | loop_stmt | for_stmt | break_stmt | continue_stmt
  | select_stmt | return_stmt | expr_stmt
  ;

let_stmt ::= "let" WS1 ident [ ":" WS? type_expr ] WS? "=" WS? expr ;
make_stmt ::= "make" WS1 ident [ WS1 "as" WS1 type_expr ] WS? "=" WS? expr ;
set_stmt ::= "set" WS1 ident WS? "=" WS? expr ;
give_stmt ::= "give" WS1 expr ;
emit_stmt ::= "emit" WS1 expr ;

if_stmt ::= "if" WS1 expr WS? block [ WS? ("else" | "otherwise") WS? block ] ;
loop_stmt ::= "loop" WS? block ;
for_stmt ::= "for" WS1 ident WS1 "in" WS1 expr WS? block ;
break_stmt ::= "break" ;
continue_stmt ::= "continue" ;

select_stmt ::= "select" WS1 expr WS? { when_stmt WS? } [ WS? "otherwise" WS? block ] ;
when_stmt ::= "when" WS1 pattern WS? block ;

return_stmt ::= "return" [ WS1 expr ] ;
expr_stmt ::= expr ;
```
