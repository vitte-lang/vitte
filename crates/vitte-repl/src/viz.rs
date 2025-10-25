//! Support de visualisation (export JSON et rendu ASCII) pour le REPL.

use serde::{Deserialize, Serialize};
use std::fmt::Write;

/// Représentation générique d'une valeur visualisable.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
#[serde(tag = "kind", rename_all = "snake_case")]
pub enum ValueViz {
    /// Valeur nulle.
    Null,
    /// Booléen.
    Bool(bool),
    /// Nombre (f64 pour simplifier).
    Number(f64),
    /// Chaîne de caractères.
    String(String),
    /// Tableau indexé.
    Array(Vec<ValueViz>),
    /// Objet clé/valeur.
    Object(Vec<Field>),
    /// Matrice numérique.
    Matrix {
        /// Lignes de la matrice, chaque sous-vecteur représentant une ligne.
        rows: Vec<Vec<f64>>,
    },
    /// Arbre hiérarchique.
    Tree(TreeNode),
    /// Trace temporelle ou échantillonnée.
    Trace(Vec<TraceSample>),
}

/// Champ d'un objet.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct Field {
    /// Nom du champ.
    pub name: String,
    /// Valeur associée.
    pub value: ValueViz,
}

/// Nœud d'un arbre.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct TreeNode {
    /// Libellé du nœud.
    pub label: String,
    /// Enfants.
    pub children: Vec<TreeNode>,
}

/// Point dans une série temporelle.
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub struct TraceSample {
    /// Horodatage (en secondes).
    pub timestamp: f64,
    /// Valeur.
    pub value: f64,
}

impl ValueViz {
    /// Sérialise en JSON (indenté).
    pub fn to_json(&self) -> serde_json::Result<String> {
        serde_json::to_string_pretty(self)
    }

    /// Désérialise depuis un JSON.
    pub fn from_json_str(src: &str) -> serde_json::Result<Self> {
        serde_json::from_str(src)
    }

    /// Rend un affichage ASCII.
    pub fn render_ascii(&self) -> String {
        let mut out = String::new();
        self.render_ascii_inner(&mut out, 0)
            .expect("writing to string");
        out
    }

    fn render_ascii_inner(&self, out: &mut String, indent: usize) -> std::fmt::Result {
        match self {
            ValueViz::Null => writeln!(out, "null"),
            ValueViz::Bool(b) => writeln!(out, "{b}"),
            ValueViz::Number(n) => writeln!(out, "{n}"),
            ValueViz::String(s) => writeln!(out, "\"{s}\""),
            ValueViz::Array(items) => {
                writeln!(out, "[")?;
                for item in items {
                    indent_line(out, indent + 2)?;
                    item.render_ascii_inner(out, indent + 2)?;
                }
                indent_line(out, indent)?;
                writeln!(out, "]")
            }
            ValueViz::Object(fields) => {
                writeln!(out, "{{")?;
                for field in fields {
                    indent_line(out, indent + 2)?;
                    writeln!(out, "{}:", field.name)?;
                    indent_line(out, indent + 4)?;
                    field.value.render_ascii_inner(out, indent + 4)?;
                }
                indent_line(out, indent)?;
                writeln!(out, "}}")
            }
            ValueViz::Matrix { rows } => {
                let width = rows.iter().map(|r| r.len()).max().unwrap_or(0);
                for row in rows {
                    indent_line(out, indent)?;
                    write!(out, "|")?;
                    for val in row {
                        write!(out, " {:>8.3}", val)?;
                    }
                    for _ in row.len()..width {
                        write!(out, " {:>8}", "")?;
                    }
                    writeln!(out, " |")?;
                }
                Ok(())
            }
            ValueViz::Tree(node) => render_tree(node, out, indent),
            ValueViz::Trace(samples) => {
                indent_line(out, indent)?;
                writeln!(out, "timestamp    value")?;
                for sample in samples {
                    indent_line(out, indent)?;
                    writeln!(out, "{:>9.3}  {:>8.3}", sample.timestamp, sample.value)?;
                }
                Ok(())
            }
        }
    }
}

fn render_tree(node: &TreeNode, out: &mut String, indent: usize) -> std::fmt::Result {
    indent_line(out, indent)?;
    writeln!(out, "{}", node.label)?;
    for child in &node.children {
        render_tree(child, out, indent + 2)?;
    }
    Ok(())
}

fn indent_line(out: &mut String, indent: usize) -> std::fmt::Result {
    for _ in 0..indent {
        out.push(' ');
    }
    Ok(())
}
