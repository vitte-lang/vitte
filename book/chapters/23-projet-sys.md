# 23. Projet guide Système

Niveau: Avancé

Prérequis: chapitre précédent `docs/book/chapters/22-projet-http.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/22-projet-http.md`, `docs/book/chapters/24-projet-kv.md`, `book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


## Niveau local

- Niveau local section coeur: Avancé.
- Niveau local exemples guidés: Intermédiaire.
- Niveau local exercices de diagnostic: Avancé.


Ce chapitre poursuit un objectif clair: construire un noyau système Vitte qui reste auditable sous contrainte réelle. Nous allons dépasser le simple exemple mémoire pour poser une architecture complète: modèle de région, accès bornés, points `unsafe` minimaux, composition d'opérations et plan de tests.

L'enjeu n'est pas de « faire marcher » une instruction machine, mais de conserver des invariants lisibles dans tout le flux. En code système, la lisibilité n'est pas un luxe: c'est une mesure de sécurité.


Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.

Schéma pipeline du chapitre:
- Entrée: données initiales ou requête.
- Traitement: validation, logique métier, transformations.
- Sortie: résultat métier ou code de statut.
- Invariant: propriété qui doit rester vraie à chaque étape.

## 23.1 Définir l'état système et le contrat de résultat

```vit
form Region {
  base: int
  size: int
}
form SysMem {
  region: Region
  cells: int[]
}
pick SysResult {
  case Ok
  case ErrBounds
  case ErrState
}
```

Lecture ligne par ligne (débutant):
1. `form Region {` cette ligne ouvre la structure `Region` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `Region` sans redéfinir ses champs.
2. `base: int` cette ligne déclare le champ `base` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
3. `size: int` cette ligne déclare le champ `size` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
4. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `form SysMem {` cette ligne ouvre la structure `SysMem` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `SysMem` sans redéfinir ses champs.
6. `region: Region` cette ligne déclare le champ `region` avec le type `Region`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `Region`.
7. `cells: int[]` cette ligne déclare le champ `cells` avec le type `int[]`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int[]`.
8. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
9. `pick SysResult {` cette ligne ouvre le type fermé `SysResult` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. Exemple concret: toute valeur hors des `case` déclarés devient impossible à représenter.
10. `case Ok` cette ligne décrit le cas `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok`, ce bloc devient le chemin actif.
11. `case ErrBounds` cette ligne décrit le cas `ErrBounds` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `ErrBounds`, ce bloc devient le chemin actif.
12. `case ErrState` cette ligne décrit le cas `ErrState` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `ErrState`, ce bloc devient le chemin actif.
13. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Ce socle impose deux règles:
- toute opération mémoire travaille dans une `Region`.
- toute issue est explicitée par `SysResult`.

À l'exécution, aucune opération critique ne renvoie un code arbitraire. Tout passe par `Ok` / `ErrBounds` / `ErrState`.

Erreurs fréquentes à éviter:
- laisser des entiers « magiques » circuler comme codes d'erreur.
- mélanger erreur de borne et erreur d'état dans la même branche.
- exposer la mémoire brute sans contrat de région.

## 23.2 Centraliser les préconditions de borne

```vit
proc contains(r: Region, addr: int) -> bool {
  if addr < r.base { give false }
  if addr >= r.base + r.size { give false }
  give true
}
proc to_index(r: Region, addr: int) -> int {
  give addr - r.base
}
```

Lecture ligne par ligne (débutant):
1. `proc contains(r: Region, addr: int) -> bool {` ici, le contrat complet est défini pour `contains`: entrées `r: Region, addr: int` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `contains` retourne toujours une valeur compatible avec `bool`.
2. `if addr < r.base { give false }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `addr < r.base` est vrai, `give false` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if addr >= r.base + r.size { give false }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `addr >= r.base + r.size` est vrai, `give false` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give true` ici, la branche renvoie immédiatement `true` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `true`.
5. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
6. `proc to_index(r: Region, addr: int) -> int {` sur cette ligne, le contrat complet est posé pour `to_index`: entrées `r: Region, addr: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `to_index` retourne toujours une valeur compatible avec `int`.
7. `give addr - r.base` sur cette ligne, la sortie est renvoyée immédiatement `addr - r.base` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `addr - r.base`.
8. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `addr < r.base` est vrai, la sortie devient `false`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `true`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

`contains` devient la porte d'entrée de toutes les lectures/écritures. `to_index` isole la projection adresse -> index local.

Lecture pas à pas:
- adresse hors région => refus immédiat.
- adresse en région => conversion index contrôlée.

Erreurs fréquentes à éviter:
- recalculer les bornes différemment selon les fonctions.
- convertir en index avant de valider la région.
- manipuler une adresse physique comme un index logique.

## 23.3 Implémenter lecture et écriture bornées

```vit
proc read_cell(m: SysMem, addr: int) -> int {
  if not contains(m.region, addr) { give 0 }
  let i: int = to_index(m.region, addr)
  give m.cells[i]
}
proc write_cell(m: SysMem, addr: int, v: int) -> SysResult {
  if not contains(m.region, addr) { give ErrBounds }
  let i: int = to_index(m.region, addr)
  m.cells[i] = v
  give Ok
}
```

Lecture ligne par ligne (débutant):
1. `proc read_cell(m: SysMem, addr: int) -> int {` ce passage fixe le contrat complet de `read_cell`: entrées `m: SysMem, addr: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `read_cell` retourne toujours une valeur compatible avec `int`.
2. `if not contains(m.region, addr) { give 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `not contains(m.region, addr)` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `let i: int = to_index(m.region, addr)` cette ligne crée la variable locale `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `i` reçoit ici le résultat de `to_index(m.region, addr)` et peut être réutilisé ensuite sans recalcul.
4. `give m.cells[i]` ce passage retourne immédiatement `m.cells[i]` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `m.cells[i]`.
5. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
6. `proc write_cell(m: SysMem, addr: int, v: int) -> SysResult {` ici, le contrat complet est défini pour `write_cell`: entrées `m: SysMem, addr: int, v: int` et sortie `SysResult`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `write_cell` retourne toujours une valeur compatible avec `SysResult`.
7. `if not contains(m.region, addr) { give ErrBounds }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `not contains(m.region, addr)` est vrai, `give ErrBounds` est exécuté immédiatement; sinon on continue sur la ligne suivante.
8. `let i: int = to_index(m.region, addr)` cette ligne crée la variable locale `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `i` reçoit ici le résultat de `to_index(m.region, addr)` et peut être réutilisé ensuite sans recalcul.
9. `m.cells[i] = v` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
10. `give Ok` ici, la branche renvoie immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Ok`.
11. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `not contains(m.region, addr)` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `m.cells[i]`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Le contrat est volontairement explicite:
- lecture invalide -> sentinelle `0`.
- écriture invalide -> `ErrBounds`.

À l'exécution:
- `write_cell` ne mute jamais la mémoire hors région.
- `read_cell` ne déréférence jamais hors borne.

Erreurs fréquentes à éviter:
- renvoyer `Ok` même quand l'adresse est invalide.
- muter avant la validation de `contains`.
- confondre sentinelle de lecture et erreur d'écriture.

## 23.4 Encadrer les points `unsafe`

```vit
proc cpu_pause() {
  unsafe { asm("pause") }
}
proc cpu_halt_if(flag: bool) -> SysResult {
  if not flag { give ErrState }
  unsafe { asm("hlt") }
  give Ok
}
```

Lecture ligne par ligne (débutant):
1. `proc cpu_pause() {` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `unsafe { asm("pause") }` cette ligne marque une zone sensible qui doit rester courte, justifiée et facile à auditer dans un contexte système. Exemple concret: on y place seulement l'opération technique impossible à exprimer en mode sûr.
3. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
4. `proc cpu_halt_if(flag: bool) -> SysResult {` sur cette ligne, le contrat complet est posé pour `cpu_halt_if`: entrées `flag: bool` et sortie `SysResult`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `cpu_halt_if` retourne toujours une valeur compatible avec `SysResult`.
5. `if not flag { give ErrState }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `not flag` est vrai, `give ErrState` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `unsafe { asm("hlt") }` cette ligne marque une zone sensible qui doit rester courte, justifiée et facile à auditer dans un contexte système. Exemple concret: on y place seulement l'opération technique impossible à exprimer en mode sûr.
7. `give Ok` sur cette ligne, la sortie est renvoyée immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Ok`.
8. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `not flag` est vrai, la sortie devient `ErrState`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Ok`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Règle d'or système: le `unsafe` doit être court, local, justifié.

À l'exécution:
- `cpu_halt_if(false)` retourne `ErrState`.
- `cpu_halt_if(true)` exécute `hlt`.

Erreurs fréquentes à éviter:
- étendre `unsafe` à du code qui peut rester sûr.
- placer des validations *dans* le bloc `unsafe`.
- masquer la précondition d'état avant `hlt`.

## 23.5 Composer une opération atomique mémoire + synchronisation

```vit
proc write_then_pause(m: SysMem, addr: int, v: int) -> SysResult {
  let w: SysResult = write_cell(m, addr, v)
  match w {
    case Ok {
      cpu_pause()
      give Ok
    }
  case ErrBounds { give ErrBounds }
  otherwise { give ErrState }
}
}
```

Lecture ligne par ligne (débutant):
1. `proc write_then_pause(m: SysMem, addr: int, v: int) -> SysResult {` ce passage fixe le contrat complet de `write_then_pause`: entrées `m: SysMem, addr: int, v: int` et sortie `SysResult`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `write_then_pause` retourne toujours une valeur compatible avec `SysResult`.
2. `let w: SysResult = write_cell(m, addr, v)` cette ligne crée la variable locale `w` de type `SysResult` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `w` reçoit ici le résultat de `write_cell(m, addr, v)` et peut être réutilisé ensuite sans recalcul.
3. `match w {` cette ligne démarre un dispatch déterministe sur `w`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `w`, la même branche sera toujours exécutée.
4. `case Ok {` cette ligne décrit le cas `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok`, ce bloc devient le chemin actif.
5. `cpu_pause()` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
6. `give Ok` ce passage retourne immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Ok`.
7. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
8. `case ErrBounds { give ErrBounds }` cette ligne décrit le cas `ErrBounds` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `ErrBounds`, ce bloc devient le chemin actif.
9. `otherwise { give ErrState }` cette ligne définit le chemin de secours pour couvrir les situations non capturées par les cas explicites. Exemple concret: si aucun `case` ne correspond, `give ErrState` est exécuté pour garantir une sortie stable.
10. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
11. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Ok`.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Cette composition conserve un flux clair:
- valider et écrire.
- synchroniser si succès.
- propager l'erreur sinon.

À l'exécution:
- succès: `write_cell` puis `cpu_pause`.
- échec de borne: sortie immédiate `ErrBounds`.

Erreurs fréquentes à éviter:
- synchroniser même après échec d'écriture.
- convertir une erreur de borne en erreur d'état générique.
- dupliquer ce flux dans plusieurs fonctions au lieu d'unifier.

## 23.6 Ajouter une initialisation sûre du contexte

```vit
proc init_mem(base: int, size: int) -> SysMem {
  let cells: int[] = []
  let i: int = 0
  loop {
    if i >= size { break }
    cells.push(0)
    set i = i + 1
  }
give SysMem(Region(base, size), cells)
}
```

Lecture ligne par ligne (débutant):
1. `proc init_mem(base: int, size: int) -> SysMem {` ici, le contrat complet est défini pour `init_mem`: entrées `base: int, size: int` et sortie `SysMem`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `init_mem` retourne toujours une valeur compatible avec `SysMem`.
2. `let cells: int[] = []` cette ligne crée la variable locale `cells` de type `int[]` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `cells` reçoit ici le résultat de `[]` et peut être réutilisé ensuite sans recalcul.
3. `let i: int = 0` cette ligne crée la variable locale `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `i` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
4. `loop {` cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`). Exemple concret: à chaque tour, les gardes internes décident de continuer ou de sortir proprement.
5. `if i >= size { break }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `i >= size` est vrai, `break` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `cells.push(0)` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
7. `set i = i + 1` cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux. Exemple concret: après exécution, `i` prend la nouvelle valeur `i + 1` pour les étapes suivantes.
8. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
9. `give SysMem(Region(base, size), cells)` ici, la branche renvoie immédiatement `SysMem(Region(base, size), cells)` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `SysMem(Region(base, size), cells)`.
10. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `SysMem(Region(base, size), cells)`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Une initialisation explicite évite les états implicites et facilite les tests reproductibles.

Erreurs fréquentes à éviter:
- supposer que la mémoire est initialisée ailleurs.
- créer une région sans allouer `cells` cohérentes.
- mélanger bootstrap et logique métier.

## 23.7 Scénario complet reproductible

```vit
entry main at sys/core {
  let mem: SysMem = init_mem(100, 8)
  let r1: SysResult = write_then_pause(mem, 103, 42)
  let v1: int = read_cell(mem, 103)
  if v1 == 42 { return 0 }
  return 70
}
```

Lecture ligne par ligne (débutant):
1. `entry main at sys/core {` cette ligne fixe le point d'entrée `main` dans `sys/core` et sert de scénario exécutable de bout en bout pour le chapitre. Exemple concret: lancer cette entrée permet de vérifier la chaîne complète des fonctions appelées.
2. `let mem: SysMem = init_mem(100, 8)` cette ligne crée la variable locale `mem` de type `SysMem` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `mem` reçoit ici le résultat de `init_mem(100, 8)` et peut être réutilisé ensuite sans recalcul.
3. `let r1: SysResult = write_then_pause(mem, 103, 42)` cette ligne crée la variable locale `r1` de type `SysResult` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `r1` reçoit ici le résultat de `write_then_pause(mem, 103, 42)` et peut être réutilisé ensuite sans recalcul.
4. `let v1: int = read_cell(mem, 103)` cette ligne crée la variable locale `v1` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `v1` reçoit ici le résultat de `read_cell(mem, 103)` et peut être réutilisé ensuite sans recalcul.
5. `if v1 == 42 { return 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `v1 == 42` est vrai, `return 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `return 70` cette ligne termine l'exécution du bloc courant avec le code `70`, utile pour observer le résultat global du scénario. Exemple concret: un test d'exécution peut vérifier directement que le programme retourne `70`.
7. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return 70`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `70`.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Ce scénario fournit un test end-to-end minimal:
- init.
- write sécurisée.
- read bornée.
- assertion de sortie.

Erreurs fréquentes à éviter:
- valider le flux sans relire la valeur réellement écrite.
- testér seulement `Ok` sans cas `ErrBounds`.
- lier le succès à un effet externe non déterministe.

## 23.8 Plan de tests projet

Jeu minimal à couvrir:
1. Borne basse: adresse `base - 1` => rejet.
2. Borne haute: adresse `base + size` => rejet.
3. Cas nominal: adresse interne => write/read cohérente.
4. `cpu_halt_if(false)` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
5. Reproductibilité: même séquence => même état final.

Erreurs fréquentes à éviter:
- ignorer les deux frontières de région.
- testér `unsafe` uniquement en succès.
- ne pas vérifier l'absence de mutation en cas d'erreur.

## 23.9 Contrat système minimal

Entrée CLI minimale:
- `--op` (`read` ou `write`).
- `--addr` adresse logique.
- `--value` obligatoire si `--op=write`.
- `--trace-id` optionnel pour corrélation des logs.

Droits requis:
- lecture seule pour `read`.
- écriture minimale sur la région ciblée pour `write`.
- aucun privilège global si la région peut être isolée.

Sorties et codes erreur:
- `0`: succès.
- `2`: entrée invalide.
- `13`: permission refusée.
- `44`: ressource introuvable.
- `70`: état interne incohérent.

## 23.10 Invariants sécurité

Invariants non négociables:
- tout chemin d'entrée est canonicalisé avant usage (`path canonicalisé`).
- aucune exécution shell dynamique à partir de l'entrée (`no shell injection`).
- application du principe du moindre privilège (`droits minimaux`).
- toutes les bornes mémoire sont validées avant conversion d'index.

## 23.11 Scénario traçable complet

Pipeline concret:
1. Input brut: `--op=write --addr=103 --value=42 --trace-id=t-001`.
2. Validation: vérifier présence des champs, plage d'adresse et droits.
3. Syscall/mutation: écrire la cellule ciblée, puis synchroniser.
4. Mapping résultat: `Ok -> code 0`, `ErrBounds -> code 2`, `ErrState -> code 70`.

Sortie attendue:
- code retour déterministe.
- log corrélé par `trace-id`.
- aucune mutation si validation échoue.

## 23.12 Tests d'intégration système

1. Nominal.
- Entrée: `write` valide dans la région autorisée.
- Attendu: code `0`, valeur lisible immédiatement, log de succès.

2. Permission denied.
- Entrée: `write` sans droit suffisant.
- Attendu: code `13`, aucune mutation mémoire, log `denied`.

3. Resource not found.
- Entrée: cible absente ou région non résolue.
- Attendu: code `44`, état inchangé, diagnostic explicite.

## 23.13 Encadré Portabilité

Ce qui change selon OS:
- modèle de permissions et API de vérification de droits.
- forme des appels natifs pour pause/synchronisation.
- conventions de code retour système et messages d'erreur.

Ce qui reste stable:
- contrat d'entrée CLI.
- invariants de borne et d'atomicité.
- mapping métier `SysResult -> code de sortie`.

## 23.14 Encadré Observabilité

Exigences minimales:
- logs structurés (`ts`, `level`, `trace_id`, `op`, `addr`, `result`).
- code retour toujours cohérent avec le résultat métier.
- `trace_id` propagé de l'entrée à la sortie.

Format de log recommandé:
- `ts=<unix_ms> trace_id=<id> op=<read|write> addr=<n> result=<Ok|Err...> code=<n>`.

## 23.15 Coûts

I/O:
- coût dominant quand l'opération touche disque, périphérique ou IPC.

Mémoire:
- coût local faible sur le chemin nominal, mais sensible aux copies inutiles.

Appels système bloquants:
- risque de latence et d'effet domino en chaîne.
- imposer timeout explicite et stratégie de reprise.

## 23.16 Erreurs classiques projet système

- TOCTOU: vérifier puis utiliser une ressource sans verrou cohérent.
- Path traversal: accepter un chemin non canonicalisé.
- Timeouts oubliés: laisser un appel bloquer indéfiniment.
- Permissions implicites: supposer un droit non vérifié.
- Mapping tardif: convertir trop tard en codes de sortie observables.

## Table erreur -> diagnostic -> correction

| Erreur OS | Symptôme | Diagnostic | Correction |
| --- | --- | --- | --- |
| `EACCES` / permission denied | écriture refusée. | Droits insuffisants sur la cible. | Abaisser le périmètre ou demander explicitement le droit minimal requis. |
| `ENOENT` / resource not found | ressource introuvable. | Chemin/région inexistant(e) ou résolu(e) trop tard. | Valider existence et résolution avant mutation. |
| `EINVAL` / invalid argument | refus immédiat de l'opération. | Entrée CLI incomplète ou hors contrat. | Rejeter tôt avec message précis et code `2`. |
| `ETIMEDOUT` / timeout | appel bloqué puis échec. | Timeout absent ou trop grand. | Définir timeout borné et chemin de reprise déterministe. |
| `EIO` / I/O error | échec intermittent d'accès. | Couche I/O instable ou périphérique dégradé. | Remonter diagnostic, préserver atomicité, éviter mutation partielle. |

## Micro challenge final

Ajoutez un mode `dry-run` sans casser les invariants.

Contraintes:
- `dry-run` valide et journalise, mais ne mute jamais l'état.
- le mapping de sortie reste identique au mode réel pour les erreurs de validation.
- les tests nominal/permission/not-found doivent passer dans les deux modes.

## À retenir

Un noyau système robuste se construit par couches lisibles: préconditions centralisées, accès bornés, `unsafe` minimal, composition déterministe. Plus le code est bas niveau, plus le contrat doit être hautement explicite.

Critère pratique de qualité pour ce chapitre:
- vous pouvez prouver qu'aucune écriture hors borne n'est possible.
- vous pouvez pointer chaque zone `unsafe` en quelques secondes.
- vous savez rejouer un scénario système complet de façon déterministe.


## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## ABI/runtime checklist (commun)

- signatures ABI cohérentes entre frontières Vitte et natif.
- symboles runtime effectivement exportés et linkés.
- headers/runtime disponibles sur la cible.
- contraintes freestanding explicitées pour les cibles kernel.
- validation d'appel natif avant optimisation.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/keywords/asm.md`.
- `docs/book/keywords/at.md`.
- `docs/book/keywords/bool.md`.
- `docs/book/keywords/break.md`.
- `docs/book/keywords/case.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.
