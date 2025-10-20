//! vitte-hir — High-level IR (HIR) pour Vitte
//!
//! But: représenter le programme après parsing, avant typage et MIR/IR.
//! Conserve la structure source (modules, items, blocs, expressions) avec
//! des annotations minimales (hints de type, spans optionnels).
//!
//! Contenu principal:
//! - [`HirModule`] / [`HirItem`]
//! - [`HirFn`], [`HirStmt`], [`HirExpr`], [`HirPattern`], [`HirTypeHint`]
//! - Visiteurs: [`Visit`] / [`VisitMut`]
//! - Helpers: builders et pretty-printer simple
//!
//! Intégration ultérieure (non bloquante ici):
//! - Abaissement AST→HIR (dépendant de `vitte-ast`) via un module `lower`.
//!   Ce module est facultatif et peut évoluer sans casser l’API HIR.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

use anyhow::Result;
use std::fmt;
use std::fmt::Write as _;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Position optionnelle dans le fichier.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Span {
    pub start: u32,
    pub end: u32,
}

impl Span {
    pub fn new(start: u32, end: u32) -> Self {
        Self { start, end }
    }
}

/// Module HIR.
#[derive(Clone, Debug, Default, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct HirModule {
    pub name: String,
    pub items: Vec<HirItem>,
    pub span: Option<Span>,
}

impl HirModule {
    pub fn new(name: impl Into<String>) -> Self {
        Self { name: name.into(), items: Vec::new(), span: None }
    }
    pub fn push(&mut self, it: HirItem) {
        self.items.push(it);
    }
}

/// Items de premier niveau.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum HirItem {
    Fn(HirFn),
    Struct(HirStruct),
    Enum(HirEnum),
    Const(HirConst),
}

/// Fonction.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct HirFn {
    pub name: String,
    pub params: Vec<HirParam>,
    pub ret: Option<HirTypeHint>,
    pub body: HirBlock,
    pub span: Option<Span>,
}

impl HirFn {
    pub fn new(name: impl Into<String>) -> Self {
        Self {
            name: name.into(),
            params: Vec::new(),
            ret: None,
            body: HirBlock { stmts: Vec::new(), span: None },
            span: None,
        }
    }
    pub fn param(mut self, p: HirParam) -> Self {
        self.params.push(p);
        self
    }
    pub fn ret(mut self, t: HirTypeHint) -> Self {
        self.ret = Some(t);
        self
    }
    pub fn body(mut self, b: HirBlock) -> Self {
        self.body = b;
        self
    }
}

/// Paramètre de fonction.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct HirParam {
    pub pat: HirPattern,
    pub ty: Option<HirTypeHint>,
    pub span: Option<Span>,
}

impl HirParam {
    pub fn new(name: impl Into<String>) -> Self {
        Self { pat: HirPattern::Binding(name.into()), ty: None, span: None }
    }
    pub fn ty(mut self, t: HirTypeHint) -> Self {
        self.ty = Some(t);
        self
    }
}

/// Struct.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct HirStruct {
    pub name: String,
    pub fields: Vec<HirField>,
    pub span: Option<Span>,
}

/// Champ de struct.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct HirField {
    pub name: String,
    pub ty: HirTypeHint,
    pub span: Option<Span>,
}

/// Enum.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct HirEnum {
    pub name: String,
    pub variants: Vec<HirVariant>,
    pub span: Option<Span>,
}

/// Variante d'enum.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct HirVariant {
    pub name: String,
    pub fields: Vec<HirTypeHint>, // tuple-like pour simplicité
    pub span: Option<Span>,
}

/// Constante globale.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct HirConst {
    pub name: String,
    pub ty: Option<HirTypeHint>,
    pub value: HirExpr,
    pub span: Option<Span>,
}

/// Type hint minimal (avant typage).
#[derive(Clone, Debug, PartialEq, Eq, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum HirTypeHint {
    Named(String),
    Generic(String, Vec<HirTypeHint>), // Foo<T, U>
    Array(Box<HirTypeHint>, usize),
    Tuple(Vec<HirTypeHint>),
    Unknown, // non annoté
}

