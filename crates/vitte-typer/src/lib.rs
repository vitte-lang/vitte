//! vitte-typer — inférence et vérification de types pour Vitte
//!
//! Portée : HIR de base (entiers, booléens, variables, blocs, binaires `+ - * /`,
//! `if ... else ...`). API stable, sans `unsafe`.
//!
//! Points clés :
//! - Unification avec substitution (occurs-check).
//! - Environnement simple (monomorphique) pour variables.
//! - Inférence d’expression `infer_expr`, vérification `check_binop`, `check_if`.
//! - Export JSON optionnel de l’état (env + substitutions) si `feature = "json"`.
//!
//! Intégrations : dépend de `vitte-hir` et `vitte-ast` (pour signatures futures).
//!
//! Limitations actuelles : pas de polymorphisme let-généralisation, ni d’annotations users.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::doc_markdown,
    clippy::module_name_repetitions,
    clippy::too_many_lines,
    clippy::match_same_arms
)]

use std::collections::HashMap;

use anyhow::{bail, Context, Result};
use thiserror::Error;

use vitte_hir::{HirExpr, HirStmt};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/* ============================== Types & Subst =============================== */

/// Identifiant d’une variable de type.
#[derive(Clone, Copy, PartialEq, Eq, Hash, Debug)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct TyVar(u32);

/// Représentation des types.
#[derive(Clone, PartialEq, Eq, Debug)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Type {
    Var(TyVar),
    Int,
    Bool,
    Unit,
    /// Fonction (args -> ret). Non encore inférée par défaut mais prêt.
    Func(Vec<Type>, Box<Type>),
}

impl Type {
    pub fn var(id: TyVar) -> Self { Self::Var(id) }
    pub fn is_var(&self) -> bool { matches!(self, Type::Var(_)) }
    pub fn as_var(&self) -> Option<TyVar> { if let Type::Var(v) = self { Some(*v) } else { None } }
}

/// Substitution de variables de types vers types concrets.
#[derive(Default, Clone, Debug)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Subst {
    map: HashMap<TyVar, Type>,
}

impl Subst {
    pub fn new() -> Self { Self { map: HashMap::new() } }

    pub fn apply(&self, t: &Type) -> Type {
        match t {
            Type::Var(v) => {
                if let Some(tt) = self.map.get(v) {
                    self.apply(tt)
                } else {
                    Type::Var(*v)
                }
            }
            Type::Func(ps, r) => {
                let ps2 = ps.iter().map(|p| self.apply(p)).collect();
                let r2 = Box::new(self.apply(r));
                Type::Func(ps2, r2)
            }
            _ => t.clone(),
        }
    }

    pub fn bind(&mut self, v: TyVar, t: Type) {
        self.map.insert(v, t);
    }

    pub fn merge(&mut self, other: &Subst) {
        for (k, v) in &other.map {
            self.map.insert(*k, v.clone());
        }
    }
}

/* ============================== Erreurs ===================================== */

/// Erreurs de typage.
#[derive(Error, Debug)]
pub enum TypeError {
    #[error("incompatibilité de types: attendu {expected:?}, trouvé {found:?}")]
    Mismatch { expected: Type, found: Type },

    #[error("variable non définie: {0}")]
    UnknownVar(String),

    #[error("occurs check échoué: {0:?} ∋ {1:?}")]
    Occurs(TyVar, Type),

    #[error("expression non supportée par l’inférence")]
    Unsupported,
}

/* ============================== Contexte ==================================== */

/// Environnement d’inférence (mono) + générateur de tyvars.
#[derive(Clone, Debug, Default)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct InferCtx {
    next: u32,
    pub env: HashMap<String, Type>,
    pub subst: Subst,
}

impl InferCtx {
    pub fn new() -> Self { Self::default() }

    pub fn fresh(&mut self) -> Type {
        let id = TyVar(self.next);
        self.next += 1;
        Type::Var(id)
    }

    pub fn lookup(&self, name: &str) -> Option<Type> {
        self.env.get(name).cloned()
    }

    pub fn annotate(&mut self, name: impl Into<String>, ty: Type) {
        self.env.insert(name.into(), ty);
    }

    pub fn apply(&self, t: &Type) -> Type { self.subst.apply(t) }
}

/* ============================== Unification ================================= */

