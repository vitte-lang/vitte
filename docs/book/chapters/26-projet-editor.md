# 26. Projet 6 : éditeur de code en terminal

Ce chapitre est un tutoriel long, narratif, et volontairement pédagogique. L’objectif n’est pas de battre `vim` ou `nano`, mais de comprendre ce qu’implique un éditeur en terminal. Vous allez construire un “éditeur minimal” qui ouvre un fichier, affiche les lignes, permet de déplacer un curseur, d’insérer du texte, et d’enregistrer le résultat. Chaque étape est un petit contrat entre votre code et l’utilisateur.

Ce projet est important parce qu’il combine tout ce que vous avez appris : I/O, parsing, état interne, gestion d’erreurs, et clarté du flux. Il est aussi parfait pour pratiquer une règle simple : un éditeur qui perd des données est un bug impardonnable.

## Note éditoriale (style O’Reilly)

Un projet comme celui-ci se juge à la qualité de son récit. Chaque étape doit dire trois choses : ce que l’on fait, pourquoi on le fait, et comment on saura que c’est correct. Si une section ne respecte pas ces trois questions, elle est trop courte ou trop technique. Gardez en tête que vous écrivez pour un lecteur intelligent mais occupé. Il veut comprendre vite, mais il ne veut pas deviner.

## Prérequis et contraintes

Dans l’état actuel de la stdlib, nous disposons d’outils bas niveau (lecture/écriture, buffers, terminal minimal). Nous ne disposons pas encore d’un mode “raw” complet qui désactive l’écho et les raccourcis du terminal. Nous allons donc construire un éditeur « ligne par ligne » qui fonctionne en mode standard. Le projet reste pertinent : il vous apprend à structurer l’état, la logique de rendu, et la persistance.

À la fin du chapitre, vous aurez aussi une version “API idéale” qui montre comment vous adapter lorsque le mode raw sera disponible.

## Étape 1 : définir le contrat utilisateur

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

Avant d’écrire du code, écrivez le contrat en une page. Un éditeur terminal minimal doit répondre à quatre questions simples :

Comment ouvrir un fichier ? Comment se déplacer ? Comment modifier ? Comment sauvegarder ?

Pour rester réaliste, nous choisissons un ensemble minimal de commandes textuelles :

`open <path>` ouvre un fichier. `show` affiche les lignes avec numéros. `ins <line> <text>` insère une ligne. `del <line>` supprime une ligne. `save` enregistre. `quit` quitte.

Ce choix n’est pas “moderne”, mais il est robuste et compatible avec les APIs existantes. La clarté d’abord.

## Étape 2 : modèle de données

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

Un éditeur, c’est d’abord un état. Nous avons besoin :

d’un chemin de fichier (optionnel),. d’un tableau de lignes,. d’un indicateur “modifié” pour savoir si on doit prévenir l’utilisateur.

Nous allons utiliser une structure simple. Le but est que chaque champ ait une responsabilité unique.

```vit
form Editor {
path: string
lines: [string]
dirty: bool
}
```

L’état `dirty` est crucial. Il n’est pas décoratif. Il représente un contrat moral : ne jamais perdre un travail non sauvegardé.

## Étape 3 : chargement du fichier

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

Dans un monde idéal, on lirait tout le fichier en mémoire et on le découperait en lignes. La stdlib fournit des primitives pour lire un flux, et un buffer pour lire ligne par ligne. Nous allons utiliser `std/io/buffer` et `std/kernel/fs`.

L’approche est simple :

Ouvrir le fichier en lecture. Lire ligne par ligne. Stocker chaque ligne dans `lines`. Fermer le fichier.

Le code suivant est volontairement clair, sans optimisation prématurée.

```vit
use std/io/buffer
use std/kernel/fs
use std/io/print

proc load_file(path: string) -> [string] {
let out: [string] = []
let fd = fs.open_read(path)
if fd < 0 {
let _ = eprintln("error: cannot open " + path)
give out
}
let r = buffer.reader_new(fd as usize, 4096)
loop {
let line = buffer.read_line(&r)
if line.len == 0 { break }
set out = out.push(line)
}
fs.close(fd)
give out
}
```

Notez l’usage de `eprintln`. Les erreurs doivent être visibles pour l’utilisateur, et séparées de la sortie normale.

## Étape 4 : affichage

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

Un éditeur sans affichage n’est qu’un parseur. Nous allons écrire une fonction `show` qui affiche chaque ligne avec son numéro. Cette étape semble triviale, mais elle définit le format que l’utilisateur va apprendre à reconnaître.

La règle : le format doit être stable et lisible. Un utilisateur doit pouvoir lire “3: let x = 1” et comprendre immédiatement qu’il s’agit de la ligne 3.

```vit
use std/io/print

proc show(lines: [string]) {
let i: i32 = 0
loop {
if i >= lines.len as i32 { break }
println_or_panic(i.to_string() + ": " + lines[i as usize])
i = i + 1
}
}
```

Si `to_string` n’est pas disponible pour votre type, remplacez‑le par une conversion manuelle ou un petit helper. L’important n’est pas la conversion, mais la stabilité de la sortie.

## Étape 5 : parser des commandes

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

Nous allons maintenant lire des commandes ligne par ligne. Le protocole est simple : une ligne d’entrée est une commande. Le parsing est naïf : on découpe par espaces, on prend le premier mot comme verbe.

Cette approche a deux avantages :

Elle est simple à expliquer. Elle est suffisamment robuste pour ce projet.

Dans un projet réel, on utiliserait un parseur plus strict, mais ici la clarté est l’objectif.

```vit
proc split_words(s: string) -> [string] {
// Version simple : découper sur les espaces.
// À implémenter selon vos helpers disponibles.
give []
}
```

L’important n’est pas l’algorithme, mais le contrat : une commande = un verbe + des arguments.

## Étape 6 : insertion et suppression

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

L’insertion consiste à insérer une ligne à un index donné. La suppression consiste à retirer une ligne. Ce sont deux opérations qui semblent simples, mais qui provoquent la majorité des bugs d’index.

Règle simple : valider les indices, toujours. Un `ins 999` sur un fichier de 10 lignes doit être rejeté proprement.

```vit
proc insert_line(lines: [string], idx: i32, text: string) -> [string] {
if idx < 0 { give lines }
if idx > lines.len as i32 { give lines }
let out: [string] = []
let i: i32 = 0
loop {
if i >= lines.len as i32 { break }
if i == idx {
set out = out.push(text)
}
set out = out.push(lines[i as usize])
i = i + 1
}
if idx == lines.len as i32 {
set out = out.push(text)
}
give out
}

proc delete_line(lines: [string], idx: i32) -> [string] {
if idx < 0 { give lines }
if idx >= lines.len as i32 { give lines }
let out: [string] = []
let i: i32 = 0
loop {
if i >= lines.len as i32 { break }
if i != idx {
set out = out.push(lines[i as usize])
}
i = i + 1
}
give out
}
```