impl fmt::Display for HirTypeHint {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Named(s) => write!(f, "{s}"),
            Self::Generic(n, args) => {
                write!(f, "{n}<")?;
                for (i, a) in args.iter().enumerate() {
                    if i > 0 {
                        write!(f, ", ")?;
                    }
                    write!(f, "{a}")?;
                }
                write!(f, ">")
            }
            Self::Array(t, n) => write!(f, "[{}; {}]", t, n),
            Self::Tuple(ts) => {
                write!(f, "(")?;
                for (i, t) in ts.iter().enumerate() {
                    if i > 0 {
                        write!(f, ", ")?;
                    }
                    write!(f, "{t}")?;
                }
                write!(f, ")")
            }
            Self::Unknown => write!(f, "_"),
        }
    }
}

impl From<&str> for HirTypeHint {
    fn from(s: &str) -> Self {
        HirTypeHint::Named(s.to_string())
    }
}

/// Bloc `{ ... }`.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct HirBlock {
    pub stmts: Vec<HirStmt>,
    pub span: Option<Span>,
}

impl HirBlock {
    pub fn new() -> Self {
        Self { stmts: Vec::new(), span: None }
    }
    pub fn push(&mut self, s: HirStmt) {
        self.stmts.push(s);
    }
}

/// Instructions.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum HirStmt {
    Let { pat: HirPattern, ty: Option<HirTypeHint>, init: Option<HirExpr>, span: Option<Span> },
    Expr(HirExpr),
    Return(Option<HirExpr>, Option<Span>),
}

/// Motifs.
#[derive(Clone, Debug, PartialEq, Eq, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum HirPattern {
    Binding(String),
    Tuple(Vec<HirPattern>),
    Wildcard,
}

/// Expressions.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum HirExpr {
    Unit(Option<Span>),
    LitInt(i64, Option<Span>),
    LitFloat(f64, Option<Span>),
    LitBool(bool, Option<Span>),
    LitStr(String, Option<Span>),

    Var(String, Option<Span>),

    Unary { op: HirUnOp, expr: Box<HirExpr>, span: Option<Span> },
    Binary { op: HirBinOp, lhs: Box<HirExpr>, rhs: Box<HirExpr>, span: Option<Span> },

    Call { callee: Box<HirExpr>, args: Vec<HirExpr>, span: Option<Span> },

    If { cond: Box<HirExpr>, then_blk: HirBlock, else_blk: Option<HirBlock>, span: Option<Span> },

    Block(HirBlock),
}

impl HirExpr {
    pub fn span(&self) -> Option<Span> {
        match self {
            Self::Unit(s)
            | Self::LitInt(_, s)
            | Self::LitFloat(_, s)
            | Self::LitBool(_, s)
            | Self::LitStr(_, s)
            | Self::Var(_, s) => *s,
            Self::Unary { span, .. }
            | Self::Binary { span, .. }
            | Self::Call { span, .. }
            | Self::If { span, .. } => *span,
            Self::Block(b) => b.span,
        }
    }
}

/// Unaires.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum HirUnOp {
    Neg,
    Not,
}

/// Binaires.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum HirBinOp {
    Add,
    Sub,
    Mul,
    Div,
    Rem,
    And,
    Or,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    Assign,
}

/* ========================= Visiteurs ==================================== */

/// Visiteur immuable.
pub trait Visit {
    fn visit_module(&mut self, m: &HirModule) {
        for it in &m.items {
            self.visit_item(it);
        }
    }
    fn visit_item(&mut self, it: &HirItem) {
        match it {
            HirItem::Fn(f) => self.visit_fn(f),
            HirItem::Struct(s) => self.visit_struct(s),
            HirItem::Enum(e) => self.visit_enum(e),
            HirItem::Const(c) => self.visit_const(c),
        }
    }
    fn visit_fn(&mut self, f: &HirFn) {
        for p in &f.params {
            self.visit_param(p);
        }
        self.visit_block(&f.body);
    }
    fn visit_param(&mut self, _p: &HirParam) {}
    fn visit_struct(&mut self, _s: &HirStruct) {}
    fn visit_enum(&mut self, _e: &HirEnum) {}
    fn visit_const(&mut self, c: &HirConst) {
        self.visit_expr(&c.value);
    }
    fn visit_block(&mut self, b: &HirBlock) {
        for s in &b.stmts {
            self.visit_stmt(s);
        }
    }
    fn visit_stmt(&mut self, s: &HirStmt) {
        match s {
            HirStmt::Let { init, .. } => {
                if let Some(e) = init {
                    self.visit_expr(e)
                }
            }
            HirStmt::Expr(e) => self.visit_expr(e),
            HirStmt::Return(e, _) => {
                if let Some(e) = e {
                    self.visit_expr(e)
                }
            }
        }
    }
    fn visit_expr(&mut self, e: &HirExpr) {
        match e {
            HirExpr::Unary { expr, .. } => self.visit_expr(expr),
            HirExpr::Binary { lhs, rhs, .. } => {
                self.visit_expr(lhs);
                self.visit_expr(rhs);
            }
            HirExpr::Call { callee, args, .. } => {
                self.visit_expr(callee);
                for a in args {
                    self.visit_expr(a);
                }
            }
            HirExpr::If { cond, then_blk, else_blk, .. } => {
                self.visit_expr(cond);
                self.visit_block(then_blk);
                if let Some(b) = else_blk {
                    self.visit_block(b);
                }
            }
            HirExpr::Block(b) => self.visit_block(b),
            _ => {}
        }
    }
}

