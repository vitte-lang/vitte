# 4. Syntaxe essentielle

Niveau: Débutant

Prérequis: chapitre précédent `book/chapters/03-projet.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/03-projet.md`, `book/chapters/05-types.md`, `book/glossaire.md`.

## Problème Concret

Situation réelle: Syntaxe essentielle se comprend mieux en rejouant le programme comme un algorithme exécutable. Vous lisez les données entrantes, la condition évaluée, puis la valeur renvoyée.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: on conserve un même mini-programme pour comparer les effets d'une modification sans changer tout le contexte.
Objectif pédagogique: passer de la lecture passive à la preuve: même entrée, même branche, même sortie attendue.

## Objectif

Vous devez pouvoir relire un extrait, prédire son résultat, puis vérifier cette prédiction avec une exécution simple.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Syntaxe essentielle**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Syntaxe essentielle** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Syntaxe essentielle**.

## Exercice court

Prenez un exemple du chapitre sur **Syntaxe essentielle**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Syntaxe essentielle**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 4.1 Définir une procédure et sa sortie

Une procédure commence toujours par une promesse explicite: "si vous me donnez ces valeurs, je vous rends ce type de résultat". En Vitte, cette promesse se lit dans la signature. Le lecteur n'a pas à deviner les types, ni le format du retour.

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Pas de branchement critique ici: le flux est séquentiel, ligne après ligne.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `a + b`.
5. Notion débutant: une variable est une case mémoire nommée qui contient une valeur d'un type annoncé.
6. Notion débutant: une condition booléenne vaut `vrai` ou `faux`; c'est elle qui détermine la branche exécutée.
Vérification rapide: tracez une exécution avec des valeurs concrètes (`x=...`, `i=...`) pour confirmer la branche réellement prise.


Cette fonction se lit presque comme une phrase: prends `a` et `b`, additionne-les, rends le résultat. Pour `add(1, 2)`, le calcul donne `3`. Pour `add(10, -4)`, il donne `6`. Rien d'autre ne se produit, aucun effet de bord ne vient perturber la lecture.

Ce détail est plus important qu'il n'y paraît. Le mot-clé `give` rend la sortie volontaire et visible. Dans un projet réel, cette explicitation réduit énormément les malentendus: on sait où la valeur est produite, et on sait pourquoi.

Quand vous relisez une procédure, posez-vous trois questions simples:
- quelles sont les entrées autorisées.
- quel résultat est promis.
- à quel endroit précis la valeur est renvoyée.

Erreurs classiques à éviter:
- oublier d'annoncer le bon type de retour dans la signature.
- faire des calculs corrects mais renvoyer la mauvaise valeur.
- écrire une fonction simple avec une structure inutilement complexe.

## 4.2 Encadrer une valeur avec des tests

Dans la plupart des programmes, les bugs viennent moins du cas normal que des bords: valeurs négatives, dépassements, entrées inattendues. Une bonne habitude consiste donc à protéger d'abord les limites, puis à laisser le cas nominal en dernier.

