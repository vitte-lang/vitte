# 74. Documenter les documentations

Niveau: Tous niveaux

Prérequis: `book/chapters/60-documentation-technique-durable.md`.
Voir aussi: `book/STYLE.md`, `book/checklist-editoriale.md`.

## Objectif

Appliquer au contenu documentaire le meme niveau de rigueur que pour le code: contrats, tests, ownership, versioning.

## Pourquoi c'est critique

1. La documentation est une interface publique.
2. Une doc incorrecte cree des bugs d'usage.
3. Une doc non maintenue augmente le cout de support.

## Contrat de meta-documentation

Chaque document doit declarer:

1. Son objectif et son public cible.
2. Son proprietaire (owner) et sa frequence de revue.
3. Sa source de verite (fichier, regle, spec, test).
4. Son statut (`draft`, `stable`, `deprecated`).

## Template pret a copier

```md
# <Titre du document>

## Objectif
<Resultat attendu, mesurable>

## Owner
<Equipe ou personne responsable>

## Source de verite
<Fichier/rule/spec/tests qui font foi>

## Definition of Done (DoD)
1. <Critere 1>
2. <Critere 2>
3. <Critere 3>

## Derniere revue
<YYYY-MM-DD> - <Qui> - <Resume court>

## Prochaine revue
<YYYY-MM-DD> - <Scope prevu>
```

## Tests de documentation

1. Link-check automatique.
2. Vérification syntaxique des snippets.
3. Validation des commandes shell presentees.
4. Detection de sections obsoletes (date + owner).

## Politique de version documentaire

1. Changement editorial mineur: correction sans impact contrat.
2. Changement fonctionnel: mise a jour du contrat et des exemples.
3. Changement breaking: section migration obligatoire.

## Runbook de maintenance

1. Revue hebdomadaire rapide: liens, erreurs evidentes.
2. Revue mensuelle structurelle: coherence et redondances.
3. Revue trimestrielle de fond: suppression/merge des chapitres faibles.

## Checklist

1. Le lecteur cible est explicitement defini.
2. Le chapitre indique quoi faire, pas seulement quoi savoir.
3. Chaque exemple a une preuve de validite.
4. Les decisions importantes sont historisees.
5. Le plan de maintenance est visible.

## Exemples progressifs (N1 -> N3)

### N1 (base): doc avec snippet Vitte

````md
## Exemple minimal
```vit
entry main at app/docmeta {
  // Sortie programme: code de retour observable
  return 0
}
```
````

Commande de preuve:

```bash
make grammar-check
```

### N2 (intermediaire): doc avec invalide + diagnostic

````md
## Exemple invalide
```vit
entry main at app/docmeta {
  return
}
```
````

Commandes QA:

```bash
make grammar-test
make book-qa
```

### N3 (avance): doc gouvernee et verifiee en CI

```md
## Owner
Equipe Langage
## DoD
1. snippets verifies
2. liens verifies
3. revue datee
```

Commandes gouvernance:

```bash
make grammar-gate
make book-qa-strict
```

### Anti-exemple

```text
Documentation sans snippets testables ni commandes de vérification.
```

## Validation rapide

1. Verifier snippet nominal et invalide.
2. Verifier QA documentaire.
3. Verifier owner + DoD + revues datees.

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

Thème: **documenter les documentations**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/docmeta {
  // Sortie programme: code de retour observable
  return 0
}
```

Lecture ligne par ligne:
1. `entry main at app/docmeta {` -> définit le point d'entrée du scénario.
2. `return 0` -> renvoie la sortie vérifiable.
3. `}` -> participe au déroulé du traitement.

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

Exemple approfondi pour **documenter les documentations**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/documenter-les-documentations

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

Scénarios recommandés (documenter les documentations):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
