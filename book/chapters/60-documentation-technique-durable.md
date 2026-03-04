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

