# Declarations

Top-level:
```
toplevel ::=
    space_decl | pull_decl | share_decl | use_decl
  | const_decl | type_decl | macro_decl
  | form_decl | pick_decl | proc_decl | entry_decl
  | comment
  ;
```

Key forms:
```
space_decl ::= "space" WS1 module_path ;
pull_decl ::= "pull" WS1 module_path [ WS1 "as" WS1 ident ] ;
use_decl  ::= "use" WS1 module_path [ use_group | use_glob ] [ WS1 "as" WS1 ident ] ;
use_glob ::= "." "*" ;
use_group ::= "." "{" WS? ident_list WS? "}" ;
share_decl ::= "share" WS1 ( "all" | ident_list ) ;

const_decl ::= "const" WS1 ident [ ":" WS? type_expr ] WS? "=" WS? expr ;
type_decl  ::= "type" WS1 ident [ type_params ] WS? "=" WS? type_expr ;
macro_decl ::= "macro" WS1 ident "(" param_list? ")" WS? block ;

form_decl ::= ("form" | "trait") WS1 ident [ type_params ] WS? { field_decl WS? } ".end" ;
pick_decl ::= "pick" WS1 ident [ type_params ] WS? { case_decl WS? } ".end" ;

proc_decl ::= { attribute WS? }
              "proc" WS1 ident [ type_params ]
              "(" WS? param_list? WS? ")"
              [ WS? "->" WS? type_expr ]
              [ WS? block ] ;

entry_decl ::= "entry" WS1 ident WS1 "at" WS1 module_path WS? block ;

attribute ::= "#[" ident [ "(" attr_args? ")" ] "]" ;
attr_args ::= attr_arg { WS? "," WS? attr_arg } ;
attr_arg ::= ident | string_lit | int_lit ;
```
