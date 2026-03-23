# 52. Tests de grammaire (valid/invalid corpus)

Niveau: Avance

Prérequis: `book/chapters/27-grammaire.md`, `book/chapters/31-erreurs-build.md`.
Voir aussi: `book/grammar/diagnostics/expected`.

## Objectif

Verifier que la grammaire accepte les formes valides et rejette proprement les invalides.

## Corpus

1. `valid`: exemples representatifs de syntaxe autorisee.
2. `invalid`: erreurs ciblees avec diagnostics attendus.

## Checklist

1. Un test par regle critique.
2. Message d'erreur attendu versionne.
3. Couverture des ambiguities connues.


## Exemples progressifs (N1 -> N3)

### N1 (base): valid

```vit
entry main at app/demo {
  return 0
}
```

### N2 (intermediaire): invalid

```vit
entry main at app/demo {
  return
}
```

### N3 (avance): regle ciblee

```vit
match 1 {
  case 1 { give 1 }
  otherwise { give 0 }
}
```

### Anti-exemple

```vit
# corpus contient seulement des cas valides
```

## Validation rapide

1. 1 valid + 1 invalid par regle critique.
2. Diagnostics attendus versionnes.
3. Couverture ambiguities mise a jour.

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

Le coeur de **tests de grammaire (valid/invalid corpus)** est de prendre des décisions lisibles et vérifiables.
Dans un projet réel, la compréhension rapide prime sur la complexité apparente.
L'objectif de cette section est de transformer le chapitre en guide opérationnel,
pas en résumé théorique.

Trois idées pratiques gouvernent ce sujet:
1. faire un changement à la fois;
2. garder des invariants explicites;
3. valider le résultat avec une preuve simple (test, sortie, diagnostic).

### 2. Carte mentale utilisable en équipe

Quand vous travaillez sur **tests de grammaire (valid/invalid corpus)**, posez systématiquement ces questions:
- quel est le contrat d'entrée;
- quel est le résultat attendu;
- quels sont les cas limites visibles;
- quelle erreur doit être compréhensible en moins de 30 secondes.

Cette carte mentale évite les refactors fragiles.
Elle permet aussi d'aligner débutants et profils avancés sur le même langage de travail.

### Étude de cas pratique

Cas: un module lié à **tests de grammaire (valid/invalid corpus)** (52-tests-grammaire-valid-invalid.md) doit évoluer sans casser l'existant.
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

Cas: un module lié à **tests de grammaire (valid/invalid corpus)** (52-tests-grammaire-valid-invalid.md) doit évoluer sans casser l'existant.
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

Cas: un module lié à **tests de grammaire (valid/invalid corpus)** (52-tests-grammaire-valid-invalid.md) doit évoluer sans casser l'existant.
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

Cas: un module lié à **tests de grammaire (valid/invalid corpus)** (52-tests-grammaire-valid-invalid.md) doit évoluer sans casser l'existant.
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

Cas: un module lié à **tests de grammaire (valid/invalid corpus)** (52-tests-grammaire-valid-invalid.md) doit évoluer sans casser l'existant.
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

Cas: un module lié à **tests de grammaire (valid/invalid corpus)** (52-tests-grammaire-valid-invalid.md) doit évoluer sans casser l'existant.
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

Cas: un module lié à **tests de grammaire (valid/invalid corpus)** (52-tests-grammaire-valid-invalid.md) doit évoluer sans casser l'existant.
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

Cas: un module lié à **tests de grammaire (valid/invalid corpus)** (52-tests-grammaire-valid-invalid.md) doit évoluer sans casser l'existant.
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

Cas: un module lié à **tests de grammaire (valid/invalid corpus)** (52-tests-grammaire-valid-invalid.md) doit évoluer sans casser l'existant.
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
