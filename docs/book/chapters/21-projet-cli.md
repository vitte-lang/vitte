# 21. Projet 1 : CLI robuste

Nous allons construire un outil de ligne de commande propre, avec parsing d’arguments, validation, et sorties lisibles. Ce projet sert de base à plusieurs chapitres. L’objectif n’est pas de faire un « outil parfait », mais un outil que vous pouvez expliquer et maintenir.

## Cahier des charges

Notre CLI, que l’on appellera `vitte-cat`, va :

Lire un fichier et l’écrire sur `stdout`. Offrir une option `--lines` pour limiter le nombre de lignes. Fournir des erreurs claires en cas de fichier manquant.

## Étape 1 : définir l’interface

Avant d’écrire la moindre ligne, décrivez l’interface en une phrase :

`vitte-cat <path>` affiche le contenu. `vitte-cat --lines 10 <path>` affiche les 10 premières lignes.

Une interface claire réduit la complexité de tout le reste.

## Étape 2 : parser les arguments

Utilisez la stdlib pour obtenir les arguments, puis validez :

```vit
use std/cli

entry main at core/app {
  let args = args()
  let _ = has_flag(args, "--help")
  return 0
}
```

Le parsing n’est pas un détail. C’est la première interaction avec l’utilisateur, et c’est souvent là que les bugs se cachent.

### Variante : valeurs par défaut

Décidez si `--lines` a une valeur par défaut. Si oui, documentez‑la et testez‑la. Les valeurs implicites sont utiles, mais dangereuses si elles ne sont pas expliquées.

### Erreur courante

Accepter `--lines -1` sans validation.

## Étape 3 : lire le fichier

Créez une fonction dédiée, courte, et testable. Le but est de séparer l’I/O de l’interface.

### Variante : support de `stdin`

Vous pouvez décider que `-` signifie « lire depuis stdin ». C’est un comportement classique, mais il doit être explicitement documenté.

## Étape 4 : gérer les erreurs

Chaque erreur doit expliquer :

Ce qui s’est passé. Où ça s’est passé. Quelle action est possible.

### Erreur courante

Retourner un code d’erreur sans message.

## Étape 5 : tests

Écrivez au moins trois tests :

Fichier existant. Fichier manquant. Option `--lines`.

### Variante : tests de performance

Testez un fichier volumineux pour vérifier que la mémoire n’explose pas.

## Étape 6 : documentation minimale

Une CLI sans `--help` est une CLI incomplète. Même une documentation courte évite des tickets et des bugs.

## À retenir

Un outil CLI fiable se juge à la qualité de ses erreurs. Un outil lisible est un outil qui survit à son auteur.


## Pas‑à‑pas détaillé

Écrire un parser d’arguments minimal. Définir un mode `--help` explicite. Implémenter la lecture de fichier en mode streaming. Ajouter la limite `--lines`. Ajouter des erreurs claires.

## Erreurs fréquentes

Ne pas valider les arguments numériques. Oublier de fermer le fichier. Écrire sur `stdout` des erreurs qui doivent aller sur `stderr`.

## Variantes avancées

Ajouter un mode `--bytes`. Supporter plusieurs fichiers et concaténer proprement. Ajouter des tests de performance.


## Code complet (version pédagogique)

Le code ci‑dessous est volontairement verbeux et commenté. Il privilégie la lisibilité. Les appels d’I/O sont schématiques : adaptez‑les aux APIs exactes de la stdlib.

```vit
use std/cli

proc parse_lines(args) -> int {
  // Trouver "--lines" et lire la valeur suivante.
  // Si absent, retourner 0 pour "pas de limite".
  return 0
}

proc read_all(path: str) -> str {
  // Lecture simple : à remplacer par l’API stdlib.
  return ""
}

proc print_lines(text: str, limit: int) {
  // Si limit == 0, tout imprimer.
  // Sinon, imprimer les N premières lignes.
}

entry main at core/app {
  let args = args()

  if has_flag(args, "--help") {
    // Afficher l’aide et sortir.
    return 0
  }

  let limit = parse_lines(args)
  let path = arg_or(args, 0, "")

  if path == "" {
    // Erreur explicite : chemin manquant.
    return 1
  }

  let text = read_all(path)
  print_lines(text, limit)
  return 0
}
```

### Pourquoi ce style

Chaque fonction fait une seule chose. Les erreurs sont gérées tôt. Les noms racontent l’intention.

### À améliorer ensuite

Gestion de `stdin`. Limiter la mémoire en streaming. Codes de sortie distincts.

## Atelier : durcir l’outil

Ajoutez ces comportements :

`--lines` doit refuser les valeurs négatives. `--lines` doit refuser les valeurs non numériques. Un message d’erreur doit aller sur `stderr`.

Le but est d’apprendre que la robustesse se construit par petites décisions.


## Code complet (API actuelle)

Ce code utilise les modules `std/cli`, `std/io/print`, `std/io/buffer` et `std/kernel/fs`.

```vit
use std/cli
use std/io/print
use std/io/buffer
use std/kernel/fs
use std/core/types.usize
use std/core/types.i32

proc parse_usize(s: string) -> i32 {
  let i: i32 = 0
  let n: i32 = 0
  if s.len == 0 { give -1 }
  loop {
    if i >= s.len as i32 { break }
    let ch = s.slice(i as usize, (i + 1) as usize)
    if ch < "0" || ch > "9" { give -1 }
    n = n * 10 + (ch.as_bytes()[0] as i32 - 48)
    i = i + 1
  }
  give n
}

proc first_path(args: [string]) -> string {
  let i: i32 = 1
  loop {
    if i >= args.len as i32 { break }
    let cur = args[i as usize]
    if cur == "--lines" {
      i = i + 2
      continue
    }
    if cur.len > 8 && cur.slice(0, 8) == "--lines=" {
      i = i + 1
      continue
    }
    if cur.len > 0 && cur.slice(0, 1) == "-" {
      i = i + 1
      continue
    }
    give cur
  }
  give ""
}

proc read_lines(path: string, limit: i32) -> i32 {
  let fd = fs.open_read(path)
  if fd < 0 {
    let _ = eprintln("error: cannot open " + path)
    give 1
  }
  let r = buffer.reader_new(fd as usize, 4096)
  let count: i32 = 0
  loop {
    let line = buffer.read_line(&r)
    if line.len == 0 { break }
    println_or_panic(line)
    if limit > 0 {
      count = count + 1
      if count >= limit { break }
    }
  }
  fs.close(fd)
  give 0
}

entry main at core/app {
  let args = args()
  if has_flag(args, "--help") {
    println_or_panic("usage: vitte-cat [--lines N] <path>")
    give 0
  }
  let lines_str = flag_value(args, "--lines", "")
  let limit: i32 = 0
  if lines_str.len > 0 {
    let v = parse_usize(lines_str)
    if v < 0 {
      let _ = eprintln("error: invalid --lines value")
      give 2
    }
    limit = v
  }
  let path = first_path(args)
  if path.len == 0 {
    let _ = eprintln("error: missing path")
    give 2
  }
  give read_lines(path, limit)
}
```

## API idéale (future)

`std/fs.read_to_string(path)`. `std/cli/app` pour parser les options et générer l’aide automatiquement. `std/io/lines(reader)` pour itérer sans ambiguïté sur les lignes vides.

