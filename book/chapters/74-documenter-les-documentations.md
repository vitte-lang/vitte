# 74. Documenter les documentations

Niveau: Tous niveaux

Prérequis: `book/chapters/60-documentation-technique-durable.md`.
Voir aussi: `book/STYLE.md`, `book/checklist-editoriale.md`.

## Objectif

Appliquer au contenu documentaire le meme niveau de rigueur que pour le code: contrats, tests, ownership, versioning.

## Pourquoi c'est critique

1. La documentation est une interface publique.
2. Une doc incorrecte cree des bugs d'usage.
3. Une doc non maintenue augmente le cout de support.

## Contrat de meta-documentation

Chaque document doit declarer:

1. Son objectif et son public cible.
2. Son proprietaire (owner) et sa frequence de revue.
3. Sa source de verite (fichier, regle, spec, test).
4. Son statut (`draft`, `stable`, `deprecated`).

## Template pret a copier

```md
# <Titre du document>

## Objectif
<Resultat attendu, mesurable>

## Owner
<Equipe ou personne responsable>

## Source de verite
<Fichier/rule/spec/tests qui font foi>

## Definition of Done (DoD)
1. <Critere 1>
2. <Critere 2>
3. <Critere 3>

## Derniere revue
<YYYY-MM-DD> - <Qui> - <Resume court>

## Prochaine revue
<YYYY-MM-DD> - <Scope prevu>
```

## Tests de documentation

1. Link-check automatique.
2. Vérification syntaxique des snippets.
3. Validation des commandes shell presentees.
4. Detection de sections obsoletes (date + owner).

## Politique de version documentaire

1. Changement editorial mineur: correction sans impact contrat.
2. Changement fonctionnel: mise a jour du contrat et des exemples.
3. Changement breaking: section migration obligatoire.

## Runbook de maintenance

1. Revue hebdomadaire rapide: liens, erreurs evidentes.
2. Revue mensuelle structurelle: coherence et redondances.
3. Revue trimestrielle de fond: suppression/merge des chapitres faibles.

## Checklist

1. Le lecteur cible est explicitement defini.
2. Le chapitre indique quoi faire, pas seulement quoi savoir.
3. Chaque exemple a une preuve de validite.
4. Les decisions importantes sont historisees.
5. Le plan de maintenance est visible.


## Exemples progressifs (N1 -> N3)

### N1 (base): doc avec snippet Vitte

````md
## Exemple minimal
```vit
entry main at app/docmeta {
  return 0
}
```
````

Commande de preuve:

```bash
make grammar-check
```

### N2 (intermediaire): doc avec invalide + diagnostic

````md
## Exemple invalide
```vit
entry main at app/docmeta {
  return
}
```
````

Commandes QA:

```bash
make grammar-test
make book-qa
```

### N3 (avance): doc gouvernee et verifiee en CI

```md
## Owner
Equipe Langage
## DoD
1. snippets verifies
2. liens verifies
3. revue datee
```

Commandes gouvernance:

```bash
make grammar-gate
make book-qa-strict
```

### Anti-exemple

```text
Documentation sans snippets testables ni commandes de vérification.
```

## Validation rapide

1. Verifier snippet nominal et invalide.
2. Verifier QA documentaire.
3. Verifier owner + DoD + revues datees.
