

# vitte-completion

Génération **d’autocomplétion shell** pour la suite d’outils Vitte.  
`vitte-completion` fournit les scripts et interfaces permettant la complétion dynamique dans les shells `bash`, `zsh`, `fish` et `powershell`.

---

## Objectifs

- Fournir des complétions précises et rapides pour toutes les sous-commandes `vitte`.  
- Détecter automatiquement le shell courant.  
- Supporter les options longues/courtes, valeurs, énumérations et chemins.  
- Offrir un format stable, versionné et testable en CI.  
- Permettre la génération locale ou embarquée (`vitte --generate-completion`).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `model`       | Schéma des commandes, options et arguments |
| `introspect`  | Extraction dynamique depuis `vitte-cli` / `vitte-args` |
| `render`      | Rendu des scripts pour chaque shell (`bash`, `zsh`, `fish`, `pwsh`) |
| `detect`      | Détection du shell et des chemins d’installation |
| `install`     | Helpers pour installer/désinstaller les scripts |
| `tests`       | Snapshots et tests de cohérence par shell |

---

## Exemple d’utilisation

```bash
# Générer la complétion pour bash
vitte completion bash > /etc/bash_completion.d/vitte

# Zsh
vitte completion zsh > ~/.zfunc/_vitte

# Fish
vitte completion fish > ~/.config/fish/completions/vitte.fish

# Powershell
vitte completion pwsh > $PROFILE
```

---

## Intégration

- `vitte-cli` : expose la sous-commande `completion`.  
- `vitte-args` : introspection des options et sous-commandes.  
- `vitte-utils` : utilitaires pour sérialisation et chemins d’accès.  
- `vitte-docgen` : réutilise le modèle pour générer les manuels CLI.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-cli = { path = "../vitte-cli", version = "0.1.0" }
vitte-args = { path = "../vitte-args", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
clap = { version = "4", features = ["derive"] }
``` 

---

## Tests

```bash
cargo test -p vitte-completion
```

- Tests unitaires par shell.  
- Snapshots des complétions générées.  
- Validation de cohérence entre sous-commandes et CLI réelle.

---

## Roadmap

- [ ] Détection automatique des chemins d’installation selon l’OS.  
- [ ] Intégration CI pour vérifier la stabilité des complétions.  
- [ ] Support des shells exotiques (`xonsh`, `elvish`).  
- [ ] Complétion contextuelle dynamique (répertoires, fichiers Vitte).  
- [ ] Installation via `vitte setup completion`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau