# 18. Tests et validation

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/17-stdlib.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/17-stdlib.md`, `book/chapters/19-performance.md`, `book/glossaire.md`.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Tests et validation**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Tests et validation**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Tests et validation**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Tests et validation** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Tests et validation**.

## Exercice court

Prenez un exemple du chapitre sur **Tests et validation**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Tests et validation**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 18.1 Tester une fonction de saturation

```vit
proc clamp(x: int, lo: int, hi: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < lo { give lo }
  // Garde: bloque un cas invalide avant de continuer
  if x > hi { give hi }
  // Sortie locale: valeur retournee par la procedure
  give x
}
```

Lecture simple du code:
1. `proc clamp(x: int, lo: int, hi: int) -> int {` : le contrat est défini pour `clamp`: entrées `x: int, lo: int, hi: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if x < lo { give lo }` : cette garde traite le cas limite avant le calcul.
3. `if x > hi { give hi }` : cette garde traite le cas limite avant le calcul.
4. `give x` : la branche renvoie immédiatement `x` pour la branche courante, la sortie de branche est explicite et vérifiable.
5. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: si `x < lo` est vrai, la sortie devient `lo`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: couvrir systématiquement les trois classes de cas d'un contrat borné.

Cette grille simple évite les tests "au hasard": on testé sous la borne, dans l'intervalle, au-dessus de la borne.

À l'exécution:
- `clamp(-1,0,10)` retourne `0`.
- `clamp(5,0,10)` retourne `5`.
- `clamp(99,0,10)` retourne `10`.

Erreurs classiques à éviter:
- testér uniquement le cas nominal et ignorer les frontières.
- confondre le symptôme observé et la cause réelle.
- traiter les erreurs dans tous les sens au lieu de centraliser la politique.

## 18.2 Verrouiller les frontières d'un parseur

```vit
proc parse_port(x: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give -1 }
  // Garde: bloque un cas invalide avant de continuer
  if x > 65535 { give -1 }
  // Sortie locale: valeur retournee par la procedure
  give x
}
```

Lecture simple du code:
1. `proc parse_port(x: int) -> int {` : le contrat est posé pour `parse_port`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if x < 0 { give -1 }` : cette garde traite le cas limite avant le calcul.
3. `if x > 65535 { give -1 }` : cette garde traite le cas limite avant le calcul.
4. `give x` : la sortie est renvoyée immédiatement `x` pour la branche courante, la sortie de branche est explicite et vérifiable.
5. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: si `x < 0` est vrai, la sortie devient `-1`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: verrouiller les frontières du domaine de validité.

Le choix des valeurs de test est volontaire:
- juste en dessous de la borne basse.
- exactement sur les bornes.
- juste au-dessus de la borne haute.

À l'exécution:
- `parse_port(0)` retourne `0`.
- `parse_port(65535)` retourne `65535`.
- `parse_port(65536)` retourne `-1`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 18.3 Capturer une regression sur division

```vit
proc non_reg_demo(x: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x == 0 { give 0 }
  // Sortie locale: valeur retournee par la procedure
  give 10 / x
}
```

Lecture simple du code:
1. `proc non_reg_demo(x: int) -> int {` : le contrat est fixé pour `non_reg_demo`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if x == 0 { give 0 }` : cette garde traite le cas limite avant le calcul.
3. `give 10 / x` : retourne immédiatement `10 / x` pour la branche courante, la sortie de branche est explicite et vérifiable.
4. `}` : cette accolade clôt le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: si `x == 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `10 / x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: capturer explicitement un bug déjà rencontré pour empêcher son retour.

Ce test sert de mémoire technique: il documente un risque réel et verrouille la correction.

À l'exécution:
- `non_reg_demo(0)` retourne `0`.
- `non_reg_demo(2)` retourne `5`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Vos tests couvrent nominal, bornes et historiques de bugs avec des attentes explicites. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez justifier chaque cas de test par une règle du contrat.
- vous savez distinguer test de frontière et test de non-régression.
- vous pouvez identifier rapidement quel invariant échoue quand un test casse.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Repère: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## Mini défi transverse

Défi: combinez au moins deux notions des trois derniers chapitres dans une fonction courte (garde + transformation + sortie).
Vérification minimale: montrez un cas nominal et un cas invalide, puis expliquez quelle branche est prise.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/const.md`.
- `book/keywords/continue.md`.
- `book/keywords/give.md`.
- `book/keywords/if.md`.
- `book/keywords/int.md`.

## Checkpoint synthèse

Mini quiz:
1. Quelle est l'invariant central de ce chapitre ?
2. Quelle garde évite l'état invalide le plus fréquent ?
3. Quel test simple prouve le comportement nominal ?



## Exemple Étendu

Exemple approfondi pour **tests**: harnais complet (cas unitaires, agrégation, projection CI).

```vit
// Exemple long: flux complet et vérifiable
space demo/tests

form TestCase { id: int input: int expected: int }
pick CaseResult { case Pass(id: int) case Fail(id: int, got: int, expected: int) }

proc subject(x: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give 0 }
  // Garde: bloque un cas invalide avant de continuer
  if x > 100 { give 100 }
  // Sortie locale: valeur retournee par la procedure
  give x
}

// Exécute un cas de test et retourne un résultat typé
proc run(c: TestCase) -> CaseResult {
  let got: int = subject(c.input)
  // Garde: bloque un cas invalide avant de continuer
  if got == c.expected { give CaseResult.Pass(c.id) }
  // Sortie locale: valeur retournee par la procedure
  give CaseResult.Fail(c.id, got, c.expected)
}

// Agrège les résultats et projette un code CI
proc ci_exit(a: CaseResult, b: CaseResult, c: CaseResult) -> int {
  let ok: int = 0
  // Match: decision explicite selon l'etat
  match a { case Pass(_) { set ok = ok + 1 } otherwise { } }
  // Match: decision explicite selon l'etat
  match b { case Pass(_) { set ok = ok + 1 } otherwise { } }
  // Match: decision explicite selon l'etat
  match c { case Pass(_) { set ok = ok + 1 } otherwise { } }
  // Garde: bloque un cas invalide avant de continuer
  if ok == 3 { give 0 }
  // Garde: bloque un cas invalide avant de continuer
  if ok == 2 { give 11 }
  // Garde: bloque un cas invalide avant de continuer
  if ok == 1 { give 12 }
  // Sortie locale: valeur retournee par la procedure
  give 13
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let c1: TestCase = TestCase(1, -3, 0)
  let c2: TestCase = TestCase(2, 42, 42)
  let c3: TestCase = TestCase(3, 160, 100)
  let r1: CaseResult = run(c1)
  let r2: CaseResult = run(c2)
  let r3: CaseResult = run(c3)
  // Sortie programme: code de retour observable
  return ci_exit(r1, r2, r3)
}
```

Scénarios recommandés (tests):
- 3 sur 3 réussis -> sortie 0.
- Régression partielle -> sortie 11.
- Échec global -> sortie 13.
