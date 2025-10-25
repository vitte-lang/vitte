//! vitte-green — graphe de dépendances léger pour l'analyseur Vitte.

#![forbid(unsafe_code)]
#![deny(missing_docs, rust_2018_idioms)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{collections::BTreeMap, collections::BTreeSet, fmt, format, string::String, vec::Vec};
#[cfg(feature = "std")]
use std::{collections::BTreeMap, collections::BTreeSet, fmt, format, string::String, vec::Vec};

/// Graphe de dépendances directionnel : `A -> B` signifie "A dépend de B".
#[derive(Default, Clone)]
pub struct DependencyGraph {
    edges: BTreeMap<String, BTreeSet<String>>,
}

impl fmt::Debug for DependencyGraph {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_map().entries(self.edges.iter()).finish()
    }
}

impl DependencyGraph {
    /// Crée un graphe vide.
    pub fn new() -> Self {
        Self::default()
    }

    /// Réinitialise le graphe.
    pub fn clear(&mut self) {
        self.edges.clear();
    }

    /// Ajoute un nœud s'il n'existe pas.
    pub fn add_node<S: Into<String>>(&mut self, node: S) {
        let key = node.into();
        self.edges.entry(key).or_default();
    }

    /// Ajoute une dépendance `from -> to`.
    pub fn add_edge<S, T>(&mut self, from: S, to: T)
    where
        S: Into<String>,
        T: Into<String>,
    {
        let from = from.into();
        let to = to.into();
        self.edges.entry(from).or_default().insert(to);
    }

    /// Retourne les dépendances directes d'un nœud.
    pub fn dependencies(&self, node: &str) -> Vec<String> {
        self.edges.get(node).map(|deps| deps.iter().cloned().collect()).unwrap_or_default()
    }

    /// Itérateur immuable sur les arêtes.
    pub fn iter(&self) -> impl Iterator<Item = (&String, &BTreeSet<String>)> {
        self.edges.iter()
    }

    /// Exporte le graphe au format DOT (graphviz).
    pub fn to_dot(&self) -> String {
        let mut out = String::from("digraph vitte {\n");
        for (node, deps) in &self.edges {
            if deps.is_empty() {
                out.push_str(&format!("    \"{}\";\n", node));
            } else {
                for dep in deps {
                    out.push_str(&format!("    \"{}\" -> \"{}\";\n", node, dep));
                }
            }
        }
        out.push('}');
        out
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn add_edge_records_dependency() {
        let mut graph = DependencyGraph::new();
        graph.add_edge("A", "B");
        assert_eq!(graph.dependencies("A"), vec!["B".into()]);
    }

    #[test]
    fn dot_export_contains_edges() {
        let mut graph = DependencyGraph::new();
        graph.add_edge("A", "B");
        let dot = graph.to_dot();
        assert!(dot.contains("A"));
        assert!(dot.contains("B"));
    }
}