Le code ci‑dessus est volontairement clair et verbeux. Un éditeur est un lieu où la simplicité est un avantage.

## Étape 7 : sauvegarde

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

La sauvegarde doit être fiable, même si elle est lente. Le principe minimal :

Ouvrir le fichier en écriture. Écrire chaque ligne + un `\n`. Fermer le fichier.

En l’absence d’un API de “write text file” haut niveau, nous allons simplement décrire la logique. Si vous avez accès aux primitives d’écriture (`std/io/write`), adaptez le code. L’important est de comprendre le flux.

## Étape 8 : boucle principale

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

L’éditeur est une boucle :

Lire une commande. Exécuter. Afficher si nécessaire.

Cette boucle doit être lisible. Elle est la pièce maîtresse de l’éditeur.

```vit
entry main at core/app {
let editor = Editor(path = "", lines = [], dirty = false)
loop {
// Lire une commande.
// Parser.
// Appliquer.
// Mettre dirty si besoin.
}
}
```

## Étape 9 : gestion des erreurs

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

Un éditeur ne doit jamais perdre de données. Ajoutez une confirmation quand `dirty == true` et que l’utilisateur demande `quit`. C’est une règle morale, pas un détail technique.

## Étape 10 : améliorer la lisibilité

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

Même dans un projet simple, ajoutez des paragraphes de documentation dans le code. Un éditeur est un outil de lecture ; il mérite un code lisible.

## API idéale (future)

Quand le mode raw et la gestion fine du terminal seront disponibles, vous pourrez ajouter :

Mouvement de curseur en temps réel. Rendu plein écran. Recherche incrémentale. Undo/redo.

Ce chapitre vous prépare à ces étapes : vous avez déjà séparé l’état, le rendu, et la logique.

## Conclusion

Vous avez construit un éditeur minimal, lisible, et sûr. Ce n’est pas un exercice de performance, c’est un exercice de clarté. Un éditeur est un miroir : si votre code est confus, l’expérience utilisateur sera confuse. Vous avez donc appris une leçon essentielle : la lisibilité est une fonctionnalité.


## Étape 11 : parser robuste des commandes

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

Jusqu’ici, nous avons décrit un parsing « à la main ». Nous allons maintenant écrire un parseur minimal mais complet : il découpe une ligne en mots, gère les espaces multiples, et renvoie un tableau de tokens. Ce n’est pas un parseur riche, mais c’est un parseur fiable. Il doit faire une chose et la faire bien : transformer une chaîne en arguments.

Le principe est simple : avancer dans la chaîne, ignorer les espaces, puis capturer un mot jusqu’au prochain espace. Cette approche donne un comportement stable, prévisible, et facile à documenter pour l’utilisateur.

```vit
proc split_words(s: string) -> [string] {
let out: [string] = []
let i: usize = 0
loop {
if i >= s.len { break }
// ignorer les espaces
loop {
if i >= s.len { break }
let ch = s.slice(i, i + 1)
if ch != " " { break }
i = i + 1
}
if i >= s.len { break }
let start = i
loop {
if i >= s.len { break }
let ch = s.slice(i, i + 1)
if ch == " " { break }
i = i + 1
}
let word = s.slice(start, i)
set out = out.push(word)
}
give out
}
```

Ce code est volontairement explicite. Le lecteur doit pouvoir comprendre la logique d’un coup d’œil. Le parsing n’est pas un lieu pour l’astuce.

## Étape 12 : sauvegarde réelle (API actuelle)

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

Un éditeur existe pour sauver des modifications. Nous allons maintenant écrire une sauvegarde réelle, basée sur les APIs existantes. La règle est simple : écrire les lignes une par une, en ajoutant un `\n` à chaque ligne, puis fermer correctement le fichier. Cela donne un résultat stable et compatible avec les outils classiques.

```vit
use std/kernel/fs
use std/io/write
use std/io/print
use std/core/result.Result

proc save_file(path: string, lines: [string]) -> bool {
let fd = fs.open_write(path)
if fd < 0 {
let _ = eprintln("error: cannot write " + path)
give false
}
let i: i32 = 0
loop {
if i >= lines.len as i32 { break }
let line = lines[i as usize] + "\n"
let r = write.write_string_fd(fd as usize, line)
when r is Result.Err {
let _ = eprintln("error: write failed")
fs.close(fd)
give false
}
i = i + 1
}
fs.close(fd)
give true
}
```

Cette version est lente pour de très gros fichiers, mais elle est robuste. Et c’est exactement ce que nous voulons dans un éditeur minimal : la fiabilité d’abord.

## Étape 13 : boucle complète (API actuelle)

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

Nous avons maintenant tous les morceaux nécessaires. Il nous faut une boucle principale qui :

lit une ligne de commande,. parse les mots,. applique l’action,. met à jour l’état.

Voici une version claire et structurée :

```vit
use std/io/buffer
use std/io/stdin
use std/io/print

proc run(editor: *Editor) {
let s = stdin.stdin()
let r = buffer.reader_new(s.fd, 4096)
loop {
println_or_panic("> ")
let line = buffer.read_line(&r)
if line.len == 0 { continue }
let words = split_words(line)
if words.len == 0 { continue }
let cmd = words[0]

if cmd == "open" {
if words.len < 2 { let _ = eprintln("usage: open <path>"); continue }
editor.path = words[1]
editor.lines = load_file(editor.path)
editor.dirty = false
continue
}

if cmd == "show" {
show(editor.lines)
continue
}

if cmd == "ins" {
if words.len < 3 { let _ = eprintln("usage: ins <line> <text>"); continue }
let idx = parse_usize(words[1])
if idx < 0 { let _ = eprintln("invalid index"); continue }
editor.lines = insert_line(editor.lines, idx, words[2])
editor.dirty = true
continue
}

if cmd == "del" {
if words.len < 2 { let _ = eprintln("usage: del <line>"); continue }
let idx = parse_usize(words[1])
if idx < 0 { let _ = eprintln("invalid index"); continue }
editor.lines = delete_line(editor.lines, idx)
editor.dirty = true
continue
}

if cmd == "save" {
if editor.path.len == 0 { let _ = eprintln("no file opened"); continue }
if save_file(editor.path, editor.lines) { editor.dirty = false }
continue
}

if cmd == "quit" {
if editor.dirty { let _ = eprintln("unsaved changes; use save") ; continue }
break
}

let _ = eprintln("unknown command")
}
}
```

Ce code est long, mais il est lisible. Chaque commande est un bloc clair. Le lecteur peut suivre le flux sans effort. Dans un livre O’Reilly, la lisibilité est un objectif, pas une conséquence.

## Étape 14 : mode raw (API idéale)

Dans cette étape, l’objectif est de transformer une idée en une règle simple et lisible.

