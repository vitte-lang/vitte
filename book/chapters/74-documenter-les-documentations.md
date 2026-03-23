# 74. Documenter les documentations

Niveau: Tous niveaux

Prérequis: `book/chapters/60-documentation-technique-durable.md`.
Voir aussi: `book/STYLE.md`, `book/checklist-editoriale.md`.

## Problème Concret

Situation réelle: dans ce chapitre sur Documenter les documentations, l'échec vient souvent d'une décision mal ordonnée plutôt que d'une faute de syntaxe. On suit donc le flux exact: entrée, test, branche, sortie.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: chaque section reprend le même scénario pour isoler une seule décision technique à la fois.
Objectif pédagogique: passer de la lecture passive à la preuve: même entrée, même branche, même sortie attendue.

## Objectif

Appliquer au contenu documentaire le meme niveau de rigueur que pour le code: règles, tests, ownership, versioning.

## Pourquoi c'est critique

1. La documentation est une interface publique.
2. Une doc incorrecte cree des bugs d'usage.
3. Une doc non maintenue augmente le cout de support.

## Règle de meta-documentation

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

1. Changement editorial mineur: correction sans impact règle.
2. Changement fonctionnel: mise a jour de la règle et des exemples.
3. Changement breaking: section migration obligatoire.

## Runbook de maintenance

1. Revue hebdomadaire rapide: liens, erreurs evidentes.
2. Revue mensuelle structurelle: coherence et redondances.
3. Revue trimestrielle de fond: suppression/merge des chapitres faibles.

## Checklist

1. Le lecteur cible est explicitement defini.
2. Le chapitre indique quoi faire, pas seulement quoi savoir.
3. Chaque exemple a une preuve de validite.
4. Les décisions importantes sont historisees.
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

## Test mental

Question de contrôle: si vous modifiez une hypothèse clé, quel résultat doit changer et pourquoi?

## À faire

1. Exécuter l’exemple nominal.
2. Introduire un cas d'erreur.
3. Vérifier la sortie et documenter l’écart.

## Corrigé minimal

Corrigé: conserver la version la plus simple qui respecte la règle, puis ajouter un test de non-régression.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **documenter les documentations**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/docmeta {
  return 0
}
```

Lecture ligne par ligne:
1. `entry main at app/docmeta {` -> définit le point d'entrée du scénario.
2. `return 0` -> renvoie la sortie vérifiable.
3. `}` -> participe au déroulé du traitement.

### Exemple B: variante cas d'erreur (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant un test explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter un test avant cette ligne.
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


```vit
// Scenario documenter les documentations: execution complete et verifiable
space demo/documenter-les-documentations

form Input { id: int value: int quota: int }
pick Eval { case Accepted(score: int) case Rejected(code: int) }

proc validate(x: Input) -> Eval {

  if x.id <= 0 { give Eval.Rejected(21) }

  if x.quota < 0 { give Eval.Rejected(22) }

  if x.value < 0 { give Eval.Rejected(23) }

  give Eval.Accepted(x.value)
}

proc transform(score: int, quota: int) -> int {
  let capped: int = score
  if capped > quota { set capped = quota }

  if capped < 0 { give 0 }

  give capped * 2
}

proc decide(r: Eval, quota: int) -> Eval {

  match r {
    case Accepted(s) {
      let out: int = transform(s, quota)

      if out >= 10 { give Eval.Accepted(out) }

      give Eval.Rejected(31)
    }
    case Rejected(c) { give Eval.Rejected(c) }
    otherwise { give Eval.Rejected(70) }
  }
}

