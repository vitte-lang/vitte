# 27. Grammaire du langage

Niveau: Avancé

Prérequis: chapitre précédent `docs/book/chapters/26-projet-editor.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/26-projet-editor.md`, `docs/book/chapters/28-conventions.md`, `book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre poursuit un objectif clair: lire la grammaire Vitte comme un guide de construction de phrases valides pour accélérer diagnostic et écriture. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
Source normative: `src/vitte/grammar/vitte.ebnf` (grammaire de surface). Copie doc: `docs/book/grammar-surface.ebnf`.
## 27.1 Construire une déclaration de procédure valide

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Lecture ligne par ligne (débutant):
1. `proc add(a: int, b: int) -> int {` ici, le contrat complet est défini pour `add`: entrées `a: int, b: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `add` retourne toujours une valeur compatible avec `int`.
2. `give a + b` ici, la branche renvoie immédiatement `a + b` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `a + b`.
3. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `a + b`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: valider la forme canonique d'une déclaration de procédure.

Lecture grammaticale:
- en-tête `proc` avec nom et paramètres typés.
- type de retour explicite `-> int`.
- bloc délimité contenant une instruction de retour.

À l'exécution, `add(1,2)=3`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 27.2 Construire une entrée programme explicite

```vit
entry main at core/app {
  return 0
}
```

Lecture ligne par ligne (débutant):
1. `entry main at core/app {` cette ligne fixe le point d'entrée `main` dans `core/app` et sert de scénario exécutable de bout en bout pour le chapitre. Exemple concret: lancer cette entrée permet de vérifier la chaîne complète des fonctions appelées.
2. `return 0` cette ligne termine l'exécution du bloc courant avec le code `0`, utile pour observer le résultat global du scénario. Exemple concret: un test d'exécution peut vérifier directement que le programme retourne `0`.
3. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return 0`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `0`.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: rendre explicite le point d'entrée du programme.

La forme `entry ... at ...` fixe à la fois le nom logique et le module de rattachement.

À l'exécution, le programme quitte immédiatement avec le code `0`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 27.3 Construire un `match` avec repli (patterns)

```vit
pick Resp { case Ok, case Err }
proc to_code(r: Resp) -> int {
  match r {
    case Ok { give 0 }
    otherwise { give 1 }
  }
}
```

Lecture ligne par ligne (débutant):
1. `pick Resp { case Ok, case Err }` cette ligne définit un type fermé pour disposer de patterns valides dans `match`.
2. `proc to_code(r: Resp) -> int {` cette ligne fixe un contrat simple: entrée variante, sortie code.
3. `match r {` cette ligne démarre le dispatch par pattern.
4. `case Ok { give 0 }` cette ligne couvre explicitement le pattern `Ok`.
5. `otherwise { give 1 }` cette ligne couvre le repli.
5. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
6. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: construire un `match` lisible et exhaustif, conforme aux patterns de la grammaire.

Ici, `otherwise` ferme explicitement l'arbre de décision, ce qui évite les trous de contrôle de flux.

À l'exécution, `to_code(Ok)=0` et `to_code(Err)=1`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Les formes syntaxiques critiques sont maîtrisées, ce qui réduit les erreurs de structure avant type-check. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez reconnaître immédiatement une forme syntaxique invalide.
- vous savez distinguer erreur grammaticale et erreur de type.
- vous pouvez relire une fonction comme une phrase de grammaire complète.


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

- `docs/book/keywords/at.md`.
- `docs/book/keywords/case.md`.
- `docs/book/keywords/entry.md`.
- `docs/book/keywords/give.md`.
- `docs/book/keywords/int.md`.


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
