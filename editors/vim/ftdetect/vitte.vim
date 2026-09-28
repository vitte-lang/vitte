" ============================================================================
" Vim filetype detection for the Vitte programming language
" File: ftdetect/vitte.vim
" ============================================================================

augroup vitte_filetype_detection
  autocmd!
  autocmd BufRead,BufNewFile *.vit   setfiletype vitte
  autocmd BufRead,BufNewFile *.vitl  setfiletype vitte
  autocmd BufRead,BufNewFile *.vitte setfiletype vitte
augroup END