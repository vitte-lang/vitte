" ============================================================================
" Vim compiler file for the Vitte programming language
" File: compiler/vitte.vim
" ============================================================================

if exists("current_compiler")
  finish
endif

let current_compiler = "vitte"

if exists(":CompilerSet") != 2
  command -nargs=* CompilerSet setlocal <args>
endif

" ============================================================================
" Compiler command
" ============================================================================

CompilerSet makeprg=vitte\ check\ %

" ============================================================================
" Diagnostic formats
"
" Supported examples:
"
" file.vit:12:8: error: invalid expression
" file.vit:12:8: error[E1001]: invalid expression
" file.vit:12:8: warning: unused variable
" file.vit:12:8: warning[W2001]: unused variable
" file.vit:12:8: note: declared here
" file.vit:12:8: help: remove this declaration
" file.vit:12: error: missing semicolon
" file.vit:12: warning: unused import
" error: backend command rejected emitted source
" warning: experimental feature
" ============================================================================

let &l:errorformat = '%E%f:%l:%c: error[%n]: %m,%E%f:%l:%c: error: %m,%E%f:%l: error[%n]: %m,%E%f:%l: error: %m,%E%f: error[%n]: %m,%E%f: error: %m,%Eerror[%n]: %m,%Eerror: %m,%W%f:%l:%c: warning[%n]: %m,%W%f:%l:%c: warning: %m,%W%f:%l: warning[%n]: %m,%W%f:%l: warning: %m,%W%f: warning[%n]: %m,%W%f: warning: %m,%Wwarning[%n]: %m,%Wwarning: %m,%I%f:%l:%c: note: %m,%I%f:%l: note: %m,%I%f: note: %m,%Inote: %m,%I%f:%l:%c: help: %m,%I%f:%l: help: %m,%I%f: help: %m,%Ihelp: %m,%C%\\s%#--> %f:%l:%c,%C%\\s%#| %m,%C%\\s%#= note: %m,%C%\\s%#= help: %m,%C%\\s%#= warning: %m,%C%\\s%#= error: %m,%C%\\s%#%m,%-G%.%#'

" ============================================================================
" Local compiler behavior
" ============================================================================

CompilerSet makeencoding=utf-8

if exists("+shellpipe")
  CompilerSet shellpipe=2>&1\|tee
endif

" Keep the quickfix list focused on Vitte diagnostics.
setlocal makeef=

" ============================================================================
" Optional commands
" ============================================================================

if !exists(":VitteCheck")
  command -buffer VitteCheck
        \ silent make! |
        \ cwindow
endif

if !exists(":VitteCheckClose")
  command -buffer VitteCheckClose
        \ silent make! |
        \ cclose
endif

if !exists(":VitteCheckOpen")
  command -buffer VitteCheckOpen
        \ silent make! |
        \ copen
endif

if !exists(":VitteNextError")
  command -buffer VitteNextError
        \ try |
        \   cnext |
        \ catch /^Vim\%((\a\+)\)\=:E553/ |
        \   echohl WarningMsg |
        \   echomsg "No more Vitte diagnostics" |
        \   echohl None |
        \ endtry
endif

if !exists(":VittePreviousError")
  command -buffer VittePreviousError
        \ try |
        \   cprevious |
        \ catch /^Vim\%((\a\+)\)\=:E553/ |
        \   echohl WarningMsg |
        \   echomsg "No previous Vitte diagnostic" |
        \   echohl None |
        \ endtry
endif
