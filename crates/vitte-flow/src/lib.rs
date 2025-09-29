//! vitte-flow — analyses de flux de contrôle et de données pour Vitte
//!
//! Fournit des passes IR pour :
//! - Construire un graphe de flot de contrôle (CFG)
//! - Analyser la portée et la vivacité des variables
//! - Simplifier le code en éliminant le code mort
//! - Exporter en DOT (option `dot`)
//!
//! API principale :
//! - [`ControlFlowGraph`] : graphe CFG
//! - [`Liveness`] : analyse de vivacité
//! - [`FlowAnalyzer`] : orchestrateur
//! - Fonctions `build_cfg`, `analyze_liveness`

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use anyhow::Result;
use vitte_ir::{BasicBlock, Function, Module};
use vitte_errors::Error;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "dot")]
use petgraph::dot::{Config, Dot};
#[cfg(feature = "dot")]
use petgraph::graph::Graph;

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

/// Construit un CFG pour une fonction.
pub fn build_cfg(func: &Function) -> Result<ControlFlowGraph> {
    #[cfg(feature = "dot")]
    {
        let mut g = Graph::<String, String>::new();
        let mut nodes = Vec::new();
        for (i, bb) in func.blocks.iter().enumerate() {
            let name = format!("B{}", i);
            let idx = g.add_node(name.clone());
            nodes.push((bb, idx, name));
        }
        for (i, (bb, idx, _)) in nodes.iter().enumerate() {
            for succ in &bb.successors {
                if let Some(&(_, tgt_idx, _)) = nodes.get(*succ) {
                    g.add_edge(*idx, tgt_idx, String::new());
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
    use vitte_ir::{Instr, ModuleBuilder};

    #[test]
    fn cfg_and_liveness_smoke() {
        let mut builder = ModuleBuilder::new("test");
        let f = builder.start_function("foo");
        let b0 = builder.add_block();
        builder.set_block(b0);
        builder.add_instr(Instr::assign("x", vec!["1".into()]));
        builder.add_instr(Instr::assign("y", vec!["x".into()]));
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