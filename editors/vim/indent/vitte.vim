" ============================================================================
" Vim indent file for the Vitte programming language
" Language: Vitte
" File: indent/vitte.vim
" ============================================================================

if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetVitteIndent()
setlocal indentkeys=0{,0},0),0],:,!^F,o,O,e,
      \=else,=when

let b:undo_indent = "setlocal indentexpr< indentkeys<"

" ============================================================================
" Vitte block keywords
" ============================================================================

" Canonical Vitte constructs which may introduce a block.
"
" Braces remain authoritative. Keywords are useful when the opening brace is
" placed on the following line.

let s:block_open_keywords =
      \ '\v<(space|region|' .
      \ 'form|class|union|bits|pick|flags|' .
      \ 'trait|impl|' .
      \ 'proc|intrinsic|macro|compiler|pass|backend|diagnostic|' .
      \ 'entry|query|test|bench|' .
      \ 'if|else|select|match|when|' .
      \ 'while|loop|for|with|critical|unsafe|asm|defer)>'

" Constructs aligned with their containing block rather than its body.

let s:block_mid_keywords =
      \ '\v^\s*(else|when)>'

" ============================================================================
" Helpers
" ============================================================================

function! s:TrimLineComment(line) abort
  " Strip // comments.
  "
  " This deliberately does not attempt to implement a lexer. The syntax file
  " is responsible for highlighting strings/comments; indentation only needs
  " a conservative approximation.
  let l:line = substitute(a:line, '//.*$', '', '')

  return substitute(l:line, '\s\+$', '', '')
endfunction

function! s:PrevCodeLine(lnum) abort
  let l:lnum = a:lnum - 1

  while l:lnum > 0
    let l:line = s:TrimLineComment(getline(l:lnum))

    if l:line !~# '^\s*$'
      return l:lnum
    endif

    let l:lnum -= 1
  endwhile

  return 0
endfunction

" ============================================================================
" Block detection
" ============================================================================

function! s:LineOpensBlock(line) abort
  let l:line = s:TrimLineComment(a:line)

  " Braces are authoritative.
  if l:line =~# '{\s*$'
    return 1
  endif

  " Support canonical constructs where the opening brace is placed on the
  " following line.
  if l:line =~# s:block_open_keywords
    " Avoid treating a complete one-line block as an opener.
    if l:line =~# '{.*}\s*$'
      return 0
    endif

    return 1
  endif

  return 0
endfunction

" ============================================================================
" Continuation detection
" ============================================================================

function! s:LineContinues(line) abort
  let l:line = s:TrimLineComment(a:line)

  return l:line =~#
        \ '\v(,|\[|\(|:|=|\+|-|\*|/|%|\.|->|=>|::|:=|' .
        \ '&&|\|\||and|or|as|is)\s*$'
endfunction

" ============================================================================
" Delimiter handling
" ============================================================================

function! s:CountChar(line, char) abort
  return strlen(a:line) - strlen(substitute(a:line, a:char, '', 'g'))
endfunction

function! s:OpeningDelimiterDelta(line) abort
  let l:line = s:TrimLineComment(a:line)

  let l:open =
        \ s:CountChar(l:line, '(') +
        \ s:CountChar(l:line, '\[')

  let l:close =
        \ s:CountChar(l:line, ')') +
        \ s:CountChar(l:line, '\]')

  return l:open - l:close
endfunction

function! s:StartsWithClosingDelimiter(line) abort
  return a:line =~# '^\s*[]})]'
endfunction

" ============================================================================
" Mid-block handling
" ============================================================================

function! s:IsMidBlock(line) abort
  return a:line =~# s:block_mid_keywords
endfunction

" ============================================================================
" Main indentation function
" ============================================================================

function! GetVitteIndent() abort
  let l:lnum = v:lnum

  if l:lnum <= 1
    return 0
  endif

  let l:prev_lnum = s:PrevCodeLine(l:lnum)

  if l:prev_lnum == 0
    return 0
  endif

  let l:prev = s:TrimLineComment(getline(l:prev_lnum))
  let l:curr = s:TrimLineComment(getline(l:lnum))

  let l:ind = indent(l:prev_lnum)

  " --------------------------------------------------------------------------
  " Previous line opened a block
  " --------------------------------------------------------------------------

  if s:LineOpensBlock(l:prev)
    let l:ind += shiftwidth()
  endif

  " --------------------------------------------------------------------------
  " Multiline expressions
  " --------------------------------------------------------------------------

  if s:LineContinues(l:prev)
    let l:ind += shiftwidth()
  elseif s:OpeningDelimiterDelta(l:prev) > 0
    let l:ind += shiftwidth()
  endif

  " --------------------------------------------------------------------------
  " Closing delimiters
  " --------------------------------------------------------------------------

  if s:StartsWithClosingDelimiter(l:curr)
    let l:ind -= shiftwidth()
  endif

  " --------------------------------------------------------------------------
  " Mid-block constructs
  "
  " if condition {
  "   ...
  " } else {
  "
  " match value {
  "   when pattern {
  "     ...
  "   }
  " }
  " --------------------------------------------------------------------------

  if s:IsMidBlock(l:curr)
    let l:ind -= shiftwidth()
  endif

  " Do not double-dedent constructs written after a closing brace.
  if l:curr =~# '^\s*}\s*\%(else\|when\)\>'
    let l:ind += shiftwidth()
  endif

  " --------------------------------------------------------------------------
  " Clamp
  " --------------------------------------------------------------------------

  return max([0, l:ind])
endfunction