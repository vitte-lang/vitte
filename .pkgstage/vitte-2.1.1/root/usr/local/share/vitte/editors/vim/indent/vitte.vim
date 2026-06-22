" Vitte indent file
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetVitteIndent()
setlocal indentkeys=0{,0},0),0],:,0#,!^F,o,O,e,=else,=otherwise,=case

let b:undo_indent = "setlocal indentexpr< indentkeys<"

let s:block_open_keywords = '\v<(form|pick|case|trait|type|macro|proc|entry|match|if|else|otherwise|select|when|loop|for)>'
let s:block_mid_keywords = '\v^\s*(case|otherwise|else)>'

function! s:TrimComment(line) abort
  let line = substitute(a:line, '//.*$', '', '')
  let line = substitute(line, '#.*$', '', '')
  return substitute(line, '\s\+$', '', '')
endfunction

function! s:PrevCodeLine(lnum) abort
  let lnum = a:lnum - 1
  while lnum > 0
    let line = s:TrimComment(getline(lnum))
    if line !~# '^\s*$'
      return lnum
    endif
    let lnum -= 1
  endwhile
  return 0
endfunction

function! s:LineOpensBlock(line) abort
  return a:line =~# '{\s*$' || a:line =~# s:block_open_keywords
endfunction

function! s:LineContinues(line) abort
  return a:line =~# '\v(,|\[|\(|\{|\:|=|\+|-|\*|/|\.|->|=>|::|:=|and|or|as|is)\s*$'
endfunction

function! s:OpeningDelimiterDelta(line) abort
  let open = len(split(a:line, '[([{]', 1)) - 1
  let close = len(split(a:line, '[])}]', 1)) - 1
  return open - close
endfunction

function! GetVitteIndent() abort
  let lnum = v:lnum
  if lnum <= 1
    return 0
  endif

  let prev_lnum = s:PrevCodeLine(lnum)
  if prev_lnum == 0
    return 0
  endif

  let prev = s:TrimComment(getline(prev_lnum))
  let curr = s:TrimComment(getline(lnum))
  let ind = indent(prev_lnum)

  if s:LineOpensBlock(prev)
    let ind += &shiftwidth
  endif

  if s:LineContinues(prev) || s:OpeningDelimiterDelta(prev) > 0
    let ind += &shiftwidth
  endif

  if curr =~# '^\s*[]})]'
    let ind -= &shiftwidth
  endif

  if curr =~# s:block_mid_keywords
    let ind -= &shiftwidth
  endif

  if prev =~# s:block_mid_keywords && prev =~# '{\s*$'
    let ind += &shiftwidth
  endif

  if ind < 0
    let ind = 0
  endif
  return ind
endfunction
