# 37. Conventions de modules a grande echelle

Niveau: Avance

Prérequis: `book/chapters/09-modules.md`.
Voir aussi: `book/chapters/32-catalogue-modules.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

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


```vit
// Scenario conventions modules echelle: execution complete et verifiable
space demo/conventions-modules-echelle

form ModuleSpec { name_len: int major: int minor: int }
pick Resolve { case Loaded(path_code: int) case Missing(code: int) }

proc normalize_len(n: int) -> int {

  if n <= 0 { give 0 }

  give n
}

// Résolution: valide la référence et produit une décision de chargement
proc resolve(m: ModuleSpec) -> Resolve {
  let n: int = normalize_len(m.name_len)

  if n == 0 { give Resolve.Missing(71) }

  if m.major <= 0 { give Resolve.Missing(72) }

  if m.minor < 0 { give Resolve.Missing(73) }
  let code: int = 100 + (m.major * 10) + m.minor

  give Resolve.Loaded(code)
}

// Conversion finale vers un code de sortie
proc to_exit(r: Resolve) -> int {

  match r {
    case Loaded(_) { give 0 }
    case Missing(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let m: ModuleSpec = ModuleSpec(8, 1, 2)
  let r: Resolve = resolve(m)

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
- Spécification valide -> sortie 0.
- Nom invalide -> sortie 71.
- Version invalide -> sortie 72 ou 73.


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


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
