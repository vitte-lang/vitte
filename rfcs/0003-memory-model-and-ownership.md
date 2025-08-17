# RFC 0003 — Memory Model and Ownership

## Résumé
Ce RFC définit le modèle mémoire du langage **Vitte** ainsi que le système d’ownership et de gestion des ressources.  
Il s’inspire du modèle de Rust (ownership, borrowing, lifetimes) tout en conservant une syntaxe simple et une ergonomie proche de C#.

## Motivation
Une gestion mémoire claire et sûre est essentielle pour :
- éviter les erreurs classiques (use-after-free, double free, fuite mémoire),
- garantir des performances natives comparables au C et à Rust,
- fournir une base cohérente pour le bytecode, la VM et l’interopérabilité FFI,
- permettre une programmation concurrente sans data races.

## Design proposé

### Principes fondamentaux
1. **Ownership** : chaque valeur a un propriétaire unique responsable de sa libération.
2. **Borrowing** : les valeurs peuvent être empruntées temporairement :
   - immuables (`&T`) en nombre illimité,
   - mutables (`&mut T`) en un seul exemplaire.
3. **Lifetimes** : la durée de vie d’une référence est vérifiée statiquement par le compilateur.
4. **Move semantics** : l’affectation déplace la valeur, sauf si le type implémente `Copy`.

### Exemple simple
```vitte
fn main() {
    let s = String::new("bonjour")
    let t = s        // move : `s` n’est plus valide
    print(t)
}
```

### Emprunts
```vitte
fn longueur(s: &String) -> usize {
    s.len()
}

fn main() {
    let texte = String::new("abc")
    let l = longueur(&texte) // emprunt immuable
    print(l)
}
```

### Emprunt mutable exclusif
```vitte
fn ajoute_point(s: &mut String) {
    s.push('.')
}

fn main() {
    let mut t = String::new("test")
    ajoute_point(&mut t)
    print(t) // "test."
}
```

### Règles d’aliasing
- Plusieurs emprunts immuables (`&T`) sont autorisés en parallèle.  
- Un seul emprunt mutable (`&mut T`) peut exister, et il exclut toute référence immuable active.

### Libération mémoire
- Automatique à la sortie de portée.  
- Déterministe, pas de garbage collector.  
- Basé sur **RAII** (Resource Acquisition Is Initialization).

### Types spéciaux
- `Box<T>` : allocation sur le tas avec ownership unique.  
- `Rc<T>` : compteur de références non thread-safe.  
- `Arc<T>` : compteur de références thread-safe.  
- `Weak<T>` : référence non possédante évitant les cycles.  

### Concurrence et sécurité
- `Arc<Mutex<T>>` et `Arc<RwLock<T>>` fournis dans la stdlib.  
- Pas de **data races** possibles si les règles d’emprunts sont respectées.  
- L’ownership s’applique aussi aux canaux (`Channel<T>`).  

## Alternatives considérées
- **Garbage Collector (Go, Java)** : rejeté pour conserver la performance déterministe et éviter les pauses.  
- **Comptage de références implicite (Swift, Python)** : rejeté car sujet aux cycles mémoire et overhead runtime.  
- **Gestion manuelle (C, C++)** : rejetée pour la sûreté et l’ergonomie.  

## Impact et compatibilité
- Impact majeur : ce RFC est au cœur de la sûreté mémoire de Vitte.  
- Compatible avec les optimisations du compilateur (inline, escape analysis).  
- Simplifie l’interopérabilité FFI grâce à la prévisibilité des durées de vie.  
- Facilite l’écriture de bibliothèques concurrentes et embarquées.  

## Références
- [Rust Ownership and Borrowing](https://doc.rust-lang.org/book/ch04-00-understanding-ownership.html)  
- [C++ RAII Idiom](https://en.cppreference.com/w/cpp/language/raii)  
- [Go Memory Management](https://go.dev/doc/faq#garbage_collection)  
