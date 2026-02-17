# 12. Pointeurs, références et mémoire

Niveau: Intermédiaire

Prérequis: chapitre précédent `docs/book/chapters/11-collections.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/11-collections.md`, `docs/book/chapters/13-generiques.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre poursuit un objectif clair: encadrer le travail mémoire en Vitte pour que chaque accès sensible soit protégé par des préconditions visibles. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 12.1 Confiner une instruction machine dans une frontière courte

```vit
proc cpu_pause() {
  unsafe {
    asm("pause")
  }
}
```

Lecture ligne par ligne (débutant):
1. `proc cpu_pause() {` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `unsafe {` cette ligne marque une zone sensible qui doit rester courte, justifiée et facile à auditer dans un contexte système. Exemple concret: on y place seulement l'opération technique impossible à exprimer en mode sûr.
3. `asm("pause")` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
4. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: confiner le code machine dans une zone `unsafe` très courte.

Ce confinement est crucial: l'audit de sécurité reste local, au lieu d'être diffus dans tout le programme.

À l'exécution, l'appel exécute une pause CPU sans effet métier supplémentaire.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 12.2 Lecture bornée dans un buffer

```vit
form Buffer {
  data: int[]
}
proc read_at(b: Buffer, i: int) -> int {
  if i < 0 { give 0 }
  if i >= b.data.len() { give 0 }
  give b.data[i]
}
```

Lecture ligne par ligne (débutant):
1. `form Buffer {` cette ligne ouvre la structure `Buffer` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `Buffer` sans redéfinir ses champs.
2. `data: int[]` cette ligne déclare le champ `data` avec le type `int[]`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int[]`.
3. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
4. `proc read_at(b: Buffer, i: int) -> int {` ici, le contrat complet est défini pour `read_at`: entrées `b: Buffer, i: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `read_at` retourne toujours une valeur compatible avec `int`.
5. `if i < 0 { give 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `i < 0` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `if i >= b.data.len() { give 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `i >= b.data.len()` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
7. `give b.data[i]` ici, la branche renvoie immédiatement `b.data[i]` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `b.data[i]`.
8. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `i < 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `b.data[i]`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: vérifier les bornes avant tout accès mémoire.

Le contrat est simple: un index invalide retourne `0`, un index valide retourne la donnée réelle.

À l'exécution, avec `data=[10,20,30]`:
- `read_at(...,1)` retourne `20`.
- `read_at(...,9)` retourne `0`.
- `read_at(...,-1)` retourne `0`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 12.3 Écriture bornée avec contrat symétrique

```vit
proc write_at(b: Buffer, i: int, v: int) -> int {
  if i < 0 { give 0 }
  if i >= b.data.len() { give 0 }
  b.data[i] = v
  give 1
}
```

Lecture ligne par ligne (débutant):
1. `proc write_at(b: Buffer, i: int, v: int) -> int {` sur cette ligne, le contrat complet est posé pour `write_at`: entrées `b: Buffer, i: int, v: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `write_at` retourne toujours une valeur compatible avec `int`.
2. `if i < 0 { give 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `i < 0` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if i >= b.data.len() { give 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `i >= b.data.len()` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `b.data[i] = v` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
5. `give 1` sur cette ligne, la sortie est renvoyée immédiatement `1` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `1`.
6. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `i < 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `1`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: appliquer la même politique de bornes en lecture et en écriture.

Cette symétrie réduit les incohérences: ce qui est interdit en lecture l'est aussi en écriture.

À l'exécution:
- `write_at([10,20,30],1,99)` retourne `1` et produit `[10,99,30]`.
- `write_at(...,7,99)` retourne `0` sans mutation.
- `write_at(...,-1,99)` retourne `0` sans mutation.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
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
Réponse attendue: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
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

- `docs/book/keywords/asm.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/field.md`.
- `docs/book/keywords/form.md`.
- `docs/book/keywords/give.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

## Checkpoint synthèse

Mini quiz:
1. Quelle est l'invariant central de ce chapitre ?
2. Quelle garde évite l'état invalide le plus fréquent ?
3. Quel test simple prouve le comportement nominal ?