// Conversion finale vers un code de sortie
proc to_exit(r: Eval) -> int {

  match r {
    case Accepted(_) { give 0 }
    case Rejected(code) { give code }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let x: Input = Input(1, 8, 9)
  let v: Eval = validate(x)
  let d: Eval = decide(v, x.quota)

  return to_exit(d)
}
```

## Explication détaillée du gros bloc

Ce gros bloc montre un programme entier, pas un extrait isolé: on suit le flux du début à la fin.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `validate`: lit `x: Input` et renvoie `Eval`.
- `transform`: lit `score: int, quota: int` et renvoie `int`.
- `decide`: lit `r: Eval, quota: int` et renvoie `Eval`.
- `to_exit`: lit `r: Eval` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `validate` est appelé pour traiter l'étape suivante.
3. `decide` est appelé pour traiter l'étape suivante.
4. `to_exit` est appelé pour traiter l'étape suivante.
5. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `x.id <= 0`.
- Test évalué: `x.quota < 0`.
- Test évalué: `x.value < 0`.
- Test évalué: `capped > quota`.
- Test évalué: `capped < 0`.
- Test évalué: `out >= 10`.
- Sélection par `match r`: le chemin dépend de l'état reçu.
- Sélection par `match r`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> validate -> decide -> to_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> validate renvoie un code d'erreur -> sortie non nulle`.

### 5. Pourquoi ce découpage est utile
- Vous testez chaque fonction seule, puis le flux complet.
- Vous savez où modifier une règle sans casser tout le programme.
- Vous pouvez expliquer la sortie en suivant simplement les appels.

### 6. Vérification rapide
1. Relancer avec une entrée normale et noter la sortie.
2. Relancer avec une entrée invalide et vérifier le code d'erreur.
3. Confirmer que la même entrée donne toujours la même sortie.


## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les tests sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas d'erreur réel:
- Entree degradee ou incomplete: le test doit couper le flux tot avec une sortie explicite.

A tester:
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement stable.
- Cas invalide id<=0 -> sortie 21.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `validate` lit `x: Input` puis renvoie `Eval`.
- `transform` lit `score: int, quota: int` puis renvoie `int`.
- `decide` lit `r: Eval, quota: int` puis renvoie `Eval`.
- `to_exit` lit `r: Eval` puis renvoie `int`.

- Variables créées (valeur initiale):
- `capped: int` démarre avec `score`.
- `out: int` démarre avec `transform(s, quota)`.
- `x: Input` démarre avec `Input(1, 8, 9)`.
- `v: Eval` démarre avec `validate(x)`.
- `d: Eval` démarre avec `decide(v, x.quota)`.

- Variables modifiées pendant le traitement:
- `capped` est mis à jour avec `quota`.

- Conditions qui changent le chemin:
- si `x.id <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `x.quota < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `x.value < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `capped > quota` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `capped < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `out >= 10` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: capped=score -> out=transform(s, quota) -> x=Input(1, 8, 9) -> v=validate(x)
- enchaînement: validate -> decide -> to_exit
- sortie finale sur ce chemin: `to_exit(d)`.

- Trace d'erreur (valeurs exemple):
- si `x.id <= 0` devient vrai, la fonction renvoie immédiatement `Eval.Rejected(21)`.

- Vérification rapide:
- relancer avec une entrée normale et noter la sortie,
- relancer avec une entrée invalide et noter le code d'erreur,
- confirmer qu'une même entrée produit toujours la même sortie.

## Trade-offs

| Contrainte | Option A | Option B | Décision recommandée |
| --- | --- | --- | --- |
| Lisibilité prioritaire | Branches explicites | Code compact | A si l'équipe maintient le code longtemps |
| Perf critique | Spécialisation ciblée | Généralisation | A si profiling confirme le gain |
| Évolution rapide | Contrats stricts | Conventions implicites | A pour réduire les régressions |


## Décision Selon Contrainte

- Si la contrainte dominante est la sûreté: valider tôt, échouer explicitement.
- Si la contrainte dominante est la latence: mesurer d'abord, optimiser ensuite.
- Si la contrainte dominante est l'évolutivité: isoler orchestration, décisions et conversion de sortie.


## Diagnostic Rapide

| Symptôme | Cause probable | Vérification | Correction |
| --- | --- | --- | --- |
| Sortie inattendue | Test absente ou mal ordonnée | Rejouer avec cas d'erreur | Remonter le test avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Règle implicite | Comparer nominal vs limite | Formaliser la règle dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas d'erreur réel et prévoir sa sortie,
- identifier où ajouter un test sans casser le nominal.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
