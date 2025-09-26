#![cfg_attr(not(feature = "std"), no_std)]
//! vitte-typer — **inférence & vérification de types** (style Hindley–Milner) pour Vitte
//!
//! Objectifs solides :
//! - **Agnostique de l’AST** : tu peux brancher n’importe quel parseur/IR.
//! - **Hindley–Milner (W)** : inférence polymorphe rang‑1, `let` généralise, `let rec` supporté.
//! - **Unification** robuste (occurs‑check), substitutions immuables appliquées partout.
//! - **Types usuels** (prim, tuples, fonctions, enregistrements minimalistes) + schémas (`∀`).
//! - **Diagnostics** précis avec `NodeId`.
//! - `no_std`‑friendly via `alloc`, `serde` optionnelle.
//!
//! Un petit **AST de commodité** (`Expr`) est fourni pour tests et pour te montrer l’intégration.
//! Tu peux l’ignorer et pousser tes propres contraintes via l’API de haut niveau.
//!
//! ```ignore
//! use vitte_typer as ty;
//! use vitte_mm::{StrInterner};
//! let mut inter = StrInterner::new();
//! let (x, id) = (inter.intern("x"), inter.intern("id"));
//! let mut t = ty::Engine::default();
//! let e = ty::Expr::Let{
//!     rec: false,
//!     binds: vec![(id, None, ty::Expr::Lam{x, body: Box::new(ty::Expr::Var(x, ty::NodeId(2))), node: ty::NodeId(1)}, ty::NodeId(1))],
//!     body: Box::new(ty::Expr::App(Box::new(ty::Expr::Var(id, ty::NodeId(3))), Box::new(ty::Expr::Lit(ty::Lit::Int(1), ty::NodeId(3))), ty::NodeId(3))),
//!     node: ty::NodeId(0)
//! };
//! let mut env = ty::Env::new();
//! let ty = t.infer_expr(&mut env, &e).unwrap();
//! assert_eq!(format!("{}", ty), "Int");
//! ```

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{
    boxed::Box,
    collections::{BTreeMap as HashMap, BTreeSet as HashSet},
    string::String,
    vec::Vec,
};
#[cfg(feature = "std")]
use std::{
    boxed::Box,
    collections::{HashMap, HashSet},
    string::String,
    vec::Vec,
};

use core::fmt;

// ───────────────────────────── Dépendances douces ─────────────────────────

// Identifiants pour diagnostics : aligne-toi avec vitte-resolver si dispo
#[cfg(feature = "resolver")]
pub use vitte_resolver::NodeId;
#[cfg(not(feature = "resolver"))]
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash, Default)]
pub struct NodeId(pub u32);

// Noms symboliques : réutilise vitte-mm (Symbol)
pub use vitte_mm::{StrInterner, Symbol};

// ───────────────────────────── Types de base ──────────────────────────────

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum Prim {
    Unit,
    Bool,
    Int,
    Float,
    Str,
    Never,
}
impl fmt::Display for Prim {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use Prim::*;
        write!(
            f,
            "{}",
            match self {
                Unit => "()",
                Bool => "Bool",
                Int => "Int",
                Float => "Float",
                Str => "Str",
                Never => "!",
            }
        )
    }
}

/// Identifiant d’une variable de type.
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub struct Tv(u32);
impl Tv {
    fn idx(self) -> u32 {
        self.0
    }
}

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq, Hash)]
pub enum Ty {
    Var(Tv),
    Prim(Prim),
    /// `a -> b`
    Fun(Box<Ty>, Box<Ty>),
    /// `(t1, t2, ...)`
    Tuple(Vec<Ty>),
    /// `{ field: Ty, ... }` — **record très simple** (ordre non significatif)
    Record(Vec<(Symbol, Ty)>),
}

impl Ty {
    pub fn fun(a: Ty, b: Ty) -> Self {
        Ty::Fun(Box::new(a), Box::new(b))
    }
    pub fn tuple(v: Vec<Ty>) -> Self {
        Ty::Tuple(v)
    }
    pub fn record(mut fields: Vec<(Symbol, Ty)>) -> Self {
        fields.sort_by_key(|(s, _)| s.as_u32());
        Ty::Record(fields)
    }
}

