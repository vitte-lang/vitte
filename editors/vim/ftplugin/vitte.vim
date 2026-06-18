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
setlocal comments=s1:/*,mb:*,ex:*/,://,b:#
setlocal suffixesadd=.vit,.vitte,.vitl
setlocal makeprg=vitte\ check\ %
setlocal formatprg=vitte\ fmt\ -

if exists(":compiler") == 2
  compiler vitte
endif

let b:undo_ftplugin = "setlocal shiftwidth< tabstop< softtabstop< expandtab< commentstring< comments< suffixesadd< makeprg< formatprg<"
