# 37. Conventions de modules a grande echelle

Niveau: Avance

Prérequis: `book/chapters/09-modules.md`.
Voir aussi: `book/chapters/32-catalogue-modules.md`.

## Objectif

Structurer une base de code importante avec `space`, `pull`, `use`, `share` sans couplage excessif.

## Conventions recommandees

1. `space` aligne sur le domaine (ex: `core/auth`, `core/http`).
2. `pull` reserve aux dependances internes claires.
3. `use` pour l'API consommee localement.
4. `share` limite aux symboles stables.

## Exemple minimal

```vit
space core/auth
pull core/crypto as crypto
use core/user.{UserRepo}
share login, logout
```

## Regles d'echelle

- Interdire les dependances circulaires entre espaces.
- Limiter la profondeur de chemin module.
- Stabiliser les points d'entree publics.

## Checklist

1. Le module a-t-il une responsabilite unique?
2. Les imports sont-ils minimaux?
3. Les symboles `share` sont-ils versionnes?

## Exemples progressifs (N1 -> N3)

### N1 (base): module minimal

```vit
space core/auth
proc login() -> int { give 0 }
share login
```

### N2 (intermediaire): imports explicites

```vit
space core/auth
pull core/crypto as crypto
use core/user.{UserRepo}
proc login() -> int { give 0 }
share login
```

### N3 (avance): surface publique stable

```vit
space core/auth
proc login() -> int { give 0 }
proc logout() -> int { give 0 }
share login, logout
```

### Anti-exemple

```vit
space core/auth
proc internal_only() -> int { give 0 }
share all
```

## Validation rapide

1. Verifier frontiere `share`.
2. Eviter cycles modules.
3. Limiter imports au necessaire.

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

Thème: **conventions de modules a grande echelle**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
space core/auth
pull core/crypto as crypto
use core/user.{UserRepo}
share login, logout
```

Lecture ligne par ligne:
1. `space core/auth` -> participe au déroulé du traitement.
2. `pull core/crypto as crypto` -> participe au déroulé du traitement.
3. `use core/user.{UserRepo}` -> participe au déroulé du traitement.
4. `share login, logout` -> participe au déroulé du traitement.

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

Exemple approfondi pour **conventions modules echelle**: résolution de module (normalisation, versionnage, décision de chargement).

```vit
// Exemple long: flux complet et vérifiable
space demo/conventions-modules-echelle

form ModuleSpec { name_len: int major: int minor: int }
pick Resolve { case Loaded(path_code: int) case Missing(code: int) }

proc normalize_len(n: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if n <= 0 { give 0 }
  // Sortie locale: valeur retournee par la procedure
  give n
}

// Résolution: valide la référence et produit une décision de chargement
proc resolve(m: ModuleSpec) -> Resolve {
  let n: int = normalize_len(m.name_len)
  // Garde: bloque un cas invalide avant de continuer
  if n == 0 { give Resolve.Missing(71) }
  // Garde: bloque un cas invalide avant de continuer
  if m.major <= 0 { give Resolve.Missing(72) }
  // Garde: bloque un cas invalide avant de continuer
  if m.minor < 0 { give Resolve.Missing(73) }
  let code: int = 100 + (m.major * 10) + m.minor
  // Sortie locale: valeur retournee par la procedure
  give Resolve.Loaded(code)
}

// Projection finale: convertit l'état métier en code de sortie
proc to_exit(r: Resolve) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Loaded(_) { give 0 }
    case Missing(c) { give c }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let m: ModuleSpec = ModuleSpec(8, 1, 2)
  let r: Resolve = resolve(m)
  // Sortie programme: code de retour observable
  return to_exit(r)
}
```

Scénarios recommandés (conventions modules echelle):
- Spécification valide -> sortie 0.
- Nom invalide -> sortie 71.
- Version invalide -> sortie 72 ou 73.
