# 25. Projet guide Arduino

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Construire une boucle embarquee Vitte deterministe en separant acquisition, nettoyage, normalisation et commande actionneur.

Etape 1. Modeliser etat de controle et echantillon capteur.

```vit
pick ControlState {
  case Idle
  case Alert
}

form Sample {
  raw: int
}
```

Pourquoi cette etape est solide. Le domaine est explicite et remplace les drapeaux numeriques ambigus.

Ce qui se passe a l'execution. Toutes les decisions de commande seront exprimees via `ControlState`.

Etape 2. Acquerir puis assainir la mesure brute.

```vit
proc read_raw(v: int) -> Sample {
  give Sample(v)
}

proc sanitize_raw(s: Sample) -> int {
  if s.raw < 0 { give 0 }
  if s.raw > 1023 { give 1023 }
  give s.raw
}
```

Pourquoi cette etape est solide. La saturation 10 bits protege l'etape de normalisation et evite la propagation de bruit hors domaine.

Ce qui se passe a l'execution. `read_raw(1200)` puis `sanitize_raw` retourne `1023`.

Etape 3. Normaliser en pourcentage et classifier.

```vit
proc to_percent(v: int) -> int {
  give (v * 100) / 1023
}

proc classify_percent(p: int) -> ControlState {
  if p >= 80 { give Alert }
  give Idle
}
```

Pourquoi cette etape est solide. Les seuils metier vivent dans une procedure dediee. Vous pouvez retuner sans toucher au pipeline d'acquisition.

Ce qui se passe a l'execution. `v=512 -> p=50 -> Idle`. `v=950 -> p=92 -> Alert`.

Etape 4. Composer une iteration complete de controle.

```vit
proc control_step(raw: int) -> ControlState {
  let s: Sample = read_raw(raw)
  let clean: int = sanitize_raw(s)
  let p: int = to_percent(clean)
  give classify_percent(p)
}
```

Pourquoi cette etape est solide. La boucle est pure et deterministic. Chaque sous-etape reste testable isolee.

Ce qui se passe a l'execution. `control_step(-5)=Idle`. `control_step(1023)=Alert`.

Etape 5. Projeter l'etat vers une commande actionneur.

```vit
proc actuator_code(state: ControlState) -> int {
  match state {
    case Idle { give 0 }
    case Alert { give 1 }
    otherwise { give -1 }
  }
}
```

Pourquoi cette etape est solide. La couche hardware recoit une valeur simple sans connaitre la logique de classification.

Ce qui se passe a l'execution. `actuator_code(Idle)=0` et `actuator_code(Alert)=1`.

Ce que vous devez maitriser en sortie de chapitre. La boucle est deterministic, les seuils sont explicites et la projection materielle est proprement decouplee.
