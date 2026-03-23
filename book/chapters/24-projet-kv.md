# 24. Projet guide KV store

Niveau: Avancé

Prérequis: chapitre précédent `book/chapters/23-projet-sys.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/23-projet-sys.md`, `book/chapters/25-projet-arduino.md`, `book/glossaire.md`.

## Problème Concret

Situation réelle: Projet guide KV store devient clair quand on trace chaque étape du calcul. L'objectif est de relier ligne de code et effet concret sur la sortie.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: on conserve un même mini-programme pour comparer les effets d'une modification sans changer tout le contexte.
Objectif pédagogique: comprendre pourquoi une ligne existe et ce qu'elle change dans la trajectoire du programme.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Projet guide KV store**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Projet guide KV store** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Projet guide KV store**.

## Exercice court

Prenez un exemple du chapitre sur **Projet guide KV store**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Projet guide KV store**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## Niveau local

- Niveau local section coeur: Avancé.
- Niveau local exemples guidés: Intermédiaire.
- Niveau local exercices de diagnostic: Avancé.

Ce chapitre poursuit un objectif clair: construire un KV store Vitte exploitable, avec un protocole d'opérations clair, une validation de clé unique et un flux d'erreurs stable. Nous allons passer d'un simple test-fou de validation à un mini moteur clé-valeur complet (put/get/delete) avec backend mémoire.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.

Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `book/checklist-editoriale.md`. Complément: `book/erreurs-classiques.md`.

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
1. `form Entry {` : cette ligne ouvre la structure `Entry` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable.
2. `key: string` : cette ligne déclare le champ `key` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation.
3. `value: string` : cette ligne déclare le champ `value` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation.
4. `}` : cette accolade ferme le bloc logique.
5. `pick KvResult {` : cette ligne ouvre le type fermé `KvResult` pour forcer un ensemble fini de cas possibles et supprimer les états implicites.
6. `case Ok` : ce cas décrit `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
7. `case ErrKey` : ce cas décrit `ErrKey` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
8. `case ErrState` : ce cas décrit `ErrState` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
9. `}` : cette accolade ferme le bloc logique.
10. `pick KvValue {` : cette ligne ouvre le type fermé `KvValue` pour forcer un ensemble fini de cas possibles et supprimer les états implicites.
11. `case Some(value: string)` : ce cas décrit `Some(value: string)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
12. `case None` : ce cas décrit `None` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
13. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: un test explicite du bloc gère les entrées hors règle avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie stable.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: fermer l'espace des issues possibles dès le modèle de domaine.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, les opérations critiques utilisent `KvResult`, et la lecture utilise `KvValue` pour distinguer présence/absence sans ambiguïté.

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
1. `proc key_valid(k: string) -> bool {` : la règle est défini pour `key_valid`: entrées `k: string` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if k == "" { give false }` : cette test traite le cas d'erreur avant le calcul.
3. `give true` : la branche renvoie immédiatement `true` pour la branche courante, la sortie de branche est explicite et vérifiable.
4. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: si `k == ""` est vrai, la sortie devient `false`.
- Cas nominal: sans test bloquante, la branche principale renvoie `true`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: créer une seule source de vérité pour la validité de clé.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution:
- `key_valid("")` retourne `false`.
- `key_valid("id")` retourne `true`.
- `key_valid("user:42")` retourne `true`.

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

  // Boucle: progression controlee jusqu'a la borne
  loop {
    // Borne d'arret: stoppe la boucle de maniere explicite
    if i >= m.entries.len() { break }

    if m.entries[i].key == k { give i }
    set i = i + 1
  }

  give -1
}
```

Lecture ligne par ligne (débutant):
1. `form KvMem {` : cette ligne ouvre la structure `KvMem` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable.
2. `entries: Entry[]` : cette ligne déclare le champ `entries` avec le type `Entry[]`, ce qui documente son rôle et limite les erreurs de manipulation.
3. `}` : cette accolade ferme le bloc logique.
4. `proc find_index(m: KvMem, k: string) -> int {` : la règle est posé pour `find_index`: entrées `m: KvMem, k: string` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
5. `let i: int = 0` : cette ligne crée la variable `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
6. `loop {` : cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`).
7. `if i >= m.entries.len() { break }` : cette test traite le cas d'erreur avant le calcul.
8. `if m.entries[i].key == k { give i }` : cette test traite le cas d'erreur avant le calcul.
9. `set i = i + 1` : cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux.
10. `}` : cette accolade clôt le bloc logique.
11. `give -1` : la sortie est renvoyée immédiatement `-1` pour la branche courante, la sortie de branche est explicite et vérifiable.
12. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: si `m.entries[i].key == k` est vrai, la sortie devient `i`.
- Cas nominal: sans test bloquante, la branche principale renvoie `-1`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

Ce backend est volontairement simple (tableau d'entrées), mais il suffit pour tester le protocole complet.

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
1. `proc put_guard(entries_len: int, k: string) -> KvResult {` : la règle est fixé pour `put_guard`: entrées `entries_len: int, k: string` et sortie `KvResult`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if not key_valid(k) { give ErrKey }` : cette test traite le cas d'erreur avant le calcul.
3. `if entries_len < 0 { give ErrState }` : cette test traite le cas d'erreur avant le calcul.
4. `give Ok` : retourne immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable.
5. `}` : cette accolade ferme le bloc logique.
6. `proc put(m: KvMem, k: string, v: string) -> KvResult {` : la règle est défini pour `put`: entrées `m: KvMem, k: string, v: string` et sortie `KvResult`, elle clarifie l'intention avant lecture détaillée du corps.
7. `let g: KvResult = put_guard(m.entries.len(), k)` : cette ligne crée la variable `g` de type `KvResult` pour nommer explicitement une étape intermédiaire du raisonnement.
8. `match g {` : cette ligne démarre un dispatch stable sur `g`: une seule branche sera choisie selon la forme de la valeur analysée.
9. `case Ok {` : ce cas décrit `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
10. `let idx: int = find_index(m, k)` : cette ligne crée la variable `idx` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
11. `if idx < 0 {` : cette ligne définit une étape explicite du flux.
12. `m.entries.push(Entry(k, v))` : cette ligne définit une étape explicite du flux.
13. `give Ok` : la branche renvoie immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable.
14. `}` : cette accolade clôt le bloc logique.
15. `m.entries[idx] = Entry(k, v)` : cette ligne définit une étape explicite du flux.
16. `give Ok` : la sortie est renvoyée immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable.
17. `}` : cette accolade ferme le bloc logique.
18. `case ErrKey { give ErrKey }` : ce cas décrit `ErrKey` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
19. `otherwise { give ErrState }` : cette ligne définit un chemin de secours explicite.
20. `}` : cette accolade ferme le bloc logique.
21. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: si `not key_valid(k)` est vrai, la sortie devient `ErrKey`.
- Cas nominal: sans test bloquante, la branche principale renvoie `Ok`.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: connecter le protocole de validation au backend concret d'écriture.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution:
- clé valide absente -> insertion.
- clé valide existante -> mise à jour.
- clé invalide -> `ErrKey`.

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
1. `proc get_guard(entries_len: int, k: string) -> KvResult {` : la règle est posé pour `get_guard`: entrées `entries_len: int, k: string` et sortie `KvResult`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if not key_valid(k) { give ErrKey }` : cette test traite le cas d'erreur avant le calcul.
3. `if entries_len == 0 { give ErrState }` : cette test traite le cas d'erreur avant le calcul.
4. `give Ok` : retourne immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable.
5. `}` : cette accolade ferme le bloc logique.
6. `proc delete_guard(entries_len: int, k: string) -> KvResult {` : la règle est fixé pour `delete_guard`: entrées `entries_len: int, k: string` et sortie `KvResult`, elle clarifie l'intention avant lecture détaillée du corps.
7. `if not key_valid(k) { give ErrKey }` : cette test traite le cas d'erreur avant le calcul.
8. `if entries_len <= 0 { give ErrState }` : cette test traite le cas d'erreur avant le calcul.
9. `give Ok` : la branche renvoie immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable.
10. `}` : cette accolade ferme le bloc logique.
11. `proc get(m: KvMem, k: string) -> KvValue {` : la règle est défini pour `get`: entrées `m: KvMem, k: string` et sortie `KvValue`, elle clarifie l'intention avant lecture détaillée du corps.
12. `let g: KvResult = get_guard(m.entries.len(), k)` : cette ligne crée la variable `g` de type `KvResult` pour nommer explicitement une étape intermédiaire du raisonnement.
13. `match g {` : cette ligne démarre un dispatch stable sur `g`: une seule branche sera choisie selon la forme de la valeur analysée.
14. `case Ok {` : ce cas décrit `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
15. `let idx: int = find_index(m, k)` : cette ligne crée la variable `idx` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
16. `if idx < 0 { give None }` : cette test traite le cas d'erreur avant le calcul.
17. `give Some(m.entries[idx].value)` : la sortie est renvoyée immédiatement `Some(m.entries[idx].value)` pour la branche courante, la sortie de branche est explicite et vérifiable.
18. `}` : cette accolade clôt le bloc logique.
19. `otherwise { give None }` : cette ligne définit un chemin de secours explicite.
20. `}` : cette accolade ferme le bloc logique.
21. `}` : cette accolade ferme le bloc logique.
22. `proc delete(m: KvMem, k: string) -> KvResult {` : la règle est posé pour `delete`: entrées `m: KvMem, k: string` et sortie `KvResult`, elle clarifie l'intention avant lecture détaillée du corps.
23. `let g: KvResult = delete_guard(m.entries.len(), k)` : cette ligne crée la variable `g` de type `KvResult` pour nommer explicitement une étape intermédiaire du raisonnement.
24. `match g {` : cette ligne démarre un dispatch stable sur `g`: une seule branche sera choisie selon la forme de la valeur analysée.
25. `case Ok {` : ce cas décrit `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
26. `let idx: int = find_index(m, k)` : cette ligne crée la variable `idx` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
27. `if idx < 0 { give ErrState }` : cette test traite le cas d'erreur avant le calcul.
28. `m.entries.remove_at(idx)` : cette ligne définit une étape explicite du flux.
29. `give Ok` : retourne immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable.
30. `}` : cette accolade clôt le bloc logique.
31. `case ErrKey { give ErrKey }` : ce cas décrit `ErrKey` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
32. `otherwise { give ErrState }` : cette ligne définit un chemin de secours explicite.
33. `}` : cette accolade ferme le bloc logique.
34. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: si `not key_valid(k)` est vrai, la sortie devient `ErrKey`.
- Cas nominal: sans test bloquante, la branche principale renvoie `Ok`.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: mutualiser la grammaire de validation et varier seulement la règle d'état selon l'opération.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution:
- `get` sur clé absente -> `None`.
- `delete` sur clé présente -> `Ok`.
- `delete` sur clé absente -> `ErrState`.

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
1. `proc kv_roundtrip(m: KvMem, k: string, v: string) -> KvResult {` : la règle est fixé pour `kv_roundtrip`: entrées `m: KvMem, k: string, v: string` et sortie `KvResult`, elle clarifie l'intention avant lecture détaillée du corps.
2. `let p: KvResult = put(m, k, v)` : cette ligne crée la variable `p` de type `KvResult` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `match p {` : cette ligne démarre un dispatch stable sur `p`: une seule branche sera choisie selon la forme de la valeur analysée.
4. `case Ok {` : ce cas décrit `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
5. `let g: KvValue = get(m, k)` : cette ligne crée la variable `g` de type `KvValue` pour nommer explicitement une étape intermédiaire du raisonnement.
6. `match g {` : cette ligne démarre un dispatch stable sur `g`: une seule branche sera choisie selon la forme de la valeur analysée.
7. `case Some(_) { give Ok }` : ce cas décrit `Some(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
8. `otherwise { give ErrState }` : cette ligne définit un chemin de secours explicite.
9. `}` : cette accolade clôt le bloc logique.
10. `}` : cette accolade ferme le bloc logique.
11. `case ErrKey { give ErrKey }` : ce cas décrit `ErrKey` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
12. `otherwise { give ErrState }` : cette ligne définit un chemin de secours explicite.
13. `}` : cette accolade ferme le bloc logique.
14. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: un test explicite du bloc gère les entrées hors règle avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie stable.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

L'intention de cette étape est directe: prouver que `put` et `get` composent correctement dans un scénario nominal.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution:
- `kv_roundtrip(m,"id","42")` retourne `Ok`.
- `kv_roundtrip(m,"","42")` retourne `ErrKey`.

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
1. `entry main at kv/app {` : cette ligne fixe le point d'entrée `main` dans `kv/app` et sert de scénario exécutable de bout en bout pour le chapitre.
2. `let m: KvMem = KvMem([])` : cette ligne crée la variable `m` de type `KvMem` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `let a: KvResult = put(m, "user:1", "alice")` : cette ligne crée la variable `a` de type `KvResult` pour nommer explicitement une étape intermédiaire du raisonnement.
4. `let b: KvValue = get(m, "user:1")` : cette ligne crée la variable `b` de type `KvValue` pour nommer explicitement une étape intermédiaire du raisonnement.
5. `let c: KvResult = delete(m, "user:1")` : cette ligne crée la variable `c` de type `KvResult` pour nommer explicitement une étape intermédiaire du raisonnement.
6. `if a == Ok and c == Ok { return 0 }` : cette test traite le cas d'erreur avant le calcul.
7. `return 70` : cette ligne termine l'exécution du bloc courant avec le code `70`, utile pour observer le résultat global du scénario.
8. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: un test explicite du bloc gère les entrées hors règle avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return 70`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `70`.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

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

Un KV store robuste n'est pas un tableau de couples clé-valeur. C'est un protocole explicite: validation unique, backend isolé, opérations stables et composition testable de bout en bout.

Critère pratique de qualité pour ce chapitre:
- vous savez tracer `put/get/delete` en distinguant nominal, absence et erreur.
- vous savez tester la validité de clé sans dépendre du backend.
- vous pouvez changer le backend sans changer la règle public des opérations.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: un test explicite ou un chemin de secours stable doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de test pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez le test ou la branche de secours réellement utilisée.

## Mini défi transverse

Défi: combinez au moins deux notions des trois derniers chapitres dans une fonction courte (test + transformation + sortie).
Vérification minimale: montrez un cas nominal et un cas invalide, puis expliquez quelle branche est prise.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/and.md`.
- `book/keywords/at.md`.
- `book/keywords/bool.md`.
- `book/keywords/break.md`.
- `book/keywords/case.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

## Checkpoint synthèse

Mini quiz:
1. Quelle est l'invariant central de ce chapitre ?
2. Quelle test évite l'état invalide le plus fréquent ?
3. Quel test simple prouve le comportement nominal ?

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **projet guide kv store**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

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

Lecture ligne par ligne:
1. `form Entry {` -> participe au déroulé du traitement.
2. `key: string` -> participe au déroulé du traitement.
3. `value: string` -> participe au déroulé du traitement.
4. `}` -> participe au déroulé du traitement.
5. `pick KvResult {` -> participe au déroulé du traitement.
6. `case Ok` -> participe au déroulé du traitement.
7. `case ErrKey` -> participe au déroulé du traitement.
8. `case ErrState` -> participe au déroulé du traitement.
9. `}` -> participe au déroulé du traitement.
10. `pick KvValue {` -> participe au déroulé du traitement.

### Exemple B: variante cas d'erreur (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant un test explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter un test avant cette ligne.
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


```vit
// Scenario projet kv: execution complete et verifiable
space demo/projet-kv

form Request { id: int amount: int quota: int }
pick Result { case Accepted(total: int) case Rejected(code: int) }

// Entrée applicative: validation des invariants de requête
proc parse_request(r: Request) -> Result {

  if r.id <= 0 { give Result.Rejected(91) }

  if r.quota < 0 { give Result.Rejected(92) }

  if r.amount < 0 { give Result.Rejected(93) }

  give Result.Accepted(r.amount)
}

// Politique métier: applique les règles de décision
proc apply_policy(total: int, quota: int) -> Result {
  let capped: int = total
  if capped > quota { set capped = quota }

  if capped < 5 { give Result.Rejected(94) }

  give Result.Accepted(capped)
}

// Persistance simulée: matérialise un résultat sans I/O réel
proc persist_sim(x: Result) -> Result {

  match x {
    case Accepted(v) {
      if v % 13 == 0 { give Result.Rejected(95) }

      give Result.Accepted(v)
    }
    case Rejected(c) { give Result.Rejected(c) }
    otherwise { give Result.Rejected(70) }
  }
}

// Conversion finale vers un code de sortie
proc to_exit(x: Result) -> int {

  match x {
    case Accepted(_) { give 0 }
    case Rejected(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let req: Request = Request(7, 12, 15)
  let p: Result = parse_request(req)
  let d: Result = apply_policy(12, req.quota)
  let s: Result = persist_sim(d)
  let _probe: int = to_exit(p)

  return to_exit(s)
}
```

## Explication détaillée du gros bloc

Ce gros bloc montre un programme entier, pas un extrait isolé: on suit le flux du début à la fin.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `parse_request`: lit `r: Request` et renvoie `Result`.
- `apply_policy`: lit `total: int, quota: int` et renvoie `Result`.
- `persist_sim`: lit `x: Result` et renvoie `Result`.
- `to_exit`: lit `x: Result` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `parse_request` est appelé pour traiter l'étape suivante.
3. `apply_policy` est appelé pour traiter l'étape suivante.
4. `persist_sim` est appelé pour traiter l'étape suivante.
5. `to_exit` est appelé pour traiter l'étape suivante.
6. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `r.id <= 0`.
- Test évalué: `r.quota < 0`.
- Test évalué: `r.amount < 0`.
- Test évalué: `capped > quota`.
- Test évalué: `capped < 5`.
- Test évalué: `v % 13 == 0`.
- Sélection par `match x`: le chemin dépend de l'état reçu.
- Sélection par `match x`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> parse_request -> apply_policy -> persist_sim -> to_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> parse_request renvoie un code d'erreur -> sortie non nulle`.

### 5. Pourquoi ce découpage est utile
- Vous testez chaque fonction seule, puis le flux complet.
- Vous savez où modifier une règle sans casser tout le programme.
- Vous pouvez expliquer la sortie en suivant simplement les appels.

### 6. Vérification rapide
1. Relancer avec une entrée normale et noter la sortie.
2. Relancer avec une entrée invalide et vérifier le code d'erreur.
3. Confirmer que la même entrée donne toujours la même sortie.


## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les tests sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas d'erreur réel:
- Entree degradee ou incomplete: le test doit couper le flux tot avec une sortie explicite.

A tester:
- Requête nominale -> sortie 0.
- Entrée invalide id<=0 -> sortie 91.
- Refus métier valeur<5 -> sortie 94.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `parse_request` lit `r: Request` puis renvoie `Result`.
- `apply_policy` lit `total: int, quota: int` puis renvoie `Result`.
- `persist_sim` lit `x: Result` puis renvoie `Result`.
- `to_exit` lit `x: Result` puis renvoie `int`.

- Variables créées (valeur initiale):
- `capped: int` démarre avec `total`.
- `req: Request` démarre avec `Request(7, 12, 15)`.
- `p: Result` démarre avec `parse_request(req)`.
- `d: Result` démarre avec `apply_policy(12, req.quota)`.
- `s: Result` démarre avec `persist_sim(d)`.
- `_probe: int` démarre avec `to_exit(p)`.

- Variables modifiées pendant le traitement:
- `capped` est mis à jour avec `quota`.

- Conditions qui changent le chemin:
- si `r.id <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `r.quota < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `r.amount < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `capped > quota` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `capped < 5` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `v % 13 == 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: capped=total -> req=Request(7, 12, 15) -> p=parse_request(req) -> d=apply_policy(12, req.quota)
- enchaînement: parse_request -> apply_policy -> persist_sim -> to_exit
- sortie finale sur ce chemin: `to_exit(s)`.

- Trace d'erreur (valeurs exemple):
- si `r.id <= 0` devient vrai, la fonction renvoie immédiatement `Result.Rejected(91)`.

- Vérification rapide:
- relancer avec une entrée normale et noter la sortie,
- relancer avec une entrée invalide et noter le code d'erreur,
- confirmer qu'une même entrée produit toujours la même sortie.

## Trade-offs

| Contrainte | Option A | Option B | Décision recommandée |
| --- | --- | --- | --- |
| Lisibilité prioritaire | Branches explicites | Code compact | A si l'équipe maintient le code longtemps |
| Perf critique | Spécialisation ciblée | Généralisation | A si profiling confirme le gain |
| Évolution rapide | Contrats stricts | Conventions implicites | A pour réduire les régressions |


## Décision Selon Contrainte

- Si la contrainte dominante est la sûreté: valider tôt, échouer explicitement.
- Si la contrainte dominante est la latence: mesurer d'abord, optimiser ensuite.
- Si la contrainte dominante est l'évolutivité: isoler orchestration, décisions et conversion de sortie.


## Diagnostic Rapide

| Symptôme | Cause probable | Vérification | Correction |
| --- | --- | --- | --- |
| Sortie inattendue | Test absente ou mal ordonnée | Rejouer avec cas d'erreur | Remonter le test avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Règle implicite | Comparer nominal vs limite | Formaliser la règle dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas d'erreur réel et prévoir sa sortie,
- identifier où ajouter un test sans casser le nominal.


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: tests d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
