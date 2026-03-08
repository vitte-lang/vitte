# VITTE Highlighting (Vim/Emacs/Nano)

## Source of truth
- Grammar: `editors/grammar/vitte_highlight_grammar.json`
- Generated assets:
  - `editors/vim/syntax/vitte.vim`
  - `editors/vim/after/syntax/vitte.vim`
  - `editors/vim/colors/vitte_reference.vim`
  - `editors/emacs/vitte-mode.el`
  - `editors/nano/vitte.nanorc`
  - `editors/geany/filetypes.vitte.conf`
- `editors/tree-sitter/queries/highlights.scm`
- `editors/tree-sitter/grammar.js`

## Commands
- Regenerate: `make generate-highlights`
- Verify snapshots + coverage: `make highlights-ci`
- Refresh snapshots: `make highlight-snapshots-update`
- Geany lint: `make geany-lint`
- Tree-sitter validate/smoke: `make tree-sitter-vitte-ci`

## Nano setup
Add in `~/.nanorc`:

```nanorc
include "/absolute/path/to/vitte/editors/nano/vitte.nanorc"
```

## Strict highlight mode
The generated rules explicitly highlight:
- diagnostics codes: `VITTE-*`
- package aliases: `*_pkg`
- contract sections: `ROLE-CONTRACT`
- APIs: `diagnostics_*`, `quickfix_*`, `doctor_*`

## CI gate
`make highlights-ci` fails if declaration constructs are not covered in **both** Vim and Emacs, or if strict constructs are missing.

## Geany install/troubleshooting
- Install only Geany config: `make install-geany`
- Uninstall Geany config: `make uninstall-geany`
- Check local Geany wiring: `make geany-install-check`
- `VITTE_GEANY_WD_MODE=file|project|current` controls build-menu working dir token (`%d`, `%p`, `%c`).
- Linux default config path: `${XDG_CONFIG_HOME:-~/.config}/geany`
- macOS default config path: `~/Library/Application Support/geany`
- Windows default config path: `%APPDATA%\\geany`

## Tree-sitter
- Parser sources live in `editors/tree-sitter/`.
- `make tree-sitter-vitte-validate` always runs static checks.
- `make tree-sitter-vitte-smoke` runs `tree-sitter generate/test` only if CLI is installed; otherwise it reports a skip.
