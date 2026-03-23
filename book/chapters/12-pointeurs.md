# 12. Pointeurs, références et mémoire

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/11-collections.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/11-collections.md`, `book/chapters/13-generiques.md`, `book/glossaire.md`.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Pointeurs, références et mémoire**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Pointeurs, références et mémoire**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Pointeurs, références et mémoire**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Pointeurs, références et mémoire** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Pointeurs, références et mémoire**.

## Exercice court

Prenez un exemple du chapitre sur **Pointeurs, références et mémoire**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Pointeurs, références et mémoire**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 12.1 Confiner une instruction machine dans une frontière courte

```vit
proc cpu_pause() {
  unsafe {
    asm("pause")
  }
}
```

Lecture simple du code:
1. `proc cpu_pause() {` : cette ligne définit une étape explicite du flux.
2. `unsafe {` : cette ligne marque une zone sensible qui doit rester courte, justifiée et facile à auditer dans un contexte système.
3. `asm("pause")` : cette ligne définit une étape explicite du flux.
4. `}` : cette accolade ferme le bloc logique.
5. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: confiner le code machine dans une zone `unsafe` très courte.

Ce confinement est crucial: l'audit de sécurité reste local, au lieu d'être diffus dans tout le programme.

À l'exécution, l'appel exécute une pause CPU sans effet métier supplémentaire.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 12.2 Lecture bornée dans un buffer

```vit
form Buffer {
  data: int[]
}
proc read_at(b: Buffer, i: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if i < 0 { give 0 }
  // Garde: bloque un cas invalide avant de continuer
  if i >= b.data.len() { give 0 }
  // Sortie locale: valeur retournee par la procedure
  give b.data[i]
}
```

Lecture simple du code:
1. `form Buffer {` : cette ligne ouvre la structure `Buffer` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable.
2. `data: int[]` : cette ligne déclare le champ `data` avec le type `int[]`, ce qui documente son rôle et limite les erreurs de manipulation.
3. `}` : cette accolade clôt le bloc logique.
4. `proc read_at(b: Buffer, i: int) -> int {` : le contrat est défini pour `read_at`: entrées `b: Buffer, i: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
5. `if i < 0 { give 0 }` : cette garde traite le cas limite avant le calcul.
6. `if i >= b.data.len() { give 0 }` : cette garde traite le cas limite avant le calcul.
7. `give b.data[i]` : la branche renvoie immédiatement `b.data[i]` pour la branche courante, la sortie de branche est explicite et vérifiable.
8. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: si `i < 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `b.data[i]`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: vérifier les bornes avant tout accès mémoire.

Le contrat est simple: un index invalide retourne `0`, un index valide retourne la donnée réelle.

À l'exécution, avec `data=[10,20,30]`:
- `read_at(..,1)` retourne `20`.
- `read_at(..,9)` retourne `0`.
- `read_at(..,-1)` retourne `0`.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 12.3 Écriture bornée avec contrat symétrique

```vit
proc write_at(b: Buffer, i: int, v: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if i < 0 { give 0 }
  // Garde: bloque un cas invalide avant de continuer
  if i >= b.data.len() { give 0 }
  b.data[i] = v
  // Sortie locale: valeur retournee par la procedure
  give 1
}
```

Lecture simple du code:
1. `proc write_at(b: Buffer, i: int, v: int) -> int {` : le contrat est posé pour `write_at`: entrées `b: Buffer, i: int, v: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if i < 0 { give 0 }` : cette garde traite le cas limite avant le calcul.
3. `if i >= b.data.len() { give 0 }` : cette garde traite le cas limite avant le calcul.
4. `b.data[i] = v` : cette ligne définit une étape explicite du flux.
5. `give 1` : la sortie est renvoyée immédiatement `1` pour la branche courante, la sortie de branche est explicite et vérifiable.
6. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: si `i < 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `1`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: appliquer la même politique de bornes en lecture et en écriture.

Cette symétrie réduit les incohérences: ce qui est interdit en lecture l'est aussi en écriture.

À l'exécution:
- `write_at([10,20,30],1,99)` retourne `1` et produit `[10,99,30]`.
- `write_at(..,7,99)` retourne `0` sans mutation.
- `write_at(..,-1,99)` retourne `0` sans mutation.

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Toute opération mémoire est bornée, toute zone `unsafe` est courte et chaque contrat de retour est stable. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez montrer où commence et où finit la zone `unsafe`.
- vous pouvez prouver qu'un accès hors borne n'écrit jamais en mémoire.
- vous pouvez expliquer le contrat de retour pour chaque cas invalide.

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

- `book/keywords/asm.md`.
- `book/keywords/continue.md`.
- `book/keywords/field.md`.
- `book/keywords/form.md`.
- `book/keywords/give.md`.

## Checkpoint synthèse

Mini quiz:
1. Quelle est l'invariant central de ce chapitre ?
2. Quelle garde évite l'état invalide le plus fréquent ?
3. Quel test simple prouve le comportement nominal ?



## Exemple Étendu

Exemple approfondi pour **pointeurs**: pipeline validation -> transformation -> décision -> projection.

```vit
// Exemple long: flux complet et vérifiable
space demo/pointeurs

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

Scénarios recommandés (pointeurs):
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement déterministe.
- Cas invalide id<=0 -> sortie 21.
