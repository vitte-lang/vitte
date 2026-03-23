# 1. Démarrer avec Vitte

Niveau: Fondations

Prérequis: `book/chapters/00-avant-propos.md`, `book/glossaire.md`.
Voir aussi: `book/chapters/02-philosophie.md`.

## Problème Concret

Quand un programme démarre mal, tout le reste devient flou.
Dans ce chapitre, vous allez lire le code comme une suite d'actions: entrée, test, action, sortie.

## Fil Rouge (Projet Unique)

On conserve le même mini-programme pour éviter de changer de contexte à chaque section.
Le but est simple: comprendre pourquoi chaque ligne existe et ce qu'elle produit.

## Objectif technique

À la fin de ce chapitre, vous devez savoir:
1. où démarre le programme
2. où se fait le calcul
3. où la sortie finale est décidée

## 1.1 Point d'entrée

```vit
// Point d'entree du scenario
entry main at core/app {
  return 0
}
```

Lecture directe:
1. `main` est la porte d'entrée du programme.
2. `return 0` arrête tout de suite l'exécution.
3. le résultat final est `0`.

Ce bloc sert de base propre: il montre le démarrage le plus simple possible.

## 1.2 Séparer calcul et orchestration

```vit
proc add(a: int, b: int) -> int {
  give a + b
}

// Point d'entree du scenario
entry main at core/app {
  let r: int = add(20, 22)

  return r
}
```

Lecture directe:
1. `add` fait une seule chose: additionner deux entiers.
2. `main` appelle `add(20, 22)` puis récupère le résultat dans `r`.
3. `main` renvoie `r`.

Pourquoi c'est bien:
- la fonction de calcul est facile à tester seule.
- le point d'entrée reste court et lisible.

Test rapide:
- entrée de `add`: `20` et `22`
- sortie attendue: `42`

## 1.3 Boucle avec arrêt clair

```vit
proc sum_to(n: int) -> int {
  let i: int = 0
  let s: int = 0

  // Boucle: progression controlee jusqu'a la borne

  loop {
    // Borne d'arret: stoppe la boucle de maniere explicite
    if i > n { break }
    set s = s + i
    set i = i + 1
  }

  give s
}
```

Lecture directe:
1. départ: `i = 0`, `s = 0`.
2. tant que `i <= n`, on ajoute `i` dans `s`.
3. on avance `i` de `1` à chaque tour.
4. quand `i > n`, on s'arrête.
5. on renvoie `s`.

Exemple concret avec `n = 3`:
- tour 1: `i=0`, `s=0`
- tour 2: `i=1`, `s=1`
- tour 3: `i=2`, `s=3`
- tour 4: `i=3`, `s=6`
- arrêt quand `i=4`, sortie `6`

## 1.4 Erreurs fréquentes

1. mettre toute la logique dans `entry main`
2. oublier la condition d'arrêt d'une boucle
3. mélanger calcul métier et code de sortie système

## 1.5 Check-list de fin de chapitre

1. le point d'entrée est-il unique et visible ?
2. chaque fonction a-t-elle un rôle simple ?
3. chaque boucle a-t-elle une condition d'arrêt claire ?
4. peut-on prédire la sortie sans deviner ?

## Keywords à revoir

- `book/keywords/entry.md`
- `book/keywords/proc.md`
- `book/keywords/loop.md`
- `book/keywords/give.md`
- `book/keywords/return.md`

## Exemple Étendu

```vit
// Scenario demarrer: execution complete et verifiable
space demo/demarrer

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

Ici, l'objectif est de comprendre le chemin réel du programme, ligne par ligne, jusqu'au code de sortie.

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


## Lecture du flux complet

Ordre d'exécution réel:
1. `main` crée `x`.
2. `validate(x)` vérifie les valeurs d'entrée.
3. si les données sont correctes, on obtient `Accepted(...)`; sinon `Rejected(code)`.
4. `decide(...)` transforme le score puis décide réussite/échec.
5. `to_exit(...)` convertit le résultat métier en code de sortie.

Exécution type:
- entrée: `Input(1, 8, 9)`
- `validate` -> `Accepted(8)`
- `transform(8,9)` -> `16`
- `decide` -> `Accepted(16)`
- `to_exit` -> `0`

Exécution d'erreur:
- entrée: `Input(0, 8, 9)`
- `validate` -> `Rejected(21)`
- `decide` laisse `Rejected(21)`
- `to_exit` -> `21`

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

| Besoin | Option 1 | Option 2 | Choix conseillé |
| --- | --- | --- | --- |
| Lecture facile | Fonctions courtes | Gros bloc unique | Option 1 |
| Debug rapide | Codes d'erreur explicites | Message implicite | Option 1 |
| Évolution du code | Pipeline par étapes | Logique mélangée | Option 1 |

## Diagnostic Rapide

| Problème observé | Cause probable | Comment vérifier | Correction |
| --- | --- | --- | --- |
| Sortie inattendue | ordre des tests mauvais | rejouer avec une entrée simple | remonter les tests d'entrée |
| Mauvais code retour | conversion finale oubliée | tracer `to_exit` | centraliser la conversion |
| Résultat incohérent | fonction trop chargée | relire rôle de chaque fonction | découper en étapes courtes |

## Checkpoint

À ce stade, vous devez savoir:
- suivre l'exécution du début à la fin,
- expliquer pourquoi une branche est prise,
- relier chaque fonction à une responsabilité claire.

## Ce Que Je Ferais En Revue De Code

1. vérifier que `main` orchestre sans faire tout le calcul
2. vérifier que chaque fonction a une seule responsabilité
3. vérifier un scénario normal + un scénario d'erreur exécutables
