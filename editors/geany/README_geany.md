# Geany Integration For Vitte

This folder provides Geany support for `.vit` files.

Included files:
- `filetypes.vitte.conf`: syntax, keywords, and editor behavior.
- `snippets.vitte.conf`: Vitte code snippets.
- `filetype_extensions.conf`: maps `*.vit` to Vitte.
- `filetypes.common`: shared Geany settings used by installer.
- `install_geany.sh`: installs config into user Geany directory.
- `uninstall_geany.sh`: removes installed Geany Vitte config.

Manual install:
1. Run `install_geany.sh`.
2. Restart Geany.
3. Open a `.vit` file and confirm language mode is Vitte.

Installed package path:
- `/usr/local/share/vitte/editors/geany`
