function! vitte#fmt() abort
  let l:view = winsaveview()
  silent %s/\s\+$//e
  silent %s/\n\{3,}/\r\r/ge
  call winrestview(l:view)
endfunction
