# 40. Gestion de compatibilite (breaking/non-breaking)

Niveau: Avance

Prérequis: `book/chapters/39-design-api-publiques-stables.md`.
Voir aussi: `book/grammar/grammar-changelog.md`.

## Objectif

Classer les changements correctement pour informer les utilisateurs et planifier les migrations.

## Classification

- Non-breaking: refactor interne, ajout optionnel, nouvelle API additive.
- Breaking: suppression/renommage public, changement de type de retour, semantique incompatible.

## Processus

1. Evaluer l'impact utilisateur.
2. Tagger le changement (breaking/non-breaking).
3. Documenter avant/apres et plan de migration.
4. Ajouter tests de compatibilite.

## Exemple

- `proc load(x: string) -> Item` devient `proc load(x: string) -> Result[Item]`.
- Impact: breaking pour tous les call sites.

## Checklist

1. Niveau d'impact publie.
2. Strategie de transition fournie.
3. Date de retrait annoncee si deprecation.

## Exemples progressifs (N1 -> N3)

### N1 (base): non-breaking

```vit
proc load_user(id: int) -> int { give id }
proc load_user_safe(id: int) -> int { give id }
```

### N2 (intermediaire): breaking explicite

```vit
proc load_user(id: int) -> bool { give id > 0 }
```

### N3 (avance): transition propre

```vit
proc load_user_v1(id: int) -> int { give id }
proc load_user_v2(id: int) -> bool { give id > 0 }
```

### Anti-exemple

```vit
proc load_user(id: int) -> int { give id }
proc load_user(id: int) -> bool { give true }
```

## Validation rapide

1. Classer impact (breaking/non-breaking).
2. Fournir migration explicite.
3. Bloquer regressions en CI.

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

Thème: **gestion de compatibilite (breaking/non-breaking)**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc load_user(id: int) -> int { give id }
proc load_user_safe(id: int) -> int { give id }
```

Lecture ligne par ligne:
1. `proc load_user(id: int) -> int { give id }` -> pose un contrat clair de fonction.
2. `proc load_user_safe(id: int) -> int { give id }` -> pose un contrat clair de fonction.

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

Exemple approfondi pour **compatibilite breaking**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/compatibilite-breaking

form Input { id: int value: int quota: int }
pick Eval { case Accepted(score: int) case Rejected(code: int) }

proc validate(x: Input) -> Eval {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x.id <= 0 { give Eval.Rejected(21) }
  // Garde: bloque un cas invalide avant de continuer
  if x.quota < 0 { give Eval.Rejected(22) }
  // Garde: bloque un cas invalide avant de continuer
  if x.value < 0 { give Eval.Rejected(23) }
  // Sortie locale: valeur retournee par la procedure
  give Eval.Accepted(x.value)
}

proc transform(score: int, quota: int) -> int {
  let capped: int = score
  if capped > quota { set capped = quota }
  // Garde: bloque un cas invalide avant de continuer
  if capped < 0 { give 0 }
  // Sortie locale: valeur retournee par la procedure
  give capped * 2
}

proc decide(r: Eval, quota: int) -> Eval {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Accepted(s) {
      let out: int = transform(s, quota)
      // Garde: bloque un cas invalide avant de continuer
  if out >= 10 { give Eval.Accepted(out) }
      // Sortie locale: valeur retournee par la procedure
  give Eval.Rejected(31)
    }
    case Rejected(c) { give Eval.Rejected(c) }
    otherwise { give Eval.Rejected(70) }
  }
}

// Projection finale: convertit l'état métier en code de sortie
proc to_exit(r: Eval) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Accepted(_) { give 0 }
    case Rejected(code) { give code }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let x: Input = Input(1, 8, 9)
  let v: Eval = validate(x)
  let d: Eval = decide(v, x.quota)
  // Sortie programme: code de retour observable
  return to_exit(d)
}
```

Scénarios recommandés (compatibilite breaking):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
