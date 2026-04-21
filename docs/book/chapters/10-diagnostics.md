# 10. Diagnostics et erreurs

Niveau: Intermédiaire

Prérequis: chapitre précédent `docs/book/chapters/09-modules.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/09-modules.md`, `docs/book/chapters/11-collections.md`, `docs/book/glossaire.md`.

## Problème Concret

Situation réelle: Diagnostics et erreurs se comprend mieux en rejouant le programme comme un algorithme exécutable. Vous lisez les données entrantes, la condition évaluée, puis la valeur renvoyée.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: chaque section reprend le même scénario pour isoler une seule décision technique à la fois.
Objectif pédagogique: comprendre pourquoi une ligne existe et ce qu'elle change dans la trajectoire du programme.

## Objectif

Vous devez pouvoir relire un extrait, prédire son résultat, puis vérifier cette prédiction avec une exécution simple.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Diagnostics et erreurs**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Diagnostics et erreurs** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Diagnostics et erreurs**.

## Exercice court

Prenez un exemple du chapitre sur **Diagnostics et erreurs**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Diagnostics et erreurs**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 10.1 Test de division

```vit
proc safe_div(num: int, den: int) -> int {

  if den == 0 { give 0 }

  give num / den
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Condition évaluée en premier: `den == 0`. Si elle est vraie, le chemin de test est exécuté immédiatement.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `0`.
5. Notion intermédiaire: une fonction est une transformation `Entrée -> Sortie`; sa règle sert à limiter les ambiguïtés.
6. Notion intermédiaire: un invariant est une propriété qui reste vraie pendant la boucle ou pendant un pipeline d'appels.
Vérification rapide: testez une entrée nominale puis une entrée limite, et comparez les deux sorties obtenues.


L'intention de cette étape est directe: placer la frontière d'erreur au plus près de l'opération risquée.

Ce principe évite les crashs évitables: la division est protégée avant d'être exécutée.

À l'exécution:
- `safe_div(12,3)` retourne `4`.
- `safe_div(12,0)` retourne `0` sans tenter la division.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 10.2 Erreur typée

```vit
pick ParsePort {
  case Ok(value: int)
  case Err(code: int)
}
proc parse_port(x: int) -> ParsePort {

  if x < 0 { give Err(400) }

  if x > 65535 { give Err(422) }

  give Ok(x)
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Condition évaluée en premier: `x < 0`. Si elle est vraie, le chemin de test est exécuté immédiatement.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `Err(400)`.
5. Notion intermédiaire: une fonction est une transformation `Entrée -> Sortie`; sa règle sert à limiter les ambiguïtés.
6. Notion intermédiaire: un invariant est une propriété qui reste vraie pendant la boucle ou pendant un pipeline d'appels.
Vérification rapide: testez une entrée nominale puis une entrée limite, et comparez les deux sorties obtenues.


L'intention de cette étape est directe: encoder le diagnostic dans le type de retour, pour rendre les échecs aussi explicites que les succès.

Avec ce modèle, on ne perd pas l'information d'erreur: chaque cas porte son code associé.

À l'exécution:
- `parse_port(-1)` retourne `Err(400)`.
- `parse_port(8080)` retourne `Ok(8080)`.
- `parse_port(70000)` retourne `Err(422)`.

Erreurs classiques à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 10.3 Projection technique

```vit
// Conversion finale vers un code de sortie
proc to_exit(p: ParsePort) -> int {

  match p {
    case Ok(_) { give 0 }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}
```

Lecture algorithmique guidée:
1. Entrée lue: identifiez d'abord les paramètres et leur type, ce sont les données de départ du calcul.
2. Sélection de branche: `match p` choisit un cas selon l'état courant.
3. Traitement: appliquez les opérations dans l'ordre écrit, sans sauter d'étape implicite.
4. Sortie produite: le chemin courant renvoie `0`.
5. Notion intermédiaire: une fonction est une transformation `Entrée -> Sortie`; sa règle sert à limiter les ambiguïtés.
6. Notion intermédiaire: un invariant est une propriété qui reste vraie pendant la boucle ou pendant un pipeline d'appels.
Vérification rapide: tracez une exécution avec des valeurs concrètes (`x=...`, `i=...`) pour confirmer la branche réellement prise.


L'intention de cette étape est directe: séparer la logique métier de sa projection technique (ici, le code de sortie).

Ce découplage est important: la politique système peut évoluer sans réécrire la logique de parsing.

À l'exécution:
- `to_exit(Ok(_))` retourne `0`.
- `to_exit(Err(422))` retourne `422`.
- `otherwise` test un code de secours (`70`).

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Cause localisée, typée, projetée proprement. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, règle explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez localiser la cause d'une erreur sans parcourir tout le code.
- vous savez distinguer résultat métier et projection technique.
- vous pouvez tester séparément le parsing et la politique d'exit code.

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

- `docs/book/chapters/keywords/case.md`.
- `docs/book/chapters/keywords/continue.md`.
- `docs/book/chapters/keywords/field.md`.
- `docs/book/chapters/keywords/form.md`.
- `docs/book/chapters/keywords/give.md`.


## Exemple Étendu


```vit
// Scenario diagnostics: execution complete et verifiable
space demo/diagnostics

form Event { code: int severity: int payload_len: int }
pick Diagnostic { case Info(code: int) case Warn(code: int) case Error(code: int) }

// Classification: mappe un événement vers un niveau explicite
proc classify(e: Event) -> Diagnostic {

  if e.code == 0 { give Diagnostic.Info(0) }

  if e.severity <= 2 { give Diagnostic.Warn(e.code) }

  give Diagnostic.Error(e.code)
}

// Redaction: borne la charge utile avant diffusion
proc redact(e: Event) -> int {

  if e.payload_len < 0 { give 81 }

  if e.payload_len > 4096 { give 82 }

  give 0
}

proc handle(e: Event) -> int {
  let r: int = redact(e)

  if r != 0 { give r }
  let d: Diagnostic = classify(e)

  match d {
    case Info(_) { give 0 }
    case Warn(_) { give 0 }
    case Error(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let e: Event = Event(17, 3, 120)

  return handle(e)
}
```

## Explication détaillée du gros bloc

Ce gros bloc montre un programme entier, pas un extrait isolé: on suit le flux du début à la fin.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `classify`: lit `e: Event` et renvoie `Diagnostic`.
- `redact`: lit `e: Event` et renvoie `int`.
- `handle`: lit `e: Event` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `handle` est appelé pour traiter l'étape suivante.
3. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `e.code == 0`.
- Test évalué: `e.severity <= 2`.
- Test évalué: `e.payload_len < 0`.
- Test évalué: `e.payload_len > 4096`.
- Test évalué: `r != 0`.
- Sélection par `match d`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> handle -> sortie 0`.
- Exemple erreur: `entrée invalide -> handle renvoie un code d'erreur -> sortie non nulle`.

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
- Niveau info ou warn -> sortie 0.
- Erreur métier code 17 -> sortie 17.
- Payload hors limites -> sortie 82.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `classify` lit `e: Event` puis renvoie `Diagnostic`.
- `redact` lit `e: Event` puis renvoie `int`.
- `handle` lit `e: Event` puis renvoie `int`.

- Variables créées (valeur initiale):
- `r: int` démarre avec `redact(e)`.
- `d: Diagnostic` démarre avec `classify(e)`.
- `e: Event` démarre avec `Event(17, 3, 120)`.

- Conditions qui changent le chemin:
- si `e.code == 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `e.severity <= 2` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `e.payload_len < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `e.payload_len > 4096` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `r != 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: r=redact(e) -> d=classify(e) -> e=Event(17, 3, 120)
- enchaînement: handle
- sortie finale sur ce chemin: `handle(e)`.

- Trace d'erreur (valeurs exemple):
- si `e.code == 0` devient vrai, la fonction renvoie immédiatement `Diagnostic.Info(0)`.

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


## Pourquoi Cette Erreur Arrive En Prod

Cause fréquente: entrée partiellement valide, hypothèse implicite dans une branche, puis projection de sortie trop tardive.
Symptôme: comportement correct en nominal mais instable sous charge ou données incomplètes.
Mesure utile: tracer l'entrée effective, rejouer le cas d'erreur, verrouiller le test au bon niveau.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
