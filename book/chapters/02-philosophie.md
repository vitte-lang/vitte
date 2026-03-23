# 2. Philosophie du langage

Niveau: Fondations avancées

Prérequis: `book/chapters/01-demarrer.md`, `book/glossaire.md`.
Voir aussi: `book/chapters/03-projet.md`.

## Problème Concret

Un langage devient utile quand il permet d'éviter les ambiguïtés.
Ici, on ne parle pas de théorie: on regarde comment écrire un code clair, vérifiable et maintenable.

## Fil Rouge (Projet Unique)

On conserve le même mini-programme pour comparer les choix d'écriture.
Vous allez voir ce qui change en pratique quand le code est explicite.

## Idée principale

Vitte fonctionne bien quand on rend visible:
1. le traitement des erreurs
2. l'état métier
3. la séparation entre lancement et logique

## 2.1 Traiter l'erreur avant l'opération sensible

```vit
proc safe_div(num: int, den: int) -> int {

  if den == 0 { give 0 }

  give num / den
}
```

Lecture directe:
1. le programme lit `num` et `den`.
2. il teste `den == 0`.
3. si c'est vrai, il renvoie `0` tout de suite.
4. sinon, il fait `num / den`.

Pourquoi c'est utile:
- vous évitez une division impossible.
- la règle est lisible dès les premières lignes.

Exemple:
- `safe_div(10, 2)` -> `5`
- `safe_div(10, 0)` -> `0`

## 2.2 Représenter l'état métier avec un type clair

```vit
pick Auth {
  case Granted(user: int)
  case Denied(code: int)
}

proc can_access(a: Auth) -> bool {

  match a {
    case Granted(_) { give true }
    case Denied(_) { give false }
    otherwise { give false }
  }
}
```

Lecture directe:
1. `Auth` peut être `Granted` ou `Denied`.
2. `can_access` lit la valeur reçue.
3. si c'est `Granted`, sortie `true`.
4. si c'est `Denied`, sortie `false`.

Pourquoi c'est utile:
- on comprend tout de suite les états possibles.
- on évite les codes numériques flous (`0`, `1`, `2`) qui prêtent à confusion.

## 2.3 Séparer lancement et logique

```vit
// Exécute un cas de test et retourne un résultat typé
proc run() -> int {
  give 0
}

// Point d'entree du scenario
entry main at core/app {
  return run()
}
```

Lecture directe:
1. `run` contient la logique du traitement.
2. `main` lance `run` puis renvoie sa valeur.

Pourquoi c'est utile:
- vous testez `run` facilement.
- vous changez l'entrée du programme sans casser la logique métier.

## 2.4 Erreurs fréquentes

1. tester les erreurs trop tard
2. utiliser des nombres au lieu de types métier explicites
3. mettre toute la logique dans `entry main`

## 2.5 Check-list d'audit local

1. les tests d'entrée sont-ils en début de fonction ?
2. les états métier sont-ils nommés clairement ?
3. `entry` sert-il à lancer, et non à tout faire ?
4. peut-on expliquer le comportement en 30 secondes ?

## Keywords à revoir

- `book/keywords/if.md`
- `book/keywords/pick.md`
- `book/keywords/match.md`
- `book/keywords/case.md`
- `book/keywords/entry.md`

## Exemple Étendu

```vit
// Scenario philosophie: execution complete et verifiable
space demo/philosophie

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

Ordre d'exécution:
1. `main` crée `x`.
2. `validate(x)` vérifie les données.
3. `decide(...)` calcule la décision finale.
4. `to_exit(...)` transforme le résultat en code de sortie.

Exécution normale:
- entrée: `Input(1, 8, 9)`
- sortie finale: `0`

Exécution en erreur:
- entrée: `Input(0, 8, 9)`
- `validate` renvoie `Rejected(21)`
- sortie finale: `21`

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
| Compréhension rapide | Types explicites | Codes numériques | Option 1 |
| Debug simple | Fonctions courtes | Gros bloc unique | Option 1 |
| Évolution sûre | Pipeline en étapes | Logique mélangée | Option 1 |

## Diagnostic Rapide

| Problème observé | Cause probable | Comment vérifier | Correction |
| --- | --- | --- | --- |
| Résultat inattendu | test placé trop tard | rejouer avec une entrée simple | remonter le test en haut |
| État confus | codes numériques dispersés | tracer les valeurs intermédiaires | utiliser un type `pick` clair |
| Code difficile à tester | logique dans `entry` | isoler chaque fonction | déplacer la logique dans `proc` |

## Checkpoint

À ce stade, vous devez savoir:
- lire un flux de traitement de haut en bas,
- expliquer le rôle de chaque fonction,
- prévoir la sortie selon l'entrée.

## Ce Que Je Ferais En Revue De Code

1. vérifier que les tests d'entrée sont faits tôt
2. vérifier que les états métier sont lisibles (`pick`)
3. vérifier un scénario normal + un scénario d'erreur
