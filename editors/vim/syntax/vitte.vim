" Vim syntax file
" Language: Vitte

if exists("b:current_syntax")
  finish
endif

syn case match

" ============================================================
" Top-level keywords
" ============================================================

syn keyword vitteKeyword
      \ space use export const static type opaque
      \ form pick trait impl proc extern intrinsic macro test

" ============================================================
" Statements
" ============================================================

syn keyword vitteStatement
      \ let set return give break continue defer assert
      \ if else while loop for in match asm

" ============================================================
" Procedure modifiers
" ============================================================

syn keyword vitteModifier
      \ pub async unsafe comptime where requires ensures
      \ mut ref move await not and or self as

" ============================================================
" Declarations
" ============================================================

syn keyword vitteDecl
      \ space use export const static
      \ form pick trait impl
      \ type opaque
      \ proc extern intrinsic macro test

" ============================================================
" Built-in types
"
" Keep this list limited to types exposed by the Vitte frontend.
" User-defined type names are handled separately.
" ============================================================

syn keyword vitteType
      \ void never unit bool char rune string str bytes cstr int
      \ i8 i16 i32 i64 i128 isize
      \ u8 u16 u32 u64 u128 usize
      \ intptr uintptr f16 f32 f64 f128
      \ c_char c_int c_uint c_long c_ulong c_void

" ============================================================
" Built-in literals
" ============================================================

syn keyword vitteBuiltin
      \ true false null

" ============================================================
" TODO markers
" ============================================================

syn keyword vitteTodo
      \ TODO FIXME BUG HACK NOTE XXX
      \ contained

" ============================================================
" Numbers
" ============================================================

syn match vitteNumber
      \ "\v<0[xX][0-9A-Fa-f](_?[0-9A-Fa-f])*>"

syn match vitteNumber
      \ "\v<0[bB][01](_?[01])*>"

syn match vitteNumber
      \ "\v<0[oO][0-7](_?[0-7])*>"

syn match vitteNumber
      \ "\v<[0-9](_?[0-9])*>"

syn match vitteFloat
      \ "\v<[0-9](_?[0-9])*\.([0-9](_?[0-9])*)?([eE][+-]?[0-9](_?[0-9])*)?>"

syn match vitteFloat
      \ "\v<[0-9](_?[0-9])*[eE][+-]?[0-9](_?[0-9])*>"

syn match vitteFloat
      \ "\v<0[xX][0-9A-Fa-f](_?[0-9A-Fa-f])*(\.([0-9A-Fa-f](_?[0-9A-Fa-f])*)?)?[pP][+-]?[0-9](_?[0-9])*>"

" ============================================================
" Strings / characters
" ============================================================

syn match vitteEscape
      \ "\\\(\\\|'\|\"\|0\|a\|b\|f\|n\|r\|t\|v\|x[0-9A-Fa-f]\{2}\|u[0-9A-Fa-f]\{4}\|U[0-9A-Fa-f]\{8}\)"
      \ contained

syn region vitteString
      \ start=+"+
      \ skip=+\\\\\|\\"+
      \ end=+"+
      \ contains=vitteEscape

syn region vitteChar
      \ start=+'+
      \ skip=+\\\\\|\\'+
      \ end=+'+
      \ contains=vitteEscape

" ============================================================
" Spaces / imports / exports
" ============================================================

syn match vitteUsePath
      \ "\v<use>\s+\zs[A-Za-z_][A-Za-z0-9_./:]*"

syn match vitteExport
      \ "\v<export>\s+\zs(\*|[A-Za-z_][A-Za-z0-9_./:]*)"

" Alias syntax used by imports.
syn match vitteUseAlias
      \ "\v<as>\s+\zs[A-Za-z_][A-Za-z0-9_]*"

" ============================================================
" Declaration names
" ============================================================

syn match vitteProcName
      \ "\v<proc>\s+\zs[A-Za-z_][A-Za-z0-9_]*"

syn match vitteIntrinsicName
      \ "\v<intrinsic>\s+\zs[A-Za-z_][A-Za-z0-9_]*"

syn match vitteMacroName
      \ "\v<macro>\s+\zs[A-Za-z_][A-Za-z0-9_]*"

syn match vitteTestName
      \ "\v<test>\s+\zs[A-Za-z_][A-Za-z0-9_]*"

syn match vitteTypeDeclName
      \ "\v<(form|pick|trait|type|opaque)>\s+\zs[A-Za-z_][A-Za-z0-9_]*"

" ============================================================
" Bindings
" ============================================================

syn match vitteBinding
      \ "\v<(let|const|static)>\s+\zs[A-Za-z_][A-Za-z0-9_]*"

syn match vitteAssignmentTarget
      \ "\v<set>\s+\zs[A-Za-z_][A-Za-z0-9_]*"

" ============================================================
" Loop bindings
" ============================================================