/// Visiteur mutable.
pub trait VisitMut {
    fn visit_module_mut(&mut self, m: &mut HirModule) {
        for it in &mut m.items {
            self.visit_item_mut(it);
        }
    }
    fn visit_item_mut(&mut self, it: &mut HirItem) {
        match it {
            HirItem::Fn(f) => self.visit_fn_mut(f),
            HirItem::Struct(s) => self.visit_struct_mut(s),
            HirItem::Enum(e) => self.visit_enum_mut(e),
            HirItem::Const(c) => self.visit_const_mut(c),
        }
    }
    fn visit_fn_mut(&mut self, f: &mut HirFn) {
        for p in &mut f.params {
            self.visit_param_mut(p);
        }
        self.visit_block_mut(&mut f.body);
    }
    fn visit_param_mut(&mut self, _p: &mut HirParam) {}
    fn visit_struct_mut(&mut self, _s: &mut HirStruct) {}
    fn visit_enum_mut(&mut self, _e: &mut HirEnum) {}
    fn visit_const_mut(&mut self, c: &mut HirConst) {
        self.visit_expr_mut(&mut c.value);
    }
    fn visit_block_mut(&mut self, b: &mut HirBlock) {
        for s in &mut b.stmts {
            self.visit_stmt_mut(s);
        }
    }
    fn visit_stmt_mut(&mut self, s: &mut HirStmt) {
        match s {
            HirStmt::Let { init, .. } => {
                if let Some(e) = init {
                    self.visit_expr_mut(e)
                }
            }
            HirStmt::Expr(e) => self.visit_expr_mut(e),
            HirStmt::Return(e, _) => {
                if let Some(e) = e {
                    self.visit_expr_mut(e)
                }
            }
        }
    }
    fn visit_expr_mut(&mut self, e: &mut HirExpr) {
        match e {
            HirExpr::Unary { expr, .. } => self.visit_expr_mut(expr),
            HirExpr::Binary { lhs, rhs, .. } => {
                self.visit_expr_mut(lhs);
                self.visit_expr_mut(rhs);
            }
            HirExpr::Call { callee, args, .. } => {
                self.visit_expr_mut(callee);
                for a in args {
                    self.visit_expr_mut(a);
                }
            }
            HirExpr::If { cond, then_blk, else_blk, .. } => {
                self.visit_expr_mut(cond);
                self.visit_block_mut(then_blk);
                if let Some(b) = else_blk {
                    self.visit_block_mut(b);
                }
            }
            HirExpr::Block(b) => self.visit_block_mut(b),
            _ => {}
        }
    }
}

/* ========================= Pretty printer =============================== */

/// Impression lisible et stable.
pub fn to_pretty_string(module: &HirModule) -> String {
    let mut s = String::new();
    writeln!(&mut s, "module {} {{", module.name).ok();
    for it in &module.items {
        match it {
            HirItem::Fn(f) => {
                write!(&mut s, "  fn {}(", f.name).ok();
                for (i, p) in f.params.iter().enumerate() {
                    if i > 0 {
                        write!(&mut s, ", ").ok();
                    }
                    match &p.pat {
                        HirPattern::Binding(n) => write!(&mut s, "{n}").ok(),
                        HirPattern::Tuple(_) => write!(&mut s, "_").ok(),
                        HirPattern::Wildcard => write!(&mut s, "_").ok(),
                    };
                    if let Some(t) = &p.ty {
                        write!(&mut s, ": {t}").ok();
                    }
                }
                write!(&mut s, ")").ok();
                if let Some(t) = &f.ret {
                    write!(&mut s, " -> {t}").ok();
                }
                writeln!(&mut s, " {{ ... }}").ok();
            }
            HirItem::Struct(st) => {
                writeln!(&mut s, "  struct {} {{ ... }}", st.name).ok();
            }
            HirItem::Enum(en) => {
                writeln!(&mut s, "  enum {} {{ ... }}", en.name).ok();
            }
            HirItem::Const(c) => {
                writeln!(&mut s, "  const {} = <expr>;", c.name).ok();
            }
        }
    }
    writeln!(&mut s, "}}").ok();
    s
}

