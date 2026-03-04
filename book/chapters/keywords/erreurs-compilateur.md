# Glossaire des erreurs compilateur fréquentes

Ce mini glossaire normalise les diagnostics utilisés dans les fiches keywords.

| Message type | Signification | Action recommandée |
| --- | --- | --- |
| `unexpected token` | La grammaire attendait une autre forme. | Vérifier la syntaxe canonique du mot-clé. |
| `type mismatch` | Un type fourni ne correspond pas au contrat attendu. | Aligner les types d’entrée/sortie avant exécution. |
| `unknown symbol` | Référence à un nom non déclaré ou hors portée. | Vérifier `space`, `use`, `share`, et l’ordre de déclaration. |
| `incomplete branch` | Couverture incomplète d’un flux conditionnel. | Ajouter le cas manquant (`otherwise`) ou simplifier la logique. |
| `unsafe context required` | Instruction sensible utilisée hors zone autorisée. | Encadrer par `unsafe` et ajouter les gardes métier avant. |
| `invalid entrypoint` | Point d’entrée mal déclaré. | Corriger la forme `entry ... at ...`. |
