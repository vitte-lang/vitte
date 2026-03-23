# 22. Projet guide HTTP

Niveau: Avancé

Prérequis: chapitre précédent `book/chapters/21-projet-cli.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/21-projet-cli.md`, `book/chapters/23-projet-sys.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Projet guide HTTP**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Projet guide HTTP**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Projet guide HTTP**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Projet guide HTTP** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Projet guide HTTP**.

## Exercice court

Prenez un exemple du chapitre sur **Projet guide HTTP**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Projet guide HTTP**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## Niveau local

- Niveau local section coeur: Avancé.
- Niveau local exemples guidés: Intermédiaire.
- Niveau local exercices de diagnostic: Avancé.

Ce chapitre poursuit un objectif clair: construire un service HTTP réaliste en séparant strictement transport, routage métier et projection finale de statut. Le but n'est pas d'empiler des conditions, mais de poser un pipeline stable, testable et traçable.

Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `book/checklist-editoriale.md`. Complément: `book/erreurs-classiques.md`.

Schéma pipeline du chapitre:
- Request -> Validate -> Route -> Execute -> Map Response.
- Entrée: requête brute (méthode, chemin, payload, token).
- Traitement: validation transport puis routage métier.
- Sortie: `HttpResult` puis code HTTP final.
- Invariant: une erreur donnée produit toujours le même statut.

## 22.0 Contrat HTTP minimal

Contrat retenu pour ce chapitre:
- Méthode: `GET` uniquement (les autres méthodes sont rejetées).
- Chemins supportés: `/health`, `/metrics`.
- Payload: longueur `body_len >= 0`.
- Authentification: `auth_token` non vide.
- Statut attendu:
- nominal `/health` ou `/metrics` validé: `200`.
- méthode invalide ou payload invalide: `400`.
- token absent: `401`.
- chemin inconnu: `404`.
- cas non prévu: `500`.

Tableau de statuts standardisé:
- `400`: cause `requête invalide` ; couche responsable `validation transport` ; action corrective `corriger méthode/payload côté client`.
- `401`: cause `auth manquante/invalide` ; couche responsable `validation transport` ; action corrective `fournir un token valide`.
- `404`: cause `route absente` ; couche responsable `routage` ; action corrective `utiliser un chemin exposé`.
- `500`: cause `cas inattendu` ; couche responsable `projection finale` ; action corrective `journaliser + corriger la branche manquante`.

## 22.1 Poser le modèle de transport et d'erreur

Frontière: ce bloc ne connaît pas HTTP au niveau socket/serveur, il modélise seulement des données et des résultats.

```vit
form HttpRequest {
  method: string
  path: string
  body_len: int
  auth_token: string
}

pick HttpError {
  case BadRequest
  case Unauthorized
  case NotFound
}

