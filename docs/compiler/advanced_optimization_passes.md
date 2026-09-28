# Advanced Optimization Passes (175)

Objectif: améliorer les performances réelles via un pipeline MIR structuré.

## Passes couvertes (fondation)

- inline (candidats comptés)
- loop simplification (candidats de simplification de boucle)
- branch folding (branches constantes identifiées)
- scalar replacement (surface réservée)
- dead store elimination (candidats détectés)
- value numbering (surface réservée)

## Intégration actuelle

- Les stats de transformation MIR exposent ces compteurs dans `MirTransformStats`.
- Le résumé de passes inclut inline/loops/branch-fold/dead-store.
- Le pipeline conserve la simplification CFG existante comme transformation active principale.

## Étapes suivantes

- rendre inline/branch-fold/dead-store transformationnels (pas uniquement analytiques),
- ajouter valeur de coût (cost model),
- activer value numbering inter-blocs.
