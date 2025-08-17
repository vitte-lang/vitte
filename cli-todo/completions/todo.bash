_todo_complete() {
  COMPREPLY=()
  local cur="${COMP_WORDS[COMP_CWORD]}"
  local cmds="add ls list done rm del edit snooze clear archive import export stats help --help -h"
  if [[ $COMP_CWORD -eq 1 ]]; then
    COMPREPLY=( $(compgen -W "$cmds" -- "$cur") )
  else
    COMPREPLY=( $(compgen -W "--all --proj -P --tag --prio -p --due -d --search --recur --notes" -- "$cur") )
  fi
  return 0
}
complete -F _todo_complete todo
