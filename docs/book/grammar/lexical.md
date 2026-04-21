# Lexical

```
ident ::= (LETTER | "_") { LETTER | DIGIT | "_" } ;

WS ::= { " " | "\t" | NEWLINE } ;
WS1 ::= ( " " | "\t" | NEWLINE ) { " " | "\t" | NEWLINE } ;

NEWLINE ::= "\n" | "\r\n" ;

LETTER ::= "a"…"z" | "A"…"Z" ;
DIGIT ::= "0"…"9" ;
```

Literals (summary):
```
bool_lit   ::= "true" | "false" ;
int_lit    ::= [ "-" ] DIGIT { DIGIT } [ ("u8"|"u16"|"u32"|"u64"|"i8"|"i16"|"i32"|"i64") ] ;
string_lit ::= "\"" { string_char } "\"" | raw_string_lit ;
```
