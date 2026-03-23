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

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si une étape détecte une faute, elle renvoie `Err(...)` plutôt qu'un entier magique.
- Cas nominal: un succès transporte `Ok(code)`.
- Observation testable: les transitions possibles sont bornées par `HttpResult`.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: la valeur passe en `Err(...)` dès la première garde concernée.

## 22.2 Valider la requête au bord du système

Frontière: ce bloc projette des contraintes HTTP d'entrée, mais ne décide pas encore de la logique métier.

```vit
proc validate_transport(r: HttpRequest) -> HttpResult {
  if r.method != "GET" { give Err(BadRequest) }
  if r.path == "" { give Err(BadRequest) }
  if r.body_len < 0 { give Err(BadRequest) }
  if r.auth_token == "" { give Err(Unauthorized) }
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

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: `path=""` donne `Err(BadRequest)`.
- Cas nominal: `GET /health` avec token non vide donne `Ok(200)`.
- Observation testable: une même requête invalide produit toujours la même erreur.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: la fonction s'arrête sur la première garde invalide.

## 22.3 Isoler le routage de chemin

Frontière: ce bloc ne connaît pas le transport brut, il route uniquement un chemin déjà validé.

```vit
proc route_path(path: string) -> HttpResult {
  if path == "/health" { give Ok(200) }
  if path == "/metrics" { give Ok(200) }
  give Err(NotFound)
}
```

Lecture ligne par ligne (débutant):
1. `if path == "/health" { give Ok(200) }` route nominale de santé.
2. `if path == "/metrics" { give Ok(200) }` route nominale de métriques.
3. `give Err(NotFound)` route inconnue explicitement typée.

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: `/unknown` donne `Err(NotFound)`.
- Cas nominal: `/health` donne `Ok(200)`.
- Observation testable: le routage est déterministe et indépendant de l'auth.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: la route inconnue est convertie en `Err(NotFound)`.

## 22.4 Composer les étapes dans un handler

Frontière: ce bloc orchestre transport + routage, sans dupliquer les règles locales de chaque couche.

```vit
proc handle(r: HttpRequest) -> HttpResult {
  let t: HttpResult = validate_transport(r)
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

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: token absent -> `Err(Unauthorized)` sans passer au routage.
- Cas nominal: transport valide + `/metrics` -> `Ok(200)`.
- Observation testable: aucune duplication de validation dans le routeur.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: la branche `Err(e)` renvoie immédiatement l'erreur déjà qualifiée.

## 22.5 Convertir le résultat en statut HTTP final

Frontière: ce bloc projette en HTTP.

```vit
proc to_http_code(r: HttpResult) -> int {
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

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: erreur non prévue -> `500`.
- Cas nominal: `Ok(200)` -> `200`.
- Observation testable: chaque `HttpError` connu a un statut stable.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
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

- Mapper trop tôt en statut: on perd l'information métier (`Err(...)`) et on complique le debug.
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
Réponse attendue: une garde de validation ou une route de secours convertit l'entrée en `Err(...)`, puis en code HTTP stable.

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

## Exemples représentatifs (par cas d'usage)

Cette section s'appuie sur du code concret pour **projet guide http**.
Objectif: comprendre vite ce que fait le code, pourquoi, et comment le corriger.

### Exemple 1: extrait réel du chapitre (cas nominal)

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

Lecture guidée (ligne par ligne):
1. `form HttpRequest {` -> participe au flux nominal du programme.
2. `method: string` -> participe au flux nominal du programme.
3. `path: string` -> participe au flux nominal du programme.
4. `body_len: int` -> participe au flux nominal du programme.
5. `auth_token: string` -> participe au flux nominal du programme.
6. `}` -> participe au flux nominal du programme.
7. `pick HttpError {` -> participe au flux nominal du programme.
8. `case BadRequest` -> participe au flux nominal du programme.

Entrée -> Sortie attendue:
1. Entrée: données conformes au contrat.
2. Traitement: chemin nominal exécuté.
3. Sortie: valeur déterministe observable.

### Exemple 2: garde explicite (cas limite)

```vit
proc clamp_non_negative(x: int) -> int {
  if x < 0 {
    give 0
  }
  give x
}
```

Quand l'utiliser: éviter les comportements implicites sur entrées hors contrat.

### Exemple 3: erreur de type volontaire (diagnostic)

```vit
proc needs_int(x: int) -> int {
  give x
}
entry main at app/demo {
  let s: string = "42"
  return needs_int(s)
}
```

Quand l'utiliser: entraîner la lecture des diagnostics compilateur.

### Exemple 4: séparation module / API

```vit
space app/math
proc add(a: int, b: int) -> int {
  give a + b
}
share add
```

Quand l'utiliser: clarifier ce qui est public vs interne dans l'architecture.

### Exemple 5: flux de contrôle lisible

```vit
entry main at app/demo {
  let n: int = 3
  if n > 0 {
    return 1
  }
  return 0
}
```

Quand l'utiliser: expliciter une décision métier avec un chemin nominal et un fallback.

### Exemple 6: version testable d'une procédure

```vit
proc is_even(x: int) -> bool {
  give x % 2 == 0
}
```

Cas de test conseillés:
1. `is_even(2)` -> `true`.
2. `is_even(3)` -> `false`.
3. `is_even(0)` -> `true`.

Quand l'utiliser: convertir rapidement une règle en contrat vérifiable.

### Exemple 7: refactor sûr (avant/après)

Avant:
```vit
proc parse_port(s: string) -> int {
  give 0
}
```

Après:
```vit
proc parse_port(s: string) -> int {
  if s == "" {
    give 0
  }
  give 8080
}
```

Quand l'utiliser: faire évoluer le comportement sans casser la signature publique.

### Exemple 8: correction guidée basée sur le code

Procédure de correction:
1. Reproduire le bug sur un snippet minimal.
2. Corriger une seule ligne.
3. Recompiler et vérifier la sortie.
4. Ajouter un test de non-régression.

### Checklist de lecture rapide

1. Où est le contrat d'entrée?
2. Quel est le chemin nominal?
3. Quel est le cas limite traité?
4. Quelle erreur reste explicite?
5. Quel test prouve le comportement?

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->

<!-- AUTO_EXPANSION_V1 START -->

## Approfondissement guidé par le code

### 1. Snippet de référence du chapitre

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

### 2. Ce que fait ce code, ligne par ligne

1. `form HttpRequest {` -> participe au flux nominal.
2. `method: string` -> participe au flux nominal.
3. `path: string` -> participe au flux nominal.
4. `body_len: int` -> participe au flux nominal.
5. `auth_token: string` -> participe au flux nominal.
6. `}` -> participe au flux nominal.
7. `pick HttpError {` -> participe au flux nominal.
8. `case BadRequest` -> participe au flux nominal.
9. `case Unauthorized` -> participe au flux nominal.
10. `case NotFound` -> participe au flux nominal.

### 3. Lecture exécutable (entrée -> sortie)

1. Entrée: valeurs conformes au contrat.
2. Exécution: chemin nominal suivi sans ambiguïté.
3. Sortie: résultat déterministe, testable immédiatement.

### 4. Variante d'erreur + correction

Erreur typique: mélanger un type inattendu dans un appel.
Correction: ajuster l'argument au contrat attendu, puis recompiler.

### 5. Pourquoi cette méthode est concrète

On part du code réel, pas d'un discours abstrait.
Chaque modification est locale, visible, et vérifiable par test.

### Atelier concret: cas pratique sur 22-projet-http.md

Code de base:
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

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 22-projet-http.md

Code de base:
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

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 22-projet-http.md

Code de base:
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

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 22-projet-http.md

Code de base:
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

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 22-projet-http.md

Code de base:
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

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

<!-- AUTO_EXPANSION_V1 END -->
