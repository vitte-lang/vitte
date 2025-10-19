//! vitte-grammar — parseur syntactique généré via pest à partir de la grammaire officielle.

pub mod ast;
mod builder;

use pest::Parser;
use pest::error::InputLocation;
use pest_derive::Parser;
use thiserror::Error;

#[derive(Parser)]
#[grammar = "vitte.pest"]
struct VitteParser;

/// Représente un intervalle d’octets UTF-8 dans la source.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Span {
    pub start: usize,
    pub end: usize,
}

impl Span {
    pub fn new(start: usize, end: usize) -> Self {
        Self { start, end }
    }
}

/// Erreur de parsing produite par le parseur officiel.
#[derive(Debug, Error)]
#[error("{message}")]
pub struct ParseError {
    pub message: String,
    pub span: Option<Span>,
}

impl ParseError {
    fn from_pest(err: pest::error::Error<Rule>) -> Self {
        let message = err.to_string();
        let span = match err.location {
            InputLocation::Pos(pos) => Some(Span::new(pos, pos)),
            InputLocation::Span((start, end)) => Some(Span::new(start, end)),
        };
        Self { message, span }
    }
}

pub type Result<T> = std::result::Result<T, ParseError>;

/// Parse un module Vitte complet. Renvoie une erreur si le code ne respecte pas la grammaire.
pub fn parse_module(source: &str) -> Result<()> {
    VitteParser::parse(Rule::compilation_unit, source)
        .map(|_| ())
        .map_err(ParseError::from_pest)
}

pub use builder::{DiagnosticSeverity, LoweringDiagnostic, LoweringOutcome, parse_and_lower};

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_minimal_module() {
        VitteParser::parse(Rule::identifier, "main").unwrap();
        VitteParser::parse(Rule::module_path, "main").unwrap();
        VitteParser::parse(Rule::module_decl, "module main;").unwrap();
        parse_module("module main;").unwrap();
    }
}