Quand un vrai mode raw sera disponible, vous pourrez remplacer la lecture « ligne par ligne » par une lecture caractère par caractère. Cela change toute l’expérience : le curseur peut bouger en temps réel, la ligne se redessine, l’éditeur se rapproche d’un outil moderne.

Une version idéale inclurait :

un mode raw activé au démarrage,. un `render()` qui réécrit tout l’écran,. un `handle_key()` qui traite les flèches et l’insertion.

L’architecture que vous avez construite reste valide : elle sépare l’état, le rendu, et la logique. C’est précisément la bonne base pour évoluer vers un éditeur complet.

## Conclusion

Vous avez maintenant un éditeur terminal fonctionnel, même dans un environnement minimal. Vous avez appris que la qualité d’un éditeur ne se mesure pas au nombre de fonctionnalités, mais à la confiance qu’il inspire. Cette confiance vient d’une seule chose : un code clair, qui dit exactement ce qu’il fait.


### Petit helper pour les indices

Pour convertir un argument de ligne en nombre, nous réutilisons un parseur simple. Il refuse les valeurs non numériques et renvoie `-1` en cas d’erreur.

```vit
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
```


## Code complet (version compacte)

Ce bloc rassemble tout pour un éditeur minimal “ligne par ligne”. Il est volontairement long, mais chaque partie est lisible. Adaptez les détails selon les APIs exactes disponibles dans votre environnement.

```vit
use std/cli
use std/io/print
use std/io/buffer
use std/io/stdin
use std/kernel/fs
use std/io/write
use std/core/types.usize
use std/core/types.i32
use std/core/result.Result

form Editor {
path: string
lines: [string]
dirty: bool
}

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

proc split_words(s: string) -> [string] {
let out: [string] = []
let i: usize = 0
loop {
if i >= s.len { break }
loop {
if i >= s.len { break }
let ch = s.slice(i, i + 1)
if ch != " " { break }
i = i + 1
}
if i >= s.len { break }
let start = i
loop {
if i >= s.len { break }
let ch = s.slice(i, i + 1)
if ch == " " { break }
i = i + 1
}
let word = s.slice(start, i)
set out = out.push(word)
}
give out
}

proc load_file(path: string) -> [string] {
let out: [string] = []
let fd = fs.open_read(path)
if fd < 0 {
let _ = eprintln("error: cannot open " + path)
give out
}
let r = buffer.reader_new(fd as usize, 4096)
loop {
let line = buffer.read_line(&r)
if line.len == 0 { break }
set out = out.push(line)
}
fs.close(fd)
give out
}

proc save_file(path: string, lines: [string]) -> bool {
let fd = fs.open_write(path)
if fd < 0 {
let _ = eprintln("error: cannot write " + path)
give false
}
let i: i32 = 0
loop {
if i >= lines.len as i32 { break }
let line = lines[i as usize] + "\n"
let r = write.write_string_fd(fd as usize, line)
when r is Result.Err {
let _ = eprintln("error: write failed")
fs.close(fd)
give false
}
i = i + 1
}
fs.close(fd)
give true
}

proc show(lines: [string]) {
let i: i32 = 0
loop {
if i >= lines.len as i32 { break }
println_or_panic(i.to_string() + ": " + lines[i as usize])
i = i + 1
}
}

proc insert_line(lines: [string], idx: i32, text: string) -> [string] {
if idx < 0 { give lines }
if idx > lines.len as i32 { give lines }
let out: [string] = []
let i: i32 = 0
loop {
if i >= lines.len as i32 { break }
if i == idx { set out = out.push(text) }
set out = out.push(lines[i as usize])
i = i + 1
}
if idx == lines.len as i32 { set out = out.push(text) }
give out
}

proc delete_line(lines: [string], idx: i32) -> [string] {
if idx < 0 { give lines }
if idx >= lines.len as i32 { give lines }
let out: [string] = []
let i: i32 = 0
loop {
if i >= lines.len as i32 { break }
if i != idx { set out = out.push(lines[i as usize]) }
i = i + 1
}
give out
}

proc run(editor: *Editor) {
let s = stdin.stdin()
let r = buffer.reader_new(s.fd, 4096)
loop {
println_or_panic("> ")
let line = buffer.read_line(&r)
if line.len == 0 { continue }
let words = split_words(line)
if words.len == 0 { continue }
let cmd = words[0]

if cmd == "open" {
if words.len < 2 { let _ = eprintln("usage: open <path>"); continue }
editor.path = words[1]
editor.lines = load_file(editor.path)
editor.dirty = false
continue
}

if cmd == "show" { show(editor.lines); continue }

if cmd == "ins" {
if words.len < 3 { let _ = eprintln("usage: ins <line> <text>"); continue }
let idx = parse_usize(words[1])
if idx < 0 { let _ = eprintln("invalid index"); continue }
editor.lines = insert_line(editor.lines, idx, words[2])
editor.dirty = true
continue
}

if cmd == "del" {
if words.len < 2 { let _ = eprintln("usage: del <line>"); continue }
let idx = parse_usize(words[1])
if idx < 0 { let _ = eprintln("invalid index"); continue }
editor.lines = delete_line(editor.lines, idx)
editor.dirty = true
continue
}

if cmd == "save" {
if editor.path.len == 0 { let _ = eprintln("no file opened"); continue }
if save_file(editor.path, editor.lines) { editor.dirty = false }
continue
}

if cmd == "quit" {
if editor.dirty { let _ = eprintln("unsaved changes; use save") ; continue }
break
}

let _ = eprintln("unknown command")
}
}

entry main at core/app {
let editor = Editor(path = "", lines = [], dirty = false)
run(&editor)
give 0
}
```

## Mini‑framework d’UI terminal (API idéale)

Ici, on bascule dans une approche “éditeur interactif”. Le but est de séparer l’UI en trois couches : l’état, le rendu, et l’entrée clavier. Même si l’API raw n’est pas encore disponible, le design est prêt.

### 1) État d’UI

On ajoute un champ `cursor` et un viewport :

```vit
form Cursor {
row: i32
col: i32
}

form Viewport {
row: i32
height: i32
}

form EditorUI {
editor: Editor
cursor: Cursor
view: Viewport
}
```

Le curseur est un état. Le viewport est un état. Cette séparation évite la confusion et rend l’affichage déterministe.

### 2) Rendu

Le rendu doit être une fonction pure : “à partir de l’état, produire une sortie”. En mode raw, vous effacez l’écran et réécrivez tout.

Pseudo‑code :

```text
render(ui):
clear_screen()
draw_lines(ui.editor.lines, ui.view)
draw_status(ui)
move_cursor(ui.cursor)
```

Cette approche garantit que l’écran est toujours cohérent. La règle d’or : ne jamais dépendre d’un “état implicite” du terminal.

### 3) Entrée clavier

L’entrée doit produire des événements : `Key.Up`, `Key.Down`, `Key.Char`, etc. Chaque événement met à jour l’état. L’UI ne “dessine” pas directement, elle change l’état, puis le rendu s’applique.

