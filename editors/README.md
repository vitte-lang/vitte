# Vitte editor support

This folder provides syntax highlighting for Vitte (`.vit`).

## Quick install (binary + syntax for 4 editors)

From repository root:

```sh
make install
```

This installs:

- binary: `/usr/local/bin/vitte` (override with `PREFIX=...`)
- Vim files: `~/.vim/{syntax,indent,ftdetect,ftplugin,compiler}/`
- Emacs files: `~/.emacs.d/lisp/`
- Nano file: `~/.config/nano/vitte.nanorc` and auto-include in `~/.nanorc`
- Geany file: `~/.config/geany/filedefs/filetypes.vitte.conf`

Install in another home/prefix:

```sh
make install-editors USER_HOME=/tmp/demo-home PREFIX=/opt/vitte
```

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

## Geany

1. Install automatically (recommended):

```bash
./editors/geany/install_geany.sh
```

Or via Makefile:

```bash
make install-geany
```

Working-directory mode (mono/multi-root):

```bash
# file dir (%d) [default]
VITTE_GEANY_WD_MODE=file make install-geany
# project dir (%p)
VITTE_GEANY_WD_MODE=project make install-geany
# current geany process dir (%c)
VITTE_GEANY_WD_MODE=current make install-geany
```

2. Or install manually, copy:

   - `~/.config/geany/filedefs/filetypes.vitte.conf`
   - optional common fragment: `~/.config/geany/filedefs/filetypes.common` (merge from `editors/geany/filetypes.common`)
   - `~/.config/geany/filetype_extensions.conf` (contains `Vitte=*.vit;`)
   - `~/.config/geany/snippets.conf` (append section from `editors/geany/snippets.vitte.conf`)

3. Restart Geany.

4. Open a `.vit` file. Mapping is automatic via `filetype_extensions.conf`.
   If needed:

   - `Document -> Set Filetype -> Programming Languages -> vitte`

5. Use Build menu commands:

- `Check` -> `vitte check <current-file>`
- `Build` -> `vitte build <current-file>`
- `Run` -> `vitte run <current-file>`
- `Test` -> `vitte test <project-dir>`
- `Format` -> `vitte fmt <current-file>`
- `Parse` -> `vitte parse <current-file>`

Verify local install:

```bash
make geany-install-check
```

Snippets (Geany autocomplete snippets):
- `entry_main`
- `proc`
- `form`
- `trait`

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

Then open a `.vit` file in Vim, Emacs, Nano, and Geany and confirm syntax highlighting is active.

Completion notes:
- Vim: `C-x C-o` (uses `omnifunc=syntaxcomplete#Complete` in `ftplugin`).
- Emacs: `M-TAB` / `completion-at-point` (keywords/types/builtins).
- Nano: no native semantic autocompletion; syntax highlighting only.
- Geany: no native semantic autocompletion here; syntax + build commands only.

## Uninstall (manual)

```sh
rm -f /usr/local/bin/vitte
rm -f ~/.vim/syntax/vitte.vim ~/.vim/indent/vitte.vim ~/.vim/ftdetect/vitte.vim ~/.vim/ftplugin/vitte.vim ~/.vim/compiler/vitte.vim
rm -f ~/.emacs.d/lisp/vitte-mode.el ~/.emacs.d/lisp/vitte-indent.el
rm -f ~/.config/nano/vitte.nanorc
rm -f ~/.config/geany/filedefs/filetypes.vitte.conf
sed -i '/^Vitte=\\*\\.vit;$/d' ~/.config/geany/filetype_extensions.conf
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
- Geany not detected: verify file name is exactly `filetypes.vitte.conf` in `~/.config/geany/filedefs/`, then restart Geany.
- Geany on macOS: config path is usually `~/Library/Application Support/geany`.
- Geany on Windows: config path is usually `%APPDATA%\\geany`.
- If snippets duplicate, run uninstall then install:
  - `make uninstall-geany`
  - `make install-geany`

## Sources in this repo

- Vim: `editors/vim/`
- Emacs: `editors/emacs/`
- Nano: `editors/nano/`
- Geany: `editors/geany/`
  - filetype: `editors/geany/filetypes.vitte.conf`
  - extension map: `editors/geany/filetype_extensions.conf`
  - snippets: `editors/geany/snippets.vitte.conf`
  - installer: `editors/geany/install_geany.sh`
- VS Code: `editors/vscode/`
