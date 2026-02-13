# Platon Editor (Vitte) — Roadmap réaliste

Objectif produit: éditeur terminal en Vitte avec ergonomie Vim/Emacs, extensible multi-langages.

Statut actuel: `editor_core.vit` fournit un noyau de logique compilable (buffer texte, curseur, historique, modes, catalogue 30 langages).

## Ce qui est déjà prêt
- Modèle de buffer ligne par ligne: insertion, suppression, remplacement.
- Gestion du curseur avec bornage.
- Historique borné pour base d'undo/redo.
- Modes d'édition: normal, insert, command.
- Catalogue de 30 langages pré-enregistrés.

## Phases vers Vim/Emacs parity
1. Runtime terminal brut (`raw mode`): lecture touche par touche, gestion `Ctrl`, `Alt`, `Esc`.
2. Rendu terminal complet: viewport, statusline, minimap texte, redraw partiel.
3. Fichier robuste: open/save atomique, recovery, swap files.
4. Navigation avancée: motions (word, sentence, paragraph, token), marks, registers.
5. Édition avancée: multi-cursor, macros, text objects, visual/block mode.
6. Moteur commandes: `:`-commands, keymaps Vim et Emacs, remap utilisateur.
7. Syntaxe/indentation: tokenisation incremental + 30 langages + auto-indent.
8. LSP client: diagnostics, goto def, rename, completion.
9. Extensions: hooks, plugins sandboxés, config utilisateur.
10. Performance: profiling, redraw incrémental, grandes bases de code.

## Blocages techniques actuels dans ce repo
- Plusieurs modules std I/O/CLI ne sont pas encore stables au build backend.
- Une partie des opérations `string` (comparaison/index/slice) n'est pas encore fiable pour un parser de commandes complet.

Conséquence: l'éditeur interactif complet ne peut pas être finalisé immédiatement sans stabiliser d'abord ces APIs.