impl fmt::Display for Ty {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Ty::Var(Tv(i)) => write!(f, "t{}", i),
            Ty::Prim(p) => write!(f, "{}", p),
            Ty::Fun(a, b) => {
                let la = matches!(**a, Ty::Fun(_, _));
                let lb = false;
                if la {
                    write!(f, "({})", a)?;
                } else {
                    write!(f, "{}", a)?;
                }
                write!(f, " -> ")?;
                if lb {
                    write!(f, "({})", b)
                } else {
                    write!(f, "{}", b)
                }
            },
            Ty::Tuple(ts) => {
                write!(f, "(")?;
                for (i, t) in ts.iter().enumerate() {
                    if i > 0 {
                        write!(f, ", ")?;
                    }
                    write!(f, "{}", t)?;
                }
                write!(f, ")")
            },
            Ty::Record(fs) => {
                write!(f, "{{")?;
                for (i, (name, ty)) in fs.iter().enumerate() {
                    if i > 0 {
                        write!(f, ", ")?;
                    }
                    write!(f, "{}: {}", name.as_u32(), ty)?;
                }
                write!(f, "}}")
            },
        }
    }
}

/// Schéma polymorphe : `forall vars. body`
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq, Hash)]
pub struct Scheme {
    pub vars: Vec<Tv>,
    pub body: Ty,
}
impl Scheme {
    pub fn mono(t: Ty) -> Self {
        Self { vars: Vec::new(), body: t }
    }
}

// ───────────────────────────── Substitutions ─────────────────────────────

#[derive(Clone, Debug, Default)]
pub struct Subst(pub HashMap<Tv, Ty>);

impl Subst {
    pub fn new() -> Self {
        Self(HashMap::new())
    }
    pub fn get(&self, v: Tv) -> Option<&Ty> {
        self.0.get(&v)
    }
    pub fn extend(&mut self, v: Tv, t: Ty) {
        self.0.insert(v, t);
    }
    pub fn compose(&mut self, other: &Subst) {
        // self ∘ other (applique other puis self)
        for (k, v) in other.0.iter() {
            self.0.entry(*k).or_insert_with(|| v.clone());
        }
    }
}

pub trait Apply {
    fn apply(&self, s: &Subst) -> Self;
}

impl Apply for Ty {
    fn apply(&self, s: &Subst) -> Self {
        match self {
            Ty::Var(v) => s.get(*v).cloned().unwrap_or_else(|| self.clone()),
            Ty::Prim(_) => self.clone(),
            Ty::Fun(a, b) => Ty::fun(a.apply(s), b.apply(s)),
            Ty::Tuple(v) => Ty::Tuple(v.iter().map(|t| t.apply(s)).collect()),
            Ty::Record(fs) => Ty::Record(fs.iter().map(|(n, t)| (*n, t.apply(s))).collect()),
        }
    }
}
impl Apply for Scheme {
    fn apply(&self, s: &Subst) -> Self {
        // évite la capture des vars quantifiées
        let mut s2 = Subst::new();
        for (k, v) in s.0.iter() {
            if !self.vars.contains(k) {
                s2.0.insert(*k, v.clone());
            }
        }
        Scheme { vars: self.vars.clone(), body: self.body.apply(&s2) }
    }
}
impl Apply for Env {
    fn apply(&self, s: &Subst) -> Self {
        let mut out = self.clone();
        for (_k, v) in out.0.iter_mut() {
            *v = v.apply(s);
        }
        out
    }
}

// ───────────────────────────── Ensemble des TV libres ────────────────────

fn ftv_ty(t: &Ty, out: &mut HashSet<Tv>) {
    match t {
        Ty::Var(v) => {
            out.insert(*v);
        },
        Ty::Prim(_) => {},
        Ty::Fun(a, b) => {
            ftv_ty(a, out);
            ftv_ty(b, out);
        },
        Ty::Tuple(v) => {
            for t in v {
                ftv_ty(t, out);
            }
        },
        Ty::Record(fs) => {
            for (_n, t) in fs {
                ftv_ty(t, out);
            }
        },
    }
}
fn ftv_scheme(s: &Scheme, out: &mut HashSet<Tv>) {
    let mut tmp = HashSet::new();
    ftv_ty(&s.body, &mut tmp);
    for v in s.vars.iter() {
        tmp.remove(v);
    }
    out.extend(tmp);
}
fn ftv_env(env: &Env) -> HashSet<Tv> {
    let mut out = HashSet::new();
    for s in env.0.values() {
        ftv_scheme(s, &mut out);
    }
    out
}

