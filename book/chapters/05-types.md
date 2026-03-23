# 5. Types et valeurs

Niveau: Débutant

Prérequis: chapitre précédent `book/chapters/04-syntaxe.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/04-syntaxe.md`, `book/chapters/06-procedures.md`, `book/glossaire.md`.

## Problème Concret

Situation réelle: dans ce chapitre sur Types et valeurs, l'échec vient souvent d'une décision mal ordonnée plutôt que d'une faute de syntaxe. On suit donc le flux exact: entrée, test, branche, sortie.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: chaque section reprend le même scénario pour isoler une seule décision technique à la fois.
Objectif pédagogique: relire un bloc, prédire la sortie, puis confirmer la prédiction avec une exécution simple et reproductible.

## Objectif

Vous devez pouvoir relire un extrait, prédire son résultat, puis vérifier cette prédiction avec une exécution simple.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Types et valeurs**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Types et valeurs** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Types et valeurs**.

## Exercice court

Prenez un exemple du chapitre sur **Types et valeurs**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Types et valeurs**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 5.0 Types primitifs (table canonique)

Règle explicite de ce livre:
- `int` est le type entier canonique pour les exemples pédagogiques.
- quand la largeur binaire fait partie de la règle, utiliser `i32/i64/i128/u32/u64/u128`.

| Type | Taille | Signé / non signé | Cas d’usage principal | Exemple |
| --- | --- | --- | --- | --- |
| `bool` | 1 bit logique (représentation backend dépendante) | non signé | conditions, test, test | `let ok: bool = true` |
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
  give p.x + p.y
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Pas de branchement critique ici: le flux est séquentiel, ligne après ligne.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `p.x + p.y`.
5. Notion débutant: une variable est une case mémoire nommée qui contient une valeur d'un type annoncé.
6. Notion débutant: une condition booléenne vaut `vrai` ou `faux`; c'est elle qui détermine la branche exécutée.
Vérification rapide: gardez la même entrée avant/après modification pour prouver que le comportement attendu est conservé.


`Point` exprime une idée claire: une valeur avec deux coordonnées `x` et `y`. La procédure `manhattan` n'accepte donc pas n'importe quelles données; elle exige un `Point`. Cette contrainte est précieuse, car elle empêche les appels ambigus et rend l'intention évidente dans la signature.

À l'exécution, le comportement reste simple:
- avec `Point(3,4)`, la fonction renvoie `7`.
- avec `Point(10,-2)`, elle renvoie `8`.

Mais la vraie valeur de l'exemple n'est pas le calcul en lui-même. Elle est dans la lisibilité de la règle: en voyant `manhattan(p: Point)`, le lecteur comprend immédiatement la nature de la donnée attendue.

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

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Pas de branchement critique ici: le flux est séquentiel, ligne après ligne.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: ce bloc prépare un état, la sortie est calculée plus loin dans le pipeline.
5. Notion débutant: une variable est une case mémoire nommée qui contient une valeur d'un type annoncé.
6. Notion débutant: une condition booléenne vaut `vrai` ou `faux`; c'est elle qui détermine la branche exécutée.
Vérification rapide: tracez une exécution avec des valeurs concrètes (`x=...`, `i=...`) pour confirmer la branche réellement prise.


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

  match r {
    case Ok(v) { give v }
    case Err(_) { give 0 }
    otherwise { give 0 }
  }
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Sélection de branche: `match r` choisit un cas selon l'état courant.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `v`.
5. Notion débutant: une variable est une case mémoire nommée qui contient une valeur d'un type annoncé.
6. Notion débutant: une condition booléenne vaut `vrai` ou `faux`; c'est elle qui détermine la branche exécutée.
Vérification rapide: tracez une exécution avec des valeurs concrètes (`x=...`, `i=...`) pour confirmer la branche réellement prise.


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
Repère: un test explicite ou un chemin de secours stable doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de test pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez le test ou la branche de secours réellement utilisée.

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


```vit
// Scenario types: execution complete et verifiable
space demo/types

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

Vous lisez ce gros bloc comme un scénario complet: préparation des données, traitement, puis sortie finale.

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
