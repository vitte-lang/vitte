# Vitte Compiler — Frontend

Ce dossier contient **le frontend du compilateur Vitte**.  
Il est responsable de la transformation du **code source Vitte** en une **représentation AST typée**, prête pour l’analyse sémantique et le lowering vers HIR / MIR.

---

## Rôle du frontend

Le frontend assure les étapes suivantes :

1. **Lexing**
   - Transformation du texte source en flux de tokens
   - Gestion des spans (positions exactes dans le fichier)
   - Détection des erreurs lexicales

2. **Parsing**
   - Construction de l’AST
   - Respect strict de la grammaire *Vitte 1.0 (surface syntax)*
   - Récupération d’erreurs sans abort immédiat

3. **Diagnostics**
   - Centralisation des erreurs, warnings et notes
   - Localisation précise via `SourceSpan`
   - Sortie texte déterministe (CI / tests)

Le frontend **ne fait pas** :
- d’inférence de types
- de vérification sémantique avancée
- de génération de code

Ces étapes sont déléguées aux couches **HIR / MIR / backends**.

---

## AST (Abstract Syntax Tree)

### Principes

- **AST fortement typé**
- **Ownership explicite** (`std::unique_ptr`)
- **NodeKind centralisé**
- **SourceSpan partout**

### SourceSpan

Chaque nœud contient :
- pointeur vers `SourceFile`
- offset `start`
- offset `end`

Cela permet :
- diagnostics précis
- affichage d’extraits
- future intégration IDE / LSP

---

## Lexer

### Responsabilités

- Lecture du flux source
- Production de `Token`
- Attribution des `SourceSpan`
- Ignorer les espaces et commentaires
- Émettre des erreurs lexicales non fatales

### Contraintes

- Aucun état global
- Déterministe
- Zéro allocation cachée critique

---

## Parser

### Stratégie

- Descente récursive
- Lookahead simple (1 token)
- Pas de backtracking
- Erreurs récupérables

### Entrée principale

```cpp
Parser parser(lexer, diagnostics);
Module module = parser.parse_module();