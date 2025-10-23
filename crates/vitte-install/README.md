# vitte-install

Gestionnaire **d’installation et de distribution** pour l’écosystème Vitte.  
`vitte-install` permet l’installation, la mise à jour et la suppression des outils, modules et dépendances de Vitte à partir de sources locales ou distantes.

---

## Objectifs

- Offrir un **système d’installation unifié** pour tous les modules Vitte.  
- Supporter les installations depuis **crates.io**, **GitHub**, **tarballs** et **sources locales**.  
- Vérifier automatiquement les versions, signatures et compatibilités.  
- Gérer les installations globales et par projet.  
- Intégration transparente avec `vitte-cli`, `vitte-build` et `vitte-update`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `registry`    | Gestion des sources d’installation (index, URLs, Git) |
| `resolver`    | Résolution des versions et dépendances |
| `installer`   | Téléchargement et extraction des artefacts |
| `manifest`    | Lecture et validation des fichiers `vitte.toml` |
| `checksum`    | Vérification d’intégrité et de signature SHA/BLAKE3 |
| `linker`      | Liaison symbolique et gestion du PATH |
| `uninstall`   | Nettoyage et suppression des paquets installés |
| `tests`       | Vérification des processus d’installation et rollback |

---

## Exemple d’utilisation

```bash
# Installer la suite complète
vitte install all

# Installer un module spécifique
vitte install vitte-docgen

# Installer depuis une archive
vitte install https://github.com/vitte-lang/vitte/releases/download/V2025.09/vitte.tar.gz

# Désinstaller un module
vitte uninstall vitte-cache
```

---

## Intégration

- `vitte-cli` : sous-commande principale `vitte install`.  
- `vitte-build` : installation automatique des dépendances.  
- `vitte-update` : gestion des mises à jour incrémentales.  
- `vitte-docgen` : installation des fichiers de documentation locale.  
- `vitte-lsp` : installation des composants du serveur de langage.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-cache = { path = "../vitte-cache", version = "0.1.0" }

reqwest = { version = "0.12", features = ["blocking", "json"] }
tar = "0.4"
flate2 = "1.0"
indicatif = "0.17"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `tempfile` pour installation atomique, `tokio` pour mode async.

---

## Tests

```bash
cargo test -p vitte-install
```

- Tests d’installation locale et distante.  
- Tests de vérification d’intégrité SHA/BLAKE3.  
- Tests de compatibilité multi-version.  
- Tests de rollback en cas d’échec.

---

## Roadmap

- [ ] Support complet des dépôts Git privés.  
- [ ] Vérification cryptographique PGP des binaires.  
- [ ] Installation parallèle des dépendances.  
- [ ] Intégration Homebrew/Debian automatique.  
- [ ] Mode offline avec cache persistant.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
