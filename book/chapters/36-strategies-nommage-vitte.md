# 36. Strategies de nommage en Vitte

Niveau: Intermediaire

Prérequis: `book/chapters/28-conventions.md`.
Voir aussi: `book/chapters/09-modules.md`, `book/chapters/06-procedures.md`.

## Objectif

Choisir des noms predictibles pour reduire les erreurs de lecture et de maintenance.

## Regles pratiques

1. Noms de procedures: verbe + objet (`parse_port`, `load_user`).
2. Predicats booleens: prefixes `is_`, `has_`, `can_`.
3. Types: noms metier (`User`, `OrderStatus`), pas noms techniques opaques.
4. Modules: noms courts, semantiques et stables.

## Exemple minimal

```vit
form User {
  id: int
  name: string
}

proc is_admin(role: int) -> bool {
  // Sortie locale: valeur retournee par la procedure
  give role == 9
}
```

## Anti-patterns

- Prefixes techniques partout (`tmp_`, `data_`, `obj_`).
- Abreviations ambiguës (`cfg2`, `usrx`).
- Renommages massifs sans migration.

## Checklist

1. Le nom exprime-t-il l'intention metier?
2. Le nom est-il coherent avec le module?
3. Le nom reste-t-il stable dans le temps?

## Exemples progressifs (N1 -> N3)

### N1 (base): nommage clair

```vit
proc parse_port(x: int) -> int { give x }
```

### N2 (intermediaire): predicat booleen explicite

```vit
proc is_admin(role: int) -> bool { give role == 9 }
```

### N3 (avance): cohesion de noms

```vit
proc load_user(id: int) -> int { give id }
proc validate_user(id: int) -> bool { give id > 0 }
proc save_user(id: int) -> int { give id }
```

### Anti-exemple

```vit
proc do_it(x: int) -> int { give x }
```

## Validation rapide

1. Verbe + objet.
2. Predicats en `is_`/`has_`.
3. Vocabulaire coherent sur tout le module.

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

Thème: **strategies de nommage en vitte**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
form User {
  id: int
  name: string
}

proc is_admin(role: int) -> bool {
  // Sortie locale: valeur retournee par la procedure
  give role == 9
}
```

Lecture ligne par ligne:
1. `form User {` -> participe au déroulé du traitement.
2. `id: int` -> participe au déroulé du traitement.
3. `name: string` -> participe au déroulé du traitement.
4. `}` -> participe au déroulé du traitement.
5. `proc is_admin(role: int) -> bool {` -> pose un contrat clair de fonction.
6. `give role == 9` -> renvoie la sortie vérifiable.
7. `}` -> participe au déroulé du traitement.

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

Exemple approfondi pour **strategies nommage vitte**: résolution de module (normalisation, versionnage, décision de chargement).

```vit
// Exemple long: flux complet et vérifiable
space demo/strategies-nommage-vitte

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

Scénarios recommandés (strategies nommage vitte):
- Spécification valide -> sortie 0.
- Nom invalide -> sortie 71.
- Version invalide -> sortie 72 ou 73.
