# 68. Architecture hexagonale en Vitte

Niveau: Avancé

Prérequis: `book/chapters/29-style.md`, `book/chapters/37-conventions-modules-echelle.md`.
Voir aussi: à définir.

## Objectif

Isoler domaine metier, adaptateurs externes et infrastructure pour faciliter tests et evolutions.

## Structure cible

1. Coeur metier pur.
2. Ports explicites.
3. Adaptateurs I/O remplaçables.
4. Composition au bord de l'application.

## Cas fil rouge: AuthService

Ports:
1. `UserRepoPort` (charger utilisateur).
2. `PasswordHasherPort` (verifier mot de passe).
3. `TokenIssuerPort` (emettre token).

Adaptateurs:
1. `SqlUserRepoAdapter` -> base SQL.
2. `BcryptHasherAdapter` -> hash natif.
3. `JwtTokenAdapter` -> emission de token.

Flux:
1. `AuthService.login(email, password)` appelle `UserRepoPort`.
2. Vérification via `PasswordHasherPort`.
3. Emission via `TokenIssuerPort`.

Arborescence module cible:

```text
core/auth/
  domain/
    auth_service.vit
    model.vit
  ports/
    user_repo_port.vit
    password_hasher_port.vit
    token_issuer_port.vit
  adapters/
    sql_user_repo_adapter.vit
    bcrypt_hasher_adapter.vit
    jwt_token_adapter.vit
  app/
    compose_auth.vit
```

## Checklist

1. Domaine sans dependance I/O directe.
2. Tests du coeur sans mocks lourds.
3. Adaptateurs remplaçables sans casser le domaine.

## Documentation a produire

1. Diagramme des ports/adaptateurs.
2. Contrat d'interface pour chaque port.
3. Exemples de composition application.
4. Anti-patterns d'architecture observes et correctifs.
5. Guide de migration d'une architecture existante vers l'hexagonal.

## Exemples progressifs (N1 -> N3)

### N1 (base): coeur metier seul

```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  // Sortie locale: valeur retournee par la procedure
  give is_active
}
```

Objectif:
1. Garder une logique pure sans I/O.

### N2 (intermediaire): port explicite

```vit
space core/auth

proc login_flow(found_user: bool, pass_ok: bool) -> bool {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if not found_user { give false }
  // Garde: bloque un cas invalide avant de continuer
  if not pass_ok { give false }
  // Sortie locale: valeur retournee par la procedure
  give true
}
```

Objectif:
1. Modeliser le contrat du port sans adapter concret.

### N3 (avance): composition hexagonale

```text
core/auth/domain/auth_service.vit
core/auth/ports/user_repo_port.vit
core/auth/ports/password_hasher_port.vit
core/auth/adapters/sql_user_repo_adapter.vit
core/auth/adapters/bcrypt_hasher_adapter.vit
core/auth/app/compose_auth.vit
```

Objectif:
1. Isoler domaine, ports, adaptateurs et composition.

### Anti-exemple (invalide architectural)

```vit
space core/auth

proc login(email: string, password: string) -> bool {
  # invalide architecturalement: SQL direct dans le coeur domaine
  // Sortie locale: valeur retournee par la procedure
  give db_query_user(email) == password
}
```

Pourquoi c'est un probleme:
1. Couplage I/O dans le coeur.
2. Testabilite fortement degradee.

## Validation rapide

1. Verifier que N1 et N2 restent sans dependance externe.
2. Verifier que N3 place l'I/O uniquement dans `adapters`.
3. Refuser tout anti-exemple qui melange domaine et infrastructure.

## Pourquoi

Cette section explicite la valeur pratique: réduire les erreurs, accélérer le diagnostic et stabiliser les évolutions.

## Test mental

Question de contrôle: si vous modifiez une hypothèse clé, quel résultat doit changer et pourquoi?

## À faire

1. Exécuter l’exemple nominal.
2. Introduire un cas limite.
3. Vérifier la sortie et documenter l’écart.

## Corrigé minimal

Corrigé: conserver la version la plus simple qui respecte le contrat, puis ajouter un test de non-régression.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **architecture hexagonale en vitte**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  // Sortie locale: valeur retournee par la procedure
  give is_active
}
```

Lecture ligne par ligne:
1. `space core/auth` -> participe au déroulé du traitement.
2. `proc can_login(is_active: bool) -> bool {` -> pose un contrat clair de fonction.
3. `give is_active` -> renvoie la sortie vérifiable.
4. `}` -> participe au déroulé du traitement.

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

Exemple approfondi pour **architecture hexagonale vitte**: flux applicatif complet (entrée, politique métier, persistance simulée, code de sortie).

```vit
// Exemple long: flux complet et vérifiable
space demo/architecture-hexagonale-vitte

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

Scénarios recommandés (architecture hexagonale vitte):
- Requête nominale -> sortie 0.
- Entrée invalide id<=0 -> sortie 91.
- Refus métier valeur<5 -> sortie 94.
