# RFC 0008 — Macro System

## Résumé
Ce RFC définit le système de **macros** dans le langage **Vitte**.  
Les macros offrent un moyen d’étendre la syntaxe du langage, de générer du code à la compilation et de réduire le boilerplate.  
Elles doivent rester sûres, lisibles et intégrées avec le modèle d’ownership et de typage statique.

## Motivation
Un langage moderne a besoin de macros pour :  
- générer automatiquement du code répétitif,  
- implémenter des patterns comme `println!`, `assert!`, `dbg!`,  
- permettre la métaprogrammation tout en gardant la sûreté,  
- offrir une alternative puissante aux annotations et à la réflexion.  

## Design proposé

### Macros by Example (`macro!`)
Inspirées de Rust, les macros sont définies par motifs et expansions.  

```vitte
macro! println($fmt:expr, $($arg:expr),*) {
    builtin::print(format($fmt, $($arg),*))
}

fn main() {
    println("Hello {}", "Vitte")
}
```

### Macros procédurales
Permettent de transformer le code source en AST.  
Elles s’écrivent comme des fonctions spéciales annotées.  

```vitte
@proc_macro
fn auto_debug(input: TokenStream) -> TokenStream {
    // génère automatiquement une implémentation de Debug
}
```

### Macros dérivées (`@derive`)
Utilisées pour générer du code standard sur les structs/enums.  

```vitte
@derive(Debug, Clone, Eq)
struct Point { x: i32, y: i32 }
```

### Hygienic Macros
- Les macros doivent éviter la capture accidentelle de variables.  
- Chaque expansion est isolée dans son propre scope lexical.  

### Exemple avancé
```vitte
macro! vec($($elem:expr),*) {
    {
        let mut v = Vec::new()
        $( v.push($elem) )*
        v
    }
}

fn main() {
    let nombres = vec![1, 2, 3, 4]
}
```

### Debugging de macros
- Les développeurs peuvent inspecter l’AST généré avec `--dump-macros`.  
- Intégration prévue avec LSP/IDE pour afficher le code expansé.  

## Alternatives considérées
- **Préprocesseur à la C** : rejeté pour manque de sûreté et absence de typage.  
- **Annotations uniquement (Java, C#)** : rejetées pour leur rigidité.  
- **Templates uniquement (C++)** : rejetés pour leur complexité excessive.  

## Impact et compatibilité
- Impact fort : les macros enrichissent la productivité des développeurs.  
- Compatibles avec le modèle d’ownership et le typage statique.  
- Introduisent une complexité supplémentaire pour le compilateur (parser + expansion AST).  
- Risque de surabstraction, mais atténué par les règles d’hygiène.  

## Références
- [Rust Macros](https://doc.rust-lang.org/book/ch19-06-macros.html)  
- [Hygienic Macros](https://en.wikipedia.org/wiki/Hygienic_macro)  
- [Lisp Macros](https://en.wikipedia.org/wiki/Macro_(computer_science))  
