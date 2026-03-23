# 16. Interop et ABI

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/15-pipeline.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/15-pipeline.md`, `book/chapters/17-stdlib.md`, `book/glossaire.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Interop et ABI**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Interop et ABI**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Interop et ABI**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Interop et ABI** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Interop et ABI**.

## Exercice court

Prenez un exemple du chapitre sur **Interop et ABI**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Interop et ABI**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 16.1 Encoder explicitement les données de frontière

```vit
form Request {
  code: int
  payload: string
}
proc encode_code(r: Request) -> int {
  give r.code
}
```

Lecture simple du code:
1. `form Request {` : cette ligne ouvre la structure `Request` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable.
2. `code: int` : cette ligne déclare le champ `code` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation.
3. `payload: string` : cette ligne déclare le champ `payload` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation.
4. `}` : cette accolade ferme le bloc logique.
5. `proc encode_code(r: Request) -> int {` : le contrat est défini pour `encode_code`: entrées `r: Request` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
6. `give r.code` : la branche renvoie immédiatement `r.code` pour la branche courante, la sortie de branche est explicite et vérifiable.
7. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `r.code`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: rendre explicites les données qui traversent la frontière ABI.

Ce niveau d'explicitation évite les ambiguïtés de sérialisation et de mapping: chaque champ est nommé, typé et lisible.

À l'exécution, `encode_code(Request(200,"ok"))=200`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 16.2 Mapper un code externe vers un type somme interne

```vit
pick IoResult {
  case Ok(value: int)
  case Err(errno: int)
}
proc map_errno(e: int) -> IoResult {

  if e == 0 { give Ok(0) }

  give Err(e)
}
```

Lecture simple du code:
1. `pick IoResult {` : cette ligne ouvre le type fermé `IoResult` pour forcer un ensemble fini de cas possibles et supprimer les états implicites.
2. `case Ok(value: int)` : ce cas décrit `Ok(value: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
3. `case Err(errno: int)` : ce cas décrit `Err(errno: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
4. `}` : cette accolade clôt le bloc logique.
5. `proc map_errno(e: int) -> IoResult {` : le contrat est posé pour `map_errno`: entrées `e: int` et sortie `IoResult`, elle clarifie l'intention avant lecture détaillée du corps.
6. `if e == 0 { give Ok(0) }` : cette garde traite le cas limite avant le calcul.
7. `give Err(e)` : la sortie est renvoyée immédiatement `Err(e)` pour la branche courante, la sortie de branche est explicite et vérifiable.
8. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: si `e == 0` est vrai, la sortie devient `Ok(0)`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Err(e)`.
- Observation testable: forcer le cas `Ok(value: int)` permet de confirmer la branche attendue.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: convertir un protocole d'erreur externe (codes entiers) en modèle interne typé.

Avec ce mapping, le reste du code ne manipule plus des nombres "magiques". Il manipule des variantes explicites (`Ok` ou `Err`).

À l'exécution:
- `map_errno(0)` retourne `Ok(0)`.
- `map_errno(13)` retourne `Err(13)`.

Erreurs classiques à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 16.3 Isoler le point machine `unsafe`

```vit
proc syscall_halt() -> int {
  unsafe {
    asm("hlt")
  }

  give 0
}
```

Lecture simple du code:
1. `proc syscall_halt() -> int {` : le contrat est fixé pour `syscall_halt`: entrées `` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `unsafe {` : cette ligne marque une zone sensible qui doit rester courte, justifiée et facile à auditer dans un contexte système.
3. `asm("hlt")` : cette ligne définit une étape explicite du flux.
4. `}` : cette accolade ferme le bloc logique.
5. `give 0` : retourne immédiatement `0` pour la branche courante, la sortie de branche est explicite et vérifiable.
6. `}` : cette accolade clôt le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `0`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: isoler strictement le point `unsafe` lié à l'instruction machine.

Cette isolation rend l'audit concret: un seul point à inspecter, un seul point à encadrer, un seul point à testér indirectement.

À l'exécution, en contexte autorisé, `hlt` est exécuté puis la procédure retourne `0` si le flot revient.

Erreurs classiques à éviter:
- étendre la zone sensible au lieu de la garder courte et auditable.
- placer la validation après l'opération risquée.
- masquer la frontière technique, ce qui rend le diagnostic plus coûteux.

## À retenir

Les données ABI sont explicites, les erreurs externes sont typées et la zone `unsafe` est strictement bornée. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez identifier exactement quelles données traversent l'ABI.
- vous savez traduire un code externe en variante métier interne.
- vous pouvez montrer que la zone `unsafe` reste minimale et localisée.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Repère: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## ABI/runtime checklist (commun)

- signatures ABI cohérentes entre frontières Vitte et natif.
- symboles runtime effectivement exportés et linkés.
- headers/runtime disponibles sur la cible.
- contraintes freestanding explicitées pour les cibles kernel.
- validation d'appel natif avant optimisation.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/as.md`.
- `book/keywords/asm.md`.
- `book/keywords/case.md`.
- `book/keywords/continue.md`.
- `book/keywords/form.md`.



## Exemple Étendu


```vit
// Scenario interop: execution complete et verifiable
space demo/interop

form AbiEnvelope { version: int payload_size: int flags: int }
pick NativeCall { case Ok(code: int) case Err(code: int) }

proc abi_version() -> int { give 3 }

// Validation ABI: refuse toute incompatibilité avant appel natif
proc validate_abi(e: AbiEnvelope) -> int {

  if e.version != abi_version() { give 51 }

  if e.payload_size <= 0 { give 52 }

  if e.payload_size > 4096 { give 53 }

  if e.flags < 0 { give 54 }

  give 0
}

// Appel natif simulé: exécution seulement si le contrat est valide
proc call_native(e: AbiEnvelope) -> NativeCall {
  let v: int = validate_abi(e)

  if v != 0 { give NativeCall.Err(v) }

  if e.payload_size % 2 == 0 { give NativeCall.Ok(0) }

  give NativeCall.Err(55)
}

// Conversion finale vers un code de sortie
proc to_exit(r: NativeCall) -> int {

  match r {
    case Ok(c) { give c }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let e: AbiEnvelope = AbiEnvelope(3, 128, 1)
  let r: NativeCall = call_native(e)

  return to_exit(r)
}
```

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- ABI valide -> sortie 0.
- Version incompatible -> sortie 51.
- Payload hors contrat -> sortie 52 ou 53.


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
