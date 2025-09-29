//! vitte-linker — éditeur de liens pour Vitte
//!
//! Objectifs :
//! - Combiner plusieurs modules/objets en un seul binaire ou librairie
//! - Résoudre les symboles importés/exportés
//! - Produire un exécutable natif (via LLVM, option `llvm`) ou un binaire minimal
//! - Optionnel : sérialisation JSON du graphe de dépendances
//!
//! API :
//! - [`Linker`] : orchestrateur
//! - [`LinkInput`] : entrée (module, objet, lib)
//! - [`LinkOutput`] : sortie (bytecode, binaire natif, lib dynamique)

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use anyhow::{bail, Context, Result};
use std::{collections::HashMap, fs, path::PathBuf};
use vitte_ir::Module;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Entrée pour le linker.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct LinkInput {
    pub name: String,
    pub module: Module,
}

/// Sortie après linkage.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct LinkOutput {
    pub binary: Vec<u8>,
    pub symbols: HashMap<String, usize>,
}

/// Résultats possibles.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LinkKind {
    Executable,
    StaticLib,
    SharedLib,
}

/// Éditeur de liens.
pub struct Linker {
    inputs: Vec<LinkInput>,
    pub kind: LinkKind,
}

impl Linker {
    pub fn new(kind: LinkKind) -> Self {
        Self { inputs: Vec::new(), kind }
    }

    pub fn add_input(&mut self, input: LinkInput) {
        self.inputs.push(input);
    }

    /// Lie tous les modules ensemble.
    pub fn link(&self) -> Result<LinkOutput> {
        if self.inputs.is_empty() {
            bail!("aucune entrée fournie au linker");
        }
        // Naïf: concatène le bytecode des modules IR
        let mut binary = Vec::new();
        let mut symbols = HashMap::new();
        for (i, inp) in self.inputs.iter().enumerate() {
            let bc = inp.module.encode()?;
            binary.extend(bc);
            symbols.insert(inp.name.clone(), i);
        }
        Ok(LinkOutput { binary, symbols })
    }

    /// Écrit le binaire en sortie.
    pub fn write_output(&self, out_path: &PathBuf) -> Result<()> {
        let output = self.link()?;
        fs::write(out_path, &output.binary)
            .with_context(|| format!("écriture du binaire {}", out_path.display()))?;
        Ok(())
    }
}

/// Helpers pour fabriquer des entrées de test.
pub fn input_from_module(name: &str, m: Module) -> LinkInput {
    LinkInput { name: name.to_string(), module: m }
}

// ============================= Tests ======================================

#[cfg(test)]
mod tests {
    use super::*;
    use vitte_ir::ModuleBuilder;

    #[test]
    fn link_concat() {
        let mut b = ModuleBuilder::new("a");
        b.start_function("foo");
        b.end_function();
        let m1 = b.finish();

        let mut b2 = ModuleBuilder::new("b");
        b2.start_function("bar");
        b2.end_function();
        let m2 = b2.finish();

        let mut linker = Linker::new(LinkKind::Executable);
        linker.add_input(input_from_module("mod1", m1));
        linker.add_input(input_from_module("mod2", m2));

        let out = linker.link().unwrap();
        assert!(out.binary.len() > 0);
        assert_eq!(out.symbols.len(), 2);
    }
}