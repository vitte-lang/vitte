# 60. Documentation technique durable

Niveau: Intermédiaire

Prérequis: `book/STYLE.md`, `book/chapters/29-style.md`.
Voir aussi: `book/chapters/74-documenter-les-documentations.md`.

## Objectif

Produire une documentation maintenable qui reste utile apres plusieurs evolutions du code.

## Principes

1. Documenter decisions et contrats, pas seulement syntaxe.
2. Fournir exemples minimaux executables.
3. Lier doc et tests pour eviter divergence.
4. Marquer clairement deprecated et migration.

## Doc-as-code (cadre commun)

1. Une source de verite par sujet (pas de duplication concurrente).
2. Chaque chapitre a un proprietaire explicite.
3. Chaque changement de code qui impacte un contrat met a jour la doc.
4. Les liens, exemples et commandes sont verifies en CI.

## Contrat minimal par chapitre

Chaque chapitre doit contenir au minimum:

1. `Objectif` mesurable.
2. `Prerequis` et `Voir aussi`.
3. Un exemple minimal executable ou verifiable.
4. Une checklist de validation.
5. Une section de limites et risques.

## Definition of Done documentaire

Un chapitre est considere "done" si:

1. Les exemples compilent (ou sont explicitement non executables avec raison).
2. Les commandes sont testees sur un environnement propre.
3. Les liens internes/externe sont valides.
4. Les termes critiques sont harmonises avec le glossaire.
5. La migration est documentee en cas de breaking change.

## Matrice de tracabilite

Relier chaque section a une preuve:

1. Regle de grammaire -> test `valid/invalid`.
2. Contrat API -> test unitaire/intégration.
3. Diagnostic attendu -> snapshot de message.
4. Procedure d'exploitation -> runbook valide.

## Processus de maintenance

1. Triage mensuel: liens morts, exemples obsoletes, sections sans tests.
2. Revue trimestrielle: coherence globale de vocabulaire et structure.
3. Nettoyage semestriel: fusion/suppression des contenus redondants.
4. Journal des changements: date, raison, impact lecteur.

## Exemple concret: avant/apres

### Avant (chapitre mal structure)

```md
# Parseur

Le parseur est important.
Il faut faire attention aux erreurs.
On a plusieurs cas.
```

Problemes:
1. Aucun objectif mesurable.
2. Pas de prérequis ni public cible.
3. Pas d'exemple executable.
4. Pas de checklist ni preuve.

### Apres (chapitre conforme)

````md
# Parseur: triage des erreurs de syntaxe

## Objectif
Identifier en moins de 2 minutes la premiere erreur de parse.

## Prerequis
`book/chapters/27-grammaire.md`

## Exemple minimal
```vit
entry main at app/demo {
  emit 1
}
```

## Checklist
1. Reproduire l'erreur.
2. Isoler la premiere cause.
3. Valider le correctif sur un cas minimal.
````

## Checklist de revue documentaire

1. Le lecteur sait quoi faire en moins de 60 secondes.
2. Les sections vont du concret vers l'avance.
3. Les erreurs frequentes ont un correctif minimal.
4. Le chapitre reste lisible sans connaissance implicite.
5. Les termes sont consistants avec le reste du livre.

## Checklist

1. Chaque section a un objectif testable.
2. Les liens internes sont valides.
3. Les exemples compilent.
4. Le chapitre declare ses limites.
5. Le plan de maintenance est defini.

## Exemples progressifs (N1 -> N3)

### N1 (base): doc avec snippet executable

Snippet Vitte documente:

```vit
entry main at app/doc {
  // Sortie programme: code de retour observable
  return 0
}
```

Commande de vérification:

```bash
make grammar-check
```

### N2 (intermediaire): doc + cas invalide

Snippet Vitte invalide:

```vit
entry main at app/doc {
  return
}
```

Commandes de vérification:

```bash
make grammar-test
make book-qa
```

### N3 (avance): doc gouvernee par DoD

Snippet Vitte nominal:

```vit
entry main at app/doc {
  let x: int = 2
  // Sortie programme: code de retour observable
  return x
}
```

Commandes governance:

```bash
make grammar-gate
make book-qa-strict
```

### Anti-exemple

```text
Chapitre sans owner, sans preuve d execution, sans date de revue.
```

## Validation rapide

1. Verifier snippet nominal + invalide.
2. Verifier commandes QA vertes.
3. Verifier DoD + revue datee.

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

Thème: **documentation technique durable**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/demo {
  emit 1
}
```

Lecture ligne par ligne:
1. `entry main at app/demo {` -> définit le point d'entrée du scénario.
2. `emit 1` -> participe au déroulé du traitement.
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

Exemple approfondi pour **documentation technique durable**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/documentation-technique-durable

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

Scénarios recommandés (documentation technique durable):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
