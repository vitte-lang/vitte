//! vitte-flow — analyses de flux de contrôle et de données
//!
//! Ce crate est autonome. Il définit un IR minimal interne
//! pour construire un CFG et une analyse de vivacité.
//!
//! Types principaux exposés :
//! - `Module`
//! - `Function`
//! - `BasicBlock`
//! - `Instr`
//!
//! API principale :
//! - [`ControlFlowGraph`], [`build_cfg`]
//! - [`Liveness`], [`analyze_liveness`]
//! - [`FlowAnalyzer`]

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use anyhow::Result;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "dot")]
use petgraph::dot::{Config, Dot};
#[cfg(feature = "dot")]
use petgraph::graph::Graph;

/* ------------------------------ IR minimal ------------------------------ */

/// Instruction très simple : destination optionnelle et opérandes textuelles.
#[derive(Debug, Clone, Default)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Instr {
    pub dest: Option<String>,
    pub operands: Vec<String>,
}

impl Instr {
    pub fn assign<D: Into<String>, S: Into<String>>(dest: D, operands: Vec<S>) -> Self {
        Self {
            dest: Some(dest.into()),
            operands: operands.into_iter().map(Into::into).collect(),
        }
    }
}

/// Bloc de base : liste d'instructions et successeurs par indice.
#[derive(Debug, Clone, Default)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct BasicBlock {
    pub instrs: Vec<Instr>,
    pub successors: Vec<usize>,
}

/// Fonction IR minimale.
#[derive(Debug, Clone, Default)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Function {
    pub name: String,
    pub blocks: Vec<BasicBlock>,
}

/// Module IR minimal : liste de fonctions.
#[derive(Debug, Clone, Default)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Module {
    pub name: String,
    pub functions: Vec<Function>,
}

/* ------------------------------ Résultats ------------------------------- */

/// Représente un graphe de flot de contrôle.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct ControlFlowGraph {
    pub function: String,
    #[cfg(feature = "dot")]
    pub graph: Graph<String, String>,
}

impl ControlFlowGraph {
    #[cfg(feature = "dot")]
    pub fn to_dot(&self) -> String {
        format!("{:?}", Dot::with_config(&self.graph, &[Config::EdgeNoLabel]))
    }
}

/// Analyse de vivacité simple : pour chaque bloc, variables vivantes à l’entrée/sortie.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Liveness {
    pub function: String,
    pub live_in: Vec<Vec<String>>,
    pub live_out: Vec<Vec<String>>,
}

/* --------------------------------- API ---------------------------------- */

/// Construit un CFG pour une fonction.
pub fn build_cfg(func: &Function) -> Result<ControlFlowGraph> {
    #[cfg(feature = "dot")]
    {
        let mut g = Graph::<String, String>::new();
        let mut nodes = Vec::new();
        for (i, _bb) in func.blocks.iter().enumerate() {
            let name = format!("B{}", i);
            let idx = g.add_node(name.clone());
            nodes.push((i, idx, name));
        }
        for (i, (_n, idx, _)) in nodes.iter().enumerate() {
            for &succ in &func.blocks[i].successors {
                if let Some((_, tgt_idx, _)) = nodes.iter().find(|(n, _, _)| *n == succ) {
                    g.add_edge(*idx, *tgt_idx, String::new());
                }
            }
        }
        Ok(ControlFlowGraph { function: func.name.clone(), graph: g })
    }
    #[cfg(not(feature = "dot"))]
    {
        Ok(ControlFlowGraph { function: func.name.clone() })
    }
}

/// Analyse de vivacité très simplifiée (heuristique).
pub fn analyze_liveness(func: &Function) -> Result<Liveness> {
    let mut live_in = Vec::new();
    let mut live_out = Vec::new();

    for bb in &func.blocks {
        // naive: variables mentionnées dans instructions
        let mut in_vars = Vec::new();
        let mut out_vars = Vec::new();
        for inst in &bb.instrs {
            in_vars.extend(inst.operands.iter().cloned());
            if let Some(dest) = &inst.dest {
                out_vars.push(dest.clone());
            }
        }
        live_in.push(in_vars);
        live_out.push(out_vars);
    }

    Ok(Liveness { function: func.name.clone(), live_in, live_out })
}

/// Orchestrateur d’analyses.
pub struct FlowAnalyzer;

impl FlowAnalyzer {
    pub fn analyze_module(module: &Module) -> Result<Vec<(ControlFlowGraph, Liveness)>> {
        let mut out = Vec::new();
        for func in &module.functions {
            let cfg = build_cfg(func)?;
            let live = analyze_liveness(func)?;
            out.push((cfg, live));
        }
        Ok(out)
    }
}

// --- Tests ---------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    /// Petit builder de test pour rendre les scénarios lisibles.
    struct ModuleBuilder {
        m: Module,
        cur: Option<usize>,
    }

    impl ModuleBuilder {
        fn new(name: &str) -> Self {
            Self { m: Module { name: name.into(), functions: Vec::new() }, cur: None }
        }
        fn start_function(&mut self, name: &str) {
            self.m.functions.push(Function { name: name.into(), blocks: Vec::new() });
            self.cur = Some(self.m.functions.len() - 1);
        }
        fn add_block(&mut self) -> usize {
            let f = self.cur.expect("no current function");
            self.m.functions[f].blocks.push(BasicBlock::default());
            self.m.functions[f].blocks.len() - 1
        }
        fn set_block(&mut self, _b: usize) {
            // no-op in this minimal builder
        }
        fn add_instr(&mut self, i: Instr) {
            let f = self.cur.expect("no current function");
            let last = self.m.functions[f].blocks.last_mut().expect("no block");
            last.instrs.push(i);
        }
        fn add_edge(&mut self, from: usize, to: usize) {
            let f = self.cur.expect("no current function");
            self.m.functions[f].blocks[from].successors.push(to);
        }
        fn end_function(&mut self) {
            self.cur = None;
        }
        fn finish(self) -> Module {
            self.m
        }
    }

    #[test]
    fn cfg_and_liveness_smoke() {
        let mut builder = ModuleBuilder::new("test");
        builder.start_function("foo");
        let b0 = builder.add_block();
        let b1 = builder.add_block();
        builder.set_block(b0);
        builder.add_instr(Instr::assign("x", vec!["1"]));
        builder.add_instr(Instr::assign("y", vec!["x"]));
        builder.add_edge(b0, b1);
        builder.end_function();
        let m = builder.finish();

        let func = &m.functions[0];
        let cfg = build_cfg(func).unwrap();
        assert_eq!(cfg.function, "foo");

        let live = analyze_liveness(func).unwrap();
        assert_eq!(live.function, "foo");
        assert_eq!(live.live_in.len(), func.blocks.len());
    }
}