Pseudo‑code :

```text
loop:
key = read_key()
ui = handle_key(ui, key)
render(ui)
```

### 4) Barres et messages

Une barre de statut est un élément clé d’un bon éditeur. Elle doit indiquer :

le nom du fichier,. le nombre de lignes,. l’état “dirty”.

Cette barre rend l’éditeur “parlant” et réduit les erreurs utilisateur.

### 5) Scrolling

Le scrolling est l’un des premiers pièges. Vous devez l’implémenter explicitement :

si le curseur sort du viewport, décalez le viewport. si le curseur remonte, remontez le viewport.

Cette logique est purement mathématique. Elle est aussi la source de beaucoup de bugs subtils si elle est cachée.

### 6) Rendu incrémental (plus tard)

Une fois tout stable, vous pourrez optimiser en ne redessinant que ce qui change. Mais ne faites pas cette optimisation trop tôt : la clarté prime.

## Conclusion

Le mini‑framework d’UI vous donne la structure nécessaire pour évoluer vers un éditeur complet. Vous avez une base claire : état, rendu, entrée. Ce triptyque est la colonne vertébrale de tout éditeur terminal moderne.


## Erreurs courantes et diagnostics (édition terminal)

Cette section est volontairement longue. Un éditeur terminal est une machine à bugs si vous ne prenez pas au sérieux les frontières entre l’état, l’entrée et la sortie. Les erreurs suivantes reviennent presque toujours, même chez des développeurs expérimentés.

### 1) Index hors bornes

Symptôme : une insertion ou suppression casse l’ordre des lignes, ou provoque un crash silencieux. Cause : un index non validé, souvent parce qu’on fait confiance aux entrées utilisateur. La règle est simple : toute commande qui contient un index doit être validée. Si l’index est invalide, l’éditeur doit répondre avec un message explicite, pas un silence.

Diagnostic : ajoutez un message d’erreur qui indique l’index reçu et la taille actuelle. Ce n’est pas une excuse, c’est un outil de compréhension.

### 2) Perte de données par omission

Symptôme : l’utilisateur tape `quit` et perd son travail. Cause : le flag `dirty` n’est pas mis à jour, ou la sortie n’est pas conditionnée à la sauvegarde. Ce bug est plus grave qu’un crash : il casse la confiance.

Diagnostic : forcez une confirmation quand `dirty == true`. Même un éditeur minimal doit respecter ce contrat moral.

### 3) Boucle principale illisible

Symptôme : les commandes s’enchaînent avec des `if` imbriqués et la logique devient opaque. Cause : on mélange parsing, exécution et affichage.

Diagnostic : séparez “parser” et “exécuter”. Ajoutez une fonction `run_command(editor, command)` qui ne fait qu’une chose. La lisibilité devient votre meilleur outil de debug.

### 4) Confusion entre stdout et stderr

Symptôme : les erreurs se mélangent à la sortie normale, surtout quand l’éditeur est utilisé dans un script. Cause : on utilise `println` pour tout.

Diagnostic : utilisez `eprintln` pour les erreurs, `println` pour la sortie normale. Ce n’est pas un détail : c’est une convention universelle.

### 5) Parser trop permissif

Symptôme : une commande mal formée est interprétée comme autre chose. Cause : un parsing “au hasard”.

Diagnostic : quand une commande est invalide, répondez avec un `usage:`. Le but n’est pas de faire plaisir, mais de réduire les ambiguïtés.

### 6) Fichier ouvert mais jamais sauvegardé

Symptôme : après plusieurs insertions, `save` échoue en silence ou ne fait rien. Cause : vous avez oublié de stocker `path` lors de `open`.

Diagnostic : assurez-vous que chaque commande qui ouvre un fichier met à jour `editor.path`.

### 7) Rendu incohérent (mode raw futur)

Symptôme : le curseur saute, l’écran contient des artefacts. Cause : rendu partiel ou “incremental” sans cohérence globale.

Diagnostic : en phase 1, redessinez tout l’écran à chaque étape. C’est plus lent, mais beaucoup plus robuste.

### 8) Scrolling mal géré

Symptôme : le curseur disparaît hors écran, ou les lignes ne correspondent plus aux numéros. Cause : viewport non mis à jour.

Diagnostic : maintenez un invariant explicite : `cursor.row` doit toujours être dans `[view.row, view.row + view.height)`.

### 9) Sauvegarde non atomique

Symptôme : le fichier est partiellement écrit si un crash arrive au mauvais moment. Cause : écriture directe.

Diagnostic : en version avancée, écrivez dans un fichier temporaire puis remplacez. Même un éditeur simple peut le faire.

### 10) Format de fichier “surprise”

Symptôme : chaque `save` ajoute une ligne vide ou change les fins de ligne. Cause : ajout systématique de `\n` sans vérifier l’état.

Diagnostic : choisissez un format stable. Si vous forcez `\n` à la fin, dites-le clairement. La stabilité est préférable à la magie.


**À retenir**

Gardez la règle simple et visible. Une erreur doit être actionnable, pas silencieuse. La lisibilité est une fonctionnalité.

## Pas à pas par étapes (version narrative)

Cette section présente un parcours chronologique, comme si vous écriviez l’éditeur par petites étapes cohérentes. Chaque étape est petite, explicable, et testable. C’est un excellent exercice de discipline : vous apprenez à découper un problème en livrables clairs.

### Étape 1 : squelette minimal

Objectif : un programme qui compile et affiche un message. Ce commit ne fait rien d’utile, mais il établit la structure du projet.

### Étape 2 : structure `Editor`

Objectif : introduire l’état interne (`path`, `lines`, `dirty`). À ce stade, l’état ne sert à rien, mais il formalise la direction.

### Étape 3 : lecture d’un fichier

Objectif : `load_file(path)` qui retourne des lignes. Ici, vous validez l’I/O et les erreurs de base.

### Étape 4 : affichage `show`

Objectif : afficher les lignes avec numéros. Vous formalisez le format de sortie.

### Étape 5 : parsing des commandes

Objectif : `split_words` + lecture d’une commande depuis stdin. Vous pouvez déjà écrire `show`.

### Étape 6 : insertion et suppression

Objectif : `ins` et `del` fonctionnent et modifient l’état. Le flag `dirty` est mis à jour.

### Étape 7 : sauvegarde

Objectif : `save` écrit réellement le fichier. Ici, la robustesse devient centrale.

### Étape 8 : sécurité de sortie

Objectif : empêcher `quit` si `dirty == true`. Ce commit protège l’utilisateur.

### Étape 9 : messages d’erreur propres

Objectif : `usage:` pour chaque commande invalide. Vous améliorez la “qualité de dialogue” de l’outil.

### Étape 10 : refactoring de la boucle

Objectif : extraire `run_command` ou des petites fonctions par commande. La lisibilité devient durable.

