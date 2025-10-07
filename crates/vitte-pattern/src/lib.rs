//! vitte-pattern — moteur de correspondance de motifs pour Vitte
//!
//! Objectif :
//! - Permettre la correspondance sur les HIR/IR (patterns de structure, constantes, tuples).
//! - Support de variables de motif (bindings).
//! - Fournir une API simple d’évaluation booléenne ou de capture.
//!
//! API :
//! - [`Pattern`] : description d’un motif
//! - [`Match`] : résultat avec captures
//! - [`match_expr`] : vérifie si un HIR expr correspond
//! - [`match_instr`] : vérifie si une instruction IR correspond

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

use anyhow::{Result, bail};
use std::collections::HashMap;
use vitte_hir::{HirExpr, HirBinOp};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Motif générique (HIR ou IR).
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Pattern {
    Wildcard,
    Var(String),
    LitInt(i64),
    LitBool(bool),
    Tuple(Vec<Pattern>),
    Op { op: HirBinOp, args: Vec<Pattern> },
}

/// Résultat de correspondance.
#[derive(Debug, Clone, Default, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Match {
    pub bindings: HashMap<String, String>,
}

impl Match {
    pub fn bind(&mut self, name: &str, val: &str) {
        self.bindings.insert(name.to_string(), val.to_string());
    }
}

/// Tente de faire matcher un motif sur une expression HIR.
pub fn match_expr(pat: &Pattern, expr: &HirExpr) -> Result<Option<Match>> {
    let mut m = Match::default();
    if match_expr_rec(pat, expr, &mut m)? {
        Ok(Some(m))
    } else {
        Ok(None)
    }
}

fn match_expr_rec(pat: &Pattern, expr: &HirExpr, m: &mut Match) -> Result<bool> {
    match (pat, expr) {
        (Pattern::Wildcard, _) => Ok(true),
        (Pattern::Var(name), HirExpr::Var(v, _)) => {
            m.bind(name, v);
            Ok(true)
        }
        (Pattern::LitInt(n), HirExpr::LitInt(v, _)) => Ok(n == v),
        (Pattern::LitBool(b), HirExpr::LitBool(v, _)) => Ok(b == v),
        (Pattern::Tuple(pats), HirExpr::Block(b)) => {
            if pats.len() != b.stmts.len() {
                return Ok(false);
            }
            for (pi, stmt) in pats.iter().zip(&b.stmts) {
                if let vitte_hir::HirStmt::Expr(e) = stmt {
                    if !match_expr_rec(pi, e, m)? {
                        return Ok(false);
                    }
                } else {
                    return Ok(false);
                }
            }
            Ok(true)
        }
        (Pattern::Op { op, args }, HirExpr::Binary { op: eop, lhs, rhs, .. }) => {
            if op != eop {
                return Ok(false);
            }
            if args.len() != 2 {
                bail!("binary op doit avoir 2 args");
            }
            Ok(match_expr_rec(&args[0], lhs, m)? && match_expr_rec(&args[1], rhs, m)?)
        }
        _ => Ok(false),
    }
}

// ============================== Tests =====================================

#[cfg(test)]
mod tests {
    use super::*;
    use vitte_hir::builder;

    #[test]
    fn match_var_expr() {
        let e = builder::var("x");
        let pat = Pattern::Var("a".into());
        let r = match_expr(&pat, &e).unwrap();
        assert!(r.is_some());
        let m = r.unwrap();
        assert_eq!(m.bindings.get("a").unwrap(), "x");
    }
}