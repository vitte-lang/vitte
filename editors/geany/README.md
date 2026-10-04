# Vitte support for Geany

This integration provides syntax highlighting, file associations for `.vit`,
`.vitl`, and `.vitte`, snippets, project-aware build commands, diagnostics,
formatting, and symbol extraction through Geany's standard filetype system.

## Install

From the repository:

```sh
./editors/geany/install_geany.sh
```

The installer detects Linux, macOS, and Windows-compatible Geany config paths.
Use `GEANY_HOME=/path/to/geany` to target one configuration explicitly.

Useful options:

```sh
./editors/geany/install_geany.sh --dry-run
./editors/geany/install_geany.sh --wd=project
./editors/geany/install_geany.sh --no-backup
./editors/geany/uninstall_geany.sh
```

Restart Geany after installation so it reloads filetype definitions.

## Commands

The Vitte filetype adds current-file actions for check, build, run, test,
format, and parse. Project actions use Geany's `%p` project directory and
file actions use `%d`, so paths containing spaces are handled safely.

The compiler executable is resolved through the environment inherited by
Geany. If `vitte` is not on that PATH, configure Geany's PATH or invoke the
installer from a desktop environment with the same PATH as Geany.
