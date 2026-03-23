# 46. Performance: structures de donnees adaptees

Niveau: Avance

Prérequis: `book/chapters/08-structures.md`, `book/chapters/11-collections.md`.
Voir aussi: à définir.

## Objectif

Choisir la structure de donnees selon les operations dominantes.

## Regle simple

1. Lectures indexees frequentes: structure indexable.
2. Recherche par cle frequente: map.
3. Donnees heterogenes stables: form/pick.
4. Flux sequentiel: liste/slice optimisee.

## Checklist

1. Operation dominante identifiee.
2. Complexite attendue explicitee.
3. Mesure avant/apres validee.


## Exemples progressifs (N1 -> N3)

### N1 (base): liste adaptee iteration

```vit
proc first(xs: [int]) -> int { give 0 }
```

### N2 (intermediaire): map adaptee lookup

```vit
form User { id: int name: string }
proc get_user_id(u: User) -> int { give u.id }
```

### N3 (avance): choix motive

```vit
pick LookupMode { case Linear, case Keyed }
proc pick_mode(by_key: bool) -> LookupMode {
  if by_key { give Keyed }
  give Linear
}
```

### Anti-exemple

```vit
proc choose_unknown() -> int { give 0 }
# aucun critere de choix structure
```

## Validation rapide

1. Identifier operation dominante.
2. Justifier structure.
3. Verifier performance cible.

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

## Exemples représentatifs (par cas d'usage)

Cette section s'appuie sur du code concret pour **performance: structures de donnees adaptees**.
Objectif: comprendre vite ce que fait le code, pourquoi, et comment le corriger.

### Exemple 1: extrait réel du chapitre (cas nominal)

```vit
proc first(xs: [int]) -> int { give 0 }
```

Lecture guidée (ligne par ligne):
1. `proc first(xs: [int]) -> int { give 0 }` -> déclare un contrat clair (entrées/sortie).

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
proc first(xs: [int]) -> int { give 0 }
```

### 2. Ce que fait ce code, ligne par ligne

1. `proc first(xs: [int]) -> int { give 0 }` -> déclare une procédure avec contrat explicite.

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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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

### Atelier concret: cas pratique sur 46-performance-structures-adaptees.md

Code de base:
```vit
proc first(xs: [int]) -> int { give 0 }
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
