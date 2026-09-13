" AUTO-GENERATED reference colorscheme
hi clear

if exists("syntax_on")
  syntax reset
endif

let g:colors_name = "vitte_reference"

" ---------------------------------------------------------------------------
" Core syntax groups
" ---------------------------------------------------------------------------

hi Normal       ctermfg=252 ctermbg=NONE guifg=#d0d0d0 guibg=NONE
hi Statement    ctermfg=81  guifg=#5fd7ff
hi Keyword      ctermfg=75  guifg=#5fafff
hi Conditional  ctermfg=81  guifg=#5fd7ff
hi Repeat       ctermfg=81  guifg=#5fd7ff
hi Label        ctermfg=117 guifg=#87d7ff
hi Operator     ctermfg=153 guifg=#afd7ff
hi Exception    ctermfg=203 guifg=#ff5f5f

hi Type         ctermfg=110 guifg=#87afd7
hi StorageClass ctermfg=110 guifg=#87afd7
hi Structure    ctermfg=110 guifg=#87afd7
hi Typedef      ctermfg=110 guifg=#87afd7

hi Constant     ctermfg=214 guifg=#ffaf00
hi Boolean      ctermfg=149 guifg=#afd787
hi Number       ctermfg=214 guifg=#ffaf00
hi Float        ctermfg=208 guifg=#ff8700
hi Character    ctermfg=186 guifg=#d7d787
hi String       ctermfg=186 guifg=#d7d787

hi Identifier   ctermfg=159 guifg=#afffff
hi Function     ctermfg=117 guifg=#87d7ff

hi PreProc      ctermfg=215 guifg=#ffaf5f
hi Include      ctermfg=215 guifg=#ffaf5f
hi Define       ctermfg=215 guifg=#ffaf5f
hi Macro        ctermfg=215 guifg=#ffaf5f
hi PreCondit    ctermfg=215 guifg=#ffaf5f

hi Special      ctermfg=216 guifg=#ffaf87
hi SpecialChar  ctermfg=216 guifg=#ffaf87
hi Tag          ctermfg=117 guifg=#87d7ff
hi Delimiter    ctermfg=250 guifg=#bcbcbc
hi SpecialComment ctermfg=245 guifg=#8a8a8a
hi Debug        ctermfg=203 guifg=#ff5f5f

hi Comment      ctermfg=244 guifg=#808080
hi Todo         ctermfg=196 ctermbg=NONE cterm=bold guifg=#ff0000 gui=bold

hi Error        ctermfg=203 ctermbg=NONE cterm=bold guifg=#ff5f5f gui=bold
hi ErrorMsg     ctermfg=203 ctermbg=NONE cterm=bold guifg=#ff5f5f gui=bold
hi WarningMsg   ctermfg=214 guifg=#ffaf00

hi Title        ctermfg=222 cterm=bold guifg=#ffd787 gui=bold
hi Underlined   ctermfg=117 cterm=underline guifg=#87d7ff gui=underline

hi Ignore       ctermfg=240 guifg=#585858

" ---------------------------------------------------------------------------
" Editor UI
" ---------------------------------------------------------------------------

hi Cursor       ctermfg=16  ctermbg=252 guifg=#000000 guibg=#d0d0d0
hi CursorLine   ctermbg=235 guibg=#262626
hi CursorColumn ctermbg=235 guibg=#262626
hi ColorColumn  ctermbg=234 guibg=#1c1c1c

hi LineNr       ctermfg=240 guifg=#585858
hi CursorLineNr ctermfg=222 cterm=bold guifg=#ffd787 gui=bold

hi Visual       ctermbg=24 guibg=#005f87
hi VisualNOS    ctermbg=24 guibg=#005f87

hi Search       ctermfg=16 ctermbg=214 guifg=#000000 guibg=#ffaf00
hi IncSearch    ctermfg=16 ctermbg=222 guifg=#000000 guibg=#ffd787
hi CurSearch    ctermfg=16 ctermbg=81  guifg=#000000 guibg=#5fd7ff

hi MatchParen   ctermfg=16 ctermbg=117 cterm=bold guifg=#000000 guibg=#87d7ff gui=bold

hi StatusLine   ctermfg=252 ctermbg=238 cterm=bold guifg=#d0d0d0 guibg=#444444 gui=bold
hi StatusLineNC ctermfg=244 ctermbg=236 guifg=#808080 guibg=#303030

hi VertSplit    ctermfg=238 ctermbg=NONE guifg=#444444 guibg=NONE
hi WinSeparator ctermfg=238 guifg=#444444

hi TabLine      ctermfg=244 ctermbg=236 guifg=#808080 guibg=#303030
hi TabLineFill  ctermfg=238 ctermbg=234 guifg=#444444 guibg=#1c1c1c
hi TabLineSel   ctermfg=222 ctermbg=238 cterm=bold guifg=#ffd787 guibg=#444444 gui=bold