syn match vitteLoopVariable
      \ "\v<for>\s+\zs[A-Za-z_][A-Za-z0-9_]*"

" ============================================================
" Parameters
" ============================================================

syn match vitteParameter
      \ "\v(\(|,)\s*\zs[A-Za-z_][A-Za-z0-9_]*\ze\s*:"

" ============================================================
" Type annotations
" ============================================================

syn match vitteSignatureType
      \ "\v:\s*\zs[A-Za-z_][A-Za-z0-9_]*"

syn match vitteReturnType
      \ "\v->\s*\zs[A-Za-z_][A-Za-z0-9_]*"

" ============================================================
" Function calls
" ============================================================

syn match vitteFunctionCall
      \ "\v<[A-Za-z_][A-Za-z0-9_]*>\ze\s*\("

syn match vitteQualifiedFunctionCall
      \ "\v<[A-Za-z_][A-Za-z0-9_]*(\.[A-Za-z_][A-Za-z0-9_]*)+>\ze\s*\("

" ============================================================
" Constructors / variants / constants
" ============================================================

syn match vitteConstructor
      \ "\v<[A-Z][A-Za-z0-9_]*>"

syn match vitteVariant
      \ "::\zs[A-Z][A-Za-z0-9_]*"

syn match vitteConstant
      \ "\v<[A-Z][A-Z0-9_]+>"

" ============================================================
" Properties
" ============================================================

syn match vitteProperty
      \ "\.\zs[A-Za-z_][A-Za-z0-9_]*"

" ============================================================
" Operators
" ============================================================

" Longer operators are intentionally matched before their shorter forms.

syn match vitteOperator
      \ "\.\.="

syn match vitteOperator
      \ "\.\."

syn match vitteOperator
      \ "??"

syn match vitteOperator
      \ "->"

syn match vitteOperator
      \ "=>"

syn match vitteOperator
      \ "::"

syn match vitteOperator
      \ "=="

syn match vitteOperator
      \ "!="

syn match vitteOperator
      \ "<="

syn match vitteOperator
      \ ">="

syn match vitteOperator
      \ "&&"

syn match vitteOperator
      \ "||"

syn match vitteOperator
      \ "<<"

syn match vitteOperator
      \ ">>"

syn match vitteOperator
      \ "+="

syn match vitteOperator
      \ "-="

syn match vitteOperator
      \ "\*="

syn match vitteOperator
      \ "/="

syn match vitteOperator
      \ "%="

syn match vitteOperator
      \ "&="

syn match vitteOperator
      \ "|="

syn match vitteOperator
      \ "\^="

syn match vitteOperator
      \ "<<="

syn match vitteOperator
      \ ">>="

syn match vitteOperator
      \ "[+*/%=<>!&|^~?.-]"

" ============================================================
" Delimiters
" ============================================================

syn match vitteDelimiter
      \ "[][(){},;:]"

" ============================================================
" Diagnostics / formatting
" ============================================================

syn match vitteTrailingWhitespace
      \ "\s\+$"

" ============================================================
" Comments
" ============================================================

syn region vitteLineComment
      \ start="//"
      \ end="$"
      \ contains=vitteTodo,@Spell

syn region vitteBlockComment
      \ start="/\*"
      \ end="\*/"
      \ contains=vitteTodo,@Spell

" ============================================================
" Highlight groups
" ============================================================

hi def link vitteKeyword Keyword
hi def link vitteStatement Statement
hi def link vitteModifier StorageClass
hi def link vitteDecl Statement

hi def link vitteType Type
hi def link vitteBuiltin Boolean

hi def link vitteNumber Number
hi def link vitteFloat Float
hi def link vitteString String
hi def link vitteChar Character
hi def link vitteEscape SpecialChar

hi def link vitteUsePath Include
hi def link vitteExport Include
hi def link vitteUseAlias Identifier

hi def link vitteProcName Function
hi def link vitteIntrinsicName Function
hi def link vitteMacroName Macro
hi def link vitteTestName Function

hi def link vitteTypeDeclName Type
hi def link vitteSignatureType Type
hi def link vitteReturnType Type

hi def link vitteBinding Identifier
hi def link vitteAssignmentTarget Identifier
hi def link vitteLoopVariable Identifier
hi def link vitteParameter Identifier

hi def link vitteFunctionCall Function
hi def link vitteQualifiedFunctionCall Function

hi def link vitteConstructor Type
hi def link vitteVariant Constant
hi def link vitteConstant Constant
hi def link vitteProperty Identifier

hi def link vitteOperator Operator
hi def link vitteDelimiter Delimiter

hi def link vitteTrailingWhitespace Error
hi def link vitteTodo Todo

hi def link vitteLineComment Comment
hi def link vitteBlockComment Comment
" ============================================================
" Synchronization
" ============================================================

syn sync minlines=64
syn sync maxlines=256

let b:current_syntax = "vitte"