```vit
proc clamp01(v: int) -> int {

  if v < 0 { give 0 }

  if v > 1 { give 1 }

  give v
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Condition évaluée en premier: `v < 0`. Si elle est vraie, le chemin de test est exécuté immédiatement.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `0`.
5. Notion débutant: une variable est une case mémoire nommée qui contient une valeur d'un type annoncé.
6. Notion débutant: une condition booléenne vaut `vrai` ou `faux`; c'est elle qui détermine la branche exécutée.
Vérification rapide: testez une entrée nominale puis une entrée limite, et comparez les deux sorties obtenues.


L'intention est claire: forcer une valeur à rester dans l'intervalle `[0, 1]`. On lit le code dans l'ordre naturel. Si `v` est trop petite, on renvoie immédiatement `0`. Si elle est trop grande, on renvoie immédiatement `1`. Sinon, on la renvoie telle quelle.

Lecture pas à pas:
- `clamp01(-2)`: la premier test est vrai (`v < 0`), donc retour `0`.
- `clamp01(0)`: aucun test n'est vrai, donc retour `0`.
- `clamp01(9)`: la second test est vrai (`v > 1`), donc retour `1`.

La force de ce style, c'est sa franchise. Chaque ligne répond à une question précise: "que faire si c'est trop bas?", "que faire si c'est trop haut?", "que faire sinon?". Cette structure rend le code solide et agréable à relire.

Erreurs classiques à éviter:
- traiter le cas nominal avant les bornes, ce qui cache les exceptions.
- oublier un des deux bords de l'intervalle.
- multiplier les branches imbriquées alors que des tests simples suffisent.

## 4.3 Itérer avec une mutation explicite

Une boucle devient dangereuse dès que l'on ne sait plus exactement comment l'état évolue. En Vitte, la mutation est explicite avec `set`, ce qui vous oblige à montrer noir sur blanc la transition d'un état au suivant.

```vit
proc count(n: int) -> int {
  let i: int = 0

  // Boucle: progression controlee jusqu'a la borne
  loop {
    // Borne d'arret: stoppe la boucle de maniere explicite
    if i >= n { break }
    set i = i + 1
  }

  give i
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Condition évaluée en premier: `i >= n`. Si elle est vraie, le chemin de test est exécuté immédiatement.
3. Boucle: vérifiez la condition d'arrêt et la progression de l'état à chaque itération.
4. Sortie produite: le chemin courant renvoie `i`.
5. Notion débutant: une variable est une case mémoire nommée qui contient une valeur d'un type annoncé.
6. Notion débutant: une condition booléenne vaut `vrai` ou `faux`; c'est elle qui détermine la branche exécutée.
Vérification rapide: gardez la même entrée avant/après modification pour prouver que le comportement attendu est conservé.


Ici, la logique est volontairement simple. On initialise `i` à `0`. À chaque tour, on vérifie la condition d'arrêt: si `i >= n`, on quitte la boucle. Sinon, on avance d'un pas avec `set i = i + 1`.

Pour `n = 4`, la trajectoire est:
- départ `i = 0`.
- tour 1: `i = 1`.
- tour 2: `i = 2`.
- tour 3: `i = 3`.
- tour 4: `i = 4`.
- tour suivant: `i >= n`, arrêt, retour `4`.

Ce que cet exemple enseigne va au-delà du comptage. Il montre comment garder une boucle sous contrôle: une variable d'état clairement identifiée, une condition d'arrêt lisible, une progression unique et explicite.

Erreurs classiques à éviter:
- oublier d'incrémenter la variable, ce qui crée une boucle infinie.
- placer la condition d'arrêt au mauvais endroit.
- faire évoluer plusieurs états à la fois sans nécessité.

## À retenir

Si vous ne retenez qu'une idée, retenez celle-ci: un bon code n'est pas seulement correct, il est explicite. Dans Vitte, cette explicitation passe par des signatures nettes, des tests honnêtes sur les limites, et des boucles où chaque mutation se voit. Quand ces trois éléments sont en place, vous obtenez un programme que l'on peut comprendre sans deviner, corriger sans trembler, et faire évoluer sans le casser.

Critère pratique de qualité pour ce chapitre:
- vous pouvez expliquer chaque exemple à voix haute en moins d'une minute.
- vous savez prédire le résultat d'un appel avant de l'exécuter.
- vous savez identifier immédiatement l'endroit où le flux s'arrête.

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

- `book/keywords/break.md`.
- `book/keywords/case.md`.
- `book/keywords/continue.md`.
- `book/keywords/give.md`.
- `book/keywords/if.md`.


## Exemple Étendu


```vit
// Scenario syntaxe: execution complete et verifiable
space demo/syntaxe

form SourceUnit { bytes: int lines: int tokens_hint: int }
pick ParseState { case Parsed(nodes: int) case Failed(code: int) }

// Scan: transforme l'entrée brute en signal exploitable
proc scan(u: SourceUnit) -> int {

  if u.bytes <= 0 { give 0 }

  if u.lines <= 0 { give 0 }

  give (u.tokens_hint + u.lines)
}

// Parse: construit un état syntaxique stable
proc parse(token_count: int) -> ParseState {

  if token_count == 0 { give ParseState.Failed(101) }

  if token_count < 4 { give ParseState.Failed(102) }

  give ParseState.Parsed(token_count)
}

proc validate_structure(nodes: int) -> int {

  if nodes <= 0 { give 201 }

  if nodes > 200000 { give 202 }

  give 0
}

// Conversion finale vers un code de sortie
proc to_exit(p: ParseState) -> int {

  match p {
    case Parsed(n) {
      let v: int = validate_structure(n)

      if v != 0 { give v }

      give 0
    }
    case Failed(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let u: SourceUnit = SourceUnit(120, 12, 18)
  let t: int = scan(u)
  let p: ParseState = parse(t)

  return to_exit(p)
}
```

## Explication détaillée du gros bloc

Ce gros bloc montre un programme entier, pas un extrait isolé: on suit le flux du début à la fin.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `scan`: lit `u: SourceUnit` et renvoie `int`.
- `parse`: lit `token_count: int` et renvoie `ParseState`.
- `validate_structure`: lit `nodes: int` et renvoie `int`.
- `to_exit`: lit `p: ParseState` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `scan` est appelé pour traiter l'étape suivante.
3. `parse` est appelé pour traiter l'étape suivante.
4. `to_exit` est appelé pour traiter l'étape suivante.
5. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `u.bytes <= 0`.
- Test évalué: `u.lines <= 0`.
- Test évalué: `token_count == 0`.
- Test évalué: `token_count < 4`.
- Test évalué: `nodes <= 0`.
- Test évalué: `nodes > 200000`.
- Sélection par `match p`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> scan -> parse -> to_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> scan renvoie un code d'erreur -> sortie non nulle`.

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
- Unité valide -> sortie 0.
- Entrée vide (bytes=0) -> sortie 101.
- Structure surdimensionnée -> sortie 202.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `scan` lit `u: SourceUnit` puis renvoie `int`.
- `parse` lit `token_count: int` puis renvoie `ParseState`.
- `validate_structure` lit `nodes: int` puis renvoie `int`.
- `to_exit` lit `p: ParseState` puis renvoie `int`.

- Variables créées (valeur initiale):
- `v: int` démarre avec `validate_structure(n)`.
- `u: SourceUnit` démarre avec `SourceUnit(120, 12, 18)`.
- `t: int` démarre avec `scan(u)`.
- `p: ParseState` démarre avec `parse(t)`.

- Conditions qui changent le chemin:
- si `u.bytes <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `u.lines <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `token_count == 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `token_count < 4` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `nodes <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `nodes > 200000` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `v != 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: v=validate_structure(n) -> u=SourceUnit(120, 12, 18) -> t=scan(u) -> p=parse(t)
- enchaînement: scan -> parse -> to_exit
- sortie finale sur ce chemin: `to_exit(p)`.

- Trace d'erreur (valeurs exemple):
- si `u.bytes <= 0` devient vrai, la fonction renvoie immédiatement `0`.

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
