//! vitte-fronted — couche frontale commune du compilateur Vitte.
//!
//! Ce module est volontairement minimal pour garantir la compilation.
//! vitte-fronted — couche frontale commune du compilateur Vitte.
//!
//! Fournit les structures et outils de base utilisés par les backends LLVM/WASM,
//! notamment :
//! - gestion des unités de compilation
//! - contexte global de compilation
//! - IR symbolique simplifié (pré-IR)
//! - interface commune Frontend
//!
//! Ce module est stable et sans dépendance de bas niveau.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::too_many_lines, clippy::missing_errors_doc)]

use anyhow::{anyhow, Result};

/// Représente une unité de compilation (fichier source Vitte ou script).
#[derive(Debug, Clone)]
pub struct FrontendUnit {
    pub name: String,
    pub source: String,
}

impl FrontendUnit {
    /// Crée une nouvelle unité de compilation à partir d’un nom et du contenu source.
    pub fn new<N: Into<String>, S: Into<String>>(name: N, source: S) -> Self {
        Self { name: name.into(), source: source.into() }
    }

    /// Vérifie la validité de l’unité (par ex. source non vide).
    pub fn validate(&self) -> Result<()> {
        if self.source.trim().is_empty() {
            Err(anyhow!("Empty source for unit `{}`", self.name))
        } else {
            Ok(())
        }
    }
}

/// Contexte de compilation partagé entre frontends et backends.
#[derive(Debug, Clone)]
pub struct FrontendContext {
    pub target: String,
    pub optimize: bool,
}

impl Default for FrontendContext {
    fn default() -> Self {
        Self { target: "native".to_string(), optimize: false }
    }
}

impl FrontendContext {
    /// Change la cible (par ex. `"wasm32"` ou `"x86_64"`).
    pub fn set_target<S: Into<String>>(&mut self, target: S) {
        self.target = target.into();
    }

    /// Active les optimisations.
    pub fn enable_optimizations(&mut self) {
        self.optimize = true;
    }
}

/// IR intermédiaire symbolique minimal (pré-IR).
#[derive(Debug, Clone)]
pub struct FrontendIR {
    pub unit_name: String,
    pub symbols: Vec<String>,
}

impl FrontendIR {
    /// Décrit le contenu du pré-IR.
    pub fn describe(&self) -> String {
        format!("FrontendIR: {} symbols in {}", self.symbols.len(), self.unit_name)
    }
}

/// Frontend principal : interface commune pour l’analyse et la préparation.
#[derive(Debug, Default)]
pub struct Frontend;

impl Frontend {
    /// Analyse une unité et génère un IR symbolique de base.
    pub fn analyze(&self, unit: &FrontendUnit) -> Result<FrontendIR> {
        unit.validate()?;
        Ok(FrontendIR { unit_name: unit.name.clone(), symbols: vec!["main".to_string()] })
    }

    /// Prépare la compilation pour un backend spécifique (LLVM, WASM, etc.).
    pub fn prepare(&self, ctx: &FrontendContext, ir: &FrontendIR) -> Result<String> {
        let mode = if ctx.optimize { "optimized" } else { "debug" };
        Ok(format!("Preparing `{}` for target `{}` in {} mode", ir.unit_name, ctx.target, mode))
    }
}

/// Exporte une fonction de vérification pour les tests unitaires.
pub fn ping() -> &'static str {
    "vitte-fronted ready"
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_unit_validation() {
        let ok = FrontendUnit::new("ok", "fn main() {}");
        assert!(ok.validate().is_ok());

        let empty = FrontendUnit::new("empty", "");
        assert!(empty.validate().is_err());
    }

    #[test]
    fn test_frontend_analysis() {
        let f = Frontend::default();
        let u = FrontendUnit::new("demo", "fn main() {}");
        let ir = f.analyze(&u).unwrap();
        assert_eq!(ir.unit_name, "demo");
        assert!(ir.symbols.contains(&"main".to_string()));
    }

    #[test]
    fn test_prepare() {
        let f = Frontend::default();
        let u = FrontendUnit::new("demo", "fn main() {}");
        let ir = f.analyze(&u).unwrap();

        let mut ctx = FrontendContext::default();
        ctx.set_target("wasm32");
        ctx.enable_optimizations();

        let out = f.prepare(&ctx, &ir).unwrap();
        assert!(out.contains("wasm32"));
        assert!(out.contains("optimized"));
    }

    #[test]
    fn test_ping() {
        assert_eq!(ping(), "vitte-fronted ready");
    }
}
