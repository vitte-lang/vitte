# Vitte in VS Code

This folder contains a minimal VS Code language definition for Vitte.

## Install locally (user settings)

1. Copy files to your VS Code user folder:

- `vitte.tmLanguage.json`
- `language-configuration.json`
- `snippets/vitte.json`

2. Add a user settings entry (or use the included `settings.json` as a base):

```json
{
  "files.associations": { "*.vit": "vitte" }
}
```

## Workspace tasks

This repo includes `/.vscode/tasks.json` with `vitte` tasks and a problem matcher that
understands `file:line:col` diagnostics.

## Install as a workspace extension

You can also create a minimal extension by placing these files into a VS Code extension template.
