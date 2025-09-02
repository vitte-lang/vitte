# Vitte — Completions

## Introduction
Le système de **completions** de Vitte est conçu pour améliorer la productivité des développeurs.  
Il s’intègre principalement via le **LSP (Language Server Protocol)** et le **CLI**, en fournissant des suggestions contextuelles et intelligentes.

---

## Objectifs
- Offrir une autocomplétion **rapide et pertinente**
- Couvrir les mots-clés, variables, fonctions, imports et modules
- Intégrer l’autocomplétion au **CLI** et aux **éditeurs modernes**
- Fournir un système **extensible** (snippets, templates, signatures)
- Permettre la complétion même en cas de code partiel ou invalide

---

## Types de Completions

### 1. Mots-clés du langage
Exemple : `fn`, `let`, `const`, `return`, `if`, `else`, `while`, `for`

### 2. Variables locales
Suggestions basées sur le scope courant :
```vit
let count = 10
cou|    # suggère: count
```

### 3. Fonctions définies
Complétion de signatures :
```vit
fn add(x: int, y: int) -> int { ... }

ad|   # suggère: add(x: int, y: int) -> int
```

### 4. Imports et modules
Complétion automatique des modules disponibles :
```vit
import ma|   # suggère: math, macros, map
```

### 5. Stdlib et API natives
Accès rapide aux fonctions de la stdlib :
```vit
math.sq|   # suggère: sqrt(), square()
```

### 6. Snippets et Templates
Exemples :
- Boucle for :
```vit
for let i = 0; i < N; i = i + 1 {
    |
}
```
- Fonction :
```vit
fn name(params) -> type {
    |
}
```

---

## Intégration CLI

Le CLI `vitte` supporte l’autocomplétion shell :

### Bash
```bash
source <(vitte completions bash)
```

### Zsh
```bash
vitte completions zsh > ~/.zsh/completions/_vitte
```

### Fish
```bash
vitte completions fish | source
```

### PowerShell
```powershell
vitte completions powershell | Out-String | Invoke-Expression
```

---

## Intégration LSP

Le serveur LSP Vitte fournit :  
- `completionItem/resolve` pour signatures et docstrings
- `completionItem/snippetSupport` pour templates
- `completionProvider` configuré pour les fichiers `.vit`

Exemple JSON (LSP response) :
```json
{
  "label": "add",
  "kind": 3,
  "detail": "fn add(x: int, y: int) -> int",
  "insertText": "add(${1:x}, ${2:y})",
  "insertTextFormat": 2
}
```

---

## Roadmap Completions
- [x] Mots-clés, variables, fonctions
- [x] Imports et stdlib
- [x] Snippets de base
- [ ] Détection intelligente de types pour suggestions
- [ ] Refactorings (rename + completion)
- [ ] API de plugins pour completions externes
- [ ] Complétions contextuelles basées sur l’analyse de flux

---

## Conclusion
Les **completions Vitte** unifient l’expérience CLI et IDE.  
Elles permettent aux développeurs d’écrire plus vite, avec moins d’erreurs, et avec une productivité accrue.  
En combinant **LSP, CLI et snippets**, elles font de Vitte un langage moderne, agréable et efficace à utiliser.