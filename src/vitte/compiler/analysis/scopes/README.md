# Vitte Compiler — Scope Analysis

Le répertoire `analysis/scopes` implémente le système de portées lexicales du compilateur Vitte.

Cette couche transforme la structure lexicale issue du HIR en un graphe explicite de scopes utilisable par les passes suivantes du compilateur :

- résolution des noms ;

- résolution des symboles ;

- résolution des chemins ;

- résolution des modules ;

- imports et exports ;

- visibilité ;

- génériques ;

- fonctions et méthodes ;

- patterns ;

- analyse sémantique ;

- vérification des types ;

- diagnostics ;

- compilation incrémentale.

Le système est construit autour de `ScopeId`.

Un `ScopeId` identifie de manière stable une portée lexicale.

Exemple :

```text
module

│

├── form User

│   │

│   ├── method name

│   │   │

│   │   └── block

│   │

│   └── method age

│

└── proc main

    │

    ├── block

    │   │

    │   └── if

    │       │

    │       └── block

    │

    └── block
```


## Architecture

```text
ScopeSubsystem
      │
      ▼
ScopeManager
      │
      ├── ScopeTable
      │     └── ScopeNode
      │
      ├── ScopeRegistry
      ├── ScopeBindingTable
      ├── ScopeSymbolTable
      ├── ScopeImportTable
      ├── ScopeExportTable
      ├── ScopeGenericTable
      ├── ScopeFunctionTable
      ├── ScopePatternTable
      ├── ScopeEdgeTable
      ├── ScopeContext
      └── ScopeLookup
```

## Construction

```text
HIR traversal
    |
    +-- enter scope
    |      ↓
    |   allocate ScopeId
    |      ↓
    |   create ScopeNode
    |
    +-- declaration
    |      ↓
    |   ScopeBinding
    |
    +-- child block
    |      ↓
    |   new ScopeId
    |
    +-- leave scope
           ↓
        close scope

```text