### Étape 11 : préparation au mode raw

Objectif : séparer “rendu” et “entrée” dans la structure, même si l’implémentation reste ligne par ligne.

### Étape 12 : documentation finale

Objectif : écrire un README court et stable qui explique l’usage. Un éditeur est aussi une documentation vivante.

Chaque commit ci‑dessus peut être un mini‑chapitre. Le résultat final n’est pas seulement un éditeur : c’est une démonstration de méthode.


## Mode exercice (avec corrigés)

Cette section propose des exercices guidés. Chaque exercice est formulé comme une petite mission, suivie d’un corrigé détaillé. Le but est de créer un vrai dialogue avec le lecteur, comme dans un manuel O’Reilly : l’exercice n’est pas un piège, c’est un outil d’apprentissage.

### Exercice 1 : index sûr

**Mission** : modifiez `insert_line` et `delete_line` pour qu’elles renvoient un `Result` au lieu de silencieusement ignorer les indices invalides.

**Pourquoi** : un échec silencieux est une dette. Un `Result` rend l’erreur visible et actionnable.

**Corrigé (principe)** : introduisez un `Result[ [string], string ]` et renvoyez `Err("index out of range")` si l’index est invalide. Dans `run`, affichez ce message sur `stderr`.

### Exercice 2 : refuser les modifications sans fichier

**Mission** : si aucun fichier n’a été ouvert, refusez `ins` et `del`.

**Pourquoi** : un éditeur minimal doit protéger l’utilisateur contre la confusion. Modifier sans fichier, c’est une erreur d’intention.

**Corrigé (principe)** : vérifiez `editor.path.len == 0` avant d’autoriser les modifications. Affichez un message clair.

### Exercice 3 : message de statut

**Mission** : après chaque commande, affichez un message court qui indique l’état `dirty`.

**Pourquoi** : un éditeur “parle” à l’utilisateur. Ce feedback réduit les erreurs de sauvegarde.

**Corrigé (principe)** : ajoutez une fonction `status(editor)` qui affiche `saved` ou `modified`.

### Exercice 4 : mode lecture seule

**Mission** : ajoutez une commande `readonly` qui empêche toute modification.

**Pourquoi** : cela vous force à centraliser les autorisations d’écriture.

**Corrigé (principe)** : ajoutez un champ `readonly: bool` dans `Editor`. Vérifiez ce champ avant `ins`/`del`.

### Exercice 5 : undo simple

**Mission** : implémentez un undo minimal, une seule étape.

**Pourquoi** : l’undo est un excellent exercice d’état. Même une version simple améliore la confiance.

**Corrigé (principe)** : gardez une copie des lignes avant chaque modification. Une commande `undo` remplace l’état courant.

## Tests manuels guidés (scénarios utilisateur)

Ici, on remplace les “tests automatiques” par des scénarios humains. Pour un éditeur terminal, ces tests sont précieux. Ils vous apprennent à penser en termes d’expérience utilisateur, pas seulement d’API.

### Scénario 1 : ouverture et lecture

Lancez l’éditeur. Tapez `open README.md`. Tapez `show`.

**Attendu** : les lignes du fichier apparaissent avec leur numéro. Le programme ne doit pas crasher si le fichier est vide.

### Scénario 2 : insertion simple

`open notes.txt`. `ins 0 Bonjour`. `show`.

**Attendu** : la ligne 0 contient “Bonjour”. Le flag `dirty` est vrai.

### Scénario 3 : suppression

`del 0`. `show`.

**Attendu** : la ligne 0 a disparu. Le flag `dirty` est vrai.

### Scénario 4 : sauvegarde

`save`. Ouvrez le fichier dans un autre outil.

**Attendu** : le contenu correspond à l’écran. Le flag `dirty` revient à faux.

### Scénario 5 : sortie sécurisée

Modifiez une ligne. Tapez `quit`.

**Attendu** : le programme refuse de quitter et affiche un message d’avertissement.

### Scénario 6 : erreurs d’index

`ins 999 X`. `del 999`.

**Attendu** : des messages d’erreur clairs, sans crash.

### Scénario 7 : fichier manquant

`open no_such_file.txt`.

**Attendu** : un message d’erreur explicite, et aucun crash.

### Scénario 8 : usage incorrect

`ins`. `del`. `open`.

**Attendu** : chaque commande affiche `usage: ..`.

### Scénario 9 : stress simple

Insérez 50 lignes. Supprimez 25 lignes. Sauvegardez.

**Attendu** : aucune corruption du fichier.

### Scénario 10 : redémarrage

Sauvegardez. Quittez. Rouvrez le fichier.

**Attendu** : l’état rechargé correspond aux modifications.

## Conclusion pédagogique

Ces exercices et scénarios ne sont pas des annexes. Ils font partie du vrai apprentissage. Un éditeur est un outil où l’utilisateur doit faire confiance au logiciel. La confiance n’est pas un sentiment : c’est une conséquence de la rigueur. Si vous respectez ces exercices, vous respecterez vos utilisateurs.


## Erreurs avancées et recovery (protection des données)

Un éditeur ne se juge pas seulement à ce qu’il fait quand tout va bien. Il se juge surtout à ce qu’il fait quand tout va mal. Cette section propose un niveau de robustesse supérieur, inspiré des pratiques des outils sérieux.

### 1) Sauvegarde atomique

**Problème** : un crash au milieu de l’écriture peut laisser un fichier partiellement écrit.

**Principe** : écrire dans un fichier temporaire, puis remplacer l’original. Sur la plupart des systèmes, un `rename` est atomique. Cela garantit qu’on ne voit jamais un fichier « moitié écrit ».

**Approche** :

Écrire dans `path + ".tmp"`. Fermer le fichier. Renommer `.tmp` → `path`.

Si l’étape 3 échoue, l’original reste intact. C’est exactement ce que l’on veut.

### 2) Fichier de secours (.bak)

**Problème** : même avec une sauvegarde atomique, il est utile de garder une version précédente.

**Principe** : avant de remplacer le fichier, faites une copie `.bak`. Cela permet à l’utilisateur de récupérer une version antérieure si un bug d’éditeur survient.

### 3) Détection de corruption

**Problème** : un fichier peut être corrompu (troncature, bytes invalides). Un éditeur naïf peut écraser un fichier déjà partiellement cassé.

**Principe** : si l’ouverture du fichier déclenche une erreur, avertissez l’utilisateur et proposez une ouverture en mode “lecture seule” par défaut.

### 4) Journaling simple

**Problème** : un crash pendant une session peut perdre des modifications non sauvegardées.

**Principe** : journaliser chaque commande (`ins`, `del`) dans un fichier `path + ".journal"`. En cas de crash, on peut rejouer le journal. Même si ce mécanisme est simple, il réduit massivement les pertes de données.

### 5) Récupération au démarrage

