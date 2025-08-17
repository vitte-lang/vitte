# Vitte CLI — TODO (ultra complet)

Gestionnaire de tâches **rapide** en ligne de commande pour l’écosystème **Vitte**.
- CRUD : add, ls, done, rm, edit
- Filtres : projet, tags, priorités, recherche textuelle
- Dates : échéance (due), snooze, overdue
- Récurrence : `--recur daily|weekly|monthly`
- Import/Export : JSON (NDJSON) ↔ TSV interne
- UX : couleurs, colonnes, tty-aware, complétions bash/zsh/fish
- Stockage : `~/.vitte/todo/todo.tsv` (TSV, robuste et diffable)

## Installation rapide
```bash
# Wrapper exécutable
sudo cp scripts/todo /usr/local/bin/todo
# Completions (optionnel)
sudo cp completions/todo.bash /etc/bash_completion.d/
sudo cp completions/_todo /usr/local/share/zsh/site-functions/
sudo cp completions/todo.fish ~/.config/fish/completions/
```

## Exemples
```bash
todo add "Acheter du lait" -p high -d 2025-08-20 -t courses,maison -P perso
todo ls
todo ls --overdue
todo done 3
todo edit 3 --due 2025-08-22 --prio normal
todo export tasks.ndjson
todo import tasks.ndjson
todo stats
```

## Format du stockage
TSV (tabulation). Colonnes : `id \t title \t project \t tags(comma) \t due(YYYY-MM-DD) \t prio(low|normal|high) \t status(open|done|arch) \t created(ms) \t updated(ms) \t recur \t notes`

## Commandes
- `add <titre>` `[-P|--project <nom>] [-t|--tags <a,b>] [-p|--prio <low|normal|high>] [-d|--due <YYYY-MM-DD>] [--recur <daily|weekly|monthly>] [--notes "<texte>"]`
- `ls` `[--all] [--proj <nom>] [--tag <t>] [--prio <p>] [--search <mot>] [--overdue] [--due <YYYY-MM-DD>]`
- `done <id>`
- `rm <id>` (supprime définitivement)
- `edit <id>` options comme `add` (modifie partiellement)
- `snooze <id> <+Nd|YYYY-MM-DD>`
- `clear` (archive les *done*), `archive <id>`
- `import <fichier.ndjson>` / `export <fichier.ndjson>`
- `stats` (ouvertes/done par prio & projet)

**Tip** : `todo --help` pour le détail.
