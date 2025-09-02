# RFC 0015 — IDE and LSP Support

## Résumé
Ce RFC définit le support des **environnements de développement (IDE)** et du protocole **LSP (Language Server Protocol)** pour le langage **Vitte**.  
L’objectif est d’offrir une expérience de développement moderne, productive et intégrée.

## Motivation
Un langage ne peut être adopté largement sans un support IDE solide :  
- autocomplétion et navigation rapide,  
- diagnostics et erreurs en temps réel,  
- refactorings sûrs et guidés,  
- intégration avec débogueur, formatter et tests.  

Le protocole LSP permet une **intégration multiplateforme** avec VS Code, Vim/Neovim, Emacs, JetBrains et d’autres outils.

## Design proposé

### Serveur LSP officiel : `vitlsp`
- Inclus dans la toolchain Vitte.  
- Communique avec l’IDE via LSP standard (JSON-RPC).  
- Fournit les fonctionnalités suivantes :  
  - **Autocomplétion** contextuelle.  
  - **Go-to-definition** et navigation symbolique.  
  - **Diagnostics** en temps réel (erreurs, warnings, lints).  
  - **Refactorings** automatiques (renommer, extraire fonction).  
  - **Hover docs** (affichage de la documentation).  
  - **Signature help** (arguments de fonction).  

### Exemple d’intégration VS Code
```jsonc
{
  "name": "Vitte",
  "version": "0.1.0",
  "publisher": "vitte-lang",
  "engines": { "vscode": "^1.70.0" },
  "activationEvents": ["onLanguage:vitte"],
  "main": "./out/extension.js",
  "contributes": {
    "languages": [
      { "id": "vitte", "extensions": [".vit"], "aliases": ["Vitte"] }
    ]
  }
}
```

### Intégration avec le compilateur
- `vitc` expose une **API interne** utilisée par `vitlsp`.  
- Recompilation incrémentale pour feedback instantané.  
- Support de `--json` pour diagnostics structurés.  

### Débogage intégré
- `vitdbg` relié à l’IDE via **DAP (Debug Adapter Protocol)**.  
- Fonctionnalités : breakpoints, step, watch, inspection mémoire.  

### Tests et documentation
- `vitpm test` intégré directement dans l’IDE.  
- `vitdoc` affichable en **hover** dans l’éditeur.  

### Extensibilité
- Plugins IDE peuvent ajouter du support (thèmes, snippets, refactorings personnalisés).  
- LSP extensible avec requêtes spécifiques (`vitte/expand-macro`, `vitte/disasm`).  

## Alternatives considérées
- **Pas de LSP, seulement plugins natifs** : rejeté pour manque de portabilité.  
- **Support limité à VS Code** : rejeté pour viser un écosystème multi-éditeurs.  
- **LSP simplifié** : rejeté car limiterait l’expérience utilisateur.  

## Impact et compatibilité
- Impact fort : adoption accrue grâce à l’expérience développeur.  
- Compatible avec tooling (`vitc`, `vitpm`, `vitdbg`, `vitdoc`).  
- Introduit un coût de maintenance pour suivre les évolutions LSP/DAP.  

## Références
- [Language Server Protocol](https://microsoft.github.io/language-server-protocol/)  
- [Debug Adapter Protocol](https://microsoft.github.io/debug-adapter-protocol/)  
- [Rust Analyzer](https://rust-analyzer.github.io/)  
- [Go Language Server gopls](https://pkg.go.dev/golang.org/x/tools/gopls)  
