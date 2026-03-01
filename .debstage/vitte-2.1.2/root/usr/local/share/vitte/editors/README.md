# Vitte editor support

This folder provides syntax highlighting for Vitte (`.vit`).

## Quick install (binary + syntax for 3 editors)

From repository root:

```sh
make install
```

This installs:

- binary: `/usr/local/bin/vitte` (override with `PREFIX=...`)
- Vim files: `~/.vim/{syntax,indent,ftdetect,ftplugin,compiler}/`
- Emacs files: `~/.emacs.d/lisp/`
- Nano file: `~/.config/nano/vitte.nanorc` and auto-include in `~/.nanorc`

## Vim

1. Copy the syntax file to your Vim runtime:

   - `~/.vim/syntax/vitte.vim`

2. Copy the indent + ftdetect files:

   - `~/.vim/indent/vitte.vim`
   - `~/.vim/ftdetect/vitte.vim`
   - `~/.vim/ftplugin/vitte.vim`
   - `~/.vim/compiler/vitte.vim`

3. (Optional) UltiSnips snippets:

   - `~/.vim/UltiSnips/vitte.snippets`

4. Add a filetype rule (if you don't already have one):

   - `~/.vim/ftdetect/vitte.vim`

```vim
au BufRead,BufNewFile *.vit set filetype=vitte
```

5. Use compiler integration:

```vim
:compiler vitte
:make
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

;; Use the new vitte binary explicitly (adjust path as needed):
(setq vitte-binary "/path/to/vitte/bin/vitte")
```

4. Useful commands in `vitte-mode`:

- `C-c C-c` -> `vitte check <current-file>`
- `C-c C-b` -> `vitte build <current-file>`
- `C-c C-p` -> `vitte parse <current-file>`

## Nano

1. Copy the nanorc:

   - `~/.config/nano/vitte.nanorc`

2. Include it in your `~/.nanorc`:

```nanorc
include "~/.config/nano/vitte.nanorc"
```

3. Use the new vitte binary from terminal (Nano handles syntax only):

```bash
/path/to/vitte/bin/vitte check path/to/file.vit
```

## VS Code

This repo includes a minimal VS Code language definition under `editors/vscode/`:

- grammar: `editors/vscode/vitte.tmLanguage.json`
- config: `editors/vscode/language-configuration.json`
- snippets: `editors/vscode/snippets/vitte.json`
- extension manifest: `editors/vscode/package.json`

For module-centric Vitte work in this repository, use:

- module snippets (`space`, `pull`, `share`, `module`) from `editors/vscode/snippets/vitte.json`
- workspace module tasks from `.vscode/tasks.json` (graph/doctor/strict-modules/modules-tests)

To use it locally, copy the files into a VS Code extension folder or embed them in your workspace.

## Verify install

```sh
which vitte
vitte --version
```

Then open a `.vit` file in Vim, Emacs, and Nano and confirm syntax highlighting is active.

Completion notes:
- Vim: `C-x C-o` (uses `omnifunc=syntaxcomplete#Complete` in `ftplugin`).
- Emacs: `M-TAB` / `completion-at-point` (keywords/types/builtins).
- Nano: no native semantic autocompletion; syntax highlighting only.

## Uninstall (manual)

```sh
rm -f /usr/local/bin/vitte
rm -f ~/.vim/syntax/vitte.vim ~/.vim/indent/vitte.vim ~/.vim/ftdetect/vitte.vim ~/.vim/ftplugin/vitte.vim ~/.vim/compiler/vitte.vim
rm -f ~/.emacs.d/lisp/vitte-mode.el ~/.emacs.d/lisp/vitte-indent.el
rm -f ~/.config/nano/vitte.nanorc
```

Also remove this line from `~/.nanorc` if present:

```nanorc
include "~/.config/nano/vitte.nanorc"
```

## Troubleshooting

- `vitte: command not found`: ensure `/usr/local/bin` (or your `PREFIX/bin`) is in `PATH`.
- No highlighting in Vim: run `:set filetype?` and verify it returns `vitte`.
- Emacs keybindings missing: verify `(require 'vitte-mode)` is loaded and reopen the buffer.
- Nano include ignored: check `~/.nanorc` contains the include line exactly once.

## Sources in this repo

- Vim: `editors/vim/`
- Emacs: `editors/emacs/`
- Nano: `editors/nano/`
- VS Code: `editors/vscode/`
