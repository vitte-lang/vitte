# 40. Gestion de compatibilite (breaking/non-breaking)

Niveau: Avance

Prérequis: `book/chapters/39-design-api-publiques-stables.md`.
Voir aussi: `book/grammar/grammar-changelog.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

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


```vit
// Scenario compatibilite breaking: execution complete et verifiable
space demo/compatibilite-breaking

form Input { id: int value: int quota: int }
pick Eval { case Accepted(score: int) case Rejected(code: int) }

proc validate(x: Input) -> Eval {

  if x.id <= 0 { give Eval.Rejected(21) }

  if x.quota < 0 { give Eval.Rejected(22) }

  if x.value < 0 { give Eval.Rejected(23) }

  give Eval.Accepted(x.value)
}

proc transform(score: int, quota: int) -> int {
  let capped: int = score
  if capped > quota { set capped = quota }

  if capped < 0 { give 0 }

  give capped * 2
}

proc decide(r: Eval, quota: int) -> Eval {

  match r {
    case Accepted(s) {
      let out: int = transform(s, quota)

      if out >= 10 { give Eval.Accepted(out) }

      give Eval.Rejected(31)
    }
    case Rejected(c) { give Eval.Rejected(c) }
    otherwise { give Eval.Rejected(70) }
  }
}

// Conversion finale vers un code de sortie
proc to_exit(r: Eval) -> int {

  match r {
    case Accepted(_) { give 0 }
    case Rejected(code) { give code }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let x: Input = Input(1, 8, 9)
  let v: Eval = validate(x)
  let d: Eval = decide(v, x.quota)

  return to_exit(d)
}
```

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.


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
