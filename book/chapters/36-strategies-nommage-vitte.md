# 36. Strategies de nommage en Vitte

Niveau: Intermediaire

Prérequis: `book/chapters/28-conventions.md`.
Voir aussi: `book/chapters/09-modules.md`, `book/chapters/06-procedures.md`.

## Problème Concret

Situation réelle: Strategies de nommage en Vitte devient clair quand on trace chaque étape du calcul. L'objectif est de relier ligne de code et effet concret sur la sortie.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: chaque section reprend le même scénario pour isoler une seule décision technique à la fois.
Objectif pédagogique: passer de la lecture passive à la preuve: même entrée, même branche, même sortie attendue.

## Objectif

Choisir des noms predictibles pour reduire les erreurs de lecture et de maintenance.

## Regles pratiques

1. Noms de procedures: verbe + objet (`parse_port`, `load_user`).
2. Predicats booleens: prefixes `is_`, `has_`, `can_`.
3. Types: noms metier (`User`, `OrderStatus`), pas noms techniques opaques.
4. Modules: noms courts, semantiques et stables.

## Exemple minimal

```vit
form User {
  id: int
  name: string
}

proc is_admin(role: int) -> bool {
  give role == 9
}
```

## Anti-patterns

- Prefixes techniques partout (`tmp_`, `data_`, `obj_`).
- Abreviations ambiguës (`cfg2`, `usrx`).
- Renommages massifs sans migration.

## Checklist

1. Le nom exprime-t-il l'intention metier?
2. Le nom est-il coherent avec le module?
3. Le nom reste-t-il stable dans le temps?

## Exemples progressifs (N1 -> N3)

### N1 (base): nommage clair

```vit
proc parse_port(x: int) -> int { give x }
```

### N2 (intermediaire): predicat booleen explicite

```vit
proc is_admin(role: int) -> bool { give role == 9 }
```

### N3 (avance): cohesion de noms

```vit
proc load_user(id: int) -> int { give id }
proc validate_user(id: int) -> bool { give id > 0 }
proc save_user(id: int) -> int { give id }
```

### Anti-exemple

```vit
proc do_it(x: int) -> int { give x }
```

## Validation rapide

1. Verbe + objet.
2. Predicats en `is_`/`has_`.
3. Vocabulaire coherent sur tout le module.

## Pourquoi

Cette section explicite la valeur pratique: réduire les erreurs, accélérer le diagnostic et stabiliser les évolutions.

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

Thème: **strategies de nommage en vitte**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
form User {
  id: int
  name: string
}

proc is_admin(role: int) -> bool {
  give role == 9
}
```

Lecture ligne par ligne:
1. `form User {` -> participe au déroulé du traitement.
2. `id: int` -> participe au déroulé du traitement.
3. `name: string` -> participe au déroulé du traitement.
4. `}` -> participe au déroulé du traitement.
5. `proc is_admin(role: int) -> bool {` -> pose une règle clair de fonction.
6. `give role == 9` -> renvoie la sortie vérifiable.
7. `}` -> participe au déroulé du traitement.

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
// Scenario strategies nommage vitte: execution complete et verifiable
space demo/strategies-nommage-vitte

form ModuleSpec { name_len: int major: int minor: int }
pick Resolve { case Loaded(path_code: int) case Missing(code: int) }

proc normalize_len(n: int) -> int {

  if n <= 0 { give 0 }

  give n
}

// Résolution: valide la référence et produit une décision de chargement
proc resolve(m: ModuleSpec) -> Resolve {
  let n: int = normalize_len(m.name_len)

  if n == 0 { give Resolve.Missing(71) }

  if m.major <= 0 { give Resolve.Missing(72) }

  if m.minor < 0 { give Resolve.Missing(73) }
  let code: int = 100 + (m.major * 10) + m.minor

  give Resolve.Loaded(code)
}

// Conversion finale vers un code de sortie
proc to_exit(r: Resolve) -> int {

  match r {
    case Loaded(_) { give 0 }
    case Missing(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let m: ModuleSpec = ModuleSpec(8, 1, 2)
  let r: Resolve = resolve(m)

  return to_exit(r)
}
```

## Explication détaillée du gros bloc

Ce gros bloc montre un programme entier, pas un extrait isolé: on suit le flux du début à la fin.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `normalize_len`: lit `n: int` et renvoie `int`.
- `resolve`: lit `m: ModuleSpec` et renvoie `Resolve`.
- `to_exit`: lit `r: Resolve` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `resolve` est appelé pour traiter l'étape suivante.
3. `to_exit` est appelé pour traiter l'étape suivante.
4. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `n <= 0`.
- Test évalué: `n == 0`.
- Test évalué: `m.major <= 0`.
- Test évalué: `m.minor < 0`.
- Sélection par `match r`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> resolve -> to_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> resolve renvoie un code d'erreur -> sortie non nulle`.

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
- Spécification valide -> sortie 0.
- Nom invalide -> sortie 71.
- Version invalide -> sortie 72 ou 73.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `normalize_len` lit `n: int` puis renvoie `int`.
- `resolve` lit `m: ModuleSpec` puis renvoie `Resolve`.
- `to_exit` lit `r: Resolve` puis renvoie `int`.

- Variables créées (valeur initiale):
- `n: int` démarre avec `normalize_len(m.name_len)`.
- `code: int` démarre avec `100 + (m.major * 10) + m.minor`.
- `m: ModuleSpec` démarre avec `ModuleSpec(8, 1, 2)`.
- `r: Resolve` démarre avec `resolve(m)`.

- Conditions qui changent le chemin:
- si `n <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `n == 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `m.major <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `m.minor < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: n=normalize_len(m.name_len) -> code=100 + (m.major * 10) + m.minor -> m=ModuleSpec(8, 1, 2) -> r=resolve(m)
- enchaînement: resolve -> to_exit
- sortie finale sur ce chemin: `to_exit(r)`.

- Trace d'erreur (valeurs exemple):
- si `n <= 0` devient vrai, la fonction renvoie immédiatement `0`.

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


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: tests d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
