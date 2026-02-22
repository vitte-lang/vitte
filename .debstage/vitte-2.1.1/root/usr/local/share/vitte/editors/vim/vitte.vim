" Vitte syntax highlighting
if exists("b:current_syntax")
  finish
endif

syn keyword vitteKeyword space pull use share const let make type form trait pick case proc entry at macro give emit asm unsafe if else otherwise loop for in break continue select when match return not and or is as
syn keyword vitteBuiltin true false null
syn keyword vitteType bool string int char i8 i16 i32 i64 isize u8 u16 u32 u64 usize f32 f64
syn keyword vitteTodo TODO FIXME XXX NOTE

syn match vitteNumber "\v\b(0x[0-9A-Fa-f]+|0b[01]+|[0-9]+)\b"
syn region vitteString start=+"+ skip=+\\"+ end=+"+
syn match vitteAttribute "#\\[[^]]\\+\\]"
syn match vitteUsePath "\\v\\<use\\>\\s+\\zs[A-Za-z0-9_./]+"

syn region vitteLineComment start="//" end="$" contains=vitteTodo
syn region vitteBlockComment start="/\*" end="\*/" contains=vitteTodo
syn region vitteZoneComment start="<<<" end=">>>" contains=vitteTodo

hi def link vitteKeyword Keyword
hi def link vitteBuiltin Boolean
hi def link vitteType Type
hi def link vitteNumber Number
hi def link vitteString String
hi def link vitteTodo Todo
hi def link vitteAttribute PreProc
hi def link vitteUsePath Constant
hi def link vitteLineComment Comment
hi def link vitteBlockComment Comment
hi def link vitteZoneComment Comment

let b:current_syntax = "vitte"