fn occurs(v: TyVar, t: &Type, s: &Subst) -> bool {
    match s.apply(t) {
        Type::Var(w) => v == w,
        Type::Func(ps, ref r) => ps.iter().any(|p| occurs(v, p, s)) || occurs(v, &*r, s),
        _ => false,
    }
}

/// Unifie `a` et `b`, retourne la substitution additionnelle.
fn unify(a: &Type, b: &Type, s: &mut Subst) -> Result<()> {
    let a = s.apply(a);
    let b = s.apply(b);
    if a == b {
        return Ok(());
    }
    match (a, b) {
        (Type::Var(v), t) | (t, Type::Var(v)) => {
            if occurs(v, &t, s) {
                bail!(TypeError::Occurs(v, t));
            }
            s.bind(v, t);
            Ok(())
        }
        (Type::Int, Type::Int) | (Type::Bool, Type::Bool) | (Type::Unit, Type::Unit) => Ok(()),
        (Type::Func(pa, ra), Type::Func(pb, rb)) => {
            if pa.len() != pb.len() {
                bail!(TypeError::Mismatch { expected: Type::Func(pa, ra), found: Type::Func(pb, rb) });
            }
            // unifier paramètres puis ret
            for (x, y) in pa.iter().zip(pb.iter()) {
                unify(x, y, s)?;
            }
            unify(&ra, &rb, s)
        }
        (x, y) => bail!(TypeError::Mismatch { expected: x, found: y }),
    }
}

/* ============================== Inférence HIR =============================== */

/// Infère le type d’une expression HIR, met à jour `ctx.subst`.
pub fn infer_expr(ctx: &mut InferCtx, e: &HirExpr) -> Result<Type> {
    use HirExpr::*;
    let t = match e {
        HirExpr::LitInt(_, _) => Type::Int,
        HirExpr::LitBool(b, _) => {
            let _ = b; // valeur non pertinente pour le type
            Type::Bool
        }
        HirExpr::Var(name, _) => {
            ctx.lookup(name).ok_or_else(|| TypeError::UnknownVar(name.clone()))?
        }
        HirExpr::Binary { op, lhs, rhs, .. } => {
            // Support: + - * / → Int × Int -> Int
            let tl = infer_expr(ctx, lhs)?;
            let tr = infer_expr(ctx, rhs)?;
            check_binop(ctx, op, &tl, &tr)?;
            Type::Int
        }
        HirExpr::If { cond, then_blk, else_blk, .. } => {
            let tc = infer_expr(ctx, cond)?;
            unify(&tc, &Type::Bool, &mut ctx.subst)?;
            let tt = infer_block(ctx, then_blk)?;
            let te = else_blk
                .as_ref()
                .map(|b| infer_block(ctx, b))
                .transpose()?
                .unwrap_or(Type::Unit);
            unify(&tt, &te, &mut ctx.subst)?;
            ctx.apply(&tt)
        }
        HirExpr::Block(b) => infer_block(ctx, b)?,
        _ => bail!(TypeError::Unsupported),
    };
    Ok(ctx.apply(&t))
}

/// Infère le type d’un bloc : type du dernier statement expression, sinon `Unit`.
pub fn infer_block(ctx: &mut InferCtx, b: &vitte_hir::Block) -> Result<Type> {
    let mut last_ty = Type::Unit;
    for s in &b.stmts {
        match s {
            HirStmt::Expr(e) => {
                last_ty = infer_expr(ctx, e)?;
            }
            // On pourrait supporter let/ret si présents dans HIR réel.
            _ => {}
        }
    }
    Ok(ctx.apply(&last_ty))
}

/// Vérifie les contraintes d’un opérateur binaire arithmétique.
fn check_binop(ctx: &mut InferCtx, op: &str, tl: &Type, tr: &Type) -> Result<()> {
    match op {
        "+" | "-" | "*" | "/" => {
            unify(tl, &Type::Int, &mut ctx.subst)?;
            unify(tr, &Type::Int, &mut ctx.subst)?;
            Ok(())
        }
        _ => bail!(TypeError::Unsupported),
    }
}

/* ============================== Utilitaires ================================= */

/// Infère le type, retourne le type normalisé (substitution appliquée) et le contexte.
pub fn infer_top(e: &HirExpr) -> Result<(Type, InferCtx)> {
    let mut ctx = InferCtx::new();
    let ty = infer_expr(&mut ctx, e)?;
    Ok((ctx.apply(&ty), ctx))
}