**Problème** : l’éditeur redémarre après un crash. Que faire du journal ?

**Principe** : si un journal existe, l’éditeur propose de restaurer. Cette décision doit être explicite et visible.

### 6) Fichiers très grands

**Problème** : charger tout en mémoire peut être impossible.

**Principe** : passer en “mode streaming” ou “mode fenêtre”. L’éditeur ne charge qu’un bloc de lignes, et défile en rechargeant. Cette fonctionnalité est avancée, mais le design que vous avez (viewport, cursor) s’y prête naturellement.

### 7) Conflits externes

**Problème** : le fichier est modifié par un autre outil pendant que vous éditez.

**Principe** : stocker l’horodatage ou la taille initiale, puis avertir l’utilisateur si le fichier a changé. L’éditeur doit alors proposer une fusion ou un rechargement.


**À retenir**

Gardez la règle simple et visible. Une erreur doit être actionnable, pas silencieuse. La lisibilité est une fonctionnalité.

## Annexe : design UI terminal (schémas et flux)

Cette annexe est volontairement “visuelle” dans un média texte. Les schémas ASCII obligent à clarifier la structure.

### 1) Architecture simple

```
+---------------------+
| EditorUI |
|---------------------|
| editor: Editor |
| cursor: Cursor |
| view: Viewport |
+----------+----------+
|
v
+-------+-------+
| render() |
+-------+-------+
|
v
+-------+-------+
| terminal I/O |
+---------------+
```

Ce schéma rappelle un principe fondamental : l’état alimente le rendu, pas l’inverse.

### 2) Boucle d’événements

```
loop:
key = read_key()
ui = handle_key(ui, key)
render(ui)
```

Ce flux est un invariant. Si vous le cassez, les bugs de rendu se multiplient.

### 3) Viewport et curseur

```
lines:
00: ...
01: ...
02: ...
03: ...
04: ...
05: ...
06: ...

viewport row=2 height=3
visible:
02
03
04

cursor row=4 col=10
```

Ce schéma montre pourquoi le viewport est indispensable : il découple la logique d’édition de la taille du terminal.

### 4) Barre de statut

```
[ filename.vit ] 120 lignes | MODIFIED
```

La barre doit être courte, stable, et informative. Elle est l’interface de confiance.

### 5) Rendu plein écran

```
+------------------------------------------+
| 001: entry main at core/app { |
| 002: let x = 1 |
| 003: return x |
| 004: } |
| |
| |
| |
| |
| |
|------------------------------------------|
| filename.vit | 4 lines | MODIFIED |
+------------------------------------------+
```

Ce rendu donne au lecteur une image mentale de ce qu’il doit obtenir. Un bon éditeur se comprend visuellement, même dans un manuel.

### 6) Flux d’édition

```
keypress -> handle_key -> modify buffer -> mark dirty -> render
```

C’est la chaîne la plus importante. Si elle est claire, l’éditeur est robuste.

### 7) Mode raw (idéal)

```
enter_raw_mode()
loop:
key = read_key()
ui = handle_key(ui, key)
render(ui)
exit_raw_mode()
```

Le mode raw n’est pas une fonctionnalité décorative : c’est un contrat avec le terminal. Il faut toujours garantir `exit_raw_mode()` même en cas d’erreur.


**À retenir**

Gardez la règle simple et visible. Une erreur doit être actionnable, pas silencieuse. La lisibilité est une fonctionnalité.

## Synthèse

Ces sections avancées ne sont pas « pour plus tard ». Elles vous donnent une vision de ce qu’un éditeur “sérieux” implique. Même si vous ne les implémentez pas toutes, le simple fait de les comprendre vous aide à écrire un code plus sûr aujourd’hui.


## Chapitre avancé : fonctionnalités d’éditeur moderne

Cette section prolonge l’éditeur minimal vers un éditeur “confortable”. L’objectif n’est pas de tout implémenter, mais de comprendre l’architecture qui rend ces fonctionnalités possibles. La règle reste la même : état clair, rendu clair, et entrée claire.

### 1) Undo multi‑niveau

Un undo simple est utile, mais un undo multi‑niveau est une question de confiance. Pour l’implémenter proprement, il faut stocker des “actions inverses” plutôt que des snapshots complets.

**Principe** : chaque action (ins, del, replace) pousse une entrée dans une pile `undo_stack`. L’entrée décrit comment revenir en arrière.

`ins idx text` ⇒ action inverse : `del idx`. `del idx text` ⇒ action inverse : `ins idx text`. `replace idx old new` ⇒ action inverse : `replace idx new old`.

Vous gardez une pile pour `undo` et une pile pour `redo`. Chaque nouvelle action vide la pile `redo`. Ce modèle est simple, stable, et prévisible.

### 2) Recherche incrémentale

La recherche incrémentale ne doit pas être “magique”. Elle doit simplement surligner la première occurrence pendant que l’utilisateur tape. Cela implique :

une zone d’entrée pour le terme,. une fonction `find_next(lines, term, start)`,. un curseur qui saute au prochain match.

La recherche incrémentale est un excellent test de votre séparation état/rendu. Si l’état est clair, la recherche se branche naturellement.

### 3) Remplacement (replace)

Un replace global est dangereux. Un replace interactif est un contrat. La version simple :

chercher la prochaine occurrence,. demander `y/n` pour remplacer,. continuer.

Cela paraît lent, mais c’est la version la plus sûre. Le but est de ne jamais surprendre l’utilisateur.

### 4) Sélection et mode visuel

Une sélection terminale est d’abord un intervalle `(start, end)`. Vous n’avez pas besoin d’un mode visuel compliqué pour commencer. Le rendu peut simplement inverser les couleurs ou entourer la zone.

L’important n’est pas l’effet graphique, mais la cohérence des indices.

### 5) Sauts et navigation

Ajoutez `goto <line>` et `search <term>`. Ces deux commandes augmentent énormément la productivité. Elles doivent être sûres : si la ligne n’existe pas, on ne bouge pas, on avertit.

### 6) Format et indentation

Un éditeur de code gagne instantanément en crédibilité si l’indentation est stable. Même un simple “auto‑indent” qui copie l’indentation de la ligne précédente améliore l’expérience.

### 7) Thèmes et couleurs

Les couleurs sont utiles, mais non essentielles. Commencez par la clarté : un fond, un texte, une barre de statut. La colorisation syntaxique est un bonus, pas un prérequis.


**À retenir**

Gardez la règle simple et visible. Une erreur doit être actionnable, pas silencieuse. La lisibilité est une fonctionnalité.

## Chapitre avancé : interop C/C++ pour un moteur externe

Ici, nous abordons un scénario réaliste : vous voulez un rendu plus riche ou un handling clavier robuste. Vous pouvez appeler une petite bibliothèque C/C++ qui gère les détails du terminal. L’interop est un contrat, pas une magie.

### 1) Définir une API minimale

