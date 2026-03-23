# 5. Types et valeurs

Niveau: Débutant

Prérequis: chapitre précédent `book/chapters/04-syntaxe.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/04-syntaxe.md`, `book/chapters/06-procedures.md`, `book/glossaire.md`.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Types et valeurs**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Types et valeurs**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Types et valeurs**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Types et valeurs** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Types et valeurs**.

## Exercice court

Prenez un exemple du chapitre sur **Types et valeurs**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Types et valeurs**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 5.0 Types primitifs (table canonique)

Règle explicite de ce livre:
- `int` est le type entier canonique pour les exemples pédagogiques.
- quand la largeur binaire fait partie du contrat, utiliser `i32/i64/i128/u32/u64/u128`.

| Type | Taille | Signé / non signé | Cas d’usage principal | Exemple |
| --- | --- | --- | --- | --- |
| `bool` | 1 bit logique (représentation backend dépendante) | non signé | conditions, garde, test | `let ok: bool = true` |
| `string` | variable | n/a | texte et messages | `let msg: string = "ok"` |
| `int` | dépend de l’implémentation cible | signé | calcul général en chapitre débutant | `let n: int = 42` |
| `i32` | 32 bits | signé | protocole binaire, API bornée 32 bits | `let code: i32 = 200` |
| `i64` | 64 bits | signé | horodatage, compteurs longs | `let ts: i64 = 1700000000` |
| `i128` | 128 bits | signé | identifiants/calculettes très grands | `let big: i128 = 123456789` |
| `u32` | 32 bits | non signé | tailles, masques, registres 32 bits | `let mask: u32 = 255` |
| `u64` | 64 bits | non signé | offsets/fichiers/protocoles 64 bits | `let size: u64 = 4096` |
| `u128` | 128 bits | non signé | IDs larges, hash partiels | `let id: u128 = 1` |

## 5.0.1 Quand éviter `int`

Éviter `int` quand la taille doit rester stable entre machines:
- API binaire et liaison native.
- protocole réseau et format de fichier.
- sérialisation persistée.
- registres matériels et code kernel/bare-metal.

Dans ces cas, imposer une largeur explicite (`i32/i64/i128/u32/u64/u128`) et conserver cette largeur de bout en bout.

Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `book/checklist-editoriale.md`. Complément: `book/erreurs-classiques.md`.
## 5.1 Décrire une donnée avec un type structurel

Un type structurel regroupe plusieurs champs nommés qui forment une seule entité logique. Il ne s'agit plus de "deux entiers quelconques", mais d'un point du plan avec un sens métier explicite.

```vit
form Point {
  x: int
  y: int
}
proc manhattan(p: Point) -> int {
  // Sortie locale: valeur retournee par la procedure
  give p.x + p.y
}
```

Lecture simple du code:
1. `form Point {` : cette ligne ouvre la structure `Point` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable.
2. `x: int` : cette ligne déclare le champ `x` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation.
3. `y: int` : cette ligne déclare le champ `y` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation.
4. `}` : cette accolade ferme le bloc logique.
5. `proc manhattan(p: Point) -> int {` : le contrat est défini pour `manhattan`: entrées `p: Point` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
6. `give p.x + p.y` : la branche renvoie immédiatement `p.x + p.y` pour la branche courante, la sortie de branche est explicite et vérifiable.
7. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `p.x + p.y`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

`Point` exprime une idée claire: une valeur avec deux coordonnées `x` et `y`. La procédure `manhattan` n'accepte donc pas n'importe quelles données; elle exige un `Point`. Cette contrainte est précieuse, car elle empêche les appels ambigus et rend l'intention évidente dans la signature.

À l'exécution, le comportement reste simple:
- avec `Point(3,4)`, la fonction renvoie `7`.
- avec `Point(10,-2)`, elle renvoie `8`.

Mais la vraie valeur de l'exemple n'est pas le calcul en lui-même. Elle est dans la lisibilité du contrat: en voyant `manhattan(p: Point)`, le lecteur comprend immédiatement la nature de la donnée attendue.

Réflexe à garder: quand une donnée a un sens métier, donnez-lui un type métier. Vous évitez ainsi les échanges implicites de "deux entiers qui se ressemblent" mais ne veulent pas dire la même chose.

Erreurs classiques à éviter:
- manipuler des tuples ou des entiers séparés au lieu d'un type métier explicite.
- donner des noms de champs trop vagues, qui masquent l'intention.
- multiplier les conversions inutiles autour d'un type déjà bien défini.

## 5.2 Représenter des alternatives avec un type somme