hi Folded       ctermfg=110 ctermbg=234 guifg=#87afd7 guibg=#1c1c1c
hi FoldColumn   ctermfg=110 ctermbg=NONE guifg=#87afd7 guibg=NONE

hi SignColumn   ctermfg=244 ctermbg=NONE guifg=#808080 guibg=NONE

hi Pmenu        ctermfg=252 ctermbg=236 guifg=#d0d0d0 guibg=#303030
hi PmenuSel     ctermfg=16  ctermbg=117 guifg=#000000 guibg=#87d7ff
hi PmenuSbar    ctermbg=238 guibg=#444444
hi PmenuThumb   ctermbg=244 guibg=#808080

hi WildMenu     ctermfg=16 ctermbg=222 guifg=#000000 guibg=#ffd787

hi Directory    ctermfg=117 guifg=#87d7ff
hi NonText      ctermfg=238 guifg=#444444
hi SpecialKey   ctermfg=238 guifg=#444444
hi EndOfBuffer  ctermfg=234 guifg=#1c1c1c

hi Question     ctermfg=149 guifg=#afd787
hi MoreMsg      ctermfg=117 guifg=#87d7ff
hi ModeMsg      ctermfg=222 cterm=bold guifg=#ffd787 gui=bold

" ---------------------------------------------------------------------------
" Diff
" ---------------------------------------------------------------------------

hi DiffAdd      ctermfg=149 ctermbg=22 guifg=#afd787 guibg=#005f00
hi DiffChange   ctermfg=222 ctermbg=58 guifg=#ffd787 guibg=#5f5f00
hi DiffDelete   ctermfg=203 ctermbg=52 guifg=#ff5f5f guibg=#5f0000
hi DiffText     ctermfg=16  ctermbg=214 cterm=bold guifg=#000000 guibg=#ffaf00 gui=bold

" ---------------------------------------------------------------------------
" Diagnostics
" ---------------------------------------------------------------------------

hi DiagnosticError ctermfg=203 guifg=#ff5f5f
hi DiagnosticWarn  ctermfg=214 guifg=#ffaf00
hi DiagnosticInfo  ctermfg=117 guifg=#87d7ff
hi DiagnosticHint  ctermfg=149 guifg=#afd787

hi DiagnosticUnderlineError cterm=underline gui=underline guisp=#ff5f5f
hi DiagnosticUnderlineWarn  cterm=underline gui=underline guisp=#ffaf00
hi DiagnosticUnderlineInfo  cterm=underline gui=underline guisp=#87d7ff
hi DiagnosticUnderlineHint  cterm=underline gui=underline guisp=#afd787

" ---------------------------------------------------------------------------
" Vitte-specific groups
" ---------------------------------------------------------------------------

hi VitteKeyword        ctermfg=75  guifg=#5fafff
hi VitteControl        ctermfg=81  guifg=#5fd7ff
hi VitteType           ctermfg=110 guifg=#87afd7
hi VitteBuiltinType    ctermfg=153 guifg=#afd7ff
hi VitteFunction       ctermfg=117 guifg=#87d7ff
hi VitteBuiltin        ctermfg=159 guifg=#afffff
hi VitteConstant       ctermfg=214 guifg=#ffaf00
hi VitteBoolean        ctermfg=149 guifg=#afd787
hi VitteNumber         ctermfg=214 guifg=#ffaf00
hi VitteFloat          ctermfg=208 guifg=#ff8700
hi VitteString         ctermfg=186 guifg=#d7d787
hi VitteCharacter      ctermfg=186 guifg=#d7d787
hi VitteComment        ctermfg=244 guifg=#808080
hi VitteDocComment     ctermfg=245 guifg=#8a8a8a
hi VitteAttribute      ctermfg=215 guifg=#ffaf5f
hi VitteAnnotation     ctermfg=215 guifg=#ffaf5f
hi VitteNamespace      ctermfg=117 guifg=#87d7ff
hi VitteModule         ctermfg=117 guifg=#87d7ff
hi VitteField          ctermfg=159 guifg=#afffff
hi VitteParameter      ctermfg=153 guifg=#afd7ff
hi VitteOperator       ctermfg=216 guifg=#ffaf87
hi VitteDelimiter      ctermfg=250 guifg=#bcbcbc
hi VitteLifetime       ctermfg=180 guifg=#d7af87
hi VitteGeneric        ctermfg=153 guifg=#afd7ff
hi VitteMacro          ctermfg=215 guifg=#ffaf5f
hi VitteError          ctermfg=203 cterm=bold guifg=#ff5f5f gui=bold
hi VitteTodo           ctermfg=196 cterm=bold guifg=#ff0000 gui=bold

" ---------------------------------------------------------------------------
" Links for compatibility
" ---------------------------------------------------------------------------

hi! link vimCommentString String
hi! link vimTodo Todo

hi! link DiagnosticFloatingError DiagnosticError
hi! link DiagnosticFloatingWarn  DiagnosticWarn
hi! link DiagnosticFloatingInfo  DiagnosticInfo
hi! link DiagnosticFloatingHint  DiagnosticHint
