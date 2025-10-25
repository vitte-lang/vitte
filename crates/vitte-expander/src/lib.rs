//! vitte-expander — pipeline d'expansion des macros et derives pour Vitte.
//!
//! Relie `vitte-derive` et `vitte-hygiene` afin d'offrir une étape
//! d'expansion pure, testable, et sans I/O implicite.

#![forbid(unsafe_code)]
#![deny(missing_docs, rust_2018_idioms)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(not(feature = "std"))]
extern crate alloc;

use core::fmt;

#[cfg(feature = "std")]
use std::{format, string::String, sync::Arc, vec::Vec};
#[cfg(not(feature = "std"))]
use alloc::{format, string::String, sync::Arc, vec::Vec};

use vitte_ast as ast;
use vitte_derive::{DeriveOutcome, DeriveRegistry};
use vitte_hygiene::{Hygiene, SpanTracker};

/// Type de macro à appliquer.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum MacroKind {
    /// Derive `#[derive(X)]`.
    Derive(String),
    /// Macro personnalisée (non gérée pour l'instant).
    Custom(String),
}

/// Représentation d'une invocation de macro (derive ou custom).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MacroInvocation {
    /// Nom de l'item ciblé (struct/enum/...).
    pub target: String,
    /// Type de macro/derive.
    pub kind: MacroKind,
}

impl MacroInvocation {
    /// Construit une invocation de derive.
    pub fn derive<T: Into<String>, K: Into<String>>(target: T, derive: K) -> Self {
        Self { target: target.into(), kind: MacroKind::Derive(derive.into()) }
    }
}

/// Diagnostic produit pendant l'expansion.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ExpansionDiagnostic {
    /// Message humain.
    pub message: String,
    /// Span associé lorsqu'il est disponible.
    pub span: Option<ast::Span>,
}

/// Résultat d'expansion : programme transformé + diagnostics.
#[derive(Debug, Clone)]
pub struct ExpansionResult {
    /// Programme après application des macros.
    pub program: ast::Program,
    /// Diagnostics produits pendant l'expansion.
    pub diagnostics: Vec<ExpansionDiagnostic>,
    /// Invocations effectivement appliquées.
    pub applied: Vec<MacroInvocation>,
    /// Carte des spans générés -> spans d'origine (macro hygiénique).
    pub span_tracker: SpanTracker,
}

impl ExpansionResult {
    /// Résultat vide (pas d'expansion).
    pub fn empty(program: ast::Program) -> Self {
        Self {
            program,
            diagnostics: Vec::new(),
            applied: Vec::new(),
            span_tracker: SpanTracker::default(),
        }
    }
}

/// Expander principal orchestrant derives et hygiène.
pub struct Expander {
    derives: Arc<DeriveRegistry>,
    hygiene: Hygiene,
}

impl fmt::Debug for Expander {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Expander")
            .field("registered_derives", &self.derives_len())
            .finish()
    }
}

impl Expander {
    /// Crée un expander avec le registre et l'hygiène fournis.
    pub fn with_hygiene(derives: Arc<DeriveRegistry>, hygiene: Hygiene) -> Self {
        Self { derives, hygiene }
    }

    /// Crée un expander avec hygiène par défaut.
    pub fn new(derives: Arc<DeriveRegistry>) -> Self {
        Self::with_hygiene(derives, Hygiene::default())
    }

    fn derives_len(&self) -> usize {
        self.derives.len()
    }

    /// Applique les invocations fournies à `program`.
    pub fn expand(&self, program: &ast::Program, invocations: &[MacroInvocation]) -> ExpansionResult {
        if invocations.is_empty() {
            return ExpansionResult::empty(program.clone());
        }

        let mut items = program.items.clone();
        let mut diagnostics = Vec::new();
        let mut applied = Vec::new();
        let mut tracker = SpanTracker::default();

        for invocation in invocations {
            match &invocation.kind {
                MacroKind::Derive(name) => match self.apply_derive(name, &invocation.target, &mut items, &mut tracker) {
                    Ok(Some(outcome)) => {
                        diagnostics.extend(outcome.diagnostics.into_iter().map(|d| ExpansionDiagnostic {
                            message: d.message,
                            span: d.span,
                        }));
                        applied.push(invocation.clone());
                    }
                    Ok(None) => diagnostics.push(ExpansionDiagnostic {
                        message: format!("derive `{}` non trouvé pour `{}`", name, invocation.target),
                        span: None,
                    }),
                    Err(diag) => diagnostics.push(diag),
                },
                MacroKind::Custom(name) => diagnostics.push(ExpansionDiagnostic {
                    message: format!("macro personnalisée `{}` non supportée", name),
                    span: None,
                }),
            }
        }

        ExpansionResult { program: ast::Program { items }, diagnostics, applied, span_tracker: tracker }
    }

    fn apply_derive(
        &self,
        derive_name: &str,
        target_name: &str,
        items: &mut Vec<ast::Item>,
        tracker: &mut SpanTracker,
    ) -> Result<Option<DeriveOutcome>, ExpansionDiagnostic> {
        let position = items.iter().position(|item| item_name(item) == Some(target_name));
        let idx = match position {
            Some(i) => i,
            None => {
                return Err(ExpansionDiagnostic {
                    message: format!("cible `{target_name}` introuvable pour derive `{derive_name}`"),
                    span: None,
                })
            }
        };

        let base_item = items[idx].clone();
        let outcome = self.derives.apply(derive_name, &base_item, &self.hygiene);
        if let Some(outcome) = &outcome {
            for generated in &outcome.generated {
                if let Some(gen) = item_first_span(generated) {
                    tracker.record(gen, item_first_span(&base_item));
                }
            }
            items.extend(outcome.generated.clone());
        }
        Ok(outcome)
    }
}

fn item_name(item: &ast::Item) -> Option<&str> {
    match item {
        ast::Item::Function(f) => Some(&f.name),
        ast::Item::Const(c) => Some(&c.name),
        ast::Item::Struct(s) => Some(&s.name),
        ast::Item::Enum(e) => Some(&e.name),
    }
}

fn item_first_span(item: &ast::Item) -> Option<ast::Span> {
    match item {
        ast::Item::Function(f) => f.span.or_else(|| f.body.span),
        ast::Item::Const(c) => c.span,
        ast::Item::Struct(s) => s.span,
        ast::Item::Enum(e) => e.span,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use vitte_derive::DeriveRegistry;

    fn sample_struct() -> ast::Program {
        let item = ast::Item::Struct(ast::StructDecl {
            name: "Point".into(),
            fields: vec![ast::Field { name: "x".into(), ty: ast::Type::Int, span: None }],
            span: None,
        });
        ast::Program { items: vec![item] }
    }

    #[test]
    fn derive_debug_generates_function() {
        let program = sample_struct();
        let expander = Expander::new(Arc::new(DeriveRegistry::with_defaults()));
        let result = expander.expand(&program, &[MacroInvocation::derive("Point", "Debug")]);
        assert!(result.applied.len() == 1);
        assert!(result.program.items.iter().any(|it| matches!(it, ast::Item::Function(func) if func.name.contains("debug"))));
    }
}