// ───────────────────────────── Environnement ─────────────────────────────

#[derive(Clone, Debug, Default)]
pub struct Env(pub HashMap<Symbol, Scheme>);
impl Env {
    pub fn new() -> Self {
        Self(HashMap::new())
    }
    pub fn insert(&mut self, name: Symbol, sc: Scheme) {
        self.0.insert(name, sc);
    }
    pub fn get(&self, name: Symbol) -> Option<&Scheme> {
        self.0.get(&name)
    }
}

// ───────────────────────────── Erreurs & Diags ───────────────────────────

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum TypeErrorKind {
    UnboundVar(Symbol),
    Mismatch { expected: Ty, found: Ty },
    OccursCheck { var: Tv, in_ty: Ty },
}
#[derive(Clone, Debug)]
pub struct TypeError {
    pub node: NodeId,
    pub kind: TypeErrorKind,
}
impl fmt::Display for TypeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use TypeErrorKind::*;
        match &self.kind {
            UnboundVar(s) => write!(f, "variable non liée: {}", s.as_u32()),
            Mismatch { expected, found } => {
                write!(f, "type incompatible: attendu {}, trouvé {}", expected, found)
            },
            OccursCheck { var, in_ty } => write!(f, "occurs-check: t{} dans {}", var.idx(), in_ty),
        }
    }
}

pub type TyResult<T> = core::result::Result<T, TypeError>;

// ───────────────────────────── Moteur d’inférence ────────────────────────

#[derive(Default)]
pub struct Engine {
    fresh: u32,
    subst: Subst,
}

impl Engine {
    pub fn fresh_tv(&mut self) -> Tv {
        let id = self.fresh;
        self.fresh += 1;
        Tv(id)
    }

    pub fn instantiate(&mut self, sc: &Scheme) -> Ty {
        // remplace ∀vars par frais
        let mut map: HashMap<Tv, Ty> = HashMap::new();
        for &v in &sc.vars {
            map.insert(v, Ty::Var(self.fresh_tv()));
        }
        let sub = Subst(map);
        sc.body.apply(&sub)
    }

    pub fn generalize(&self, env: &Env, t: &Ty) -> Scheme {
        let mut f = HashSet::new();
        ftv_ty(t, &mut f);
        let env_ftv = ftv_env(env);
        let vars: Vec<Tv> = f.into_iter().filter(|v| !env_ftv.contains(v)).collect();
        Scheme { vars, body: t.clone() }
    }

    fn occurs(&self, v: Tv, t: &Ty) -> bool {
        let t = t.apply(&self.subst);
        match t {
            Ty::Var(v2) => v == v2,
            Ty::Prim(_) => false,
            Ty::Fun(ref a, ref b) => self.occurs(v, a) || self.occurs(v, b),
            Ty::Tuple(ref ts) => ts.iter().any(|t| self.occurs(v, t)),
            Ty::Record(ref fs) => fs.iter().any(|(_, t)| self.occurs(v, t)),
        }
    }

    pub fn unify(&mut self, a: &Ty, b: &Ty, node: NodeId) -> TyResult<()> {
        use Ty::*;
        let ta = a.apply(&self.subst);
        let tb = b.apply(&self.subst);
        match (ta, tb) {
            (Var(v), t) | (t, Var(v)) => {
                if t == Ty::Var(v) {
                    return Ok(());
                }
                if self.occurs(v, &t) {
                    return Err(TypeError {
                        node,
                        kind: TypeErrorKind::OccursCheck { var: v, in_ty: t },
                    });
                }
                self.subst.extend(v, t);
                Ok(())
            },
            (Prim(pa), Prim(pb)) if pa == pb => Ok(()),
            (Fun(a1, b1), Fun(a2, b2)) => {
                self.unify(&a1, &a2, node)?;
                self.unify(&b1, &b2, node)
            },
            (Tuple(x), Tuple(y)) => {
                if x.len() != y.len() {
                    return Err(TypeError {
                        node,
                        kind: TypeErrorKind::Mismatch {
                            expected: Ty::Tuple(x),
                            found: Ty::Tuple(y),
                        },
                    });
                }
                for (t1, t2) in x.iter().zip(y.iter()) {
                    self.unify(t1, t2, node)?;
                }
                Ok(())
            },
            (Record(mut fa), Record(mut fb)) => {
                fa.sort_by_key(|(s, _)| s.as_u32());
                fb.sort_by_key(|(s, _)| s.as_u32());
                if fa.len() != fb.len() {
                    return Err(TypeError {
                        node,
                        kind: TypeErrorKind::Mismatch {
                            expected: Ty::Record(fa),
                            found: Ty::Record(fb),
                        },
                    });
                }
                for ((na, ta), (nb, tb)) in fa.iter().zip(fb.iter()) {
                    if na != nb {
                        return Err(TypeError {
                            node,
                            kind: TypeErrorKind::Mismatch {
                                expected: Ty::Record(fa.clone()),
                                found: Ty::Record(fb.clone()),
                            },
                        });
                    }
                    self.unify(ta, tb, node)?;
                }
                Ok(())
            },
            (ta, tb) => {
                Err(TypeError { node, kind: TypeErrorKind::Mismatch { expected: ta, found: tb } })
            },
        }
    }

