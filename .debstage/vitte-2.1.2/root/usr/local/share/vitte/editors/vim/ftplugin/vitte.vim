" AUTO-GENERATED vitte ftplugin
if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

setlocal shiftwidth=2
setlocal tabstop=2
setlocal softtabstop=2
setlocal expandtab
setlocal commentstring=//\ %s
setlocal omnifunc=syntaxcomplete#Complete
setlocal makeprg=vitte\ check\ %

let b:undo_ftplugin = "setlocal shiftwidth< tabstop< softtabstop< expandtab< commentstring< omnifunc< makeprg<"
