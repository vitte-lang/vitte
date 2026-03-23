# 10. Diagnostics et erreurs

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/09-modules.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/09-modules.md`, `book/chapters/11-collections.md`, `book/glossaire.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Diagnostics et erreurs**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Diagnostics et erreurs**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Diagnostics et erreurs**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Diagnostics et erreurs** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Diagnostics et erreurs**.

## Exercice court

Prenez un exemple du chapitre sur **Diagnostics et erreurs**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Diagnostics et erreurs**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 10.1 Garde de division

```vit
proc safe_div(num: int, den: int) -> int {

  if den == 0 { give 0 }

  give num / den
}
```

Lecture simple du code:
1. `proc safe_div(num: int, den: int) -> int {` : le contrat est défini pour `safe_div`: entrées `num: int, den: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if den == 0 { give 0 }` : cette garde traite le cas limite avant le calcul.
3. `give num / den` : la branche renvoie immédiatement `num / den` pour la branche courante, la sortie de branche est explicite et vérifiable.
4. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: si `den == 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `num / den`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

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

Lecture simple du code:
1. `pick ParsePort {` : cette ligne ouvre le type fermé `ParsePort` pour forcer un ensemble fini de cas possibles et supprimer les états implicites.
2. `case Ok(value: int)` : ce cas décrit `Ok(value: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
3. `case Err(code: int)` : ce cas décrit `Err(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
4. `}` : cette accolade ferme le bloc logique.
5. `proc parse_port(x: int) -> ParsePort {` : le contrat est posé pour `parse_port`: entrées `x: int` et sortie `ParsePort`, elle clarifie l'intention avant lecture détaillée du corps.
6. `if x < 0 { give Err(400) }` : cette garde traite le cas limite avant le calcul.
7. `if x > 65535 { give Err(422) }` : cette garde traite le cas limite avant le calcul.
8. `give Ok(x)` : la sortie est renvoyée immédiatement `Ok(x)` pour la branche courante, la sortie de branche est explicite et vérifiable.
9. `}` : cette accolade clôt le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: si `x < 0` est vrai, la sortie devient `Err(400)`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Ok(x)`.
- Observation testable: forcer le cas `Ok(value: int)` permet de confirmer la branche attendue.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: encoder le diagnostic dans le type de retour, pour rendre les échecs aussi explicites que les succès.

Avec ce modèle, on ne perd pas l'information d'erreur: chaque cas garde son code associé.

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

Lecture simple du code:
1. `proc to_exit(p: ParsePort) -> int {` : le contrat est fixé pour `to_exit`: entrées `p: ParsePort` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `match p {` : cette ligne démarre un dispatch déterministe sur `p`: une seule branche sera choisie selon la forme de la valeur analysée.
3. `case Ok(_) { give 0 }` : ce cas décrit `Ok(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
4. `case Err(c) { give c }` : ce cas décrit `Err(c)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
5. `otherwise { give 70 }` : cette ligne définit un chemin de secours explicite.
6. `}` : cette accolade ferme le bloc logique.
7. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok(_)` permet de confirmer la branche attendue.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: séparer la logique métier de sa projection technique (ici, le code de sortie).

Ce découplage est important: la politique système peut évoluer sans réécrire la logique de parsing.

À l'exécution:
- `to_exit(Ok(_))` retourne `0`.
- `to_exit(Err(422))` retourne `422`.
- `otherwise` garde un code de secours (`70`).

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Cause localisée, typée, projetée proprement. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez localiser la cause d'une erreur sans parcourir tout le code.
- vous savez distinguer résultat métier et projection technique.
- vous pouvez testér séparément le parsing et la politique d'exit code.

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
- `book/keywords/continue.md`.
- `book/keywords/field.md`.
- `book/keywords/form.md`.
- `book/keywords/give.md`.



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

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Niveau info ou warn -> sortie 0.
- Erreur métier code 17 -> sortie 17.
- Payload hors limites -> sortie 82.


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
| Sortie inattendue | Garde absente ou mal ordonnée | Rejouer avec cas limite | Remonter la garde avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Contrat implicite | Comparer nominal vs limite | Formaliser le contrat dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas limite réel et prévoir sa sortie,
- identifier où ajouter une garde sans casser le nominal.


## Pourquoi Cette Erreur Arrive En Prod

Cause fréquente: entrée partiellement valide, hypothèse implicite dans une branche, puis projection de sortie trop tardive.
Symptôme: comportement correct en nominal mais instable sous charge ou données incomplètes.
Mesure utile: tracer l'entrée effective, rejouer le cas limite, verrouiller la garde au bon niveau.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
