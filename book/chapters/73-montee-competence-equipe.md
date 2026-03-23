# 73. Plan de montee en competence equipe

Niveau: Tous niveaux

Prérequis: `book/chapters/33-chapitres-suggeres.md`.
Voir aussi: à définir.

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
  // Sortie programme: code de retour observable
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
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give 0 }
  // Sortie locale: valeur retournee par la procedure
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
  // Sortie programme: code de retour observable
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

## Pourquoi

Cette section explicite la valeur pratique: réduire les erreurs, accélérer le diagnostic et stabiliser les évolutions.

## Test mental

Question de contrôle: si vous modifiez une hypothèse clé, quel résultat doit changer et pourquoi?

## À faire

1. Exécuter l’exemple nominal.
2. Introduire un cas limite.
3. Vérifier la sortie et documenter l’écart.

## Corrigé minimal

Corrigé: conserver la version la plus simple qui respecte le contrat, puis ajouter un test de non-régression.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **plan de montee en competence equipe**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/learn {
  // Sortie programme: code de retour observable
  return 0
}
```

Lecture ligne par ligne:
1. `entry main at app/learn {` -> définit le point d'entrée du scénario.
2. `return 0` -> renvoie la sortie vérifiable.
3. `}` -> participe au déroulé du traitement.

### Exemple B: variante cas limite (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant une garde explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter une garde avant cette ligne.
3. Vérifier la nouvelle sortie sur une entrée limite.

### Exemple C: bug reproductible puis correction locale

Procédure:
1. Introduire une incompatibilité de type sur un appel.
2. Compiler et lire le premier diagnostic.
3. Corriger une seule ligne (pas de refactor global).
4. Recompiler et vérifier le retour nominal.

### Résultat attendu

- Le lecteur comprend ce que fait le code sans abstraction inutile.
- Chaque exemple est relié à une action concrète.
- La correction est reproductible et testable.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->



## Exemple Étendu

Exemple approfondi pour **montee competence equipe**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/montee-competence-equipe

form Input { id: int value: int quota: int }
pick Eval { case Accepted(score: int) case Rejected(code: int) }

proc validate(x: Input) -> Eval {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x.id <= 0 { give Eval.Rejected(21) }
  // Garde: bloque un cas invalide avant de continuer
  if x.quota < 0 { give Eval.Rejected(22) }
  // Garde: bloque un cas invalide avant de continuer
  if x.value < 0 { give Eval.Rejected(23) }
  // Sortie locale: valeur retournee par la procedure
  give Eval.Accepted(x.value)
}

proc transform(score: int, quota: int) -> int {
  let capped: int = score
  if capped > quota { set capped = quota }
  // Garde: bloque un cas invalide avant de continuer
  if capped < 0 { give 0 }
  // Sortie locale: valeur retournee par la procedure
  give capped * 2
}

proc decide(r: Eval, quota: int) -> Eval {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Accepted(s) {
      let out: int = transform(s, quota)
      // Garde: bloque un cas invalide avant de continuer
  if out >= 10 { give Eval.Accepted(out) }
      // Sortie locale: valeur retournee par la procedure
  give Eval.Rejected(31)
    }
    case Rejected(c) { give Eval.Rejected(c) }
    otherwise { give Eval.Rejected(70) }
  }
}

// Projection finale: convertit l'état métier en code de sortie
proc to_exit(r: Eval) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Accepted(_) { give 0 }
    case Rejected(code) { give code }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let x: Input = Input(1, 8, 9)
  let v: Eval = validate(x)
  let d: Eval = decide(v, x.quota)
  // Sortie programme: code de retour observable
  return to_exit(d)
}
```

Scénarios recommandés (montee competence equipe):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
