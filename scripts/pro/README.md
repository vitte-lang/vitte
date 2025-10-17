# scripts/pro — outils avancés Vitte

Scripts supplémentaires pour un workflow "release engineering" : couverture, audit des dépendances, docs…

## `coverage.sh`
- utilise `cargo tarpaulin` pour générer un rapport HTML dans `coverage/`
- installe automatiquement tarpaulin si nécessaire

## `deps-audit.sh`
- lance `cargo deny check` + `cargo outdated` avec surlignement des erreurs/avertissements
- installe `cargo-deny` et `cargo-outdated` si besoin

## `doclint.sh`
- construit la documentation (`cargo doc --document-private-items`) et colore les diagnostics

## `vitte-audit.sh`
- exploite `vitte deps list` / `vitte deps sync` pour auditer le `vitte.toml`
- vérifie que le binaire `vitte` est disponible (ou `bin/bin/vitte`)
