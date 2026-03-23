# 7. Instructions de contrôle

Niveau: Débutant

Prérequis: chapitre précédent `book/chapters/06-procedures.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/06-procedures.md`, `book/chapters/08-structures.md`, `book/glossaire.md`.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Instructions de contrôle**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Instructions de contrôle**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Instructions de contrôle**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Instructions de contrôle** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Instructions de contrôle**.

## Exercice court

Prenez un exemple du chapitre sur **Instructions de contrôle**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Instructions de contrôle**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 7.1 Garde de classification

```vit
proc classify(temp: int) -> int {
  if temp < 0 { give -1 }
  if temp > 100 { give 2 }
  give 1
}
```

Lecture simple du code:
1. `proc classify(temp: int) -> int {` : le contrat est défini pour `classify`: entrées `temp: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if temp < 0 { give -1 }` : cette garde traite le cas limite avant le calcul.
3. `if temp > 100 { give 2 }` : cette garde traite le cas limite avant le calcul.
4. `give 1` : la branche renvoie immédiatement `1` pour la branche courante, la sortie de branche est explicite et vérifiable.
5. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: si `temp < 0` est vrai, la sortie devient `-1`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `1`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: classer une valeur en traitant d'abord les cas hors plage, puis le cas nominal.

Ce schéma rend le contrôle facile à relire: chaque garde correspond à une règle métier visible, et la dernière ligne couvre le cas courant.

Lecture pas à pas:
- `classify(-2)` retourne `-1` (valeur sous la plage).
- `classify(50)` retourne `1` (valeur nominale).
- `classify(120)` retourne `2` (valeur au-dessus de la plage).

Erreurs classiques à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 7.2 Boucle somme

```vit
proc sum_to(limit: int) -> int {
  let i: int = 0
  let acc: int = 0
  loop {
    if i > limit { break }
    set acc = acc + i
    set i = i + 1
  }
give acc
}
```

Lecture simple du code:
1. `proc sum_to(limit: int) -> int {` : le contrat est posé pour `sum_to`: entrées `limit: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `let i: int = 0` : cette ligne crée la variable `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `let acc: int = 0` : cette ligne crée la variable `acc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
4. `loop {` : cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`).
5. `if i > limit { break }` : cette garde traite le cas limite avant le calcul.
6. `set acc = acc + i` : cette ligne réalise une mutation volontaire et visible: l'état `acc` change ici, à cet endroit précis du flux.
7. `set i = i + 1` : cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux.
8. `}` : cette accolade ferme le bloc logique.
9. `give acc` : la sortie est renvoyée immédiatement `acc` pour la branche courante, la sortie de branche est explicite et vérifiable.
10. `}` : cette accolade clôt le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: construire une boucle dont l'invariant est verbalizable, ici `acc` contient la somme des valeurs déjà parcourues.

Ordre de lecture recommandé:
- vérifier la borne d'arrêt (`i > limit`).
- suivre la mise à jour de l'accumulateur.
- suivre l'incrément de l'index.

À l'exécution, pour `sum_to(3)`:
- départ `i=0`, `acc=0`.
- après `i=0`: `acc=0`.
- après `i=1`: `acc=1`.
- après `i=2`: `acc=3`.
- après `i=3`: `acc=6`.
- `i=4` déclenche l'arrêt, retour `6`.

Erreurs classiques à éviter:
- laisser une boucle sans borne claire ou sans condition d'arrêt vérifiable.
- faire évoluer plusieurs variables d'état sans documenter leur rôle.
- optimiser trop tôt sans verrouiller d'abord le comportement attendu.

## 7.3 Filtrage en parcours

```vit
proc filtered_sum(values: int[]) -> int {
  let acc: int = 0
  for x in values {
    if x < 0 { continue }
    set acc = acc + x
  }
give acc
}
```

Lecture simple du code:
1. `proc filtered_sum(values: int[]) -> int {` : le contrat est fixé pour `filtered_sum`: entrées `values: int[]` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `let acc: int = 0` : cette ligne crée la variable `acc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `for x in values {` : cette ligne définit une étape explicite du flux.
4. `if x < 0 { continue }` : cette garde traite le cas limite avant le calcul.
5. `set acc = acc + x` : cette ligne réalise une mutation volontaire et visible: l'état `acc` change ici, à cet endroit précis du flux.
6. `}` : cette accolade ferme le bloc logique.
7. `give acc` : retourne immédiatement `acc` pour la branche courante, la sortie de branche est explicite et vérifiable.
8. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: filtrer en parcours sans casser la lisibilité de la boucle.

La règle est simple: les valeurs négatives sont ignorées (`continue`), les autres sont accumulées.

À l'exécution, sur `[5,-2,7]`:
- `5` est ajoutée, `acc=5`.
- `-2` est ignorée.
- `7` est ajoutée, `acc=12`.
- retour final `12`.

Erreurs classiques à éviter:
- laisser une boucle sans borne claire ou sans condition d'arrêt vérifiable.
- faire évoluer plusieurs variables d'état sans documenter leur rôle.
- optimiser trop tôt sans verrouiller d'abord le comportement attendu.

## À retenir

Sortie de boucle explicite, invariant verbalisable, effets localisés. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez expliquer pourquoi une branche est prise.
- vous savez suivre une boucle tour par tour sans ambiguïté.
- vous savez justifier le résultat final à partir de l'invariant.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Repère: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
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

- `book/keywords/break.md`.
- `book/keywords/continue.md`.
- `book/keywords/for.md`.
- `book/keywords/give.md`.
- `book/keywords/if.md`.
