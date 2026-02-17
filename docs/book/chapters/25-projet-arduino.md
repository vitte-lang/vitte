# 25. Projet guide Arduino

Niveau: Avancé

Prérequis: chapitre précédent `docs/book/chapters/24-projet-kv.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/24-projet-kv.md`, `docs/book/chapters/26-projet-editor.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


## Niveau local

- Niveau local section coeur: Avancé.
- Niveau local exemples guidés: Intermédiaire.
- Niveau local exercices de diagnostic: Avancé.


Ce chapitre poursuit un objectif clair: construire une boucle embarquée Vitte utilisable en conditions réelles, avec gestion du bruit capteur, seuils robustes, sécurité actionneur et comportement déterministe. Nous allons passer d'une démonstration simple à un mini contrôleur complet.

L'idée directrice est la suivante: en embarqué, la robustesse vient de la discipline des invariants. Chaque étape doit être explicite, testable et isolable: acquisition, assainissement, filtrage, décision, projection matérielle.


Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.

Schéma pipeline du chapitre:
- Entrée: données initiales ou requête.
- Traitement: validation, logique métier, transformations.
- Sortie: résultat métier ou code de statut.
- Invariant: propriété qui doit rester vraie à chaque étape.

## 25.1 Définir le modèle de contrôle

```vit
pick ControlState {
  case Idle
  case Armed
  case Alert
  case Fault(code: int)
}
form Sample {
  raw: int
}
form ControllerCfg {
  min_raw: int
  max_raw: int
  alert_on: int
  alert_off: int
}
```

Lecture ligne par ligne (débutant):
1. `pick ControlState {` cette ligne ouvre le type fermé `ControlState` pour forcer un ensemble fini de cas possibles et supprimer les états implicites. Exemple concret: toute valeur hors des `case` déclarés devient impossible à représenter.
2. `case Idle` cette ligne décrit le cas `Idle` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Idle`, ce bloc devient le chemin actif.
3. `case Armed` cette ligne décrit le cas `Armed` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Armed`, ce bloc devient le chemin actif.
4. `case Alert` cette ligne décrit le cas `Alert` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Alert`, ce bloc devient le chemin actif.
5. `case Fault(code: int)` cette ligne décrit le cas `Fault(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Fault(code: int)`, ce bloc devient le chemin actif.
6. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
7. `form Sample {` cette ligne ouvre la structure `Sample` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `Sample` sans redéfinir ses champs.
8. `raw: int` cette ligne déclare le champ `raw` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
9. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
10. `form ControllerCfg {` cette ligne ouvre la structure `ControllerCfg` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `ControllerCfg` sans redéfinir ses champs.
11. `min_raw: int` cette ligne déclare le champ `min_raw` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
12. `max_raw: int` cette ligne déclare le champ `max_raw` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
13. `alert_on: int` cette ligne déclare le champ `alert_on` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
14. `alert_off: int` cette ligne déclare le champ `alert_off` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
15. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Idle` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Ce modèle introduit deux idées fortes:
- l'état n'est pas binaire (`Idle/Alert`), il est explicite (`Armed`, `Fault`).
- la politique de seuil vit dans `ControllerCfg`, pas dans des nombres en dur.

Erreurs fréquentes à éviter:
- laisser des seuils magiques disséminés dans le code.
- confondre erreur matérielle (`Fault`) et état métier (`Alert`).
- durcir le comportement sans config révisable.

## 25.2 Acquérir et saturer la mesure brute

```vit
proc read_raw(v: int) -> Sample {
  give Sample(v)
}
proc clamp_raw(s: Sample, cfg: ControllerCfg) -> int {
  if s.raw < cfg.min_raw { give cfg.min_raw }
  if s.raw > cfg.max_raw { give cfg.max_raw }
  give s.raw
}
```

Lecture ligne par ligne (débutant):
1. `proc read_raw(v: int) -> Sample {` ici, le contrat complet est défini pour `read_raw`: entrées `v: int` et sortie `Sample`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `read_raw` retourne toujours une valeur compatible avec `Sample`.
2. `give Sample(v)` ici, la branche renvoie immédiatement `Sample(v)` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Sample(v)`.
3. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
4. `proc clamp_raw(s: Sample, cfg: ControllerCfg) -> int {` sur cette ligne, le contrat complet est posé pour `clamp_raw`: entrées `s: Sample, cfg: ControllerCfg` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `clamp_raw` retourne toujours une valeur compatible avec `int`.
5. `if s.raw < cfg.min_raw { give cfg.min_raw }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `s.raw < cfg.min_raw` est vrai, `give cfg.min_raw` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `if s.raw > cfg.max_raw { give cfg.max_raw }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `s.raw > cfg.max_raw` est vrai, `give cfg.max_raw` est exécuté immédiatement; sinon on continue sur la ligne suivante.
7. `give s.raw` sur cette ligne, la sortie est renvoyée immédiatement `s.raw` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `s.raw`.
8. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `s.raw < cfg.min_raw` est vrai, la sortie devient `cfg.min_raw`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Sample(v)`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

La saturation doit être la première barrière de robustesse:
- elle coupe les valeurs impossibles.
- elle stabilise les étapes suivantes.

À l'exécution:
- valeur trop basse -> remontée à `min_raw`.
- valeur trop haute -> rabattue à `max_raw`.

Erreurs fréquentes à éviter:
- normaliser une valeur non saturée.
- supposer une plage fixe (`0..1023`) alors que la carte change.
- traiter les dépassements dans une couche tardive.

## 25.3 Ajouter un filtrage anti-bruit

```vit
form Filter3 {
  a: int
  b: int
  c: int
}
proc filter3_push(f: Filter3, v: int) -> Filter3 {
  give Filter3(f.b, f.c, v)
}
proc filter3_mean(f: Filter3) -> int {
  give (f.a + f.b + f.c) / 3
}
```

Lecture ligne par ligne (débutant):
1. `form Filter3 {` cette ligne ouvre la structure `Filter3` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `Filter3` sans redéfinir ses champs.
2. `a: int` cette ligne déclare le champ `a` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
3. `b: int` cette ligne déclare le champ `b` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
4. `c: int` cette ligne déclare le champ `c` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
5. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
6. `proc filter3_push(f: Filter3, v: int) -> Filter3 {` ce passage fixe le contrat complet de `filter3_push`: entrées `f: Filter3, v: int` et sortie `Filter3`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `filter3_push` retourne toujours une valeur compatible avec `Filter3`.
7. `give Filter3(f.b, f.c, v)` ce passage retourne immédiatement `Filter3(f.b, f.c, v)` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Filter3(f.b, f.c, v)`.
8. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
9. `proc filter3_mean(f: Filter3) -> int {` ici, le contrat complet est défini pour `filter3_mean`: entrées `f: Filter3` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `filter3_mean` retourne toujours une valeur compatible avec `int`.
10. `give (f.a + f.b + f.c) / 3` ici, la branche renvoie immédiatement `(f.a + f.b + f.c) / 3` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `(f.a + f.b + f.c) / 3`.
11. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Filter3(f.b, f.c, v)`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Un simple filtre glissant sur 3 échantillons réduit fortement les oscillations sans coûter cher en CPU.

Lecture pas à pas:
- on pousse la nouvelle valeur.
- on calcule la moyenne entière.
- on propage la valeur filtrée au classifieur.

Erreurs fréquentes à éviter:
- classifier sur le signal brut alors qu'on veut un comportement stable.
- réinitialiser le filtre à chaque itération.
- mélanger filtre et logique d'état dans la même fonction.

## 25.4 Normaliser en pourcentage calibré

```vit
proc to_percent(v: int, cfg: ControllerCfg) -> int {
  let span: int = cfg.max_raw - cfg.min_raw
  if span <= 0 { give 0 }
  give ((v - cfg.min_raw) * 100) / span
}
```

Lecture ligne par ligne (débutant):
1. `proc to_percent(v: int, cfg: ControllerCfg) -> int {` sur cette ligne, le contrat complet est posé pour `to_percent`: entrées `v: int, cfg: ControllerCfg` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `to_percent` retourne toujours une valeur compatible avec `int`.
2. `let span: int = cfg.max_raw - cfg.min_raw` cette ligne crée la variable locale `span` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `span` reçoit ici le résultat de `cfg.max_raw - cfg.min_raw` et peut être réutilisé ensuite sans recalcul.
3. `if span <= 0 { give 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `span <= 0` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give ((v - cfg.min_raw) * 100) / span` sur cette ligne, la sortie est renvoyée immédiatement `((v - cfg.min_raw) * 100) / span` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `((v - cfg.min_raw) * 100) / span`.
5. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `span <= 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `((v - cfg.min_raw) * 100) / span`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

La normalisation doit dépendre de la calibration, pas d'une échelle figée.

À l'exécution:
- avec `min_raw=100`, `max_raw=900`, une valeur `500` donne `50`.
- si `span <= 0`, retour de sécurité `0`.

Erreurs fréquentes à éviter:
- division par `max_raw` au lieu de `span`.
- oublier le cas `span <= 0`.
- mélanger `%` logique et unité ADC brute.

## 25.5 Classifier avec hystérésis (anti-clignotement)

```vit
proc classify_hysteresis(p: int, prev: ControlState, cfg: ControllerCfg) -> ControlState {
  match prev {
    case Alert {
      if p <= cfg.alert_off { give Armed }
      give Alert
    }
  case Armed {
    if p >= cfg.alert_on { give Alert }
    give Armed
  }
case Idle {
  if p >= cfg.alert_on { give Alert }
  give Armed
}
case Fault(code) { give Fault(code) }
otherwise { give Fault(900) }
}
}
```

Lecture ligne par ligne (débutant):
1. `proc classify_hysteresis(p: int, prev: ControlState, cfg: ControllerCfg) -> ControlState {` ce passage fixe le contrat complet de `classify_hysteresis`: entrées `p: int, prev: ControlState, cfg: ControllerCfg` et sortie `ControlState`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `classify_hysteresis` retourne toujours une valeur compatible avec `ControlState`.
2. `match prev {` cette ligne démarre un dispatch déterministe sur `prev`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `prev`, la même branche sera toujours exécutée.
3. `case Alert {` cette ligne décrit le cas `Alert` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Alert`, ce bloc devient le chemin actif.
4. `if p <= cfg.alert_off { give Armed }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `p <= cfg.alert_off` est vrai, `give Armed` est exécuté immédiatement; sinon on continue sur la ligne suivante.
5. `give Alert` ce passage retourne immédiatement `Alert` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Alert`.
6. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
7. `case Armed {` cette ligne décrit le cas `Armed` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Armed`, ce bloc devient le chemin actif.
8. `if p >= cfg.alert_on { give Alert }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `p >= cfg.alert_on` est vrai, `give Alert` est exécuté immédiatement; sinon on continue sur la ligne suivante.
9. `give Armed` ici, la branche renvoie immédiatement `Armed` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Armed`.
10. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
11. `case Idle {` cette ligne décrit le cas `Idle` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Idle`, ce bloc devient le chemin actif.
12. `if p >= cfg.alert_on { give Alert }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `p >= cfg.alert_on` est vrai, `give Alert` est exécuté immédiatement; sinon on continue sur la ligne suivante.
13. `give Armed` sur cette ligne, la sortie est renvoyée immédiatement `Armed` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Armed`.
14. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
15. `case Fault(code) { give Fault(code) }` cette ligne décrit le cas `Fault(code)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Fault(code)`, ce bloc devient le chemin actif.
16. `otherwise { give Fault(900) }` cette ligne définit le chemin de secours pour couvrir les situations non capturées par les cas explicites. Exemple concret: si aucun `case` ne correspond, `give Fault(900)` est exécuté pour garantir une sortie stable.
17. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
18. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `p <= cfg.alert_off` est vrai, la sortie devient `Armed`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Alert`.
- Observation testable: forcer le cas `Alert` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'hystérésis évite les bascules rapides autour du seuil:
- passage en `Alert` au seuil haut `alert_on`.
- retour vers état non critique au seuil bas `alert_off`.

`alert_on` doit être strictement supérieur à `alert_off`.

Erreurs fréquentes à éviter:
- utiliser un seul seuil et subir le clignotement.
- oublier l'état précédent dans la décision.
- forcer la sortie de `Fault` sans diagnostic.

## 25.6 Détecter les fautes de capteur

```vit
proc detect_fault(raw: int, cfg: ControllerCfg) -> ControlState {
  if raw < cfg.min_raw - 100 { give Fault(1001) }
  if raw > cfg.max_raw + 100 { give Fault(1002) }
  give Armed
}
```

Lecture ligne par ligne (débutant):
1. `proc detect_fault(raw: int, cfg: ControllerCfg) -> ControlState {` ici, le contrat complet est défini pour `detect_fault`: entrées `raw: int, cfg: ControllerCfg` et sortie `ControlState`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `detect_fault` retourne toujours une valeur compatible avec `ControlState`.
2. `if raw < cfg.min_raw - 100 { give Fault(1001) }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `raw < cfg.min_raw - 100` est vrai, `give Fault(1001)` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if raw > cfg.max_raw + 100 { give Fault(1002) }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `raw > cfg.max_raw + 100` est vrai, `give Fault(1002)` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give Armed` ce passage retourne immédiatement `Armed` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `Armed`.
5. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `raw < cfg.min_raw - 100` est vrai, la sortie devient `Fault(1001)`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Armed`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

La sécurité embarquée exige un chemin d'erreur explicite:
- signal trop hors plage => `Fault`.
- signal plausible => autorisation du pipeline nominal.

Erreurs fréquentes à éviter:
- traiter une panne capteur comme un simple `Idle`.
- masquer le code de faute.
- continuer à piloter l'actionneur en `Fault`.

## 25.7 Composer une itération complète du contrôleur

```vit
form ControllerState {
  filter: Filter3
  control: ControlState
  percent: int
}
proc step(raw_input: int, st: ControllerState, cfg: ControllerCfg) -> ControllerState {
  let s: Sample = read_raw(raw_input)
  let f0: ControlState = detect_fault(s.raw, cfg)
  match f0 {
    case Fault(code) {
      give ControllerState(st.filter, Fault(code), st.percent)
    }
  otherwise {
    let clean: int = clamp_raw(s, cfg)
    let f1: Filter3 = filter3_push(st.filter, clean)
    let avg: int = filter3_mean(f1)
    let p: int = to_percent(avg, cfg)
    let next_ctrl: ControlState = classify_hysteresis(p, st.control, cfg)
    give ControllerState(f1, next_ctrl, p)
  }
}
}
```

Lecture ligne par ligne (débutant):
1. `form ControllerState {` cette ligne ouvre la structure `ControllerState` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. Exemple concret: plusieurs fonctions peuvent manipuler `ControllerState` sans redéfinir ses champs.
2. `filter: Filter3` cette ligne déclare le champ `filter` avec le type `Filter3`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `Filter3`.
3. `control: ControlState` cette ligne déclare le champ `control` avec le type `ControlState`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `ControlState`.
4. `percent: int` cette ligne déclare le champ `percent` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. Exemple concret: le compilateur refusera une affectation incompatible avec `int`.
5. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
6. `proc step(raw_input: int, st: ControllerState, cfg: ControllerCfg) -> ControllerState {` sur cette ligne, le contrat complet est posé pour `step`: entrées `raw_input: int, st: ControllerState, cfg: ControllerCfg` et sortie `ControllerState`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `step` retourne toujours une valeur compatible avec `ControllerState`.
7. `let s: Sample = read_raw(raw_input)` cette ligne crée la variable locale `s` de type `Sample` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `s` reçoit ici le résultat de `read_raw(raw_input)` et peut être réutilisé ensuite sans recalcul.
8. `let f0: ControlState = detect_fault(s.raw, cfg)` cette ligne crée la variable locale `f0` de type `ControlState` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `f0` reçoit ici le résultat de `detect_fault(s.raw, cfg)` et peut être réutilisé ensuite sans recalcul.
9. `match f0 {` cette ligne démarre un dispatch déterministe sur `f0`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `f0`, la même branche sera toujours exécutée.
10. `case Fault(code) {` cette ligne décrit le cas `Fault(code)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Fault(code)`, ce bloc devient le chemin actif.
11. `give ControllerState(st.filter, Fault(code), st.percent)` ici, la branche renvoie immédiatement `ControllerState(st.filter, Fault(code), st.percent)` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `ControllerState(st.filter, Fault(code), st.percent)`.
12. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
13. `otherwise {` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
14. `let clean: int = clamp_raw(s, cfg)` cette ligne crée la variable locale `clean` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `clean` reçoit ici le résultat de `clamp_raw(s, cfg)` et peut être réutilisé ensuite sans recalcul.
15. `let f1: Filter3 = filter3_push(st.filter, clean)` cette ligne crée la variable locale `f1` de type `Filter3` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `f1` reçoit ici le résultat de `filter3_push(st.filter, clean)` et peut être réutilisé ensuite sans recalcul.
16. `let avg: int = filter3_mean(f1)` cette ligne crée la variable locale `avg` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `avg` reçoit ici le résultat de `filter3_mean(f1)` et peut être réutilisé ensuite sans recalcul.
17. `let p: int = to_percent(avg, cfg)` cette ligne crée la variable locale `p` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `p` reçoit ici le résultat de `to_percent(avg, cfg)` et peut être réutilisé ensuite sans recalcul.
18. `let next_ctrl: ControlState = classify_hysteresis(p, st.control, cfg)` cette ligne crée la variable locale `next_ctrl` de type `ControlState` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `next_ctrl` reçoit ici le résultat de `classify_hysteresis(p, st.control, cfg)` et peut être réutilisé ensuite sans recalcul.
19. `give ControllerState(f1, next_ctrl, p)` sur cette ligne, la sortie est renvoyée immédiatement `ControllerState(f1, next_ctrl, p)` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `ControllerState(f1, next_ctrl, p)`.
20. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
21. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
22. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `ControllerState(st.filter, Fault(code), st.percent)`.
- Observation testable: forcer le cas `Fault(code)` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Cette fonction constitue le cœur du projet:
- entrée brute.
- pipeline de traitement.
- sortie état + métrique.

Elle est déterministe et rejouable.

Erreurs fréquentes à éviter:
- lire deux fois la mesure dans la même itération.
- muter la config pendant le step.
- calculer `percent` sans le signal filtré.

## 25.8 Projeter vers l'actionneur en mode sûr

```vit
proc actuator_pwm(c: ControlState, p: int) -> int {
  match c {
    case Idle { give 0 }
    case Armed { give p / 4 }    # pilotage doux
    case Alert { give 255 }      # pleine puissance / alarme
    case Fault(_) { give 0 }     # fail-safe
    otherwise { give 0 }
  }
}
```

Lecture ligne par ligne (débutant):
1. `proc actuator_pwm(c: ControlState, p: int) -> int {` ce passage fixe le contrat complet de `actuator_pwm`: entrées `c: ControlState, p: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `actuator_pwm` retourne toujours une valeur compatible avec `int`.
2. `match c {` cette ligne démarre un dispatch déterministe sur `c`: une seule branche sera choisie selon la forme de la valeur analysée. Exemple concret: pour la même valeur de `c`, la même branche sera toujours exécutée.
3. `case Idle { give 0 }` cette ligne décrit le cas `Idle` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Idle`, ce bloc devient le chemin actif.
4. `case Armed { give p / 4 }    # pilotage doux` cette ligne décrit le cas `Armed` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Armed`, ce bloc devient le chemin actif.
5. `case Alert { give 255 }      # pleine puissance / alarme` cette ligne décrit le cas `Alert` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Alert`, ce bloc devient le chemin actif.
6. `case Fault(_) { give 0 }     # fail-safe` cette ligne décrit le cas `Fault(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture. Exemple concret: si la valeur analysée correspond à `Fault(_)`, ce bloc devient le chemin actif.
7. `otherwise { give 0 }` cette ligne définit le chemin de secours pour couvrir les situations non capturées par les cas explicites. Exemple concret: si aucun `case` ne correspond, `give 0` est exécuté pour garantir une sortie stable.
8. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
9. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Idle` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

La projection matérielle est volontairement séparée:
- le moteur décide de l'état.
- cette fonction décide de la commande physique.

En `Fault`, la politique est claire: sortie sûre.

Erreurs fréquentes à éviter:
- piloter en direct depuis la valeur brute.
- laisser un `Fault` produire une commande active.
- fusionner calcul métier et PWM dans la même fonction.

## 25.9 Intégrer une boucle end-to-end

```vit
entry main at arduino/app {
  let cfg: ControllerCfg = ControllerCfg(0, 1023, 80, 70)
  let st0: ControllerState = ControllerState(Filter3(0,0,0), Idle, 0)
  let st1: ControllerState = step(200, st0, cfg)
  let st2: ControllerState = step(920, st1, cfg)
  let out: int = actuator_pwm(st2.control, st2.percent)
  if out >= 0 { return 0 }
  return 70
}
```

Lecture ligne par ligne (débutant):
1. `entry main at arduino/app {` cette ligne fixe le point d'entrée `main` dans `arduino/app` et sert de scénario exécutable de bout en bout pour le chapitre. Exemple concret: lancer cette entrée permet de vérifier la chaîne complète des fonctions appelées.
2. `let cfg: ControllerCfg = ControllerCfg(0, 1023, 80, 70)` cette ligne crée la variable locale `cfg` de type `ControllerCfg` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `cfg` reçoit ici le résultat de `ControllerCfg(0, 1023, 80, 70)` et peut être réutilisé ensuite sans recalcul.
3. `let st0: ControllerState = ControllerState(Filter3(0,0,0), Idle, 0)` cette ligne crée la variable locale `st0` de type `ControllerState` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `st0` reçoit ici le résultat de `ControllerState(Filter3(0,0,0), Idle, 0)` et peut être réutilisé ensuite sans recalcul.
4. `let st1: ControllerState = step(200, st0, cfg)` cette ligne crée la variable locale `st1` de type `ControllerState` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `st1` reçoit ici le résultat de `step(200, st0, cfg)` et peut être réutilisé ensuite sans recalcul.
5. `let st2: ControllerState = step(920, st1, cfg)` cette ligne crée la variable locale `st2` de type `ControllerState` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `st2` reçoit ici le résultat de `step(920, st1, cfg)` et peut être réutilisé ensuite sans recalcul.
6. `let out: int = actuator_pwm(st2.control, st2.percent)` cette ligne crée la variable locale `out` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `out` reçoit ici le résultat de `actuator_pwm(st2.control, st2.percent)` et peut être réutilisé ensuite sans recalcul.
7. `if out >= 0 { return 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `out >= 0` est vrai, `return 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
8. `return 70` cette ligne termine l'exécution du bloc courant avec le code `70`, utile pour observer le résultat global du scénario. Exemple concret: un test d'exécution peut vérifier directement que le programme retourne `70`.
9. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return 70`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `70`.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Ce scénario montre le trajet complet:
- initialisation.
- deux itérations de contrôle.
- projection actionneur.

Erreurs fréquentes à éviter:
- testér uniquement des fonctions isolées.
- oublier de testér la transition `Armed -> Alert`.
- valider le code sans valider le comportement fail-safe.

## 25.10 Plan de tests ultra concret

1. Tests de saturation:
- `raw < min_raw` => clamp à `min_raw`.
- `raw > max_raw` => clamp à `max_raw`.

2. Tests de filtre:
- pousser `100,100,100` => moyenne `100`.
- pousser `0,0,1023` => moyenne stable attendue.

3. Tests d'hystérésis:
- `Armed` + `p=81` => `Alert`.
- `Alert` + `p=75` => reste `Alert`.
- `Alert` + `p=69` => `Armed`.

4. Tests de faute:
- `raw` très bas => `Fault(1001)`.
- `raw` très haut => `Fault(1002)`.
- en `Fault`, PWM doit rester `0`.

5. Tests replay:
- même séquence d'inputs => mêmes états et mêmes sorties.

## 25.11 Feuille de route production

Pour aller jusqu'à un vrai firmware:
1. Calibration persistante EEPROM/flash.
2. Watchdog + code de reprise sur redémarrage.
3. Télémetrie série (state, raw, filtered, pwm).
4. Mode simulation offline pour rejouer des captures.
5. Profil énergétique (fréquence de boucle, sleep).


## Table erreur -> diagnostic -> correction

| Erreur | Diagnostic | Correction |
| --- | --- | --- |
| Entrée invalide | Validation absente ou trop tardive. | Centraliser la validation en entrée de pipeline. |
| État incohérent | Mutation partielle ou invariant non vérifié. | Appliquer le principe d'atomicité et rejeter sinon. |
| Sortie inattendue | Couche projection mélangée avec la logique métier. | Séparer `apply` (métier) et `project` (sortie). |

## À retenir

Un contrôleur embarqué robuste ne se résume pas à un seuil. Il combine calibration, filtrage, hystérésis, gestion explicite des fautes et projection fail-safe. Le code devient maintenable quand chaque étape du pipeline est isolée, testable et rejouable.

Critère pratique de qualité pour ce chapitre:
- vous savez expliquer le pipeline complet de `raw_input` à `pwm`.
- vous pouvez prouver que la sortie est sûre en cas de `Fault`.
- vous pouvez rejouer un scénario capteur et retrouver exactement les mêmes transitions d'état.


## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/keywords/at.md`.
- `docs/book/keywords/case.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/entry.md`.
- `docs/book/keywords/form.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

