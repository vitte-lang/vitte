
# Vitte — Grammar & Syntax

Ce dossier contient la **référence officielle de la syntaxe du langage Vitte**.
Il constitue la **source de vérité** pour le parser, le compilateur et tous les outils associés
(formateur, LSP, analyse statique, etc.).

---

## 🎯 Objectif de la grammaire

La grammaire Vitte a été conçue avec les objectifs suivants :

- lisibilité humaine maximale
- syntaxe déterministe et compiler‑friendly
- absence d’ambiguïtés (priorités simples)
- blocs explicites terminés par `.end`
- séparation claire entre **surface syntax** et **noyau sémantique**
- facilité de bootstrap (stage0 → stage2)

---

## 📂 Contenu du dossier

```
grammar/
├── README.md        # ce document
├── vitte.ebnf       # grammaire EBNF officielle (référence)
└── vitte.pest       # grammaire Pest (parser implémentation)
```

---

## 📘 Fichiers de référence

### `vitte.ebnf`

- grammaire **canonique**
- utilisée pour :
  - documentation
  - validation conceptuelle
  - génération d’outils
- fait autorité sur **toute autre représentation**

### `vitte.pest`

- grammaire **opérationnelle**
- utilisée par le frontend du compilateur
- doit rester **strictement alignée** avec `vitte.ebnf`
- toute divergence est un bug

---

## 🧩 Principes syntaxiques clés

### 1. Blocs explicites

Vitte **n’utilise pas d’accolades** `{}`.

Tous les blocs sont terminés par :

```vit
.end
```

Exemple :

```vit
proc add(a: i32, b: i32) -> i32
    give a + b
.end
```

---

### 2. Mots-clés structurants

Top-level :

- `space` — module
- `pull` — import
- `share` — export
- `type` / `form` / `pick` — types
- `proc` / `entry` — exécutables

Contrôle :

- `if / elif / else`
- `loop`
- `select / when`
- `give`
- `emit`
- `assert`

---

### 3. Syntaxe orientée lisibilité

- indentation logique (non significative)
- pas de ponctuation inutile
- priorité faible aux symboles
- verbes explicites (`give`, `emit`, `pull`…)

---

## 🧠 Séparation surface / noyau

La grammaire distingue :

### 🔹 Syntaxe de surface

Conçue pour l’utilisateur :

```vit
set x = 10
say "hello"
```

### 🔹 Noyau compilateur

Forme normalisée interne, utilisée par le lowering MIR.

---

## 🧪 Tests et validation

Toute évolution de la grammaire doit :

1. mettre à jour `vitte.ebnf`
2. mettre à jour `vitte.pest`
3. passer les tests :
   - parsing
   - diagnostics
   - round‑trip (format → parse → AST)

---

## 🚫 Règles strictes

- ❌ aucune accolade `{ }`
- ❌ aucun bloc implicite
- ❌ aucun mot‑clé ambigu
- ❌ aucune extension non documentée

---

## 🔗 Lien avec le reste du projet

- le runtime **ne dépend pas** de cette grammaire
- le backend **ne connaît pas** la syntaxe
- seule la partie **frontend C++** consomme ces fichiers

---

## 🏁 Statut

- **stable**
- **référence officielle**
- toute modification doit être discutée et validée

---

Vitte Grammar — source of truth.
