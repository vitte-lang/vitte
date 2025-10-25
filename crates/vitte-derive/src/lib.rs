//! vitte-derive — gestion centralisée des derives personnalisées.
//!
//! Le but de ce crate est de fournir une **API en mémoire**, simple à serialiser,
//! pour orchestrer des derives sur l'AST Vitte lors de l'expansion des macros.
//! Les derives sont modélisées comme des transformations pures `Item -> [Item]`
//! accompagnées d'éventuels diagnostics.

#![forbid(unsafe_code)]
#![deny(missing_docs, rust_2018_idioms)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(not(feature = "std"))]
extern crate alloc;

use core::fmt;

#[cfg(feature = "std")]
use std::{boxed::Box, format, string::String, vec::Vec};
#[cfg(not(feature = "std"))]
use alloc::{boxed::Box, format, string::String, vec::Vec};

use vitte_ast as ast;
use vitte_hygiene::HygieneProvider;

/// Diagnostic de derive (warning ou erreur non fatale).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DeriveDiagnostic {
    /// Message humain.
    pub message: String,
    /// Span associé (optionnel).
    pub span: Option<ast::Span>,
    /// Gravité du diagnostic.
    pub level: DeriveDiagnosticLevel,
}

/// Gravité d'un diagnostic produit par un derive.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DeriveDiagnosticLevel {
    /// Informationnel.
    Info,
    /// Avertissement.
    Warning,
    /// Erreur non bloquante (le derive continue mais le résultat peut être partiel).
    Error,
}

/// Résultat d'un derive : nouveaux items générés + diagnostics éventuels.
#[derive(Debug, Clone, PartialEq)]
pub struct DeriveOutcome {
    /// Items générés par le derive.
    pub generated: Vec<ast::Item>,
    /// Diagnostics éventuels associés.
    pub diagnostics: Vec<DeriveDiagnostic>,
}

impl DeriveOutcome {
    /// Crée un résultat vide.
    pub fn empty() -> Self {
        Self { generated: Vec::new(), diagnostics: Vec::new() }
    }
}

/// Représentation minimale de la cible d'un derive.
#[derive(Clone, PartialEq)]
pub enum DeriveTarget<'a> {
    /// Derive appliqué sur une structure.
    Struct(&'a ast::StructDecl),
    /// Derive appliqué sur une énumération.
    Enum(&'a ast::EnumDecl),
}

impl<'a> DeriveTarget<'a> {
    /// Nom canonique de la cible.
    pub fn name(&self) -> &str {
        match self {
            DeriveTarget::Struct(s) => &s.name,
            DeriveTarget::Enum(e) => &e.name,
        }
    }
}

/// Contexte passé aux derives afin qu'ils puissent générer des identifiants hygiéniques.
pub struct DeriveContext<'a> {
    target: DeriveTarget<'a>,
    hygiene: &'a dyn HygieneProvider,
}

impl fmt::Debug for DeriveContext<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("DeriveContext").field("target", &self.target.name()).finish()
    }
}

impl<'a> DeriveContext<'a> {
    /// Crée un nouveau contexte.
    pub fn new(target: DeriveTarget<'a>, hygiene: &'a dyn HygieneProvider) -> Self {
        Self { target, hygiene }
    }

    /// Cible du derive.
    pub fn target(&self) -> &DeriveTarget<'a> {
        &self.target
    }

    /// Génère un identifiant hygiénique basé sur `seed`.
    pub fn fresh_ident(&self, seed: &str) -> String {
        self.hygiene.fresh_name(seed)
    }

    /// Produit un span synthétique (ou réutilise le span d'origine si présent).
    pub fn synthesized_span(&self, origin: Option<ast::Span>) -> ast::Span {
        self.hygiene.synthesized_span(origin)
    }
}

/// Handler de derive.
type DeriveHandler = Box<dyn for<'a> Fn(DeriveContext<'a>) -> DeriveOutcome + Send + Sync>;

/// Registre centralisant les derives disponibles.
pub struct DeriveRegistry {
    handlers: Vec<(String, DeriveHandler)>,
}

impl fmt::Debug for DeriveRegistry {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("DeriveRegistry")
            .field("registered", &self.handlers.len())
            .finish()
    }
}

impl DeriveRegistry {
    /// Crée un registre vide.
    pub fn new() -> Self {
        Self { handlers: Vec::new() }
    }

    /// Nombre de derives enregistrés.
    pub fn len(&self) -> usize {
        self.handlers.len()
    }

    /// Crée un registre prérempli avec les derives "de base".
    pub fn with_defaults() -> Self {
        let mut reg = Self::new();
        reg.register("Debug", derive_debug_handler());
        reg.register("Display", derive_display_handler());
        reg
    }

    /// Enregistre un derive sous le nom fourni.
    pub fn register<F>(&mut self, name: &str, handler: F)
    where
        F: for<'a> Fn(DeriveContext<'a>) -> DeriveOutcome + Send + Sync + 'static,
    {
        let boxed = Box::new(move |ctx: DeriveContext<'_>| handler(ctx)) as DeriveHandler;
        self.handlers.push((name.to_string(), boxed));
    }

    /// Applique un derive enregistré sur un item.
    pub fn apply<H>(
        &self,
        name: &str,
        item: &ast::Item,
        hygiene: &H,
    ) -> Option<DeriveOutcome>
    where
        H: HygieneProvider,
    {
        let target = match item {
            ast::Item::Struct(s) => DeriveTarget::Struct(s),
            ast::Item::Enum(e) => DeriveTarget::Enum(e),
            _ => return None,
        };

        let ctx = DeriveContext::new(target, hygiene);
        self.handlers
            .iter()
            .find(|(n, _)| n == name)
            .map(|(_, handler)| handler(ctx))
    }
}

fn derive_debug_handler() -> impl for<'a> Fn(DeriveContext<'a>) -> DeriveOutcome + Send + Sync {
    move |ctx: DeriveContext<'_>| {
        let mut out = DeriveOutcome::empty();
        let name = ctx.target().name();
        let fn_name = ctx.fresh_ident(&format!("__derive_debug_{name}"));
        let span = ctx.synthesized_span(None);

        let func = ast::Function {
            name: fn_name,
            params: vec![ast::Param {
                name: "value".into(),
                ty: ast::Type::Custom(name.into()),
                span: Some(span),
            }],
            return_type: Some(ast::Type::Str),
            body: ast::Block {
                stmts: vec![ast::Stmt::Return(
                    Some(ast::Expr::Literal(ast::Literal::Str(format!("{name} {{ ... }}")))),
                    Some(span),
                )],
                span: Some(span),
            },
            span: Some(span),
        };

        out.generated.push(ast::Item::Function(func));
        out
    }
}

fn derive_display_handler() -> impl for<'a> Fn(DeriveContext<'a>) -> DeriveOutcome + Send + Sync {
    move |ctx: DeriveContext<'_>| {
        let mut out = DeriveOutcome::empty();
        let name = ctx.target().name();
        let fn_name = ctx.fresh_ident(&format!("__derive_display_{name}"));
        let span = ctx.synthesized_span(None);

        let func = ast::Function {
            name: fn_name,
            params: vec![ast::Param {
                name: "value".into(),
                ty: ast::Type::Custom(name.into()),
                span: Some(span),
            }],
            return_type: Some(ast::Type::Str),
            body: ast::Block {
                stmts: vec![ast::Stmt::Return(
                    Some(ast::Expr::Literal(ast::Literal::Str(format!("{name}")))),
                    Some(span),
                )],
                span: Some(span),
            },
            span: Some(span),
        };

        out.generated.push(ast::Item::Function(func));
        out
    }
}
