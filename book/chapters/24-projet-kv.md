# 24. Projet guide KV store

Niveau: Avancé

Prérequis: chapitre précédent `book/chapters/23-projet-sys.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/23-projet-sys.md`, `docs/book/chapters/25-projet-arduino.md`, `book/glossaire.md`.

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


Ce chapitre poursuit un objectif clair: construire un KV store Vitte exploitable, avec un protocole d'opérations clair, une validation de clé unique et un flux d'erreurs déterministe. Nous allons passer d'un simple garde-fou de validation à un mini moteur clé-valeur complet (put/get/delete) avec backend mémoire.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.

Schéma pipeline du chapitre:
- Entrée: données initiales ou requête.
- Traitement: validation, logique métier, transformations.
- Sortie: résultat métier ou code de statut.
- Invariant: propriété qui doit rester vraie à chaque étape.

## 24.1 Poser les types de base du domaine KV

```vit
form Entry {
  key: string
  value: string
}
pick KvResult {
  case Ok
  case ErrKey
  case ErrState
}
pick KvValue {
  case Some(value: string)
  case None
}
```

Lecture ligne par ligne (débutant):
1. `form Entry {` cette ligne ouvre la structure `Entry` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `Entry` sans redéfinir ses champs.
2. `key: string` cette ligne déclare le champ `key` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `string`.
3. `value: string` cette ligne déclare le champ `value` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `string`.
4. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `pick KvResult {` cette ligne ouvre le type fermé `KvResult` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. Exemple concret: toute valeur hors des `case` déclarés devient impossible à représenter.
6. `case Ok` cette ligne décrit le cas `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok`, ce bloc devient le chemin actif.
7. `case ErrKey` cette ligne décrit le cas `ErrKey` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `ErrKey`, ce bloc devient le chemin actif.
8. `case ErrState` cette ligne décrit le cas `ErrState` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `ErrState`, ce bloc devient le chemin actif.
9. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
10. `pick KvValue {` cette ligne ouvre le type fermé `KvValue` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. Exemple concret: toute valeur hors des `case` déclarés devient impossible à représenter.
11. `case Some(value: string)` cette ligne décrit le cas `Some(value: string)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Some(value: string)`, ce bloc devient le chemin actif.
12. `case None` cette ligne décrit le cas `None` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `None`, ce bloc devient le chemin actif.
13. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: fermer l'espace des issues possibles dès le modèle de domaine.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, les opérations critiques utilisent `KvResult`, et la lecture utilise `KvValue` pour distinguer présence/absence sans ambiguïté.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 24.2 Centraliser la validation de clé

```vit
proc key_valid(k: string) -> bool {
  if k == "" { give false }
  give true
}
```

