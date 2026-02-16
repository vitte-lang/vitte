# Mot-cle `entry`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
Le mot-cle `entry` declare le point d'entree executable d'un module Vitte. C'est la racine de flux runtime.

Forme de base en Vitte. `entry nom at chemin/module { ... }`.

Exemple 1, construit pas a pas.

```vit
proc add(a: int, b: int) -> int { give a + b }
entry main at core/app {
  let r: int = add(20, 22)
  return r
}
```

Pourquoi cette etape est solide. Le compilateur verifie l'existence de l'entree, la signature des appels internes et la coherence du code de retour.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
entry check at core/app {
  let code: int = 0
  if code != 0 { return 1 }
  return 0
}
```

Pourquoi cette etape est solide. `entry` doit rester mince et deleguer au metier pour conserver une architecture testable.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Surcharger `entry` avec la logique metier et l'I/O melangee rend le debug global et couteux.

Pour prolonger la logique. Voir `docs/book/chapters/01-demarrer.md` et `docs/book/chapters/29-style.md`.

Exemple 3, construit pas a pas.

```vit
entry service at app/service {
  let code: int = 0
  return code
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle entry dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer entry sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de entry au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
