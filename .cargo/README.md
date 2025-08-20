# 📦 Dossier `.cargo/`

Ce dossier contient la configuration **partagée** et les exemples de configuration **locale** pour le projet.

## Structure

- `config.local.example` → modèle de configuration locale.
  - À copier en `.cargo/config.local.toml` (non versionné).
  - Sert à définir des alias, flags de compilation, proxies, etc.
- (optionnel) `config.toml` → configuration partagée par l’ensemble du workspace.
  - Ex : définir des mirrors de crates.io, forcer des profils communs, etc.

## Bonnes pratiques

- **Ne versionnez jamais vos réglages persos** :  
  Ajoutez-les dans `.cargo/config.local.toml` (gitignore déjà configuré).
- **Gardez le repo propre** : utilisez `config.local.toml` pour vos besoins
  (proxy d’entreprise, options de debug, alias perso).
- **Aliases sûrs** : n’utilisez pas d’alias qui masquent des sous-commandes existantes (`fmt`, `test`, `build`, etc.).

## Exemple rapide

```sh
# Copier le fichier modèle
cp .cargo/config.local.example .cargo/config.local.toml

# Ajouter un alias local
echo "[alias]" >> .cargo/config.local.toml
echo "mytest = \"test --workspace --all-features\"" >> .cargo/config.local.toml
