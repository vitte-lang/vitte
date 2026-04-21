# Grammar Changelog

## 2026-03-16

### non-breaking
- Documenté la syntaxe de surface d'appel générique explicite en EBNF sous la forme `foo[T](...)`.
- Ajouté une note d'ambiguïté pour l'analyse appel générique vs index puis appel.

### breaking
- Aucune.

### diagnostic-only
- Aucun.

## 2026-03-03

### non-breaking
- Ajouté le flux généré `grammar-surface.ebnf` avec en-tête de métadonnées versionné.
- Ajouté l'ossature du corpus de grammaire (`tests/grammar/valid`, `tests/grammar/invalid`).
- Ajouté les contrats de diagnostic et les instantanés pour les exemples de grammaire invalides.

### breaking
- Aucune.

### diagnostic-only
- Établi des instantanés de texte/code pour les diagnostics de parse dans le corpus de grammaire.
