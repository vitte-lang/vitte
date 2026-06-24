# Review des traductions zh-CN

Merci de relire et affiner les traductions générées automatiquement pour les diagnostics.

Contexte
- Les fichiers ci-après ont été générés automatiquement à partir de `pkgout/diagnostics_for_translation.csv` :
  - `locales/zh-CN/diagnostics.ftl`
  - `locales/zh-CN/diagnostics_explain.ftl`
- Les clés manquantes ont été automatiquement complétées avec des messages anglais puis traduites par un script heuristique.

Objectif
- Vérifier et corriger chaque message afin qu'il soit naturel et précis en chinois (简体中文).
- Remplacer toute occurrence anglaise restante.

Ressources
- CSV source: `pkgout/diagnostics_for_translation.csv`
- PR d'origine pour l'export CSV: https://github.com/vitte-lang/vitte/pull/90
- Documentation contexte : `docs/compiler/diagnostics.md`

Tâches suggérées pour le relecteur
1. Ouvrir `locales/zh-CN/diagnostics.ftl` et corriger les messages courts.
2. Ouvrir `locales/zh-CN/diagnostics_explain.ftl` et affiner les sections `.summary`, `.cause`, `.step1`, `.fix`, `.example` pour chaque code d'erreur.
3. Faire des commits de corrections directement sur cette branche de revue.

Critères d'acceptation
- Tous les messages sont traduits en chinois (pas de placeholders anglais restants).
- Exécuter `python3 tools/check_diagnostics_locales.py` : doit sortir `OK`.

Contact
- Pour questions, répondre sur la PR et mentionner @vincent.
