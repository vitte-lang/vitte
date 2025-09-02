# RFC 0018 — Formatting Style and Lints

## Résumé
Ce RFC définit les règles de **formatage** et de **linting** du langage **Vitte**.  
L’objectif est d’assurer une cohérence stylistique, une lisibilité optimale et de prévenir les erreurs courantes grâce à des avertissements et règles automatiques.

## Motivation
Un langage sans style standardisé mène à :  
- des bases de code incohérentes et difficiles à lire,  
- des débats inutiles sur le style,  
- une adoption plus lente dans les équipes.  

Le formatage automatique et les lints intégrés permettent :  
- un code homogène,  
- la détection précoce d’erreurs,  
- l’amélioration de la qualité logicielle.  

## Design proposé

### Formateur officiel : `vitfmt`
- Aligne automatiquement le code au style officiel.  
- S’exécute via :  
```sh
vitfmt src/
```
- Peut être intégré dans les IDE (via LSP).  
- Fonctionne de manière **idempotente** (formatage stable).  

### Règles principales de style
- Indentation **2 espaces**.  
- Largeur maximale de ligne : **100 colonnes**.  
- Accolades obligatoires pour tous les blocs (`if`, `while`, `for`).  
- Espaces autour des opérateurs (`a + b`, `x = 2 * y`).  
- Noms :  
  - variables/fonctions → `snake_case`,  
  - types/structs/enums → `PascalCase`,  
  - constantes → `SCREAMING_SNAKE_CASE`.  

### Exemple
```vitte
fn main() {
  let mut total = 0
  for i in 0..10 {
    total += i
  }
  print(total)
}
```

### Linter officiel : `vitlint`
- Analyse statique des sources.  
- Catégories de lints :  
  - **erreurs** : code invalide ou dangereux,  
  - **warnings** : pratiques déconseillées,  
  - **style** : recommandations non bloquantes.  

### Exemples de lints
- Variable non utilisée → warning.  
- Shadowing de variable → erreur.  
- Bloc vide → warning.  
- Appel `unsafe` non justifié → erreur.  

### Configuration
Fichier `vitte.toml` :  
```toml
[lints]
unused_variables = "warn"
unsafe_code = "deny"
style = "info"
```

### Intégration workflow
- `vitfmt` et `vitlint` intégrés dans `vitpm`.  
- CI :  
```yaml
- run: vitfmt --check src/
- run: vitlint src/
```

## Alternatives considérées
- **Pas de style officiel** (C/C++) : rejeté car crée fragmentation.  
- **Convention libre uniquement** : rejetée pour manque de cohérence.  
- **Linting sans formateur** : rejeté pour productivité inférieure.  

## Impact et compatibilité
- Impact fort : uniformisation de l’écosystème Vitte.  
- Compatible avec LSP et IDEs.  
- Introduit une faible contrainte initiale mais bénéfique à long terme.  

## Références
- [Rustfmt](https://github.com/rust-lang/rustfmt)  
- [Clippy](https://github.com/rust-lang/rust-clippy)  
- [Go fmt](https://go.dev/blog/gofmt)  