Vous ne voulez pas exposer toute une bibliothèque. Vous voulez une surface minimale :

`term_init()`. `term_read_key()`. `term_render(buffer)`. `term_shutdown()`.

Cette API est suffisante pour brancher un moteur externe sans détruire la lisibilité de votre code Vitte.

### 2) Externs côté Vitte

```vit
#[extern]


proc term_init() -> i32

#[extern]


proc term_read_key() -> i32

#[extern]


proc term_render(buf: string) -> i32

#[extern]


proc term_shutdown() -> i32
```

Ces signatures sont volontairement simples. Elles évitent des structures complexes et gardent l’interface stable.

### 3) Adaptation côté C

Côté C/C++, vous implémentez les fonctions exportées. Le point crucial : vous devez garantir la stabilité des conventions d’appel et la gestion des erreurs.

### 4) Flux d’utilisation

`term_init()`. boucle d’événements : `term_read_key()` → `handle_key()` → `render()`. `term_shutdown()` même en cas d’erreur.

La règle est stricte : ne jamais laisser le terminal dans un état incohérent.

### 5) Gestion des erreurs

Chaque appel externe doit être vérifié. Un retour `-1` doit conduire à un message clair et à un shutdown propre. L’interop n’est pas un endroit où l’on improvise.

### 6) Documentation de l’ABI

Documentez votre ABI dans le livre. Même une page suffit : types, conventions, valeurs de retour. Sans cela, l’interop devient un piège.


**À retenir**

Gardez la règle simple et visible. Une erreur doit être actionnable, pas silencieuse. La lisibilité est une fonctionnalité.

## Résumé des extensions

Vous avez maintenant une feuille de route pour transformer un éditeur minimal en éditeur sérieux : undo, recherche, replace, navigation, et éventuellement un moteur externe. Le cœur reste le même : une architecture claire qui respecte l’utilisateur.


## Glossaire de l’éditeur

**Buffer**

Zone mémoire qui contient les lignes d’un fichier. Dans un éditeur, le buffer est la vérité. Si le buffer est cohérent, l’éditeur est cohérent.

**Cursor (curseur)**

Position logique dans le texte, exprimée en ligne/colonne. Ce n’est pas un symbole graphique : c’est un état.

**Viewport**

Fenêtre sur le buffer. Le viewport décrit la portion visible à l’écran. Sans viewport, le défilement devient un bug permanent.

**Dirty flag**

Indicateur qui signifie “modifié depuis la dernière sauvegarde”. C’est un contrat moral : si `dirty` est vrai, l’éditeur doit protéger l’utilisateur.

**Render**

Processus qui transforme l’état en affichage. Un bon rendu est déterministe : même état, même écran.

**Raw mode**

Mode terminal où l’entrée est lue caractère par caractère, sans interprétation. Indispensable pour un éditeur interactif.

**Event loop**

Boucle qui lit les entrées, met à jour l’état, puis redessine. C’est le cœur de tout éditeur moderne.

**Undo stack**

Pile d’actions inverses qui permet de revenir en arrière. Elle évite les snapshots coûteux et rend l’undo prévisible.

**Redo stack**

Pile d’actions annulées que l’on peut réappliquer. Elle se vide dès qu’une nouvelle action est commise.

**Atomic save**

Stratégie de sauvegarde qui écrit dans un fichier temporaire puis remplace l’original. Elle réduit drastiquement le risque de corruption.

**Journal**

Fichier qui enregistre les opérations d’édition. En cas de crash, il sert de base pour récupérer les changements.

**Status bar**

Ligne d’information stable qui indique le fichier, la taille, et l’état dirty. C’est une promesse de clarté.


## Conception détaillée (architecture opérationnelle)

Cette section transforme le projet en un plan d’implémentation concret. L’objectif est de rendre l’éditeur réellement opérationnel, en précisant les responsabilités, les modules, et l’outillage nécessaire. Nous allons introduire des extraits en Vitte, mais aussi des exemples en C pour la partie terminal bas niveau. Ce mélange n’est pas un gadget : c’est la manière la plus réaliste de construire un éditeur robuste aujourd’hui.

### 1) Découpage en modules

Un éditeur opérationnel devient vite un monolithe. Évitez ce piège dès le début avec un découpage clair :

`editor_state.vit` : structures d’état (`Editor`, `Cursor`, `Viewport`). `editor_buffer.vit` : opérations sur les lignes (insert, delete, replace). `editor_cmd.vit` : parsing et exécution des commandes. `editor_render.vit` : rendu (ligne par ligne ou plein écran). `editor_io.vit` : chargement/sauvegarde, conversion, erreurs. `editor_main.vit` : boucle principale.

Ce découpage est un contrat de lecture. Chaque fichier a un rôle. Quand vous revenez dans six mois, vous savez où chercher.

### 2) Interfaces minimales

Chaque module expose une interface étroite :

`editor_state` expose `Editor`, `Cursor`, `Viewport`. `editor_buffer` expose `insert_line`, `delete_line`, `replace_line`. `editor_cmd` expose `parse_command` et `run_command`. `editor_render` expose `render_line_mode` et `render_fullscreen`. `editor_io` expose `load_file` et `save_file`.

Le but est de rendre les dépendances visibles et limitées.

### 3) Format de fichier

Le format est texte brut. Ne tentez pas d’inventer un format “intelligent”. L’éditeur doit rester compatible avec `cat`, `grep`, `sed`, et tous les outils Unix. C’est une stratégie de compatibilité, pas une contrainte.

### 4) Table de commandes

Pour éviter une chaîne infinie de `if`, utilisez une table de dispatch. Même si la stdlib n’a pas de map, une liste de paires suffit.

```vit
form Cmd {
name: string
run: proc(editor: *Editor, args: [string])
}
```

Ce modèle vous force à écrire des commandes petites et testables.

## Rendu opérationnel (version ligne par ligne)

Cette version fonctionne sans mode raw. Elle est simple, mais utile. C’est une étape réaliste pour obtenir un outil stable.

### Principe

Vous affichez un prompt `> `. L’utilisateur tape une commande. Vous exécutez. Vous réaffichez.

Ce modèle est similaire à un REPL. Il n’est pas élégant, mais il est robuste.

## Rendu opérationnel (version plein écran, API idéale + C)

Le mode raw nécessite des appels bas niveau. Voici un exemple en C qui met le terminal en mode raw et lit une touche. Ce code est volontairement court et pragmatique.

### Fichier C : `term_raw.c`

```c
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static struct termios orig;

int term_init(void) {
if (tcgetattr(STDIN_FILENO, &orig) == -1) return -1;
struct termios raw = orig;
raw.c_lflag &= ~(ECHO | ICANON);
raw.c_cc[VMIN] = 1;
raw.c_cc[VTIME] = 0;
return tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int term_shutdown(void) {
return tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

int term_read_key(void) {
unsigned char c;
int n = read(STDIN_FILENO, &c, 1);
if (n <= 0) return -1;
return (int)c;
}

int term_render(const char *buf) {
size_t len = 0;
while (buf[len] != '\0') len++;
return (int)write(STDOUT_FILENO, buf, len);
}
```

