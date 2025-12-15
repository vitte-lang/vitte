

# Fuzz corpora — vitte (max)

Ce dossier contient les **corpora** (ensembles d’inputs) utilisés par les fuzzers de Vitte.

Objectifs :
- Accélérer la découverte de bugs via des seeds utiles.
- Conserver des cas minimisés qui régressent (crashers → repros → tests).
- Garder une organisation **déterministe** et **portable** (CI/local).

Non-objectifs :
- Stocker des gros artefacts binaires non nécessaires.
- Stocker des données sous licence incompatible.

---

## 1) Règles générales

- Une **corpus** = un répertoire de fichiers inputs (bytes arbitraires).
- Le contenu doit être :
  - petit (idéalement < 1–5 MiB total par fuzzer, hors cas spéciaux)
  - pertinent (inputs qui augmentent la couverture)
  - stable (pas de dépendance machine)

- Éviter :
  - inputs générés aléatoirement sans valeur (pas de couverture)
  - dumps massifs non minimisés
  - contenus sensibles ou privés

- Si un input déclenche un crash :
  1) le minimiser
  2) le convertir en test (unit/prop) quand possible
  3) garder l’input minimisé dans la corpus **uniquement** si utile pour coverage ou regression

---

## 2) Arborescence recommandée

Ce répertoire est organisé par **fuzzer**.

Exemple :

```
corpora/
  README.md
  muf_parser/
    seed_0001.muf
    seed_0002.muf
  vitte_lexer/
    seed_0001.vitte
  vitte_parser/
    seed_0001.vitte
    seed_0002.vitte
  vitte_fmt/
    seed_0001.vitte
  runtime_loader/
    seed_0001.bin
  json_emitter/
    seed_0001.json
```

Règles :
- un sous-dossier = un fuzzer (nom stable)
- les fichiers peuvent avoir une extension “humaine” (ex: `.vitte`, `.muf`) **mais** sont traités comme bytes

---

## 3) Conventions de nommage

Recommandation :
- `seed_0001.*`, `seed_0002.*` pour seeds “curatés”
- `crash_<hash>.*` (temporaire) pour inputs crash avant conversion en test
- `regress_<ticket>_<hash>.*` si besoin de conserver un input regression (rare)

Si un runner impose un format particulier (ex: libFuzzer), ces noms restent compatibles.

---

## 4) Formats d’inputs

### 4.1 Seeds textuels

- `.vitte` : sources langage Vitte
- `.muf` : manifests Muffin
- `.json` : JSON (si fuzzer cible JSON emitter/parser)

Même si “text”, l’input est traité comme **bytes**.
- UTF-8 valide recommandé mais pas requis, selon le composant fuzzé.

### 4.2 Seeds binaires

- `.bin` : bytes arbitraires
- utile pour fuzzers sur VM/bytecode/loader/dylib stubs

Règle :
- si un format binaire est défini (ex: header), documenter dans un README du sous-dossier.

---

## 5) Flux de travail (local)

### 5.1 Ajouter une seed

1) Placer un fichier dans `corpora/<fuzzer>/`.
2) Lancer le fuzzer en pointant sur ce dossier.
3) Si le fuzzer génère des nouveaux inputs (coverage), minimiser/filtrer avant commit.

### 5.2 Minimisation (recommandé)

- Toujours minimiser un crasher :
  - réduire le fichier au plus petit input qui reproduit
- Minimiser les nouveaux inputs :
  - garder seulement ceux qui apportent une couverture différente

Le “comment” dépend du moteur (libFuzzer/AFL/etc.).

### 5.3 Déduplication

Avant commit :
- supprimer doublons évidents
- limiter le nombre de seeds redondantes

---

## 6) Flux de travail (CI)

But :
- exécuter un fuzz short-run pour détecter régressions rapides.

Recommandation CI :
- durée courte (ex: 30–120s par fuzzer)
- exécution sur la corpus versionnée

Si la CI découvre un crasher :
- sauver l’input minimisé comme artefact CI
- ouvrir un ticket
- ajouter un test regression

---

## 7) Politique “crashers”

- Un input crasher ne doit pas rester indéfiniment dans la corpus.
- Après fix :
  - préférer un **test** (unit/prop) au lieu de garder le fichier
  - ne conserver dans la corpus que si l’input améliore la couverture de manière significative

---

## 8) Directories par composant (guide)

Noms recommandés (adapter au repo réel) :

- `muf_parser/` : parse MUF (`spec/muf/*`)
- `vitte_lexer/` : lexer Vitte
- `vitte_parser/` : parser Vitte
- `vitte_semantics/` : lowering / name resolution / typer (si fuzzable)
- `vitte_fmt/` : formatter
- `runtime_loader/` : loader runtime (modules, init)
- `abi_headers/` : parse/génération headers ABI
- `json_emitter/` : emitter JSON (si présent)

Chaque sous-dossier peut contenir son propre `README.md` si le format d’input a des contraintes.

---

## 9) Qualité et sécurité

### 9.1 Données interdites

Ne pas committer :
- secrets (tokens, clés, credentials)
- dumps de mémoire
- fichiers provenant de sources non libres / non redistribuables

### 9.2 Licences

Les seeds doivent être :
- soit générées en interne
- soit issues de sources redistribuables

Si une seed provient d’un projet externe (rare), ajouter une note :
- source + licence + URL (dans README du sous-dossier)

---

## 10) Bonnes pratiques (coverage)

Seeds “utiles” (exemples) :
- fichiers valides (happy paths) → donnent une base de parsing
- fichiers presque valides (erreurs proches) → explorent recovery
- combinaisons de tokens/constructs rares
- cas limites :
  - nombres extrêmes
  - strings longues
  - nesting profond
  - unicode edge cases
  - `.end` manquants / multiples

Éviter :
- inputs trop gros
- inputs purement aléatoires

---

## 11) Checklist avant commit

- [ ] seed minimisée
- [ ] seed apporte couverture (ou repro important)
- [ ] pas de données sensibles
- [ ] taille raisonnable
- [ ] nommage conforme
- [ ] si crasher : test regression ajouté (ou ticket créé)