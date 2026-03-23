# 16a. Liaison native avec `std/bridge`

Niveau: Intermédiaire.

Prérequis: chapitre précédent `book/chapters/16-interop.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/17-stdlib.md`, `book/chapters/23-projet-sys.md`, `book/chapters/30-faq.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Liaison native avec `std/bridge`**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Liaison native avec `std/bridge`**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Liaison native avec `std/bridge`**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Liaison native avec `std/bridge`** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Liaison native avec `std/bridge`**.

## Exercice court

Prenez un exemple du chapitre sur **Liaison native avec `std/bridge`**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Liaison native avec `std/bridge`**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: `use` reste au niveau module, jamais dans un bloc.
- Statements: `let`, `give`, `return` restent dans les blocs de `proc`/`entry`.
- Types primaires: `int`, `i32`, `u32`, `string`, `bool` suivent `type_primary`.

## Keywords à revoir

- `book/keywords/use.md`.
- `book/keywords/proc.md`.
- `book/keywords/entry.md`.
- `book/keywords/give.md`.
- `book/keywords/return.md`.

## Objectif

Construire un point d’entrée unique pour la liaison native afin de réduire les dépendances directes à des détails d’implémentation.

## Exemple

```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
  return 0
}
```

Lecture ligne par ligne:
1. `use std/bridge/io` importe la façade d’E/S métier; le code appelant n’a pas besoin de connaître `std/io/*`.
2. `use std/bridge/system` importe la façade système (`env`, `path`, `process`, `os`) dans un point unique.
3. `use std/bridge/time` importe l’accès temporel sans coupler le code à un backend précis.
4. `entry main at app/bridge_demo {` définit le point d’exécution du programme.
5. `let home = get("HOME")` lit une variable d’environnement via la façade système.
6. `let here = cwd()` récupère le répertoire courant via la même façade.
7. `let t0 = now()` capture un instant via la façade temps.
8. `let _ = print_line("bridge ready")` produit une sortie texte via la façade I/O.
9. `return 0` termine proprement le programme.
10. `}` ferme le bloc d’entrée.

## Pourquoi

Une façade de liaison évite la dispersion des appels natifs dans tout le code. Elle améliore la lisibilité, simplifie les migrations internes et stabilise les contrats pour les projets.

## Test mental

Question: que se passe-t-il si l’entrée est invalide ?
Réponse attendue: la validation doit être traitée dans la façade ou juste avant l’appel, pas dupliquée dans chaque appelant.

## À faire

1. Remplacer dans un module applicatif un import direct (`std/io/*` ou `std/os/*`) par `std/bridge/*`.
2. Vérifier que le comportement observable ne change pas (mêmes sorties, même code retour).
3. Identifier une règle d’invariant à conserver (exemple: pas d’appel natif direct hors façade).

## Corrigé minimal

- Import centralisé:
  - `use std/bridge/io`
  - `use std/bridge/system`
- Appels inchangés côté métier (`get`, `cwd`, `print_line`).
- Invariant respecté: un seul point d’abstraction pour la liaison native.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs (par cas d'usage)

Cette section s'appuie sur du code concret pour **liaison native avec `std/bridge`**.
Objectif: comprendre vite ce que fait le code, pourquoi, et comment le corriger.

### Exemple 1: extrait réel du chapitre (cas nominal)

```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
  return 0
}
```

Lecture guidée (ligne par ligne):
1. `use std/bridge/io` -> participe au flux nominal du programme.
2. `use std/bridge/system` -> participe au flux nominal du programme.
3. `use std/bridge/time` -> participe au flux nominal du programme.
4. `entry main at app/bridge_demo {` -> définit le point d'entrée exécutable.
5. `let home = get("HOME")` -> fixe une valeur intermédiaire réutilisable.
6. `let here = cwd()` -> fixe une valeur intermédiaire réutilisable.
7. `let t0 = now()` -> fixe une valeur intermédiaire réutilisable.
8. `let _ = print_line("bridge ready")` -> fixe une valeur intermédiaire réutilisable.

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
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
  return 0
}
```

### 2. Ce que fait ce code, ligne par ligne

1. `use std/bridge/io` -> participe au flux nominal.
2. `use std/bridge/system` -> participe au flux nominal.
3. `use std/bridge/time` -> participe au flux nominal.
4. `entry main at app/bridge_demo {` -> définit l'entrée du programme.
5. `let home = get("HOME")` -> stocke une valeur intermédiaire claire.
6. `let here = cwd()` -> stocke une valeur intermédiaire claire.
7. `let t0 = now()` -> stocke une valeur intermédiaire claire.
8. `let _ = print_line("bridge ready")` -> stocke une valeur intermédiaire claire.
9. `return 0` -> retourne le résultat observé.
10. `}` -> participe au flux nominal.

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

### Atelier concret: cas pratique sur 16a-liaison-native.md

Code de base:
```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
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

### Atelier concret: cas pratique sur 16a-liaison-native.md

Code de base:
```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
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

### Atelier concret: cas pratique sur 16a-liaison-native.md

Code de base:
```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
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

### Atelier concret: cas pratique sur 16a-liaison-native.md

Code de base:
```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
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

### Atelier concret: cas pratique sur 16a-liaison-native.md

Code de base:
```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
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

### Atelier concret: cas pratique sur 16a-liaison-native.md

Code de base:
```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
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

### Atelier concret: cas pratique sur 16a-liaison-native.md

Code de base:
```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
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

### Atelier concret: cas pratique sur 16a-liaison-native.md

Code de base:
```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
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

### Atelier concret: cas pratique sur 16a-liaison-native.md

Code de base:
```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
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

### Atelier concret: cas pratique sur 16a-liaison-native.md

Code de base:
```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
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

### Atelier concret: cas pratique sur 16a-liaison-native.md

Code de base:
```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
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
