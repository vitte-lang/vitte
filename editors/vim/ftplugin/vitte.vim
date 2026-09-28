" ============================================================================
" Vitte ftplugin for Vim
" Language: Vitte
" File: ftplugin/vitte.vim
" ============================================================================

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

" ============================================================================
" Indentation
" ============================================================================

setlocal shiftwidth=2
setlocal tabstop=2
setlocal softtabstop=2
setlocal expandtab
setlocal autoindent
setlocal smartindent
setlocal smarttab

" ============================================================================
" Comments
" ============================================================================

setlocal commentstring=//\ %s
setlocal comments=s1:/*,mb:*,ex:*/,://

" ============================================================================
" Vitte source extensions
" ============================================================================

" .vit   : primary Vitte source
" .vitl  : Vitte library / stdlib source
" .vitte : legacy compatibility source
setlocal suffixesadd=.vit,.vitl,.vitte

" ============================================================================
" Formatting
" ============================================================================

setlocal textwidth=0

setlocal formatoptions-=t
setlocal formatoptions+=c
setlocal formatoptions+=r
setlocal formatoptions+=o

" External formatter.
"
" Examples:
"
"   :%!vitte fmt -
"   gq{motion}
"
" The command must accept source through stdin and write the formatted source
" to stdout.
setlocal formatprg=vitte\ fmt\ -

" ============================================================================
" Compiler / diagnostics
" ============================================================================

" compiler/vitte.vim owns makeprg and errorformat.
"
" It provides:
"
"   :make
"   :VitteCheck
"   :VitteCheckOpen
"   :VitteCheckClose
"   :VitteErrors
"   :VitteErrorsClose
"   :VitteNextError
"   :VittePreviousError
"   :VitteFirstError
"   :VitteLastError

if exists(":compiler") == 2
  silent! compiler vitte
endif

" ============================================================================
" Navigation
" ============================================================================

" Search paths used by commands such as gf.
setlocal path+=.
setlocal path+=src
setlocal path+=stdlib

" Vitte identifiers use letters, digits and underscore.
setlocal iskeyword+=_

" ============================================================================
" Matching
" ============================================================================

" Standard pairs are already provided by Vim.
" Add generic/type angle brackets for Vitte.
if &l:matchpairs !~# '<:>'
  setlocal matchpairs+=<:>
endif

" ============================================================================
" Completion
" ============================================================================

" Vitte-specific omnifunc:
"
"   Ctrl-X Ctrl-O
"
" Implementation:
"
"   autoload/vitte/complete.vim
setlocal omnifunc=vitte#complete#Complete

" Generic Vim completion sources.
"
" .  current buffer
" w  other windows
" b  loaded buffers
" u  unloaded buffers
" t  tags
setlocal complete-=i
setlocal complete+=.,w,b,u,t

" Keep completion menu useful without forcing insertion.
if exists("+completeopt")
  setlocal completeopt=menu,menuone,noselect
endif

" ============================================================================
" Include handling
" ============================================================================

" Help Vim recognize common Vitte import lines when searching includes.
setlocal include=^\s*use\s\+

" ============================================================================
" Definition patterns
" ============================================================================

" Useful for commands based on Vim's definition search.
setlocal define=^\s*\%(proc\|form\|class\|union\|trait\|type\)\s\+

" ============================================================================
" Keyword program
" ============================================================================

" K over a word can delegate to Vitte help when supported.
setlocal keywordprg=vitte\ help

" ============================================================================
" Buffer-local commands
" ============================================================================

if !exists(":VitteFormat")
  command -buffer VitteFormat
        \ silent keepjumps %!vitte fmt -
endif

if !exists(":VitteFormatWrite")
  command -buffer VitteFormatWrite
        \ silent keepjumps %!vitte fmt - |
        \ update
endif

" ============================================================================
" Undo
" ============================================================================

let b:undo_ftplugin =
      \ "setlocal shiftwidth< " .
      \ "tabstop< " .
      \ "softtabstop< " .
      \ "expandtab< " .
      \ "autoindent< " .
      \ "smartindent< " .
      \ "smarttab< " .
      \ "commentstring< " .
      \ "comments< " .
      \ "suffixesadd< " .
      \ "textwidth< " .
      \ "formatoptions< " .
      \ "formatprg< " .
      \ "makeprg< " .
      \ "errorformat< " .
      \ "makeencoding< " .
      \ "makeef< " .
      \ "path< " .
      \ "iskeyword< " .
      \ "matchpairs< " .
      \ "omnifunc< " .
      \ "complete< " .
      \ "completeopt< " .
      \ "include< " .
      \ "define< " .
      \ "keywordprg< " .
      \ " | silent! delcommand VitteFormat" .
      \ " | silent! delcommand VitteFormatWrite"