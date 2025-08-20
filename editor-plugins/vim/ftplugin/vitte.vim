setlocal commentstring=//\ %s
command! -buffer VitteFmt call vitte#fmt()
nnoremap <buffer> <leader>vf :VitteFmt<CR>
