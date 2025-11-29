# Langage Vitte – Arborescence dédiée

Cette hiérarchie rassemble les spécifications, manifestes Muffin et exemples
nécessaires pour le nouveau langage Vitte, pensé comme une syntaxe accessible
et compilable inspirée de Python/Ruby.

```
language/
  README.md
  spec/
    lexique.md
    modules.md
    commands.md
    system.md
    concurrency.md
    instrumentation.md
    extensions.md
  examples/
    modules/
    system/
    concurrency/
    instrumentation/
    extensions/
```

Chaque sous-dossier `spec/` couvre une famille de constructions décrites dans
`grammar/vitte.ebnf`. Les répertoires `examples/` proposent des fragments de
code destinés aux étudiants et aux auteurs d’outils (compilateur, IDE, LSP).

## Lien avec l’unité de compilation Muffin

Les fichiers `.muf` restent l’unité de compilation principale du projet Vitte.
Chaque manifeste pointe vers les sources `.vitte` correspondantes et décrit
les dépendances ou cibles (`program`, `kernel`, `service`, etc.). Les exemples
ci-dessous incluent systématiquement un manifeste minimal pour illustrer ce
flux de travail.

## Prochaines étapes

1. Compléter la documentation dans `spec/` en harmonisant les sections avec
   la grammaire officielle.
2. Ajouter des scénarios concrets dans `examples/` (modules applicatifs,
   kernels, services concurrents, instrumentation).
3. Connecter ces fichiers aux scripts de tests/démo lorsque le compilateur
   Vitte évoluera.