/// Ajoute une variable typée dans l’environnement puis infère `body`.
pub fn with_binding(
    mut ctx: InferCtx,
    name: impl Into<String>,
    ty: Type,
    body: &HirExpr,
) -> Result<(Type, InferCtx)> {
    ctx.annotate(name, ty);
    let t = infer_expr(&mut ctx, body)?;
    Ok((ctx.apply(&t), ctx))
}

/// Formate un type en chaîne compacte.
pub fn fmt_type(t: &Type) -> String {
    match t {
        Type::Var(TyVar(id)) => format!("t{}", id),
        Type::Int => "Int".into(),
        Type::Bool => "Bool".into(),
        Type::Unit => "Unit".into(),
        Type::Func(ps, r) => {
            let a = ps.iter().map(fmt_type).collect::<Vec<_>>().join(", ");
            format!("({a}) -> {}", fmt_type(r))
        }
    }
}

/// Aide: crée un type fonction.
pub fn ty_func(params: &[Type], ret: Type) -> Type {
    Type::Func(params.to_vec(), Box::new(ret))
}

#[cfg(feature = "json")]
/// Exporte l’état (env + substitutions) au format JSON.
pub fn to_json_state(ctx: &InferCtx) -> anyhow::Result<String> {
    #[derive(Serialize)]
    struct J<'a> {
        env: HashMap<&'a str, String>,
        subst: HashMap<String, String>,
    }
    let env = ctx
        .env
        .iter()
        .map(|(k, v)| (k.as_str(), fmt_type(&ctx.apply(v))))
        .collect::<HashMap<_, _>>();
    let subst = ctx
        .subst
        .map
        .iter()
        .map(|(k, v)| (format!("t{}", k.0), fmt_type(&ctx.apply(v))))
        .collect::<HashMap<_, _>>();
    Ok(serde_json::to_string_pretty(&J { env, subst })?)
}

/* =================================== Tests =================================== */

#[cfg(test)]
mod tests {
    use super::*;
    use vitte_hir::{builder, HirExpr, HirStmt};

    #[test]
    fn infer_literals() {
        let i = builder::int(42);
        let b = builder::bool_(true);
        let (ti, _) = infer_top(&i).unwrap();
        let (tb, _) = infer_top(&b).unwrap();
        assert_eq!(fmt_type(&ti), "Int");
        assert_eq!(fmt_type(&tb), "Bool");
    }

    #[test]
    fn infer_binop_int() {
        let e = HirExpr::Binary {
            op: "+".into(),
            lhs: Box::new(builder::int(1)),
            rhs: Box::new(builder::int(2)),
            span: Default::default(),
        };
        let (t, _) = infer_top(&e).unwrap();
        assert_eq!(fmt_type(&t), "Int");
    }

    #[test]
    fn infer_if_merges_branch_types() {
        // if true { 1 } else { 2 }
        let e = builder::if_else(builder::bool_(true), builder::int(1), builder::int(2));
        let (t, _) = infer_top(&e).unwrap();
        assert_eq!(fmt_type(&t), "Int");
    }

    #[test]
    fn infer_block_last_expr_type() {
        // { 1; true }
        let mut blk = vitte_hir::Block { stmts: vec![], span: Default::default() };
        blk.stmts.push(HirStmt::Expr(builder::int(1)));
        blk.stmts.push(HirStmt::Expr(builder::bool_(true)));
        let e = HirExpr::Block(blk);
        let (t, _) = infer_top(&e).unwrap();
        assert_eq!(fmt_type(&t), "Bool");
    }

    #[test]
    fn unknown_variable_errors() {
        let e = builder::var("x");
        let err = infer_top(&e).unwrap_err();
        let s = err.to_string();
        assert!(s.contains("variable non définie"));
    }

    #[cfg(feature = "json")]
    #[test]
    fn json_state_exports() {
        let e = HirExpr::Binary {
            op: "+".into(),
            lhs: Box::new(builder::int(1)),
            rhs: Box::new(builder::int(2)),
            span: Default::default(),
        };
        let (_, ctx) = infer_top(&e).unwrap();
        let j = to_json_state(&ctx).unwrap();
        assert!(j.contains("env"));
        assert!(j.contains("subst"));
    }
}