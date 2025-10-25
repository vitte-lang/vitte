//! Gestion des cellules compilées au sein d'une session REPL.

use crate::lsp_client::CellDigest;
use std::collections::hash_map::DefaultHasher;
use std::fmt;
use std::hash::Hasher;

/// Résultat de `SessionDocument::submit`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum CellOutcome {
    /// Une nouvelle cellule a été ajoutée et «évaluée» (stub).
    Evaluated {
        /// Sortie textuelle produite par le stub d’évaluation.
        output: String,
    },
    /// Le contenu est identique au précédent, aucune action.
    NoChange,
}

/// Cellule constitutive de la session REPL.
#[derive(Debug, Clone)]
struct Cell {
    source: String,
    hash: u64,
    output: String,
}

impl Cell {
    fn new(source: String) -> Self {
        let hash = hash_source(&source);
        let output = format!("// stub-eval\n{}", source);
        Self { source, hash, output }
    }
}

/// Document représentant la session REPL.
#[derive(Debug, Default)]
pub struct SessionDocument {
    cells: Vec<Cell>,
}

impl SessionDocument {
    /// Ajoute une cellule au document.
    pub fn submit(&mut self, source: String) -> CellOutcome {
        let hash = hash_source(&source);
        if let Some(last) = self.cells.last() {
            if last.hash == hash {
                return CellOutcome::NoChange;
            }
        }

        let cell = Cell::new(source);
        let output = cell.output.clone();
        self.cells.push(cell);
        CellOutcome::Evaluated { output }
    }

    /// Liste les cellules.
    pub fn cells(&self) -> impl Iterator<Item = &str> {
        self.cells.iter().map(|cell| cell.source.as_str())
    }

    /// Retourne les empreintes utilisées pour la synchronisation LSP.
    pub fn digests(&self) -> Vec<CellDigest> {
        self.cells
            .iter()
            .enumerate()
            .map(|(idx, cell)| CellDigest {
                cell_id: format!("cell-{idx}"),
                hash: format!("{:016x}", cell.hash),
            })
            .collect()
    }
}

impl fmt::Display for SessionDocument {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for cell in &self.cells {
            writeln!(f, ">>> {}", cell.source)?;
        }
        Ok(())
    }
}

fn hash_source(src: &str) -> u64 {
    let mut hasher = DefaultHasher::new();
    hasher.write(src.as_bytes());
    hasher.finish()
}
