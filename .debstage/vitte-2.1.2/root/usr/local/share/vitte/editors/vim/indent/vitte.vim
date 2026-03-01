" Vitte indent file
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetVitteIndent()
setlocal indentkeys=0{,0},0),0],:,0#,!^F,o,O,e

function! GetVitteIndent()
  let lnum = v:lnum
  if lnum == 1
    return 0
  endif
  let prev = getline(lnum - 1)
  let curr = getline(lnum)

  let ind = indent(lnum - 1)

  if prev =~ '{\s*$'
    let ind += &shiftwidth
  endif
  if curr =~ '^\s*}'
    let ind -= &shiftwidth
  endif
  if curr =~ '^\s*\(case\|otherwise\|else\)\>'
    let ind -= &shiftwidth
  endif
  if prev =~ '^\s*\(case\|otherwise\)\>.*{\s*$'
    let ind += &shiftwidth
  endif
  if ind < 0
    let ind = 0
  endif
  return ind
endfunction
