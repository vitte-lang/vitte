//! Gestion des cellules compilées au sein d'une session REPL.

use crate::lsp_client::{CellDigest, ExportSymbol};
use std::collections::hash_map::DefaultHasher;
use std::fmt;
use std::hash::Hasher;

/// Résultat de `SessionDocument::submit`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum CellOutcome {
    /// Une nouvelle cellule a été ajoutée et «évaluée» (stub).
    Evaluated {
        /// Index de la cellule dans le document.
        index: usize,
        /// Sortie textuelle produite par le stub d’évaluation.
        output: String,
    },
    /// Le contenu est identique au précédent, aucune action.
    NoChange {
        /// Index de la cellule inchangée.
        index: usize,
    },
}

/// Cellule constitutive de la session REPL.
#[derive(Debug, Clone)]
struct Cell {
    source: String,
    hash: u64,
    output: String,
    version: u32,
    exports: Vec<ExportSymbol>,
}

impl Cell {
    fn new(source: String) -> Self {
        let hash = hash_source(&source);
        let output = format!("// stub-eval\n{}", source);
        Self { source, hash, output, version: 1, exports: Vec::new() }
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
        if let Some((idx, last)) = self.cells.iter().enumerate().last() {
            if last.hash == hash {
                return CellOutcome::NoChange { index: idx };
            }
        }

        let cell = Cell::new(source);
        let output = cell.output.clone();
        self.cells.push(cell);
        let index = self.cells.len() - 1;
        CellOutcome::Evaluated { index, output }
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

    /// Met à jour les exports pour la cellule donnée.
    pub fn update_exports(&mut self, index: usize, exports: Vec<ExportSymbol>) {
        if let Some(cell) = self.cells.get_mut(index) {
            cell.version += 1;
            cell.exports = exports;
        }
    }

    /// Retourne les exports cumulés jusqu'à l'index fourni (exclus).
    pub fn exports_before(&self, index: usize) -> Vec<ExportSymbol> {
        self.cells.iter().take(index).flat_map(|cell| cell.exports.clone()).collect()
    }

    /// Version de la cellule.
    pub fn cell_version(&self, index: usize) -> u32 {
        self.cells.get(index).map(|cell| cell.version).unwrap_or(1)
    }

    /// Nombre total de cellules.
    pub fn len(&self) -> usize {
        self.cells.len()
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
