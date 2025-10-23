

# vitte-readline

Bibliothèque **d’édition de ligne et d’entrée interactive** pour le langage Vitte.  
`vitte-readline` offre une interface performante et portable pour la saisie interactive, la complétion, l’historique et la gestion du terminal, utilisée dans `vitte-cli` et `vitte-repl`.

---

## Objectifs

- Offrir une **API ergonomique et portable** d’édition de ligne.  
- Supporter l’historique, la complétion, la coloration et la recherche incrémentale.  
- Fournir un comportement cohérent sur macOS, Linux et Windows.  
- Intégration directe avec `vitte-prompt`, `vitte-cli`, `vitte-repl` et `vitte-playground`.  
- Support complet du mode non-interactif et redirection stdin.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `editor`      | Gestion du buffer d’édition et des opérations clavier |
| `history`     | Stockage et navigation dans les commandes précédentes |
| `completion`  | Système de complétion contextuelle |
| `highlight`   | Coloration syntaxique et surlignage |
| `keymap`      | Définition des raccourcis clavier (Emacs, Vi) |
| `tty`         | Interaction bas-niveau avec le terminal |
| `tests`       | Vérification de cohérence et compatibilité terminal |

---

## Exemple d’utilisation

```rust
use vitte_readline::Readline;

fn main() -> anyhow::Result<()> {
    let mut rl = Readline::new("vitte> ");
    while let Some(line) = rl.read_line()? {
        if line.trim() == "exit" { break; }
        println!("Entrée: {}", line);
    }
    Ok(())
}
```

---

## Intégration

- `vitte-cli` : gestion des invites et entrées utilisateur.  
- `vitte-repl` : lecture interactive avec complétion et historique.  
- `vitte-playground` : saisie des snippets multi-lignes.  
- `vitte-prompt` : interface simplifiée pour scripts non interactifs.  
- `vitte-studio` : simulation de sessions REPL en interface graphique.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

rustyline = "12.0"
crossterm = "0.27"
console = "0.15"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `unicode-width` pour calcul précis du curseur, `dirs` pour stockage d’historique.

---

## Tests

```bash
cargo test -p vitte-readline
```

- Tests de lecture et complétion.  
- Tests de compatibilité multi-plateforme.  
- Tests de performance sur entrées longues.  
- Vérification de cohérence des séquences TTY.

---

## Roadmap

- [ ] Mode Vi complet.  
- [ ] Intégration native avec `vitte-prompt` et `vitte-cli`.  
- [ ] Système d’autocomplétion intelligent basé sur le contexte.  
- [ ] Historique partagé entre sessions.  
- [ ] Visualisation et configuration dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau