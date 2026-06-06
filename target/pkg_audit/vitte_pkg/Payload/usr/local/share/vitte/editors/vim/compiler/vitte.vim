" Vim compiler file for Vitte
if exists("current_compiler")
  finish
endif
let current_compiler = "vitte"

if exists(":CompilerSet") != 2
  command -nargs=* CompilerSet setlocal <args>
endif

CompilerSet makeprg=vitte\ check\ %
CompilerSet errorformat=%f:%l:%c:\ error:\ %m,%f:%l:%c:\ warning:\ %m,%f:%l:\ %m
