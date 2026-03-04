# 73. Plan de montee en competence equipe

Niveau: Tous niveaux

Prérequis: `book/chapters/33-chapitres-suggeres.md`.

## Objectif

Organiser la progression equipe sur 8 a 12 semaines avec objectifs mesurables.

## Plan type

1. Semaine 1-2: fondamentaux langage et grammaire.
2. Semaine 3-4: modules, types et procedures robustes.
3. Semaine 5-6: diagnostics, tests, non-regression.
4. Semaine 7-8: performance, observabilité, projets.

## Indicateurs

1. Taux de tests verts.
2. Temps moyen de resolution incident.
3. Defauts critiques en baisse.

## Documentation a produire

1. Matrice competences x personnes x niveau cible.
2. Parcours de lecture recommande par role.
3. Exercices obligatoires et criteres de reussite.
4. Journal de progression hebdomadaire.
5. Retrospective mensuelle (lacunes, plan d'action, proprietaires).

## Grille d'evaluation par niveau

| Niveau | Competences observables | Preuves attendues |
| --- | --- | --- |
| L1 | Lit et execute un exemple sans aide; corrige une erreur de syntaxe simple | PR avec exemple fonctionnel + correction validee |
| L2 | Refactorise une procedure avec types explicites; ajoute tests unitaires lisibles | PR avec refactoring + tests verts + note de migration |
| L3 | Concoit un module stable, anticipe compatibilité, pilote un incident | RFC courte + implementation + postmortem/action plan |


## Exemples progressifs (N1 -> N3)

### N1 (L1): exercice syntaxe

Snippet Vitte:

```vit
entry main at app/learn {
  return 0
}
```

Commande preuve:

```bash
make grammar-check
```

### N2 (L2): exercice refactor + tests

Snippet Vitte:

```vit
proc normalize(x: int) -> int {
  if x < 0 { give 0 }
  give x
}
```

Commande preuve:

```bash
make grammar-test
```

### N3 (L3): exercice incident + postmortem

Snippet Vitte (cas echec):

```vit
entry main at app/learn {
  return missing_symbol
}
```

Runbook formation:

```bash
# reproduire
make grammar-test
# corriger
make grammar-gate
```

### Anti-exemple

```text
Objectifs formation sans livrables ni commandes de validation.
```

## Validation rapide

1. Associer chaque niveau a un snippet.
2. Associer chaque niveau a une commande preuve.
3. Tracer progression hebdomadaire.

