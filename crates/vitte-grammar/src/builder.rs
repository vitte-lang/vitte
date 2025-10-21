use crate::ast::{self, Ast, Ident, Span as AstSpan};
use crate::{ParseError, Rule, VitteParser};
use pest::iterators::Pair;
use pest::Parser;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DiagnosticSeverity {
    Error,
    Warning,
}

#[derive(Clone, Debug)]
pub struct LoweringDiagnostic {
    pub severity: DiagnosticSeverity,
    pub message: String,
    pub span: Option<AstSpan>,
}

pub struct LoweringOutcome {
    pub ast: Ast,
    pub diagnostics: Vec<LoweringDiagnostic>,
}

impl std::fmt::Debug for LoweringOutcome {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("LoweringOutcome")
            .field("ast", &"<Ast>") // avoid requiring Ast: Debug
            .field("diagnostics", &self.diagnostics)
            .finish()
    }
}

pub fn parse_and_lower(source: &str) -> Result<LoweringOutcome, ParseError> {
    let mut pairs =
        VitteParser::parse(Rule::compilation_unit, source).map_err(ParseError::from_pest)?;
    let root_pair = pairs.next().expect("compilation_unit pair");

    let mut lowerer = Lowerer::new();
    lowerer.lower_compilation_unit(root_pair);
    Ok(lowerer.finish())
}

struct Lowerer {
    ast: Ast,
    diagnostics: Vec<LoweringDiagnostic>,
    module_declared: bool,
}

impl Lowerer {
    fn new() -> Self {
        Self { ast: Ast::new(), diagnostics: Vec::new(), module_declared: false }
    }

    fn finish(self) -> LoweringOutcome {
        LoweringOutcome { ast: self.ast, diagnostics: self.diagnostics }
    }

    fn lower_compilation_unit(&mut self, pair: Pair<'_, Rule>) {
        for inner in pair.into_inner() {
            match inner.as_rule() {
                Rule::module_decl => self.lower_module_decl(inner),
                Rule::top_item => self.unsupported(inner, "items de haut niveau"),
                Rule::spacing => {}
                other => self.unexpected(other, inner),
            }
        }
    }

    fn lower_module_decl(&mut self, pair: Pair<'_, Rule>) {
        if self.module_declared {
            let span = Self::to_ast_span(pair.as_span());
            self.push_diag(
                DiagnosticSeverity::Error,
                "Plusieurs déclarations de module sont présentes.",
                Some(span),
            );
            return;
        }
        self.module_declared = true;

        let span_id = self.alloc_span(pair.as_span());
        let mut module_name: Option<Ident> = None;

        for inner in pair.clone().into_inner() {
            match inner.as_rule() {
                Rule::module_path => {
                    let idents: Vec<Pair<'_, Rule>> = inner
                        .clone()
                        .into_inner()
                        .filter(|p| p.as_rule() == Rule::identifier)
                        .collect();
                    if let Some(first) = idents.first() {
                        module_name = Some(Self::ident_from_pair((*first).clone()));
                    } else {
                        self.push_diag(
                            DiagnosticSeverity::Error,
                            "La déclaration de module est vide.",
                            Some(Self::to_ast_span(inner.as_span())),
                        );
                    }
                    for extra in idents.iter().skip(1) {
                        self.push_diag(
                            DiagnosticSeverity::Warning,
                            "Les modules imbriqués dans `module` ne sont pas encore pris en charge.",
                            Some(Self::to_ast_span(extra.as_span())),
                        );
                    }
                }
                Rule::spacing => {}
                other => self.unexpected(other, inner),
            }
        }

        if let Some(module) = self.ast.storage.modules.get_mut(self.ast.root()) {
            module.span = span_id;
            if module.name.is_none() {
                module.name = module_name.or_else(|| Some(Ident::new("main", AstSpan::dummy())));
            } else {
                module.name = module_name;
            }
        }
    }

    fn unsupported(&mut self, pair: Pair<'_, Rule>, feature: &str) {
        self.push_diag(
            DiagnosticSeverity::Warning,
            format!("Support incomplet : `{feature}` n'est pas encore géré."),
            Some(Self::to_ast_span(pair.as_span())),
        );
    }

    fn unexpected(&mut self, rule: Rule, pair: Pair<'_, Rule>) {
        self.push_diag(
            DiagnosticSeverity::Warning,
            format!("Nœud inattendu lors du rabaissement : {:?}", rule),
            Some(Self::to_ast_span(pair.as_span())),
        );
    }

    fn push_diag(
        &mut self,
        severity: DiagnosticSeverity,
        message: impl Into<String>,
        span: Option<AstSpan>,
    ) {
        self.diagnostics.push(LoweringDiagnostic { severity, message: message.into(), span });
    }

    fn alloc_span(&mut self, span: pest::Span<'_>) -> ast::SpanId {
        self.ast.storage.spans.alloc(Self::to_ast_span(span))
    }

    fn to_ast_span(span: pest::Span<'_>) -> AstSpan {
        AstSpan::new(span.start() as u32, span.end() as u32)
    }

    fn ident_from_pair(pair: Pair<'_, Rule>) -> Ident {
        let text = pair.as_str();
        let span = Self::to_ast_span(pair.as_span());
        Ident::new(text, span)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lowers_simple_module() {
        let outcome = parse_and_lower("module main;").expect("parse lower");
        assert!(outcome.diagnostics.is_empty());
        let module = outcome.ast.storage.modules.get(outcome.ast.root()).unwrap();
        let ident = module.name.as_ref().expect("module name");
        assert_eq!(ident.name.as_str(), "main");
    }

    #[test]
    fn warns_on_nested_module_path() {
        let outcome = parse_and_lower("module foo::bar;").expect("parse lower");
        assert!(outcome
            .diagnostics
            .iter()
            .any(|diag| matches!(diag.severity, DiagnosticSeverity::Warning)));
    }
}
