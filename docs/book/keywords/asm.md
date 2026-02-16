# Mot-cle `asm`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`asm` injecte une instruction assembleur textuelle, a utiliser dans des frontieres tres bornees.

Forme de base en Vitte. `asm("instr")`, generalement en `unsafe`.

Exemple 1, construit pas a pas.

```vit
proc pause_cpu() {
  unsafe {
    asm(pause)
  }
}
```

Pourquoi cette etape est solide. L'appel reste local et nomme. La zone non prouvable est confinee.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc halt_cpu() {
  unsafe {
    asm(hlt)
  }
}
```

Pourquoi cette etape est solide. Instruction privilegiee, a proteger par contrat de contexte d'execution.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Dupliquer `asm` en divers points du code sans encapsulation est un risque majeur.

Pour prolonger la logique. Voir `docs/book/chapters/16-interop.md` et `docs/book/chapters/12-pointeurs.md`.

Exemple 3, construit pas a pas.

```vit
proc spin_hint() {
  unsafe { asm("pause") }
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle asm dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer asm sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de asm au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