Certaines valeurs n'ont pas une seule forme possible. Un résultat peut réussir, ou échouer. Un parseur peut produire une valeur, ou une erreur. Le type somme sert précisément à encoder ces alternatives sans ambiguïté.

```vit
pick Result {
  case Ok(value: int)
  case Err(code: int)
}
```

Lecture simple du code:
1. `pick Result {` : cette ligne ouvre le type fermé `Result` pour forcer un ensemble fini de cas possibles et supprimer les états implicites.
2. `case Ok(value: int)` : ce cas décrit `Ok(value: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
3. `case Err(code: int)` : ce cas décrit `Err(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
4. `}` : cette accolade clôt le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok(value: int)` permet de confirmer la branche attendue.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Ici, `Result` décrit un univers fermé d'états valides: soit `Ok` avec une valeur entière, soit `Err` avec un code d'erreur. Ce point est essentiel. Le programme n'a plus à deviner l'état réel d'une donnée à partir de conventions implicites; l'état est nommé et porté par le type.

À l'exécution, toute valeur de type `Result` est nécessairement l'une des deux variantes. Vous ne pouvez pas "inventer" un troisième état non prévu sans modifier le type lui-même. Cette fermeture réduit les états impossibles et simplifie la maintenance: lorsqu'un comportement change, vous savez exactement quelles formes de données sont concernées.

En pratique, ce modèle améliore aussi les messages d'erreur et les tests:
- les tests ciblent des variantes explicites.
- les erreurs métiers restent nommées.
- les cas non gérés deviennent visibles plus tôt.

Erreurs classiques à éviter:
- utiliser un entier "sentinelle" (par exemple `-1`) à la place d'une variante d'erreur.
- mélanger dans un même champ des données de nature différente.
- introduire des cas implicites que le type ne documente pas.

## 5.3 Traiter une variante avec `match`

Une fois le type somme défini, la lecture correcte consiste à traiter explicitement chaque variante. Le `match` aligne le contrôle de flux sur la structure du type.

```vit
proc unwrap_or_zero(r: Result) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Ok(v) { give v }
    case Err(_) { give 0 }
    otherwise { give 0 }
  }
}
```

Lecture simple du code:
1. `proc unwrap_or_zero(r: Result) -> int {` : le contrat est posé pour `unwrap_or_zero`: entrées `r: Result` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `match r {` : cette ligne démarre un dispatch déterministe sur `r`: une seule branche sera choisie selon la forme de la valeur analysée.
3. `case Ok(v) { give v }` : ce cas décrit `Ok(v)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
4. `case Err(_) { give 0 }` : ce cas décrit `Err(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
5. `otherwise { give 0 }` : cette ligne définit un chemin de secours explicite.
6. `}` : cette accolade ferme le bloc logique.
7. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok(v)` permet de confirmer la branche attendue.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

La fonction `unwrap_or_zero` exprime une règle concrète: si le résultat est `Ok`, on récupère sa valeur; s'il s'agit d'une erreur, on renvoie `0`. Le flux suit exactement les variantes de `Result`, ce qui rend le comportement immédiat à vérifier.

Lecture pas à pas:
- si `r = Ok(42)`, la première branche correspond, retour `42`.
- si `r = Err(500)`, la deuxième branche correspond, retour `0`.
- la branche `otherwise` sert ici de filet de sécurité.

Le code ne "devine" rien: il inspecte la forme de la valeur, puis applique la règle associée. C'est cette correspondance entre type et branches qui rend le système robuste.

Erreurs classiques à éviter:
- traiter seulement une variante et oublier les autres.
- écrire des branches qui contredisent le sens métier des cas.
- ajouter un `otherwise` pour masquer un oubli de conception plutôt que corriger le modèle.

## À retenir

Le typage devient réellement utile lorsqu'il porte le métier, pas seulement la forme technique des données. Un type structurel clarifie ce qu'est une entité. Un type somme clarifie les états possibles de cette entité. Un `match` bien écrit clarifie comment on réagit à chacun de ces états. Avec ces trois outils, votre code gagne en lisibilité, en sécurité et en stabilité: il devient plus difficile d'exprimer une erreur, et plus facile d'expliquer une intention.

Critère pratique de qualité pour ce chapitre:
- vous pouvez dire ce que représente chaque type en une phrase claire.
- vous savez lister toutes les variantes possibles d'un résultat.
- vous pouvez relire un `match` et vérifier qu'aucun cas important n'est oublié.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Repère: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/case.md`.
- `book/keywords/form.md`.
- `book/keywords/give.md`.
- `book/keywords/if.md`.
- `book/keywords/int.md`.



## Exemple Étendu

Exemple approfondi pour **types**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/types

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

Scénarios recommandés (types):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
