# 21. Projet guide CLI

Ce chapitre poursuit un objectif de terrain: construire un CLI Vitte qui reste propre quand les erreurs reelles arrivent. Le probleme d'un outil en ligne de commande n'est pas de produire un resultat dans le cas nominal. Le vrai probleme est de garder une logique lisible quand l'utilisateur oublie un argument, choisit un mode inconnu, ou enchaine des usages invalides. La methode suivie ici est stricte: typer les erreurs, segmenter les etapes, puis centraliser la projection finale vers le systeme.

Etape 1. Declarer le contrat de resultat avant d'ecrire l'orchestration.

```vit
pick CliError {
  case MissingArgs
  case UnknownMode(mode: int)
}

pick CliResult {
  case Ok(code: int)
  case Err(e: CliError)
}
```

Pourquoi cette etape est solide. Ce choix fonde toute la robustesse du projet. Tant que les fautes restent des entiers disperses, le programme ment sur sa propre structure. En introduisant `CliError` et `CliResult`, vous forcez chaque fonction a assumer explicitement son issue. Cela transforme le CLI en systeme verifiable: les cas sont nommes, les transitions sont visibles, et les tests peuvent cibler une variante precise au lieu d'inferer un comportement par effet de bord.

Ce qui se passe a l'execution. A ce stade, rien ne roule encore, mais l'espace des sorties possibles est deja ferme. Chaque procedure future devra choisir `Ok(...)` ou `Err(...)`, sans echappatoire implicite.

Etape 2. Isoler la validation des arguments.

```vit
proc parse_arg_count(argc: int) -> CliResult {
  if argc < 2 { give Err(MissingArgs) }
  give Ok(argc)
}
```

Pourquoi cette etape est solide. La validation est placee en frontiere d'entree, loin du coeur metier. Cette separation est decisive: vous pouvez tester la politique d'arite sans ouvrir le moindre code de routage. En architecture, c'est un principe fort: ce qui controle l'integrite des donnees d'entree ne doit pas etre noye dans les decisions metier.

Ce qui se passe a l'execution. `parse_arg_count(1)` retourne `Err(MissingArgs)` et coupe le flux nominal. `parse_arg_count(3)` retourne `Ok(3)` et autorise la suite.

Etape 3. Centraliser le routage de mode.

```vit
proc run_mode(mode: int) -> CliResult {
  if mode == 1 { give Ok(10) }
  if mode == 2 { give Ok(20) }
  give Err(UnknownMode(mode))
}
```

Pourquoi cette etape est solide. Le routeur metier est isole dans une fonction pure. Le cas inconnu n'est pas aplati dans un `-1` anonyme; il conserve la valeur fautive dans `UnknownMode(mode)`. Cette conservation est precieuse pour le diagnostic, la telemetrie et les tests de non-regression: on ne sait pas seulement qu'il y a erreur, on sait laquelle.

Ce qui se passe a l'execution. `run_mode(1)` retourne `Ok(10)`, `run_mode(2)` retourne `Ok(20)`, `run_mode(9)` retourne `Err(UnknownMode(9))`.

Etape 4. Projeter une politique systeme stable.

```vit
proc to_exit_code(r: CliResult) -> int {
  match r {
    case Ok(_) { give 0 }
    case Err(MissingArgs) { give 64 }
    case Err(UnknownMode(_)) { give 65 }
    otherwise { give 70 }
  }
}
```

Pourquoi cette etape est solide. La conversion vers les exit-codes est un sujet d'integration systeme, pas un sujet metier. La sortir dans une fonction dediee vous donne une frontiere claire: vous pouvez faire evoluer la semantique interne sans casser les conventions de shell, ou inversement. Cette couture explicite est ce qui permet a un CLI de vieillir proprement.

Ce qui se passe a l'execution. Un succes donne `0`, une arite manquante donne `64`, un mode inconnu donne `65`. Le `otherwise` garantit un filet de securite avec `70`.

Etape 5. Orchestrer dans `entry` sans reinventer la logique.

```vit
entry main at core/app {
  let argc: int = 2
  let parse_res: CliResult = parse_arg_count(argc)

  match parse_res {
    case Err(e) {
      return to_exit_code(Err(e))
    }
    case Ok(_) {
      let run_res: CliResult = run_mode(1)
      return to_exit_code(run_res)
    }
    otherwise {
      return 70
    }
  }
}
```

Pourquoi cette etape est solide. `main` ne pense pas a la place des modules. Il orchestre. Cette sobriete est une force structurelle: si vous lisez l'entree du programme, vous comprenez l'ordre du flux en quelques secondes. Et si vous devez modifier la politique de validation ou de routage, vous le faites dans des fonctions deja specialisees, pas dans un monolithe.

Ce qui se passe a l'execution. Sur le scenario nominal `argc=2` et `mode=1`, le flux est `parse -> run -> to_exit_code` et le programme sort avec `0`. Sur `argc=1`, la validation echoue immediatement et la sortie devient `64`. Le chemin d'erreur est donc court, explicite et deterministe.

Ce que vous devez maitriser en sortie de chapitre. Un CLI Vitte robuste n'est pas celui qui contient beaucoup de code, mais celui qui expose une grammaire claire de decisions. Entree validee en frontiere, metier route dans des fonctions pures, projection systeme centralisee, et `entry` reduite au role d'orchestrateur. Avec cette discipline, la complexite augmente sans detruire la lisibilite.
