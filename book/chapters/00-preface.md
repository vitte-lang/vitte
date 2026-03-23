# 0. Préface

Niveau: Débutant

Prérequis: aucun prérequis technique; lire `book/glossaire.md` pour le vocabulaire de base.
Voir aussi: `book/chapters/01-demarrer.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Préface**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Préface**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Préface**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Préface** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Préface**.

## Exercice court

Prenez un exemple du chapitre sur **Préface**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Préface**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 0.1 Comprendre la promesse du langage

```vit
proc promise(a: int, b: int) -> int {
  // Sortie locale: valeur retournee par la procedure
  give a + b
}
```

Lecture ligne par ligne (débutant):
1. `proc promise(a: int, b: int) -> int {` : le contrat est défini pour `promise`: entrées `a: int, b: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `give a + b` : la branche renvoie immédiatement `a + b` pour la branche courante, la sortie de branche est explicite et vérifiable.
3. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `a + b`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Vitte privilégie la clarté du contrat sur la magie syntaxique. Le lecteur doit toujours voir ce qui entre, ce qui sort et pourquoi.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `promise(20,22)=42`.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 0.2 Accepter la discipline des invariants

```vit
proc bounded(x: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if x < 0 { give 0 }
  // Garde: bloque un cas invalide avant de continuer
  if x > 100 { give 100 }
  // Sortie locale: valeur retournee par la procedure
  give x
}
```

Lecture ligne par ligne (débutant):
1. `proc bounded(x: int) -> int {` : le contrat est posé pour `bounded`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if x < 0 { give 0 }` : cette garde traite le cas limite avant le calcul.
3. `if x > 100 { give 100 }` : cette garde traite le cas limite avant le calcul.
4. `give x` : la sortie est renvoyée immédiatement `x` pour la branche courante, la sortie de branche est explicite et vérifiable.
5. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: si `x < 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Un invariant est une règle locale qui evite des classes entieres de bugs. Le livre insiste sur cette pratique dans tous les chapitres.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `bounded(-1)=0`, `bounded(70)=70`, `bounded(140)=100`.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 0.3 Construire une progression orientée projet

```vit
// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let v: int = bounded(140)
  // Sortie programme: code de retour observable
  return v
}
```

Lecture ligne par ligne (débutant):
1. `entry main at core/app {` : cette ligne fixe le point d'entrée `main` dans `core/app` et sert de scénario exécutable de bout en bout pour le chapitre.
2. `let v: int = bounded(140)` : cette ligne crée la variable `v` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `return v` : cette ligne termine l'exécution du bloc courant avec le code `v`, utile pour observer le résultat global du scénario.
4. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return v`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `v`.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: La théorie est toujours raccordee à un flux d'exécution concret. Chaque notion abstraite est ramenee à un comportement observable.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `main` calcule `bounded(140)` puis termine avec `100`.

Erreurs fréquentes à éviter:
- mélanger orchestration et logique métier dans la même fonction.
- dupliquer une règle transversale au lieu de la centraliser.
- faire évoluer l'architecture sans clarifier les responsabilités.

## À retenir

Le ton du livre est technique, le cheminement est progressif et chaque chapitre sert la construction d'un vrai code Vitte. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin. L'objectif final est de rendre chaque décision de code explicable à la première lecture, comme dans un texte de référence.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/at.md`.
- `book/keywords/bool.md`.
- `book/keywords/continue.md`.
- `book/keywords/entry.md`.
- `book/keywords/give.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **préface**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc promise(a: int, b: int) -> int {
  // Sortie locale: valeur retournee par la procedure
  give a + b
}
```

Lecture ligne par ligne:
1. `proc promise(a: int, b: int) -> int {` -> pose un contrat clair de fonction.
2. `give a + b` -> renvoie la sortie vérifiable.
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

Exemple approfondi pour **preface**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/preface

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

Scénarios recommandés (preface):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
