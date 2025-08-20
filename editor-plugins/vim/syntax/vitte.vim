if exists("b:current_syntax") | finish | endif
syntax keyword vitteKeyword fn let mut const if else while for return match struct enum impl use mod pub async await
syntax match vitteNumber /\v<\d+(\.\d+)?>/
syntax region vitteString start=/"/ skip=/\\./ end=/"/
syntax match vitteComment /\/\/.*$/
highlight link vitteKeyword Keyword
highlight link vitteNumber Number
highlight link vitteString String
highlight link vitteComment Comment
let b:current_syntax = "vitte"