    // ─────────── Inférence (petit AST de démo) ───────────

    pub fn infer_expr(&mut self, env: &mut Env, e: &Expr) -> TyResult<Ty> {
        match e {
            Expr::Var(x, node) => {
                let sc = env
                    .get(*x)
                    .ok_or(TypeError { node: *node, kind: TypeErrorKind::UnboundVar(*x) })?;
                Ok(self.instantiate(sc))
            },
            Expr::Lit(l, _) => Ok(match l {
                Lit::Unit => Ty::Prim(Prim::Unit),
                Lit::Bool(_) => Ty::Prim(Prim::Bool),
                Lit::Int(_) => Ty::Prim(Prim::Int),
                Lit::Float(_) => Ty::Prim(Prim::Float),
                Lit::Str(_) => Ty::Prim(Prim::Str),
            }),
            Expr::Lam { x, body, node: _ } => {
                let tv = self.fresh_tv();
                let arg = Ty::Var(tv);
                let sc = Scheme::mono(arg.clone());
                let old = env.0.insert(*x, sc);
                let tb = self.infer_expr(env, body)?;
                if let Some(o) = old {
                    env.0.insert(*x, o);
                } else {
                    env.0.remove(x);
                }
                Ok(Ty::fun(arg, tb))
            },
            Expr::App(fx, ex, node) => {
                let tf = self.infer_expr(env, fx)?;
                let tx = self.infer_expr(env, ex)?;
                let tv = Ty::Var(self.fresh_tv());
                self.unify(&tf, &Ty::fun(tx.clone(), tv.clone()), *node)?;
                Ok(tv.apply(&self.subst))
            },
            Expr::Ann(e1, t_ann, node) => {
                let t1 = self.infer_expr(env, e1)?;
                self.unify(&t1, t_ann, *node)?;
                Ok(t_ann.clone().apply(&self.subst))
            },
            Expr::Let { rec, binds, body, node: _ } => {
                // 1) si rec, pré-binde chaque nom à un type frais monomorphe
                let mut placeholders: Vec<(Symbol, Scheme)> = Vec::new();
                if *rec {
                    for (name, _ann, _rhs, _) in binds.iter() {
                        let v = Ty::Var(self.fresh_tv());
                        let sc = Scheme::mono(v.clone());
                        placeholders.push((*name, sc.clone()));
                        env.insert(*name, sc);
                    }
                }
                // 2) infère les RHS
                for (name, ann, rhs, nid) in binds.iter() {
                    let t_rhs = self.infer_expr(env, rhs)?;
                    // si annotation fournie, on force l’unification
                    if let Some(tann) = ann {
                        self.unify(&t_rhs, tann, *nid)?;
                    }
                    // généralise par rapport à l ENV (HM let-polymorphism)
                    let sc = self.generalize(env, &t_rhs.apply(&self.subst));
                    env.insert(*name, sc);
                }
                // 3) infère le corps
                let t_body = self.infer_expr(env, body)?;
                // 4) nettoie les placeholders (si rec)
                if *rec {
                    for (name, old) in placeholders {
                        let _ = old; /* déjà remplacés par les schémas finales */
                        let _ = env.0.get(&name);
                    }
                }
                Ok(t_body)
            },
        }
    }
}