Ce code active un mode raw minimal, lit un caractère, et écrit un buffer. Il est suffisant pour un prototype. Les touches spéciales (flèches, etc.) nécessitent une couche d’interprétation supplémentaire.

### Binding côté Vitte

```vit
#[extern]


proc term_init() -> i32

#[extern]


proc term_shutdown() -> i32

#[extern]


proc term_read_key() -> i32

#[extern]


proc term_render(buf: string) -> i32
```

La frontière est claire. Le code Vitte ne manipule pas la `termios`. Il parle à un petit moteur externe.

### Boucle “raw” en Vitte (concept)

```vit
proc run_raw(ui: *EditorUI) {
if term_init() < 0 { return }
loop {
let key = term_read_key()
if key < 0 { break }
ui = handle_key(ui, key)
let frame = render(ui)
let _ = term_render(frame)
}
term_shutdown()
}
```

Ce pseudo‑code montre la séparation : entrée → état → rendu. C’est l’architecture qui tient la route.

## Build opérationnel (exemple minimal)

Un éditeur multi‑langages a besoin d’un pipeline simple. Voici un exemple conceptuel :

```sh
cc -c term_raw.c -o term_raw.o
vitte build src/editor_main.vit
cc term_raw.o -o editor bin/output.o
```

Adaptez ce pipeline à votre système. Le point important est la clarté de la chaîne de build : vous savez ce qui est compilé, et dans quel ordre.

## Tests opérationnels (manuel + script)

En plus des scénarios manuels décrits plus haut, vous pouvez écrire un script de test simple qui envoie des commandes au programme.

### Exemple de script (shell)

```sh
printf "open test.txt\nins 0 Hello\nshow\nsave\nquit\n" | editor
```

Ce test ne remplace pas un framework, mais il vérifie un flux critique en un seul appel. C’est particulièrement utile avant une démo.

## Conclusion : rendre un éditeur “réel”

Un éditeur opérationnel ne dépend pas de 10 000 fonctionnalités. Il dépend d’un petit nombre de règles strictes : séparation des responsabilités, sauvegarde sûre, et feedback clair. En ajoutant un petit module C pour le terminal, vous obtenez une base technique solide, sans sacrifier la lisibilité du code Vitte.


## Build complet (Makefile Vitte + C)

Un éditeur opérationnel qui mélange Vitte et C doit avoir un build reproductible. Le Makefile suivant est volontairement explicite : il montre chaque étape de compilation et d’édition de liens. La clarté est une fonctionnalité.

### Exemple de Makefile

```makefile
CC = cc
CFLAGS = -O2 -Wall -Wextra
VITTE = vitte

# Fichiers


C_SRC = term_raw.c
C_OBJ = term_raw.o
VIT_SRC = src/editor_main.vit
VIT_OBJ = bin/editor.o
BIN = bin/editor

all: $(BIN)

$(C_OBJ): $(C_SRC)
$(CC) $(CFLAGS) -c $(C_SRC) -o $(C_OBJ)

$(VIT_OBJ): $(VIT_SRC)
$(VITTE) build $(VIT_SRC) -o $(VIT_OBJ)

$(BIN): $(C_OBJ) $(VIT_OBJ)
$(CC) $(CFLAGS) $(C_OBJ) $(VIT_OBJ) -o $(BIN)

clean:
rm -f $(C_OBJ) $(VIT_OBJ) $(BIN)
```

Ce Makefile est minimal, mais il est lisible et stable. Vous pouvez l’étendre avec des cibles `debug`, `test`, ou `run`, mais gardez le même niveau de clarté.



## Makefile complet (Vitte + C + )

Voici une version plus complète qui compile le module C, le code Vitte, et ajoute une cible `config` pour valider la présence du fichier .

```makefile
CC = cc
CFLAGS = -O2 -Wall -Wextra
VITTE = vitte

C_SRC = term_raw.c
C_OBJ = term_raw.o
VIT_SRC = src/editor_main.vit
VIT_OBJ = bin/editor.o
BIN = bin/editor
CONF =

all: $(BIN)

$(C_OBJ): $(C_SRC)
$(CC) $(CFLAGS) -c $(C_SRC) -o $(C_OBJ)

$(VIT_OBJ): $(VIT_SRC)
$(VITTE) build $(VIT_SRC) -o $(VIT_OBJ)

$(BIN): $(C_OBJ) $(VIT_OBJ)
$(CC) $(CFLAGS) $(C_OBJ) $(VIT_OBJ) -o $(BIN)

config:
@test -f $(CONF) || (echo "missing $(CONF)" && exit 1)

clean:
rm -f $(C_OBJ) $(VIT_OBJ) $(BIN)
```

Ce Makefile reste simple, mais il couvre la chaîne complète. Il est suffisamment clair pour être adapté à un vrai projet.

## Mini spec (contrats + validation)

Cette mini‑spec décrit ce que l’éditeur attend de ``. Elle sert de contrat de lecture et de validation. L’objectif n’est pas de “policer” l’utilisateur, mais de rendre les erreurs explicites.

### 1) Structure attendue

```
editor {
theme = "light" | "dark"
status_bar = true | false
key_save = "ctrl+s"
key_quit = "ctrl+q"
}
```

### 2) Règles de validation

`theme` doit être `light` ou `dark`. `status_bar` doit être un booléen. `key_save` et `key_quit` doivent être des chaînes non vides. Toute clé inconnue déclenche un warning (pas une erreur bloquante).

### 3) Valeurs par défaut

Si la section `editor` est absente ou si une clé est invalide, l’éditeur revient aux valeurs par défaut :

`theme = light`. `status_bar = true`. `key_save = ctrl+s`. `key_quit = ctrl+q`.

### 4) Exemples de diagnostics

`error: editor.theme must be light|dark`. `error: editor.key_save must be non-empty string`. `warning: unknown key editor.color`.

### 5) Conseils d’implémentation

Validez dès le chargement, pas pendant l’exécution. Un éditeur doit échouer tôt si la configuration est invalide, ou revenir à des defaults avec un message clair.

**À retenir**

La config est un contrat, pas un “best effort”. Les défauts doivent être documentés. Les warnings sont utiles pour la découverte, les erreurs pour la sécurité.


### 6) Mode strict (optionnel)

En mode strict, toute erreur de configuration devient bloquante. C’est utile en production, quand vous préférez un refus clair à un comportement implicite.

Exemple de règle :

si `theme` est invalide, l’éditeur refuse de démarrer.

Exemple de message :

`fatal: invalid config: editor.theme must be light|dark`.

Ce mode protège la cohérence au prix d’une rigidité accrue. Il doit être activé explicitement, par exemple avec `--strict-config`.