pick HttpResult {
  case Ok(code: int)
  case Err(e: HttpError)
}
```

Lecture ligne par ligne (débutant):
1. `form HttpRequest {` ouvre la structure de requête transport utilisée par tout le pipeline.
2. `method: string` ajoute la méthode HTTP (`GET`, `POST`, etc.) pour la validation d'entrée.
3. `path: string` ajoute le chemin de route à résoudre.
4. `body_len: int` ajoute la longueur de payload pour détecter un format invalide.
5. `auth_token: string` ajoute l'information d'authentification minimale.
6. `pick HttpError {` ouvre l'ensemble fermé des erreurs métier de couche HTTP.
7. `case BadRequest` représente une requête invalide côté client.
8. `case Unauthorized` représente un échec d'authentification.
9. `case NotFound` représente une route absente.
10. `pick HttpResult {` encode un résultat uniforme pour toutes les étapes.
11. `case Ok(code: int)` porte un statut nominal explicite.
12. `case Err(e: HttpError)` porte une erreur typée pour la projection finale.

Entrée -> sortie (à vérifier):
- Cas limite: si une étape détecte une faute, elle renvoie `Err(..)` plutôt qu'un entier magique.
- Cas nominal: un succès transporte `Ok(code)`.
- Observation testable: les transitions possibles sont bornées par `HttpResult`.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: la valeur passe en `Err(..)` dès la première garde concernée.

## 22.2 Valider la requête au bord du système

Frontière: ce bloc projette des contraintes HTTP d'entrée, mais ne décide pas encore de la logique métier.

```vit
proc validate_transport(r: HttpRequest) -> HttpResult {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if r.method != "GET" { give Err(BadRequest) }
  // Garde: bloque un cas invalide avant de continuer
  if r.path == "" { give Err(BadRequest) }
  // Garde: bloque un cas invalide avant de continuer
  if r.body_len < 0 { give Err(BadRequest) }
  // Garde: bloque un cas invalide avant de continuer
  if r.auth_token == "" { give Err(Unauthorized) }
  // Sortie locale: valeur retournee par la procedure
  give Ok(200)
}
```

Validation renforcée, entrées invalides concrètes:
- `method="POST"` -> `Err(BadRequest)`.
- `path=""` -> `Err(BadRequest)`.
- `auth_token=""` -> `Err(Unauthorized)`.

Lecture ligne par ligne (débutant):
1. `if r.method != "GET" { give Err(BadRequest) }` rejette immédiatement les méthodes hors contrat.
2. `if r.path == "" { give Err(BadRequest) }` rejette une route vide.
3. `if r.body_len < 0 { give Err(BadRequest) }` rejette une longueur incohérente.
4. `if r.auth_token == "" { give Err(Unauthorized) }` rejette l'absence de token.
5. `give Ok(200)` autorise la suite du pipeline lorsque le transport est valide.

Entrée -> sortie (à vérifier):
- Cas limite: `path=""` donne `Err(BadRequest)`.
- Cas nominal: `GET /health` avec token non vide donne `Ok(200)`.
- Observation testable: une même requête invalide produit toujours la même erreur.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: la fonction s'arrête sur la première garde invalide.

## 22.3 Isoler le routage de chemin

Frontière: ce bloc ne connaît pas le transport brut, il route uniquement un chemin déjà validé.

```vit
proc route_path(path: string) -> HttpResult {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if path == "/health" { give Ok(200) }
  // Garde: bloque un cas invalide avant de continuer
  if path == "/metrics" { give Ok(200) }
  // Sortie locale: valeur retournee par la procedure
  give Err(NotFound)
}
```

Lecture ligne par ligne (débutant):
1. `if path == "/health" { give Ok(200) }` route nominale de santé.
2. `if path == "/metrics" { give Ok(200) }` route nominale de métriques.
3. `give Err(NotFound)` route inconnue explicitement typée.

Entrée -> sortie (à vérifier):
- Cas limite: `/unknown` donne `Err(NotFound)`.
- Cas nominal: `/health` donne `Ok(200)`.
- Observation testable: le routage est déterministe et indépendant de l'auth.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: la route inconnue est convertie en `Err(NotFound)`.

## 22.4 Composer les étapes dans un handler

Frontière: ce bloc orchestre transport + routage, sans dupliquer les règles locales de chaque couche.

```vit
proc handle(r: HttpRequest) -> HttpResult {
  let t: HttpResult = validate_transport(r)
  // Match: decision explicite selon l'etat
  match t {
    case Err(e) { give Err(e) }
    case Ok(_) { give route_path(r.path) }
    otherwise { give Err(BadRequest) }
  }
}
```

Lecture ligne par ligne (débutant):
1. `let t: HttpResult = validate_transport(r)` exécute d'abord la frontière transport.
2. `case Err(e) { give Err(e) }` court-circuite immédiatement en cas d'échec.
3. `case Ok(_) { give route_path(r.path) }` passe au routage seulement si transport valide.
4. `otherwise { give Err(BadRequest) }` verrouille un fallback déterministe.

Entrée -> sortie (à vérifier):
- Cas limite: token absent -> `Err(Unauthorized)` sans passer au routage.
- Cas nominal: transport valide + `/metrics` -> `Ok(200)`.
- Observation testable: aucune duplication de validation dans le routeur.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: la branche `Err(e)` renvoie immédiatement l'erreur déjà qualifiée.

## 22.5 Convertir le résultat en statut HTTP final

Frontière: ce bloc projette en HTTP.

```vit
proc to_http_code(r: HttpResult) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Ok(c) { give c }
    case Err(BadRequest) { give 400 }
    case Err(Unauthorized) { give 401 }
    case Err(NotFound) { give 404 }
    otherwise { give 500 }
  }
}
```

Lecture ligne par ligne (débutant):
1. `case Ok(c) { give c }` conserve le statut nominal.
2. `case Err(BadRequest) { give 400 }` mappe l'erreur client en `400`.
3. `case Err(Unauthorized) { give 401 }` mappe l'auth invalide en `401`.
4. `case Err(NotFound) { give 404 }` mappe route absente en `404`.
5. `otherwise { give 500 }` protège la production avec un filet de sécurité.

Entrée -> sortie (à vérifier):
- Cas limite: erreur non prévue -> `500`.
- Cas nominal: `Ok(200)` -> `200`.
- Observation testable: chaque `HttpError` connu a un statut stable.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: elle est traduite en un code 4xx/5xx explicite, sans ambiguïté.

## 22.6 Idempotence (règle d'exploitation)

Règle de ce chapitre:
- `GET /health`: idempotent, aucun effet secondaire, répétable sans changement d'état.
- `GET /metrics`: idempotent dans ce modèle, lecture seule.
- Validation transport: pure, sans mutation globale.

Conséquence pratique: rejouer la même requête valide produit le même statut et ne modifie pas l'état du service.

## 22.7 Scénario complet traçable

Trajet imposé:
- Entrée brute: `method="GET"`, `path="/health"`, `body_len=0`, `auth_token="t"`.
- Validate: `Ok(200)`.
- Route: `Ok(200)`.
- Execute (handler): `Ok(200)`.
- Map Response: `200`.

Trajet invalide (auth absente):
- Entrée brute: `auth_token=""`.
- Validate: `Err(Unauthorized)`.
- Handler: court-circuit `Err(Unauthorized)`.
- Map Response: `401`.

## 22.8 Tests d'intégration minimaux

1. Nominal:
- Requête: `GET /health`, `body_len=0`, token non vide.
- Attendu: `to_http_code(handle(r)) == 200`.

2. Auth fail:
- Requête: `GET /health`, token vide.
- Attendu: `to_http_code(handle(r)) == 401`.

3. Route introuvable:
- Requête: `GET /unknown`, token non vide.
- Attendu: `to_http_code(handle(r)) == 404`.

## 22.9 Erreurs classiques HTTP

- Mapper trop tôt en statut: on perd l'information métier (`Err(..)`) et on complique le debug.
- Dupliquer la validation dans plusieurs couches: divergences et incohérences garanties.
- Mélanger transport et métier dans une seule fonction: tests fragiles, maintenance coûteuse.
- Oublier le fallback `500`: comportement non déterministe en cas inattendu.

## Table erreur -> diagnostic -> correction

| Erreur | Diagnostic | Correction |
| --- | --- | --- |
| Entrée invalide | Validation absente ou trop tardive. | Centraliser la validation en entrée de pipeline. |
| État incohérent | Mutation partielle ou invariant non vérifié. | Appliquer le principe d'atomicité et rejeter sinon. |
| Sortie inattendue | Couche projection mélangée avec la logique métier. | Séparer `apply` (métier) et `project` (sortie). |

## À retenir

Un service HTTP robuste commence par des frontières nettes: validation transport, routage indépendant, orchestration minimale, projection finale centralisée. Si chaque couche garde sa responsabilité, le système devient lisible, testable et stable.

Critère pratique de qualité pour ce chapitre:
- vous savez localiser immédiatement la couche responsable d'une erreur.
- vous pouvez rejouer un flux complet de requête sans ambiguïté.
- vous pouvez ajouter une route sans casser le mapping des statuts.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: une garde de validation ou une route de secours convertit l'entrée en `Err(..)`, puis en code HTTP stable.

## À faire

1. Ajoutez la route `GET /ready` en conservant la séparation validation/routage/projection.
2. Ajoutez une règle `body_len > 1_000_000` -> `Err(BadRequest)` et vérifiez l'impact sur les tests.

## Corrigé minimal

- La nouvelle route doit être ajoutée dans `route_path` sans modifier `validate_transport`.
- La nouvelle garde de taille doit vivre dans `validate_transport` et garder la projection centralisée dans `to_http_code`.

## Micro challenge HTTP

Ajoutez `429 Too Many Requests` sans casser les mappings existants.

Contrainte de conception:
- étendre `HttpError` avec un nouveau cas.
- ne pas dupliquer de conversion de statut hors `to_http_code`.
- garantir que les tests `200/401/404` restent inchangés.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/case.md`.
- `book/keywords/entry.md`.
- `book/keywords/form.md`.
- `book/keywords/give.md`.
- `book/keywords/if.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **projet guide http**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
form HttpRequest {
  method: string
  path: string
  body_len: int
  auth_token: string
}

pick HttpError {
  case BadRequest
  case Unauthorized
  case NotFound
}

pick HttpResult {
  case Ok(code: int)
  case Err(e: HttpError)
}
```

Lecture ligne par ligne:
1. `form HttpRequest {` -> participe au déroulé du traitement.
2. `method: string` -> participe au déroulé du traitement.
3. `path: string` -> participe au déroulé du traitement.
4. `body_len: int` -> participe au déroulé du traitement.
5. `auth_token: string` -> participe au déroulé du traitement.
6. `}` -> participe au déroulé du traitement.
7. `pick HttpError {` -> participe au déroulé du traitement.
8. `case BadRequest` -> participe au déroulé du traitement.
9. `case Unauthorized` -> participe au déroulé du traitement.
10. `case NotFound` -> participe au déroulé du traitement.

### Exemple B: variante cas limite (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant une garde explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter une garde avant cette ligne.
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

Exemple approfondi pour **projet http**: flux applicatif complet (entrée, politique métier, persistance simulée, code de sortie).

```vit
// Exemple long: flux complet et vérifiable
space demo/projet-http

form Request { id: int amount: int quota: int }
pick Result { case Accepted(total: int) case Rejected(code: int) }

// Entrée applicative: validation des invariants de requête
proc parse_request(r: Request) -> Result {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if r.id <= 0 { give Result.Rejected(91) }
  // Garde: bloque un cas invalide avant de continuer
  if r.quota < 0 { give Result.Rejected(92) }
  // Garde: bloque un cas invalide avant de continuer
  if r.amount < 0 { give Result.Rejected(93) }
  // Sortie locale: valeur retournee par la procedure
  give Result.Accepted(r.amount)
}

// Politique métier: applique les règles de décision
proc apply_policy(total: int, quota: int) -> Result {
  let capped: int = total
  if capped > quota { set capped = quota }
  // Garde: bloque un cas invalide avant de continuer
  if capped < 5 { give Result.Rejected(94) }
  // Sortie locale: valeur retournee par la procedure
  give Result.Accepted(capped)
}

// Persistance simulée: matérialise un résultat sans I/O réel
proc persist_sim(x: Result) -> Result {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match x {
    case Accepted(v) {
      // Garde: bloque un cas invalide avant de continuer
  if v % 13 == 0 { give Result.Rejected(95) }
      // Sortie locale: valeur retournee par la procedure
  give Result.Accepted(v)
    }
    case Rejected(c) { give Result.Rejected(c) }
    otherwise { give Result.Rejected(70) }
  }
}

// Projection finale: convertit l'état métier en code de sortie
proc to_exit(x: Result) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match x {
    case Accepted(_) { give 0 }
    case Rejected(c) { give c }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let req: Request = Request(7, 12, 15)
  let p: Result = parse_request(req)
  let d: Result = apply_policy(12, req.quota)
  let s: Result = persist_sim(d)
  let _probe: int = to_exit(p)
  // Sortie programme: code de retour observable
  return to_exit(s)
}
```

Scénarios recommandés (projet http):
- Requête nominale -> sortie 0.
- Entrée invalide id<=0 -> sortie 91.
- Refus métier valeur<5 -> sortie 94.