/* ========================= Builders utilitaires ========================= */

pub mod builder {
    //! Aides de construction ergonomiques pour tests et prototypage.

    use super::*;

    pub struct ModuleBuilder {
        m: HirModule,
    }

    impl ModuleBuilder {
        pub fn new(name: impl Into<String>) -> Self {
            Self { m: HirModule::new(name) }
        }
        pub fn push(mut self, it: HirItem) -> Self {
            self.m.items.push(it);
            self
        }
        pub fn finish(self) -> HirModule {
            self.m
        }
    }

    pub fn fn_(name: &str) -> HirFn {
        HirFn::new(name)
    }

    pub fn param(name: &str) -> HirParam {
        HirParam::new(name)
    }

    pub fn ty(name: &str) -> super::HirTypeHint {
        name.into()
    }

    pub fn let_(name: &str, init: Option<HirExpr>) -> HirStmt {
        HirStmt::Let { pat: HirPattern::Binding(name.into()), ty: None, init, span: None }
    }

    pub fn ret(e: Option<HirExpr>) -> HirStmt {
        HirStmt::Return(e, None)
    }

    pub fn var(name: &str) -> HirExpr {
        HirExpr::Var(name.into(), None)
    }

    pub fn lit_i(n: i64) -> HirExpr {
        HirExpr::LitInt(n, None)
    }

    pub fn call(fun: &str, args: Vec<HirExpr>) -> HirExpr {
        HirExpr::Call { callee: Box::new(var(fun)), args, span: None }
    }

    pub fn block(stmts: Vec<HirStmt>) -> HirBlock {
        HirBlock { stmts, span: None }
    }
}

/* ======================== Abaissement AST→HIR (stub) ==================== */

/// Espace réservé à l'intégration avec `vitte-ast`.
/// Implémentation réelle à fournir dans le compilateur.
pub mod lower {
    use super::*;
    use vitte_ast as ast;

    /// Exemple d’API d’abaissement. Impl incomplète volontairement.
    pub fn module_from_ast(_program: &ast::Program) -> Result<HirModule> {
        // Placeholder: construire un module vide avec le nom de l'AST si dispo.
        // Sans dépendre de la structure interne exacte d’`vitte-ast`.
        let mut m = HirModule::new("module");
        m.items.push(HirItem::Fn(HirFn::new("main")));
        Ok(m)
    }
}

/* =============================== Tests ================================== */

#[cfg(test)]
mod tests {
    use super::*;
    use crate::builder::*;

    #[test]
    fn build_minimal_module() {
        let f = fn_("add")
            .param(param("x").ty(ty("i32")))
            .param(param("y").ty(ty("i32")))
            .ret(ty("i32"))
            .body(block(vec![HirStmt::Expr(HirExpr::Binary {
                op: HirBinOp::Add,
                lhs: Box::new(var("x")),
                rhs: Box::new(var("y")),
                span: None,
            })]));
        let m = ModuleBuilder::new("math").push(HirItem::Fn(f)).finish();
        let pretty = to_pretty_string(&m);
        assert!(pretty.contains("fn add("));
    }

    #[test]
    fn visit_counts_exprs() {
        struct Counter(usize);
        impl Visit for Counter {
            fn visit_expr(&mut self, e: &HirExpr) {
                self.0 += 1;
                Visit::visit_expr(self, e);
            }
        }

        let mut blk = HirBlock::new();
        blk.push(HirStmt::Expr(builder::call("print", vec![builder::lit_i(1)])));
        let f = HirFn::new("main").body(blk);
        let m = HirModule { name: "app".into(), items: vec![HirItem::Fn(f)], span: None };

        let mut c = Counter(0);
        c.visit_module(&m);
        assert!(c.0 >= 2); // call + literal
    }
}
