# 15. HIR, MIR et pipeline

La compilation est un pipeline. La comprendre vous fait gagner un temps énorme quand vous déboguez un programme ou un bug du compilateur.

## Les grandes étapes

Parsing et AST : le texte devient structure. HIR : les formes sont normalisées. MIR : le programme est prêt pour la génération. Backend : le binaire est produit.

## Pourquoi cela compte

Quand un bug apparaît, savoir dans quel étage il se trouve réduit immédiatement l’espace de recherche. Vous pouvez poser de meilleures questions et éviter les hypothèses floues.

## Méthode de diagnostic

Reproduire le problème. Localiser l’étape qui change l’information. Inspecter les sorties intermédiaires.

## HIR : rendre explicite

Le HIR simplifie la structure pour rendre les décisions explicites. C’est la première étape où le compilateur commence à “comprendre” ce que vous avez écrit.

## MIR : préparer le terrain

Le MIR est le niveau où les transformations deviennent mécaniques. L’objectif est d’obtenir une forme qui se traduit proprement vers le backend.

## À retenir

Comprendre la pipeline, même à haut niveau, est une compétence centrale pour travailler proche de la machine.


## Exemple guidé : diagnostiquer un bug

Supposez qu’un `match` se compile mal. Essayez de localiser si le bug est dans le parsing, le HIR, ou le MIR. Cette méthode réduit drastiquement le temps de debug.

## Checklist pipeline

Vous savez reproduire le bug. Vous savez isoler l’étape fautive. Vous savez vérifier l’output intermédiaire.


## Exercice : tracer une erreur

Simulez une erreur de parsing, puis une erreur de type. Notez comment elles apparaissent dans le pipeline. Cette observation vous aide à classifier rapidement les bugs.


## Code complet (API actuelle)

Exemple conceptuel : compiler en mode normal, puis activer les sorties intermédiaires si votre driver les expose.

```sh
vitte build src/main.vit
```

## API idéale (future)

Un mode `--explain-pipeline` qui enregistre automatiquement les étapes dans un dossier daté.

