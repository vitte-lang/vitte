# Complétions CLI

La commande suivante installe automatiquement les complétions utilisateur si supportée :
```bash
vitte completions --install
```

Sinon, copie les fichiers vers les emplacements usuels (extraits du message `build.rs`) :
- bash : `$XDG_DATA_HOME/bash-completion/completions`
- zsh  : `$XDG_DATA_HOME/zsh/site-functions`
- fish : `$XDG_CONFIG_HOME/fish/completions`
- pwsh : `$XDG_CONFIG_HOME/powershell/Modules`
- elvish : `$XDG_CONFIG_HOME/elvish/lib`
- nushell : `$XDG_CONFIG_HOME/nushell/completions`
