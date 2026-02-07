# Vitte editor support

This folder provides syntax highlighting for Vitte (`.vit`).

## Vim

1. Copy the syntax file to your Vim runtime:

   - `~/.vim/syntax/vitte.vim`

2. Copy the indent + ftdetect files:

   - `~/.vim/indent/vitte.vim`
   - `~/.vim/ftdetect/vitte.vim`

3. (Optional) UltiSnips snippets:

   - `~/.vim/UltiSnips/vitte.snippets`

4. Add a filetype rule (if you don't already have one):

   - `~/.vim/ftdetect/vitte.vim`

```vim
au BufRead,BufNewFile *.vit set filetype=vitte
```

## Emacs

1. Copy the mode file:

   - `~/.emacs.d/lisp/vitte-mode.el`
   - `~/.emacs.d/lisp/vitte-indent.el`

2. (Optional) Yasnippet snippets:

   - `~/.emacs.d/snippets/vitte-mode/`

3. Add to your Emacs init:

```elisp
(add-to-list 'load-path "~/.emacs.d/lisp")
(require 'vitte-mode)
```

## Nano

1. Copy the nanorc:

   - `~/.config/nano/vitte.nanorc`

2. Include it in your `~/.nanorc`:

```nanorc
include "~/.config/nano/vitte.nanorc"
```

## VS Code

This repo includes a minimal VS Code language definition under `editors/vscode/`:

- grammar: `editors/vscode/vitte.tmLanguage.json`
- config: `editors/vscode/language-configuration.json`
- snippets: `editors/vscode/snippets/vitte.json`
- extension manifest: `editors/vscode/package.json`

To use it locally, copy the files into a VS Code extension folder or embed them in your workspace.

## Sources in this repo

- Vim: `editors/vim/`
- Emacs: `editors/emacs/`
- Nano: `editors/nano/`
- VS Code: `editors/vscode/`
