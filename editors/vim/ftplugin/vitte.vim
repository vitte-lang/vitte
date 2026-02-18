" Vitte filetype plugin
if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal shiftwidth=2
setlocal tabstop=2
setlocal softtabstop=2
setlocal expandtab
setlocal commentstring=//\ %s

" Use Vitte compiler integration (:make / :compiler vitte)
setlocal makeprg=vitte\ check\ %

let b:undo_ftplugin = "setlocal shiftwidth< tabstop< softtabstop< expandtab< commentstring< makeprg<"