Lecture ligne par ligne (débutant):
1. `proc key_valid(k: string) -> bool {` ici, le contrat complet est défini pour `key_valid`: entrées `k: string` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `key_valid` retourne toujours une valeur compatible avec `bool`.
2. `if k == "" { give false }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `k == ""` est vrai, `give false` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `give true` ici, la branche renvoie immédiatement `true` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `true`.
4. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `k == ""` est vrai, la sortie devient `false`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `true`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: créer une seule source de vérité pour la validité de clé.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution:
- `key_valid("")` retourne `false`.
- `key_valid("id")` retourne `true`.
- `key_valid("user:42")` retourne `true`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 24.3 Implémenter un backend mémoire simple

```vit
form KvMem {
  entries: Entry[]
}
proc find_index(m: KvMem, k: string) -> int {
  let i: int = 0
  loop {
    if i >= m.entries.len() { break }
    if m.entries[i].key == k { give i }
    set i = i + 1
  }
give -1
}
```

Lecture ligne par ligne (débutant):
1. `form KvMem {` cette ligne ouvre la structure `KvMem` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `KvMem` sans redéfinir ses champs.
2. `entries: Entry[]` cette ligne déclare le champ `entries` avec le type `Entry[]`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `Entry[]`.
3. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
4. `proc find_index(m: KvMem, k: string) -> int {` sur cette ligne, le contrat complet est posé pour `find_index`: entrées `m: KvMem, k: string` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `find_index` retourne toujours une valeur compatible avec `int`.
5. `let i: int = 0` cette ligne crée la variable locale `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `i` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
6. `loop {` cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`). Exemple concret: à chaque tour, les gardes internes décident de continuer ou de sortir proprement.
7. `if i >= m.entries.len() { break }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `i >= m.entries.len()` est vrai, `break` est exécuté immédiatement; sinon on continue sur la ligne suivante.
8. `if m.entries[i].key == k { give i }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `m.entries[i].key == k` est vrai, `give i` est exécuté immédiatement; sinon on continue sur la ligne suivante.
9. `set i = i + 1` cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux. Exemple concret: après exécution, `i` prend la nouvelle valeur `i + 1` pour les étapes suivantes.
10. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
11. `give -1` sur cette ligne, la sortie est renvoyée immédiatement `-1` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `-1`.
12. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `m.entries[i].key == k` est vrai, la sortie devient `i`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `-1`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Ce backend est volontairement simple (tableau d'entrées), mais il suffit pour testér le protocole complet.

Erreurs fréquentes à éviter:
- implémenter le protocole avant de disposer d'un backend testable.
- dupliquer la recherche de clé dans chaque opération.
- mélanger recherche et mutation dans la même boucle.

## 24.4 Encadrer l'écriture

```vit
proc put_guard(entries_len: int, k: string) -> KvResult {
  if not key_valid(k) { give ErrKey }
  if entries_len < 0 { give ErrState }
  give Ok
}
proc put(m: KvMem, k: string, v: string) -> KvResult {
  let g: KvResult = put_guard(m.entries.len(), k)
  match g {
    case Ok {
      let idx: int = find_index(m, k)
      if idx < 0 {
        m.entries.push(Entry(k, v))
        give Ok
      }
    m.entries[idx] = Entry(k, v)
    give Ok
  }
case ErrKey { give ErrKey }
otherwise { give ErrState }
}
}
```

Lecture ligne par ligne (débutant):
1. `proc put_guard(entries_len: int, k: string) -> KvResult {` ce passage fixe le contrat complet de `put_guard`: entrées `entries_len: int, k: string` et sortie `KvResult`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `put_guard` retourne toujours une valeur compatible avec `KvResult`.
2. `if not key_valid(k) { give ErrKey }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `not key_valid(k)` est vrai, `give ErrKey` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if entries_len < 0 { give ErrState }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `entries_len < 0` est vrai, `give ErrState` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give Ok` ce passage retourne immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Ok`.
5. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
6. `proc put(m: KvMem, k: string, v: string) -> KvResult {` ici, le contrat complet est défini pour `put`: entrées `m: KvMem, k: string, v: string` et sortie `KvResult`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `put` retourne toujours une valeur compatible avec `KvResult`.
7. `let g: KvResult = put_guard(m.entries.len(), k)` cette ligne crée la variable locale `g` de type `KvResult` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `g` reçoit ici le résultat de `put_guard(m.entries.len(), k)` et peut être réutilisé ensuite sans recalcul.
8. `match g {` cette ligne démarre un dispatch déterministe sur `g`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `g`, la même branche sera toujours exécutée.
9. `case Ok {` cette ligne décrit le cas `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok`, ce bloc devient le chemin actif.
10. `let idx: int = find_index(m, k)` cette ligne crée la variable locale `idx` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `idx` reçoit ici le résultat de `find_index(m, k)` et peut être réutilisé ensuite sans recalcul.
11. `if idx < 0 {` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
12. `m.entries.push(Entry(k, v))` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
13. `give Ok` ici, la branche renvoie immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Ok`.
14. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
15. `m.entries[idx] = Entry(k, v)` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
16. `give Ok` sur cette ligne, la sortie est renvoyée immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Ok`.
17. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
18. `case ErrKey { give ErrKey }` cette ligne décrit le cas `ErrKey` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `ErrKey`, ce bloc devient le chemin actif.
19. `otherwise { give ErrState }` cette ligne définit le chemin de secours pour couvrir les situations non capturées par les cas explicites. Exemple concret: si aucun `case` ne correspond, `give ErrState` est exécuté pour garantir une sortie stable.
20. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
21. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `not key_valid(k)` est vrai, la sortie devient `ErrKey`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Ok`.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: connecter le protocole de validation au backend concret d'écriture.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution:
- clé valide absente -> insertion.
- clé valide existante -> mise à jour.
- clé invalide -> `ErrKey`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 24.5 Encadrer lecture et suppression

```vit
proc get_guard(entries_len: int, k: string) -> KvResult {
  if not key_valid(k) { give ErrKey }
  if entries_len == 0 { give ErrState }
  give Ok
}
proc delete_guard(entries_len: int, k: string) -> KvResult {
  if not key_valid(k) { give ErrKey }
  if entries_len <= 0 { give ErrState }
  give Ok
}
proc get(m: KvMem, k: string) -> KvValue {
  let g: KvResult = get_guard(m.entries.len(), k)
  match g {
    case Ok {
      let idx: int = find_index(m, k)
      if idx < 0 { give None }
      give Some(m.entries[idx].value)
    }
  otherwise { give None }
}
}
proc delete(m: KvMem, k: string) -> KvResult {
  let g: KvResult = delete_guard(m.entries.len(), k)
  match g {
    case Ok {
      let idx: int = find_index(m, k)
      if idx < 0 { give ErrState }
      m.entries.remove_at(idx)
      give Ok
    }
  case ErrKey { give ErrKey }
  otherwise { give ErrState }
}
}
```

Lecture ligne par ligne (débutant):
1. `proc get_guard(entries_len: int, k: string) -> KvResult {` sur cette ligne, le contrat complet est posé pour `get_guard`: entrées `entries_len: int, k: string` et sortie `KvResult`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `get_guard` retourne toujours une valeur compatible avec `KvResult`.
2. `if not key_valid(k) { give ErrKey }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `not key_valid(k)` est vrai, `give ErrKey` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if entries_len == 0 { give ErrState }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `entries_len == 0` est vrai, `give ErrState` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give Ok` ce passage retourne immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Ok`.
5. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
6. `proc delete_guard(entries_len: int, k: string) -> KvResult {` ce passage fixe le contrat complet de `delete_guard`: entrées `entries_len: int, k: string` et sortie `KvResult`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `delete_guard` retourne toujours une valeur compatible avec `KvResult`.
7. `if not key_valid(k) { give ErrKey }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `not key_valid(k)` est vrai, `give ErrKey` est exécuté immédiatement; sinon on continue sur la ligne suivante.
8. `if entries_len <= 0 { give ErrState }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `entries_len <= 0` est vrai, `give ErrState` est exécuté immédiatement; sinon on continue sur la ligne suivante.
9. `give Ok` ici, la branche renvoie immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Ok`.
10. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
11. `proc get(m: KvMem, k: string) -> KvValue {` ici, le contrat complet est défini pour `get`: entrées `m: KvMem, k: string` et sortie `KvValue`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `get` retourne toujours une valeur compatible avec `KvValue`.
12. `let g: KvResult = get_guard(m.entries.len(), k)` cette ligne crée la variable locale `g` de type `KvResult` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `g` reçoit ici le résultat de `get_guard(m.entries.len(), k)` et peut être réutilisé ensuite sans recalcul.
13. `match g {` cette ligne démarre un dispatch déterministe sur `g`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `g`, la même branche sera toujours exécutée.
14. `case Ok {` cette ligne décrit le cas `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok`, ce bloc devient le chemin actif.
15. `let idx: int = find_index(m, k)` cette ligne crée la variable locale `idx` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `idx` reçoit ici le résultat de `find_index(m, k)` et peut être réutilisé ensuite sans recalcul.
16. `if idx < 0 { give None }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `idx < 0` est vrai, `give None` est exécuté immédiatement; sinon on continue sur la ligne suivante.
17. `give Some(m.entries[idx].value)` sur cette ligne, la sortie est renvoyée immédiatement `Some(m.entries[idx].value)` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Some(m.entries[idx].value)`.
18. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
19. `otherwise { give None }` cette ligne définit le chemin de secours pour couvrir les situations non capturées par les cas explicites. Exemple concret: si aucun `case` ne correspond, `give None` est exécuté pour garantir une sortie stable.
20. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
21. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
22. `proc delete(m: KvMem, k: string) -> KvResult {` sur cette ligne, le contrat complet est posé pour `delete`: entrées `m: KvMem, k: string` et sortie `KvResult`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `delete` retourne toujours une valeur compatible avec `KvResult`.
23. `let g: KvResult = delete_guard(m.entries.len(), k)` cette ligne crée la variable locale `g` de type `KvResult` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `g` reçoit ici le résultat de `delete_guard(m.entries.len(), k)` et peut être réutilisé ensuite sans recalcul.
24. `match g {` cette ligne démarre un dispatch déterministe sur `g`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `g`, la même branche sera toujours exécutée.
25. `case Ok {` cette ligne décrit le cas `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok`, ce bloc devient le chemin actif.
26. `let idx: int = find_index(m, k)` cette ligne crée la variable locale `idx` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `idx` reçoit ici le résultat de `find_index(m, k)` et peut être réutilisé ensuite sans recalcul.
27. `if idx < 0 { give ErrState }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `idx < 0` est vrai, `give ErrState` est exécuté immédiatement; sinon on continue sur la ligne suivante.
28. `m.entries.remove_at(idx)` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
29. `give Ok` ce passage retourne immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Ok`.
30. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
31. `case ErrKey { give ErrKey }` cette ligne décrit le cas `ErrKey` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `ErrKey`, ce bloc devient le chemin actif.
32. `otherwise { give ErrState }` cette ligne définit le chemin de secours pour couvrir les situations non capturées par les cas explicites. Exemple concret: si aucun `case` ne correspond, `give ErrState` est exécuté pour garantir une sortie stable.
33. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
34. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `not key_valid(k)` est vrai, la sortie devient `ErrKey`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Ok`.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: mutualiser la grammaire de validation et varier seulement la règle d'état selon l'opération.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution:
- `get` sur clé absente -> `None`.
- `delete` sur clé présente -> `Ok`.
- `delete` sur clé absente -> `ErrState`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 24.6 Assembler un flux métier complet

```vit
proc kv_roundtrip(m: KvMem, k: string, v: string) -> KvResult {
  let p: KvResult = put(m, k, v)
  match p {
    case Ok {
      let g: KvValue = get(m, k)
      match g {
        case Some(_) { give Ok }
        otherwise { give ErrState }
      }
  }
case ErrKey { give ErrKey }
otherwise { give ErrState }
}
}
```

Lecture ligne par ligne (débutant):
1. `proc kv_roundtrip(m: KvMem, k: string, v: string) -> KvResult {` ce passage fixe le contrat complet de `kv_roundtrip`: entrées `m: KvMem, k: string, v: string` et sortie `KvResult`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `kv_roundtrip` retourne toujours une valeur compatible avec `KvResult`.
2. `let p: KvResult = put(m, k, v)` cette ligne crée la variable locale `p` de type `KvResult` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `p` reçoit ici le résultat de `put(m, k, v)` et peut être réutilisé ensuite sans recalcul.
3. `match p {` cette ligne démarre un dispatch déterministe sur `p`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `p`, la même branche sera toujours exécutée.
4. `case Ok {` cette ligne décrit le cas `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Ok`, ce bloc devient le chemin actif.
5. `let g: KvValue = get(m, k)` cette ligne crée la variable locale `g` de type `KvValue` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `g` reçoit ici le résultat de `get(m, k)` et peut être réutilisé ensuite sans recalcul.
6. `match g {` cette ligne démarre un dispatch déterministe sur `g`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `g`, la même branche sera toujours exécutée.
7. `case Some(_) { give Ok }` cette ligne décrit le cas `Some(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Some(_)`, ce bloc devient le chemin actif.
8. `otherwise { give ErrState }` cette ligne définit le chemin de secours pour couvrir les situations non capturées par les cas explicites. Exemple concret: si aucun `case` ne correspond, `give ErrState` est exécuté pour garantir une sortie stable.
9. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
10. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
11. `case ErrKey { give ErrKey }` cette ligne décrit le cas `ErrKey` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `ErrKey`, ce bloc devient le chemin actif.
12. `otherwise { give ErrState }` cette ligne définit le chemin de secours pour couvrir les situations non capturées par les cas explicites. Exemple concret: si aucun `case` ne correspond, `give ErrState` est exécuté pour garantir une sortie stable.
13. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
14. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: prouver que `put` et `get` composent correctement dans un scénario nominal.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution:
- `kv_roundtrip(m,"id","42")` retourne `Ok`.
- `kv_roundtrip(m,"","42")` retourne `ErrKey`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 24.7 Intégrer un scénario bout en bout

```vit
entry main at kv/app {
  let m: KvMem = KvMem([])
  let a: KvResult = put(m, "user:1", "alice")
  let b: KvValue = get(m, "user:1")
  let c: KvResult = delete(m, "user:1")
  if a == Ok and c == Ok { return 0 }
  return 70
}
```

Lecture ligne par ligne (débutant):
1. `entry main at kv/app {` cette ligne fixe le point d'entrée `main` dans `kv/app` et sert de scénario exécutable de bout en bout pour le chapitre. Exemple concret: lancer cette entrée permet de vérifier la chaîne complète des fonctions appelées.
2. `let m: KvMem = KvMem([])` cette ligne crée la variable locale `m` de type `KvMem` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `m` reçoit ici le résultat de `KvMem([])` et peut être réutilisé ensuite sans recalcul.
3. `let a: KvResult = put(m, "user:1", "alice")` cette ligne crée la variable locale `a` de type `KvResult` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `a` reçoit ici le résultat de `put(m, "user:1", "alice")` et peut être réutilisé ensuite sans recalcul.
4. `let b: KvValue = get(m, "user:1")` cette ligne crée la variable locale `b` de type `KvValue` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `b` reçoit ici le résultat de `get(m, "user:1")` et peut être réutilisé ensuite sans recalcul.
5. `let c: KvResult = delete(m, "user:1")` cette ligne crée la variable locale `c` de type `KvResult` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `c` reçoit ici le résultat de `delete(m, "user:1")` et peut être réutilisé ensuite sans recalcul.
6. `if a == Ok and c == Ok { return 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `a == Ok and c == Ok` est vrai, `return 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
7. `return 70` cette ligne termine l'exécution du bloc courant avec le code `70`, utile pour observer le résultat global du scénario. Exemple concret: un test d'exécution peut vérifier directement que le programme retourne `70`.
8. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return 70`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `70`.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Ce scénario est le minimum vital d'un KV store:
- écrire.
- relire.
- supprimer.
- vérifier la cohérence du flux.


## Table erreur -> diagnostic -> correction

| Erreur | Diagnostic | Correction |
| --- | --- | --- |
| Entrée invalide | Validation absente ou trop tardive. | Centraliser la validation en entrée de pipeline. |
| État incohérent | Mutation partielle ou invariant non vérifié. | Appliquer le principe d'atomicité et rejeter sinon. |
| Sortie inattendue | Couche projection mélangée avec la logique métier. | Séparer `apply` (métier) et `project` (sortie). |

## À retenir

Un KV store robuste n'est pas un tableau de couples clé-valeur. C'est un protocole explicite: validation unique, backend isolé, opérations déterministes et composition testable de bout en bout.

Critère pratique de qualité pour ce chapitre:
- vous savez tracer `put/get/delete` en distinguant nominal, absence et erreur.
- vous savez testér la validité de clé sans dépendre du backend.
- vous pouvez changer le backend sans changer le contrat public des opérations.


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

- `docs/book/keywords/and.md`.
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

## Checkpoint synthèse

Mini quiz:
1. Quelle est l'invariant central de ce chapitre ?
2. Quelle garde évite l'état invalide le plus fréquent ?
3. Quel test simple prouve le comportement nominal ?

