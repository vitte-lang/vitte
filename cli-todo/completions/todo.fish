# Fish completion for `todo` CLI
# Copie ce fichier dans ~/.config/fish/completions/todo.fish
# ou utilise directement: source cli-todo/completions/todo.fish

function __fish_todo_needs_command
    test (count (commandline -opc)) -eq 1
end

# Liste des commandes principales
set -l todo_cmds add list done remove clear help

# Complétion de la commande principale
complete -c todo -n '__fish_todo_needs_command' -f -a "$todo_cmds" \
    -d 'Todo subcommand'

# Complétion pour `todo add`
complete -c todo -n '__fish_seen_subcommand_from add' -f \
    -d "Description du todo"

# Complétion pour `todo done` (suggère les IDs existants si stockés)
complete -c todo -n '__fish_seen_subcommand_from done' -a '(__fish_todo_ids)' \
    -d "ID du todo à marquer comme terminé"

# Complétion pour `todo remove` (suggère les IDs existants si stockés)
complete -c todo -n '__fish_seen_subcommand_from remove' -a '(__fish_todo_ids)' \
    -d "ID du todo à supprimer"

# Complétion pour `todo list`
complete -c todo -n '__fish_seen_subcommand_from list' -d "Lister tous les todos"

# Complétion pour `todo clear`
complete -c todo -n '__fish_seen_subcommand_from clear' -d "Effacer tous les todos"

# Complétion pour `todo help`
complete -c todo -n '__fish_seen_subcommand_from help' -d "Afficher l'aide"

# --------------------------------------------------------------------
# Helper: récupérer les IDs depuis un fichier ~/.todo/ids (optionnel)
# --------------------------------------------------------------------
function __fish_todo_ids
    if test -f ~/.todo/ids
        cat ~/.todo/ids
    end
end
