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

## Exemples représentatifs (par cas d'usage)

Cette section s'appuie sur du code concret pour **documenter les documentations**.
Objectif: comprendre vite ce que fait le code, pourquoi, et comment le corriger.

### Exemple 1: extrait réel du chapitre (cas nominal)

```vit
entry main at app/docmeta {
  return 0
}
```

Lecture guidée (ligne par ligne):
1. `entry main at app/docmeta {` -> définit le point d'entrée exécutable.
2. `return 0` -> renvoie une valeur observable et testable.
3. `}` -> participe au flux nominal du programme.

Entrée -> Sortie attendue:
1. Entrée: données conformes au contrat.
2. Traitement: chemin nominal exécuté.
3. Sortie: valeur déterministe observable.

### Exemple 2: garde explicite (cas limite)

```vit
proc clamp_non_negative(x: int) -> int {
  if x < 0 {
    give 0
  }
  give x
}
```

Quand l'utiliser: éviter les comportements implicites sur entrées hors contrat.

### Exemple 3: erreur de type volontaire (diagnostic)

```vit
proc needs_int(x: int) -> int {
  give x
}
entry main at app/demo {
  let s: string = "42"
  return needs_int(s)
}
```

Quand l'utiliser: entraîner la lecture des diagnostics compilateur.

### Exemple 4: séparation module / API

```vit
space app/math
proc add(a: int, b: int) -> int {
  give a + b
}
share add
```

Quand l'utiliser: clarifier ce qui est public vs interne dans l'architecture.

### Exemple 5: flux de contrôle lisible

```vit
entry main at app/demo {
  let n: int = 3
  if n > 0 {
    return 1
  }
  return 0
}
```

Quand l'utiliser: expliciter une décision métier avec un chemin nominal et un fallback.

### Exemple 6: version testable d'une procédure

```vit
proc is_even(x: int) -> bool {
  give x % 2 == 0
}
```

Cas de test conseillés:
1. `is_even(2)` -> `true`.
2. `is_even(3)` -> `false`.
3. `is_even(0)` -> `true`.

Quand l'utiliser: convertir rapidement une règle en contrat vérifiable.

### Exemple 7: refactor sûr (avant/après)

Avant:
```vit
proc parse_port(s: string) -> int {
  give 0
}
```

Après:
```vit
proc parse_port(s: string) -> int {
  if s == "" {
    give 0
  }
  give 8080
}
```

Quand l'utiliser: faire évoluer le comportement sans casser la signature publique.

### Exemple 8: correction guidée basée sur le code

Procédure de correction:
1. Reproduire le bug sur un snippet minimal.
2. Corriger une seule ligne.
3. Recompiler et vérifier la sortie.
4. Ajouter un test de non-régression.

### Checklist de lecture rapide

1. Où est le contrat d'entrée?
2. Quel est le chemin nominal?
3. Quel est le cas limite traité?
4. Quelle erreur reste explicite?
5. Quel test prouve le comportement?

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->

<!-- AUTO_EXPANSION_V1 START -->

## Approfondissement guidé par le code

### 1. Snippet de référence du chapitre

```vit
entry main at app/docmeta {
  return 0
}
```

### 2. Ce que fait ce code, ligne par ligne

1. `entry main at app/docmeta {` -> définit l'entrée du programme.
2. `return 0` -> retourne le résultat observé.
3. `}` -> participe au flux nominal.

### 3. Lecture exécutable (entrée -> sortie)

1. Entrée: valeurs conformes au contrat.
2. Exécution: chemin nominal suivi sans ambiguïté.
3. Sortie: résultat déterministe, testable immédiatement.

### 4. Variante d'erreur + correction

Erreur typique: mélanger un type inattendu dans un appel.
Correction: ajuster l'argument au contrat attendu, puis recompiler.

### 5. Pourquoi cette méthode est concrète

On part du code réel, pas d'un discours abstrait.
Chaque modification est locale, visible, et vérifiable par test.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Atelier concret: cas pratique sur 74-documenter-les-documentations.md

Code de base:
```vit
entry main at app/docmeta {
  return 0
}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

<!-- AUTO_EXPANSION_V1 END -->
