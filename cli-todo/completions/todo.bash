# Bash completion for `todo` CLI
# À sourcer dans ton ~/.bashrc ou via `source /path/to/todo.bash`

_todo_completions()
{
    local cur prev commands
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    commands="add list done remove clear help"

    # Complétion sur la 1ère commande
    if [[ ${COMP_CWORD} -eq 1 ]] ; then
        COMPREPLY=( $(compgen -W "${commands}" -- "${cur}") )
        return 0
    fi

    # Complétion sur les arguments selon la commande
    case "${COMP_WORDS[1]}" in
        add)
            # Suggest nothing, free text
            ;;
        done|remove)
            # Ici tu pourrais tab-completer les IDs existants depuis un fichier
            # Exemple fictif: lecture depuis ~/.todo/ids
            if [[ -f "$HOME/.todo/ids" ]]; then
                local ids
                ids=$(cat "$HOME/.todo/ids")
                COMPREPLY=( $(compgen -W "${ids}" -- "${cur}") )
            fi
            ;;
        list|clear|help)
            # Pas d’arguments attendus
            ;;
        *)
            ;;
    esac
    return 0
}

complete -F _todo_completions todo
