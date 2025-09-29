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
use vitte_hir::{HirExpr, HirPattern};
use vitte_ir::Instr;

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
    Op { op: String, args: Vec<Pattern> },
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
            match_expr_rec(&args[0], lhs, m)? && match_expr_rec(&args[1], rhs, m)
        }
        _ => Ok(false),
    }
}

/// Tente de faire matcher un motif sur une instruction IR.
pub fn match_instr(pat: &Pattern, instr: &Instr) -> Result<Option<Match>> {
    let mut m = Match::default();
    if match_instr_rec(pat, instr, &mut m)? {
        Ok(Some(m))
    } else {
        Ok(None)
    }
}

fn match_instr_rec(pat: &Pattern, instr: &Instr, m: &mut Match) -> Result<bool> {
    match (pat, instr) {
        (Pattern::Wildcard, _) => Ok(true),
        (Pattern::Var(name), Instr::Assign { dest, .. }) => {
            m.bind(name, dest);
            Ok(true)
        }
        (Pattern::LitInt(n), Instr::Assign { operands, .. }) if operands.len() == 1 => {
            if let Ok(v) = operands[0].parse::<i64>() {
                Ok(*n == v)
            } else {
                Ok(false)
            }
        }
        (Pattern::Op { op, args }, Instr::Bin { op: iop, lhs, rhs, dest }) => {
            if op != iop || args.len() != 3 {
                return Ok(false);
            }
            // args[0] = lhs, args[1] = rhs, args[2] = dest (var)
            let lhs_match = match &args[0] {
                Pattern::Var(n) => { m.bind(n, lhs); true }
                Pattern::LitInt(n) => lhs.parse::<i64>().map_or(false, |v| v == *n),
                _ => true,
            };
            let rhs_match = match &args[1] {
                Pattern::Var(n) => { m.bind(n, rhs); true }
                Pattern::LitInt(n) => rhs.parse::<i64>().map_or(false, |v| v == *n),
                _ => true,
            };
            let dest_match = match &args[2] {
                Pattern::Var(n) => { m.bind(n, dest); true }
                _ => true,
            };
            Ok(lhs_match && rhs_match && dest_match)
        }
        _ => Ok(false),
    }
}

// ============================== Tests =====================================

#[cfg(test)]
mod tests {
    use super::*;
    use vitte_hir::{builder, HirExpr};

    #[test]
    fn match_var_expr() {
        let e = builder::var("x");
        let pat = Pattern::Var("a".into());
        let r = match_expr(&pat, &e).unwrap();
        assert!(r.is_some());
        let m = r.unwrap();
        assert_eq!(m.bindings.get("a").unwrap(), "x");
    }

    #[test]
    fn match_int_instr() {
        let instr = Instr::assign("x", vec!["42".into()]);
        let pat = Pattern::LitInt(42);
        let r = match_instr(&pat, &instr).unwrap();
        assert!(r.is_some());
    }

    #[test]
    fn match_bin_instr() {
        let instr = Instr::Bin { op: "+".into(), lhs: "1".into(), rhs: "2".into(), dest: "z".into() };
        let pat = Pattern::Op {
            op: "+".into(),
            args: vec![Pattern::LitInt(1), Pattern::LitInt(2), Pattern::Var("res".into())],
        };
        let r = match_instr(&pat, &instr).unwrap();
        assert!(r.is_some());
        assert_eq!(r.unwrap().bindings.get("res").unwrap(), "z");
    }
}