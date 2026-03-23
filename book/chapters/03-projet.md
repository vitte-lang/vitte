# 3. Structure d'un projet

Niveau: Fondations avancées

Prérequis: `book/chapters/02-philosophie.md`, `book/glossaire.md`.
Voir aussi: `book/chapters/04-syntaxe.md`.

## Problème Concret

Un projet devient vite confus quand les responsabilités sont mélangées.
Le but de ce chapitre: organiser le code pour savoir immédiatement où lire, où modifier, et où tester.

## Fil Rouge (Projet Unique)

On conserve le même scénario pour voir l'impact d'une bonne structure.
Vous allez suivre un chemin simple: entrée -> traitement -> résultat.

## Idée principale

On découpe le projet en 3 zones:
1. `domain`: données métier
2. `service`: règles de calcul
3. `io`: lancement et sortie

Flux conseillé: `io -> service -> domain`.

## 3.1 Domaine: définir les données métier

```vit
space app/domain

form Ticket {
  id: int
  priority: int
}
```

Lecture directe:
1. `Ticket` décrit les données utiles au métier.
2. aucune logique d'exécution ici.
3. ce module reste stable même si l'application change de canal (CLI, HTTP, job).

## 3.2 Service: centraliser les règles métier

```vit
space app/service

pull app/domain as d

proc is_critical(t: d.Ticket) -> bool {
  give t.priority >= 9
}
```

Lecture directe:
1. `service` utilise le type de `domain`.
2. la règle métier est écrite une seule fois.
3. la sortie est booléenne: vrai ou faux.

Exemple:
- priorité `9` -> `true`
- priorité `4` -> `false`

## 3.3 IO: lancer le flux et produire la sortie

```vit
space app/io

pull app/domain as d
pull app/service as s

// Point d'entree du scenario
entry main at core/app {
  let t: d.Ticket = d.Ticket(1, 9)
  let critical: bool = s.is_critical(t)
  let code: int = critical as int

  return code
}
```

Lecture directe:
1. `main` construit une entrée.
2. `main` appelle le service.
3. `main` convertit le résultat en code de sortie.

À éviter dans `io`:
- écrire les règles métier ici
- dupliquer la logique déjà présente dans `service`

## 3.4 Signaux d'une mauvaise structure

1. une règle métier est écrite dans `entry`
2. `service` dépend de `io`
3. un type métier contient des détails techniques de sortie

Si un de ces signaux apparaît, la maintenance coûtera plus cher.

## 3.5 Test rapide de structure (30 secondes)

Pour chaque fichier, posez 3 questions:
1. est-ce une donnée métier ? -> `domain`
2. est-ce une règle métier ? -> `service`
3. est-ce du lancement/sortie ? -> `io`

Si un fichier répond à plusieurs questions, il faut le découper.

## 3.6 Check-list avant commit

1. le sens des dépendances est-il `io -> service -> domain` ?
2. les règles sont-elles dans `service` ?
3. les types métier sont-ils dans `domain` ?
4. `entry` reste-t-il court et lisible ?

## Keywords à revoir

- `book/keywords/space.md`
- `book/keywords/pull.md`
- `book/keywords/form.md`
- `book/keywords/proc.md`
- `book/keywords/entry.md`

## Exemple Étendu

```vit
// Scenario projet: execution complete et verifiable
space demo/projet

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


## Lecture du flux complet

Ordre d'exécution:
1. `main` construit `req`.
2. `parse_request` contrôle les données.
3. `apply_policy` applique la règle métier.
4. `persist_sim` simule un traitement final.
5. `to_exit` produit le code de sortie.

Exécution normale:
- entrée: `Request(7, 12, 15)`
- sortie finale: `0`

Exécution d'erreur 1:
- entrée avec `id = 0`
- sortie finale: `91`

Exécution d'erreur 2:
- entrée avec `amount = 3`
- sortie finale: `94`

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

| Besoin | Option 1 | Option 2 | Choix conseillé |
| --- | --- | --- | --- |
| Lecture rapide | Séparation en 3 zones | Tout dans un fichier | Option 1 |
| Debug rapide | Fonctions ciblées | Gros flux mélangé | Option 1 |
| Évolution sûre | Dépendances orientées | Imports croisés | Option 1 |

## Diagnostic Rapide

| Problème observé | Cause probable | Comment vérifier | Correction |
| --- | --- | --- | --- |
| Résultat imprévisible | règles dispersées | tracer les appels | regrouper dans `service` |
| Fichier difficile à lire | responsabilités mélangées | relire rôle du module | séparer `domain/service/io` |
| Tests fragiles | logique dans `entry` | isoler la logique | déplacer dans `proc` |

## Checkpoint

À ce stade, vous devez savoir:
- placer un fichier au bon endroit,
- suivre le flux d'un projet sans confusion,
- expliquer la sortie à partir des fonctions appelées.

## Mini Étude De Cas (Avant / Après)

Avant: un seul fichier mélange entrée, règles et sortie.
Après: `domain` décrit les données, `service` décide, `io` lance.
Résultat: lecture plus rapide, correction plus simple, tests plus stables.

## Ce Que Je Ferais En Revue De Code

1. vérifier le sens des dépendances (`io -> service -> domain`)
2. vérifier que les règles ne sont pas dans `entry`
3. vérifier un scénario normal + un scénario d'erreur
