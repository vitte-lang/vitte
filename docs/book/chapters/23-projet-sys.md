# 23. Projet guide Système

Niveau: Avancé

Prérequis: chapitre précédent `docs/book/chapters/22-projet-http.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/22-projet-http.md`, `docs/book/chapters/24-projet-kv.md`, `docs/book/glossaire.md`.

## Problème Concret

Situation réelle: Projet guide Système se comprend mieux en rejouant le programme comme un algorithme exécutable. Vous lisez les données entrantes, la condition évaluée, puis la valeur renvoyée.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: chaque section reprend le même scénario pour isoler une seule décision technique à la fois.
Objectif pédagogique: comprendre pourquoi une ligne existe et ce qu'elle change dans la trajectoire du programme.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Projet guide Système**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Projet guide Système** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Projet guide Système**.

## Exercice court

Prenez un exemple du chapitre sur **Projet guide Système**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Projet guide Système**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## Niveau local

- Niveau local section coeur: Avancé.
- Niveau local exemples guidés: Intermédiaire.
- Niveau local exercices de diagnostic: Avancé.

Ce chapitre poursuit un objectif clair: construire un noyau système Vitte qui reste auditable sous contrainte réelle. Nous allons dépasser le simple exemple mémoire pour poser une architecture complète: modèle de région, accès bornés, points `unsafe` minimaux, composition d'opérations et plan de tests.

L'enjeu n'est pas de « faire marcher » une instruction machine, mais de conserver des invariants lisibles dans tout le flux. En code système, la lisibilité n'est pas un luxe: c'est une mesure de sécurité.

Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.

Schéma pipeline du chapitre:
- Entrée: données initiales ou requête.
- Traitement: validation, logique métier, transformations.
- Sortie: résultat métier ou code de statut.
- Invariant: propriété qui doit rester vraie à chaque étape.

