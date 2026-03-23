# 2. Philosophie du langage

Niveau: Fondations avancées

Prérequis: `book/chapters/01-demarrer.md`, `book/glossaire.md`.
Voir aussi: `book/chapters/03-projet.md`.

## Thèse

Vitte est efficace quand on transforme les implicites en structures explicites:
- erreur explicite
- état explicite
- frontière explicite

## 2.1 Erreur explicite avant opération risquée

```vit
proc safe_div(num: int, den: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if den == 0 { give 0 }
  // Sortie locale: valeur retournee par la procedure
  give num / den
}
```

Décision d'architecture:
- traiter le cas invalide **avant** la zone sensible

Gain:
- comportement déterministe
- absence de chemin implicite pour la faute

Perte assumée:
- une branche supplémentaire à lire

Compromis choisi:
- lisibilité et robustesse priment sur la compacité syntaxique.

## 2.2 État métier explicite via type somme

```vit
pick Auth {
  case Granted(user: int)
  case Denied(code: int)
}

proc can_access(a: Auth) -> bool {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match a {
    case Granted(_) { give true }
    case Denied(_) { give false }
    otherwise { give false }
  }
}
```

Pourquoi ce modèle tient mieux que `status: int`:
1. le domaine des états est fermé et documenté par le type
2. la branche de décision est explicite et auditable
3. l'ambiguïté sémantique disparaît (`1` ne veut plus “peut-être succès”)

Risque résiduel:
- oubli d'un cas lors de l'évolution du type

Mesure de mitigation:
- garder un `otherwise` défensif tant que l'outillage d'exhaustivité n'est pas strict.

## 2.3 Frontière explicite: `entry` orchestre, `proc` décide

```vit
// Exécute un cas de test et retourne un résultat typé
proc run() -> int {
  // Sortie locale: valeur retournee par la procedure
  give 0
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  // Sortie programme: code de retour observable
  return run()
}
```

Principe:
- l'orchestration est une couche d'assemblage
- la logique est une couche de décision

Effet direct:
- testabilité: `run` se teste sans runtime complet
- maintenabilité: on peut changer l'entrée sans recoder le métier

## 2.4 Anti-patterns et coût réel

1. gérer l'erreur “après coup”
Coût: diffusion des vérifications, diagnostic tardif.

2. encoder l'état métier en codes numériques
Coût: dette sémantique, branches ambiguës, bugs de convention.

3. transformer `entry` en “god function”
Coût: tests fragiles, couplage excessif, refactorings risqués.

## 2.5 Check-list d'audit local

1. la première zone de la procédure traite-t-elle les invalides ?
2. les états métier sont-ils représentés par des types nommés ?
3. la frontière orchestration/métier est-elle nette ?
4. peut-on expliquer la politique d'erreur en 2 phrases maximum ?

## Keywords à revoir

- `book/keywords/if.md`
- `book/keywords/pick.md`
- `book/keywords/match.md`
- `book/keywords/case.md`
- `book/keywords/entry.md`



## Exemple Étendu

Exemple approfondi pour **philosophie**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/philosophie

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

Scénarios recommandés (philosophie):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