// ───────────────────────────── AST de démonstration ──────────────────────

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug, PartialEq)]
pub enum Lit {
    Unit,
    Bool(bool),
    Int(i64),
    Float(f64),
    Str(String),
}

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug, PartialEq)]
pub enum Expr {
    Var(Symbol, NodeId),
    Lit(Lit, NodeId),
    Lam { x: Symbol, body: Box<Expr>, node: NodeId },
    App(Box<Expr>, Box<Expr>, NodeId),
    Let { rec: bool, binds: Vec<(Symbol, Option<Ty>, Expr, NodeId)>, body: Box<Expr>, node: NodeId },
    Ann(Box<Expr>, Ty, NodeId),
}

// ───────────────────────────── Helpers pratiques ─────────────────────────

pub mod prelude {
    //! Un prélude minimal : `true`, `false`, `id`, opérateurs numériques currifiés.
    use super::*;
    pub fn load(env: &mut Env, inter: &mut StrInterner) {
        let t_bool = Ty::Prim(Prim::Bool);
        let t_int = Ty::Prim(Prim::Int);
        let t_float = Ty::Prim(Prim::Float);
        // bool constants
        let (s_true, s_false) = (inter.intern("true"), inter.intern("false"));
        env.insert(s_true, Scheme::mono(t_bool.clone()));
        env.insert(s_false, Scheme::mono(t_bool.clone()));
        // id : ∀a. a -> a
        let s_id = inter.intern("id");
        let mut eng = Engine::default();
        let a = Ty::Var(eng.fresh_tv());
        env.insert(
            s_id,
            Scheme {
                vars: vec![match a {
                    Ty::Var(tv) => tv,
                    _ => Tv(0),
                }],
                body: Ty::fun(a.clone(), a),
            },
        );
        // (+) int : Int -> Int -> Int (currifié)
        let s_add = inter.intern("(+)");
        env.insert(
            s_add,
            Scheme::mono(Ty::fun(t_int.clone(), Ty::fun(t_int.clone(), t_int.clone()))),
        );
        // (+.) float : Float -> Float -> Float
        let s_addf = inter.intern("(+.)");
        env.insert(
            s_addf,
            Scheme::mono(Ty::fun(t_float.clone(), Ty::fun(t_float.clone(), t_float.clone()))),
        );
    }
}

// ───────────────────────────── Tests ─────────────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;

    fn inter() -> StrInterner {
        StrInterner::new()
    }

    #[test]
    fn infer_id_int() {
        let mut i = inter();
        let (x, id) = (i.intern("x"), i.intern("id"));
        let mut eng = Engine::default();
        let mut env = Env::new();
        prelude::load(&mut env, &mut i);
        let lam = Expr::Lam { x, body: Box::new(Expr::Var(x, NodeId(2))), node: NodeId(1) };
        let e = Expr::Let {
            rec: false,
            binds: vec![(id, None, lam, NodeId(1))],
            body: Box::new(Expr::App(
                Box::new(Expr::Var(id, NodeId(3))),
                Box::new(Expr::Lit(Lit::Int(1), NodeId(3))),
                NodeId(3),
            )),
            node: NodeId(0),
        };
        let t = eng.infer_expr(&mut env, &e).unwrap();
        assert_eq!(format!("{}", t), "Int");
    }

    #[test]
    fn occurs_check() {
        let mut eng = Engine::default();
        let v = Ty::Var(eng.fresh_tv());
        let t = Ty::fun(v.clone(), Ty::Prim(Prim::Int));
        let err = eng.unify(&v, &t, NodeId(0)).unwrap_err();
        match err.kind {
            TypeErrorKind::OccursCheck { .. } => {},
            _ => panic!("expected occurs-check"),
        }
    }

    #[test]
    fn unify_records() {
        let mut i = inter();
        let a = i.intern("a");
        let b = i.intern("b");
        let t1 = Ty::record(vec![(a, Ty::Prim(Prim::Int)), (b, Ty::Prim(Prim::Bool))]);
        let t2 = Ty::record(vec![(b, Ty::Prim(Prim::Bool)), (a, Ty::Prim(Prim::Int))]);
        let mut eng = Engine::default();
        eng.unify(&t1, &t2, NodeId(0)).unwrap();
    }
}