## 23.1 Définir l'état système et la règle de résultat

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
1. `form Region {` : cette ligne ouvre la structure `Region` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable.
2. `base: int` : cette ligne déclare le champ `base` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation.
3. `size: int` : cette ligne déclare le champ `size` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation.
4. `}` : cette accolade ferme le bloc logique.
5. `form SysMem {` : cette ligne ouvre la structure `SysMem` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable.
6. `region: Region` : cette ligne déclare le champ `region` avec le type `Region`, ce qui documente son rôle et limite les erreurs de manipulation.
7. `cells: int[]` : cette ligne déclare le champ `cells` avec le type `int[]`, ce qui documente son rôle et limite les erreurs de manipulation.
8. `}` : cette accolade ferme le bloc logique.
9. `pick SysResult {` : cette ligne ouvre le type fermé `SysResult` pour forcer un ensemble fini de cas possibles et supprimer les états implicites.
10. `case Ok` : ce cas décrit `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
11. `case ErrBounds` : ce cas décrit `ErrBounds` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
12. `case ErrState` : ce cas décrit `ErrState` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
13. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: un test explicite du bloc gère les entrées hors règle avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie stable.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

Ce socle impose deux règles:
- toute opération mémoire travaille dans une `Region`.
- toute issue est explicitée par `SysResult`.

À l'exécution, aucune opération critique ne renvoie un code arbitraire. Tout passe par `Ok` / `ErrBounds` / `ErrState`.

Erreurs fréquentes à éviter:
- laisser des entiers « magiques » circuler comme codes d'erreur.
- mélanger erreur de borne et erreur d'état dans la même branche.
- exposer la mémoire brute sans règle de région.

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
1. `proc contains(r: Region, addr: int) -> bool {` : la règle est défini pour `contains`: entrées `r: Region, addr: int` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if addr < r.base { give false }` : cette test traite le cas d'erreur avant le calcul.
3. `if addr >= r.base + r.size { give false }` : cette test traite le cas d'erreur avant le calcul.
4. `give true` : la branche renvoie immédiatement `true` pour la branche courante, la sortie de branche est explicite et vérifiable.
5. `}` : cette accolade ferme le bloc logique.
6. `proc to_index(r: Region, addr: int) -> int {` : la règle est posé pour `to_index`: entrées `r: Region, addr: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
7. `give addr - r.base` : la sortie est renvoyée immédiatement `addr - r.base` pour la branche courante, la sortie de branche est explicite et vérifiable.
8. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: si `addr < r.base` est vrai, la sortie devient `false`.
- Cas nominal: sans test bloquante, la branche principale renvoie `true`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

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
1. `proc read_cell(m: SysMem, addr: int) -> int {` : la règle est fixé pour `read_cell`: entrées `m: SysMem, addr: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if not contains(m.region, addr) { give 0 }` : cette test traite le cas d'erreur avant le calcul.
3. `let i: int = to_index(m.region, addr)` : cette ligne crée la variable `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
4. `give m.cells[i]` : retourne immédiatement `m.cells[i]` pour la branche courante, la sortie de branche est explicite et vérifiable.
5. `}` : cette accolade clôt le bloc logique.
6. `proc write_cell(m: SysMem, addr: int, v: int) -> SysResult {` : la règle est défini pour `write_cell`: entrées `m: SysMem, addr: int, v: int` et sortie `SysResult`, elle clarifie l'intention avant lecture détaillée du corps.
7. `if not contains(m.region, addr) { give ErrBounds }` : cette test traite le cas d'erreur avant le calcul.
8. `let i: int = to_index(m.region, addr)` : cette ligne crée la variable `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
9. `m.cells[i] = v` : cette ligne définit une étape explicite du flux.
10. `give Ok` : la branche renvoie immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable.
11. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: si `not contains(m.region, addr)` est vrai, la sortie devient `0`.
- Cas nominal: sans test bloquante, la branche principale renvoie `m.cells[i]`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

Le règle est volontairement explicite:
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
1. `proc cpu_pause() {` : cette ligne définit une étape explicite du flux.
2. `unsafe { asm("pause") }` : cette ligne marque une zone sensible qui doit rester courte, justifiée et facile à auditer dans un contexte système.
3. `}` : cette accolade ferme le bloc logique.
4. `proc cpu_halt_if(flag: bool) -> SysResult {` : la règle est posé pour `cpu_halt_if`: entrées `flag: bool` et sortie `SysResult`, elle clarifie l'intention avant lecture détaillée du corps.
5. `if not flag { give ErrState }` : cette test traite le cas d'erreur avant le calcul.
6. `unsafe { asm("hlt") }` : cette ligne marque une zone sensible qui doit rester courte, justifiée et facile à auditer dans un contexte système.
7. `give Ok` : la sortie est renvoyée immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable.
8. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: si `not flag` est vrai, la sortie devient `ErrState`.
- Cas nominal: sans test bloquante, la branche principale renvoie `Ok`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

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
1. `proc write_then_pause(m: SysMem, addr: int, v: int) -> SysResult {` : la règle est fixé pour `write_then_pause`: entrées `m: SysMem, addr: int, v: int` et sortie `SysResult`, elle clarifie l'intention avant lecture détaillée du corps.
2. `let w: SysResult = write_cell(m, addr, v)` : cette ligne crée la variable `w` de type `SysResult` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `match w {` : cette ligne démarre un dispatch stable sur `w`: une seule branche sera choisie selon la forme de la valeur analysée.
4. `case Ok {` : ce cas décrit `Ok` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
5. `cpu_pause()` : cette ligne définit une étape explicite du flux.
6. `give Ok` : retourne immédiatement `Ok` pour la branche courante, la sortie de branche est explicite et vérifiable.
7. `}` : cette accolade ferme le bloc logique.
8. `case ErrBounds { give ErrBounds }` : ce cas décrit `ErrBounds` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
9. `otherwise { give ErrState }` : cette ligne définit un chemin de secours explicite.
10. `}` : cette accolade ferme le bloc logique.
11. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: un test explicite du bloc gère les entrées hors règle avant le chemin nominal.
- Cas nominal: sans test bloquante, la branche principale renvoie `Ok`.
- Observation testable: forcer le cas `Ok` permet de confirmer la branche attendue.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

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

  // Boucle: progression controlee jusqu'a la borne
  loop {
    // Borne d'arret: stoppe la boucle de maniere explicite
    if i >= size { break }
    cells.push(0)
    set i = i + 1
  }

  give SysMem(Region(base, size), cells)
}
```

Lecture ligne par ligne (débutant):
1. `proc init_mem(base: int, size: int) -> SysMem {` : la règle est défini pour `init_mem`: entrées `base: int, size: int` et sortie `SysMem`, elle clarifie l'intention avant lecture détaillée du corps.
2. `let cells: int[] = []` : cette ligne crée la variable `cells` de type `int[]` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `let i: int = 0` : cette ligne crée la variable `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
4. `loop {` : cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`).
5. `if i >= size { break }` : cette test traite le cas d'erreur avant le calcul.
6. `cells.push(0)` : cette ligne définit une étape explicite du flux.
7. `set i = i + 1` : cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux.
8. `}` : cette accolade ferme le bloc logique.
9. `give SysMem(Region(base, size), cells)` : la branche renvoie immédiatement `SysMem(Region(base, size), cells)` pour la branche courante, la sortie de branche est explicite et vérifiable.
10. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: un test explicite du bloc gère les entrées hors règle avant le chemin nominal.
- Cas nominal: sans test bloquante, la branche principale renvoie `SysMem(Region(base, size), cells)`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

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
1. `entry main at sys/core {` : cette ligne fixe le point d'entrée `main` dans `sys/core` et sert de scénario exécutable de bout en bout pour le chapitre.
2. `let mem: SysMem = init_mem(100, 8)` : cette ligne crée la variable `mem` de type `SysMem` pour nommer explicitement une étape intermédiaire du raisonnement.
3. `let r1: SysResult = write_then_pause(mem, 103, 42)` : cette ligne crée la variable `r1` de type `SysResult` pour nommer explicitement une étape intermédiaire du raisonnement.
4. `let v1: int = read_cell(mem, 103)` : cette ligne crée la variable `v1` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement.
5. `if v1 == 42 { return 0 }` : cette test traite le cas d'erreur avant le calcul.
6. `return 70` : cette ligne termine l'exécution du bloc courant avec le code `70`, utile pour observer le résultat global du scénario.
7. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas d'erreur: un test explicite du bloc gère les entrées hors règle avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return 70`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `70`.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer un test explicite ou un chemin de secours stable.

Ce scénario fournit un test end-to-end minimal:
- init.
- write sécurisée.
- read bornée.
- assertion de sortie.

Erreurs fréquentes à éviter:
- valider le flux sans relire la valeur réellement écrite.
- tester seulement `Ok` sans cas `ErrBounds`.
- lier le succès à un effet externe non stable.

## 23.8 Plan de tests projet

Jeu minimal à couvrir:
1. Borne basse: adresse `base - 1` => rejet.
2. Borne haute: adresse `base + size` => rejet.
3. Cas nominal: adresse interne => write/read cohérente.
4. `cpu_halt_if(false)` : cette ligne définit une étape explicite du flux.
5. Reproductibilité: même séquence => même état final.

Erreurs fréquentes à éviter:
- ignorer les deux frontières de région.
- tester `unsafe` uniquement en succès.
- ne pas vérifier l'absence de mutation en cas d'erreur.

## 23.9 Règle système minimal

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
- code retour stable.
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
- règle d'entrée CLI.
- invariants de borne et d'atomicité.
- mapping métier `SysResult -> code de sortie`.

## 23.14 Encadré Observabilité

Exigences minimales:
- logs structurés (`ts`, `level`, `trace_id`, `op`, `addr`, `result`).
- code retour toujours cohérent avec le résultat métier.
- `trace_id` propagé de l'entrée à la sortie.

Format de log recommandé:
- `ts=<unix_ms> trace_id=<id> op=<read|write> addr=<n> result=<Ok|Err..> code=<n>`.

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
| `EINVAL` / invalid argument | refus immédiat de l'opération. | Entrée CLI incomplète ou hors règle. | Rejeter tôt avec message précis et code `2`. |
| `ETIMEDOUT` / timeout | appel bloqué puis échec. | Timeout absent ou trop grand. | Définir timeout borné et chemin de reprise stable. |
| `EIO` / I/O error | échec intermittent d'accès. | Couche I/O instable ou périphérique dégradé. | Remonter diagnostic, préserver atomicité, éviter mutation partielle. |

## Micro challenge final

Ajoutez un mode `dry-run` sans casser les invariants.

Contraintes:
- `dry-run` valide et journalise, mais ne mute jamais l'état.
- le mapping de sortie reste identique au mode réel pour les erreurs de validation.
- les tests nominal/permission/not-found doivent passer dans les deux modes.

## À retenir

Un noyau système robuste se construit par couches lisibles: préconditions centralisées, accès bornés, `unsafe` minimal, composition stable. Plus le code est bas niveau, plus la règle doit être hautement explicite.

Critère pratique de qualité pour ce chapitre:
- vous pouvez prouver qu'aucune écriture hors borne n'est possible.
- vous pouvez pointer chaque zone `unsafe` en quelques secondes.
- vous savez rejouer un scénario système complet de façon stable.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: un test explicite ou un chemin de secours stable doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de test pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez le test ou la branche de secours réellement utilisée.

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

- `docs/book/chapters/keywords/asm.md`.
- `docs/book/chapters/keywords/at.md`.
- `docs/book/chapters/keywords/bool.md`.
- `docs/book/chapters/keywords/break.md`.
- `docs/book/chapters/keywords/case.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **projet guide système**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

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

Lecture ligne par ligne:
1. `form Region {` -> participe au déroulé du traitement.
2. `base: int` -> participe au déroulé du traitement.
3. `size: int` -> participe au déroulé du traitement.
4. `}` -> participe au déroulé du traitement.
5. `form SysMem {` -> participe au déroulé du traitement.
6. `region: Region` -> participe au déroulé du traitement.
7. `cells: int[]` -> participe au déroulé du traitement.
8. `}` -> participe au déroulé du traitement.
9. `pick SysResult {` -> participe au déroulé du traitement.
10. `case Ok` -> participe au déroulé du traitement.

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
// Scenario projet sys: execution complete et verifiable
space demo/projet-sys

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

Ici, l'objectif est de comprendre le chemin réel du programme, ligne par ligne, jusqu'au code de sortie.

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


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
