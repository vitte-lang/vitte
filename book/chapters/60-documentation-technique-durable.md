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

<!-- AUTO_EXPANSION_V1 START -->

## Approfondissement guidé

### 1. Ce qu'il faut vraiment retenir

Le coeur de **documentation technique durable** est de prendre des décisions lisibles et vérifiables.
Dans un projet réel, la compréhension rapide prime sur la complexité apparente.
L'objectif de cette section est de transformer le chapitre en guide opérationnel,
pas en résumé théorique.

Trois idées pratiques gouvernent ce sujet:
1. faire un changement à la fois;
2. garder des invariants explicites;
3. valider le résultat avec une preuve simple (test, sortie, diagnostic).

### 2. Carte mentale utilisable en équipe

Quand vous travaillez sur **documentation technique durable**, posez systématiquement ces questions:
- quel est le contrat d'entrée;
- quel est le résultat attendu;
- quels sont les cas limites visibles;
- quelle erreur doit être compréhensible en moins de 30 secondes.

Cette carte mentale évite les refactors fragiles.
Elle permet aussi d'aligner débutants et profils avancés sur le même langage de travail.

### Étude de cas pratique

Cas: un module lié à **documentation technique durable** (60-documentation-technique-durable.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **documentation technique durable** (60-documentation-technique-durable.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **documentation technique durable** (60-documentation-technique-durable.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **documentation technique durable** (60-documentation-technique-durable.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **documentation technique durable** (60-documentation-technique-durable.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **documentation technique durable** (60-documentation-technique-durable.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **documentation technique durable** (60-documentation-technique-durable.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **documentation technique durable** (60-documentation-technique-durable.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

<!-- AUTO_EXPANSION_V1 END -->
