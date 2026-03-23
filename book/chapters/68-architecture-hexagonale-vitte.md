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
  give is_active
}
```

Objectif:
1. Garder une logique pure sans I/O.

### N2 (intermediaire): port explicite

```vit
space core/auth

proc login_flow(found_user: bool, pass_ok: bool) -> bool {
  if not found_user { give false }
  if not pass_ok { give false }
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

## Exemples représentatifs (par cas d'usage)

Cette section s'appuie sur du code concret pour **architecture hexagonale en vitte**.
Objectif: comprendre vite ce que fait le code, pourquoi, et comment le corriger.

### Exemple 1: extrait réel du chapitre (cas nominal)

```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
}
```

Lecture guidée (ligne par ligne):
1. `space core/auth` -> positionne le code dans son module.
2. `proc can_login(is_active: bool) -> bool {` -> déclare un contrat clair (entrées/sortie).
3. `give is_active` -> renvoie une valeur observable et testable.
4. `}` -> participe au flux nominal du programme.

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
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
}
```

### 2. Ce que fait ce code, ligne par ligne

1. `space core/auth` -> positionne le code dans son module.
2. `proc can_login(is_active: bool) -> bool {` -> déclare une procédure avec contrat explicite.
3. `give is_active` -> retourne le résultat observé.
4. `}` -> participe au flux nominal.

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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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

### Atelier concret: cas pratique sur 68-architecture-hexagonale-vitte.md

Code de base:
```vit
space core/auth

proc can_login(is_active: bool) -> bool {
  give is_active
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
