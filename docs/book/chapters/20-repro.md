# 20. Reproductibilité et builds

Un build reproductible est un build fiable. Si le même code produit des binaires différents, vous perdez votre base de confiance.

## Principe

Fixer les versions d’outils. Réduire les sources d’entropie (timestamps, chemins, etc.). Comparer les artefacts.

## Pourquoi c’est important

La reproductibilité est une base de sécurité et de débogage. Elle facilite aussi la collaboration.

## Technique simple

Commencez par comparer des hashes et comprendre chaque différence. La reproductibilité est une discipline, pas un bouton magique.

## Erreurs courantes

Compiler sur deux machines sans aligner les outils. Comparer des binaires signés sans enlever les signatures. Ne pas documenter le pipeline de build.

## À retenir

Un build reproductible est un build explicable.


## Exemple guidé : build reproductible

Construisez un binaire deux fois, comparez les hashes. Puis identifiez la source de divergence (timestamp, chemin, signature). C’est un exercice fondamental.

## Checklist repro

Outils alignés. Sources d’entropie réduites. Comparaison systématique.


## Exercice : identifier l’entropie

Construisez deux fois, comparez les hashes, puis identifiez la source d’entropie. Documentez‑la dans le README du projet.


## Code complet (API actuelle)

```sh
make repro
```

## API idéale (future)

Un `vitte repro --explain` qui liste toutes les sources d’entropie détectées.

