# i18n — Vitte std module

Un module d’internationalisation pragmatique :
- **locale** : parse & match tags BCP-47, négociation + `Accept-Language`.
- **plural** : catégories `zero/one/two/few/many/other` (en, fr, ru, ar, pl).
- **number** : nombres, pourcentages, monnaie (séparateurs locaux).
- **datetime** : date/heure avec patterns simples (`yyyy-MM-dd`, …).
- **relative_time** : “il y a 3 min”, “in 2 days”, FR/EN de base.
- **message** : subset MessageFormat `{count, plural, one{…} other{…}}`.
- **catalog** : catalogues par locale, fallback en cascade.
- **loader_json** : chargement JSON des catalogues.

> Objectif : light mais correct. Extensible par data CLDR plus tard.
