//! vitte-analyzer — analyse sémantique du langage Vitte
//!
//! - validation structurelle simple
//! - vérifications de doublons
//! - pipeline d'expansion des derives / macros
//! - graphe de dépendances inter-items
//! - traçage rudimentaire des obligations de traits
//! - hooks pour l'analyse incrémentale (via `vitte-inc`)

#![deny(missing_docs)]
#![forbid(unsafe_code)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(not(feature = "std"))]
extern crate alloc;

use core::fmt;

#[cfg(not(feature = "std"))]
use alloc::{collections::BTreeMap as HashMap, string::String, sync::Arc, vec::Vec};
#[cfg(feature = "std")]
use std::{collections::HashMap, string::String, sync::Arc, vec::Vec};

use vitte_ast as ast;
use vitte_derive::DeriveRegistry;
use vitte_expander::Expander;
use vitte_green::DependencyGraph;

#[cfg(feature = "incremental")]
use vitte_inc::{IncError, IncManager};

pub use vitte_expander::{ExpansionDiagnostic, ExpansionResult, MacroInvocation, MacroKind};

/* ─────────────────────────── Erreurs ─────────────────────────── */

/// Erreur d’analyse sémantique.
#[derive(Debug, Clone)]
pub struct AnalyzeError {
    /// Message humain.
    pub message: String,
}

impl fmt::Display for AnalyzeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.message)
    }
}

#[cfg(feature = "std")]
impl std::error::Error for AnalyzeError {}

type AResult<T> = core::result::Result<T, AnalyzeError>;

/* ─────────────────────────── Structures ─────────────────────────── */

/// Informations de symbole.
#[derive(Debug, Clone)]
pub struct Symbol {
    /// Nom symbolique.
    pub name: String,
    /// Type associé (texte brut pour simplifier).
    pub ty: Option<String>,
}

/// Résolution d’une obligation de trait ou conflit d’impl.
#[derive(Debug, Clone)]
pub struct TraitResolution {
    /// Nom du trait visé.
    pub trait_name: String,
    /// Type concerné (forme canonique pour les impls).
    pub target: ast::Type,
    /// Origine (item ou impl) ayant déclenché cette résolution.
    pub origin: Option<String>,
    /// Statut final de l’obligation.
    pub status: TraitStatus,
}

/// Statut d'une résolution de trait.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum TraitStatus {
    /// Obligation satisfaite avec une impl explicite ou blanket.
    Satisfied,
    /// Aucune impl trouvée.
    Unsatisfied,
    /// Conflit/cohérence brisée lors de l’enregistrement des impls.
    Conflict {
        /// Détail du conflit détecté (impl dupliquée, blanket chevauchante, ...).
        reason: String,
    },
}

/// Entrée de l’analyse : programme + informations auxiliaires.
pub struct AnalysisInput<'a> {
    /// Programme source à analyser.
    pub program: &'a ast::Program,
    /// Invocations de macros/derives à appliquer avant analyse.
    pub macros: Vec<MacroInvocation>,
    /// Nœuds considérés comme modifiés pour l’incrémental.
    #[cfg(feature = "incremental")]
    pub changed_nodes: Vec<String>,
}

impl<'a> AnalysisInput<'a> {
    /// Construit une entrée à partir d’un programme.
    pub fn new(program: &'a ast::Program) -> Self {
        Self {
            program,
            macros: Vec::new(),
            #[cfg(feature = "incremental")]
            changed_nodes: Vec::new(),
        }
    }

    /// Spécifie les macros/derives à appliquer.
    pub fn with_macros(mut self, macros: Vec<MacroInvocation>) -> Self {
        self.macros = macros;
        self
    }

    /// Déclare des nœuds modifiés (analyse incrémentale).
    #[cfg(feature = "incremental")]
    pub fn with_changed_nodes<I, S>(mut self, nodes: I) -> Self
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        self.changed_nodes = nodes.into_iter().map(Into::into).collect();
        self
    }
}

/// Résultat détaillé d’une analyse.
#[derive(Debug, Clone)]
pub struct AnalysisReport {
    /// Résultat de l’expansion des macros.
    pub expansion: ExpansionResult,
    /// Graphe des dépendances détectées.
    pub dependency_graph: DependencyGraph,
    /// Graphe exporté en DOT pour la visualisation.
    pub dependency_dot: String,
    /// Résolutions d’obligations de traits collectées.
    pub trait_resolutions: Vec<TraitResolution>,
    /// Nœuds marqués comme « dirty » par l’incrémental.
    #[cfg(feature = "incremental")]
    pub dirty_nodes: Vec<String>,
}

/// Analyseur sémantique principal.
pub struct Analyzer {
    symbols: HashMap<String, Symbol>,
    derive_registry: Arc<DeriveRegistry>,
    expander: Expander,
    trait_solver: TraitSolver,
    dep_graph: DependencyGraph,
    last_report: Option<AnalysisReport>,
    #[cfg(feature = "incremental")]
    incremental: Option<IncManager>,
}

impl Analyzer {
    /// Crée un nouvel analyseur avec un registre de derives par défaut.
    pub fn new() -> Self {
        let registry = Arc::new(DeriveRegistry::with_defaults());
        Self::with_registry(registry)
    }

    /// Crée un analyseur à partir d’un registre de derives personnalisé.
    pub fn with_registry(registry: Arc<DeriveRegistry>) -> Self {
        let expander = Expander::new(Arc::clone(&registry));
        Self {
            symbols: HashMap::new(),
            derive_registry: registry,
            expander,
            trait_solver: TraitSolver::new(),
            dep_graph: DependencyGraph::new(),
            last_report: None,
            #[cfg(feature = "incremental")]
            incremental: None,
        }
    }

    /// Accès au registre de derives utilisé.
    pub fn derive_registry(&self) -> &DeriveRegistry {
        &self.derive_registry
    }

    /// Active l’analyse incrémentale avec un gestionnaire vierge.
    #[cfg(feature = "incremental")]
    pub fn enable_incremental(&mut self) {
        self.incremental = Some(IncManager::new());
    }

    /// Charge un snapshot incrémental préexistant.
    #[cfg(feature = "incremental")]
    pub fn load_incremental_snapshot(&mut self, bytes: &[u8]) -> Result<(), IncError> {
        self.incremental = Some(IncManager::from_bytes(bytes)?);
        Ok(())
    }

    /// Exporte l’état incrémental courant, si disponible.
    #[cfg(feature = "incremental")]
    pub fn snapshot_incremental(&self) -> Option<Result<Vec<u8>, IncError>> {
        self.incremental.as_ref().map(|mgr| mgr.to_bytes())
    }

    /// Lance l’analyse complète d’un programme (sans macros personnalisées).
    pub fn analyze(&mut self, prog: &ast::Program) -> AResult<()> {
        let report = self.analyze_with(AnalysisInput::new(prog))?;
        self.last_report = Some(report);
        Ok(())
    }

    /// Analyse avec options (macros, incrémental, ...).
    pub fn analyze_with(&mut self, input: AnalysisInput<'_>) -> AResult<AnalysisReport> {
        self.symbols.clear();
        self.trait_solver.reset();
        self.dep_graph.clear();

        let expansion = self.expander.expand(input.program, &input.macros);
        let expanded_program = expansion.program.clone();

        for item in &expanded_program.items {
            self.dep_graph.add_node(item_name(item).unwrap_or("<anon>").to_string());
            self.check_item(item)?;
        }

        let trait_resolutions = self.trait_solver.take_resolutions();
        let dependency_dot = self.dep_graph.to_dot();

        #[cfg(feature = "incremental")]
        let dirty_nodes = self.compute_dirty_nodes(&expanded_program, &input.changed_nodes);

        let report = AnalysisReport {
            expansion,
            dependency_graph: self.dep_graph.clone(),
            dependency_dot,
            trait_resolutions,
            #[cfg(feature = "incremental")]
            dirty_nodes,
        };

        self.last_report = Some(report.clone());
        Ok(report)
    }

    /// Dernier rapport d’analyse disponible.
    pub fn last_report(&self) -> Option<&AnalysisReport> {
        self.last_report.as_ref()
    }

    /// Exporte le graphe courant au format DOT.
    pub fn dependency_graph_dot(&self) -> String {
        self.dep_graph.to_dot()
    }

    /// Retourne la liste des noms de symboles connus de la dernière analyse.
    /// Utile pour l'autocomplétion dans des outils externes (REPL/IDE).
    #[cfg(feature = "std")]
    pub fn symbol_names(&self) -> Vec<String> {
        self.symbols.keys().cloned().collect()
    }

    fn check_item(&mut self, item: &ast::Item) -> AResult<()> {
        match item {
            ast::Item::Function(f) => {
                self.declare(&f.name, Some("fn".into()))?;
                if let Some(ret) = &f.return_type {
                    self.register_type_usage(Some(&f.name), ret);
                }
                for param in &f.params {
                    self.register_type_usage(Some(&f.name), &param.ty);
                    self.declare(&param.name, Some(format!("{:?}", param.ty.clone())))?;
                }
                self.check_block(Some(&f.name), &f.body)?;
            }
            ast::Item::Const(c) => {
                self.declare(&c.name, c.ty.as_ref().map(|t| format!("{:?}", t)))?;
                if let Some(ty) = &c.ty {
                    self.register_type_usage(Some(&c.name), ty);
                }
                self.check_expr(Some(&c.name), &c.value)?;
            }
            ast::Item::Struct(s) => {
                self.declare(&s.name, Some("struct".into()))?;
                for field in &s.fields {
                    self.register_type_usage(Some(&s.name), &field.ty);
                }
            }
            ast::Item::Enum(e) => {
                self.declare(&e.name, Some("enum".into()))?;
                for variant in &e.variants {
                    for ty in &variant.fields {
                        self.register_type_usage(Some(&e.name), ty);
                    }
                }
            }
        }

        self.trait_solver.observe_item(item);
        Ok(())
    }

    fn check_block(&mut self, owner: Option<&str>, block: &ast::Block) -> AResult<()> {
        for stmt in &block.stmts {
            self.check_stmt(owner, stmt)?;
        }
        Ok(())
    }

    fn check_stmt(&mut self, owner: Option<&str>, stmt: &ast::Stmt) -> AResult<()> {
        match stmt {
            ast::Stmt::Let { name, ty, value, .. } => {
                self.declare(name, ty.as_ref().map(|t| format!("{:?}", t)))?;
                if let Some(ty) = ty {
                    self.register_type_usage(owner, ty);
                }
                if let Some(expr) = value {
                    self.check_expr(owner, expr)?;
                }
            }
            ast::Stmt::Return(expr, ..) => {
                if let Some(expr) = expr {
                    self.check_expr(owner, expr)?;
                }
            }
            ast::Stmt::While { condition, body, .. } => {
                self.check_expr(owner, condition)?;
                self.check_block(owner, body)?;
            }
            ast::Stmt::For { var, iter, body, .. } => {
                self.declare(var, None)?;
                self.check_expr(owner, iter)?;
                self.check_block(owner, body)?;
            }
            ast::Stmt::If { condition, then_block, else_block, .. } => {
                self.check_expr(owner, condition)?;
                self.check_block(owner, then_block)?;
                if let Some(block) = else_block {
                    self.check_block(owner, block)?;
                }
            }
            ast::Stmt::Expr(expr) => {
                self.check_expr(owner, expr)?;
            }
        }
        Ok(())
    }

    fn check_expr(&mut self, owner: Option<&str>, expr: &ast::Expr) -> AResult<()> {
        match expr {
            ast::Expr::Ident(name) => {
                if let Some(owner) = owner {
                    self.dep_graph.add_edge(owner.to_string(), name.clone());
                }
            }
            ast::Expr::Call { func, args } => {
                self.check_expr(owner, func)?;
                for arg in args {
                    self.check_expr(owner, arg)?;
                }
            }
            ast::Expr::Binary { left, right, .. } => {
                self.check_expr(owner, left)?;
                self.check_expr(owner, right)?;
            }
            ast::Expr::Unary { expr, .. } => {
                self.check_expr(owner, expr)?;
            }
            ast::Expr::Field { expr, .. } => {
                self.check_expr(owner, expr)?;
            }
            ast::Expr::Literal(_) => {}
        }
        Ok(())
    }

    fn declare(&mut self, name: &str, ty: Option<String>) -> AResult<()> {
        if self.symbols.contains_key(name) {
            return Err(AnalyzeError { message: format!("Symbole déjà défini: {}", name) });
        }
        self.symbols.insert(name.into(), Symbol { name: name.into(), ty });
        Ok(())
    }

    fn register_type_usage(&mut self, owner: Option<&str>, ty: &ast::Type) {
        if let Some(owner) = owner {
            if let ast::Type::Custom(name) = ty {
                self.dep_graph.add_edge(owner.to_string(), name.clone());
            }
        }
        self.trait_solver.require("Resolved", ty.clone(), owner.map(|s| s.to_string()));
    }

    #[cfg(feature = "incremental")]
    fn compute_dirty_nodes(
        &mut self,
        program: &ast::Program,
        changed_nodes: &[String],
    ) -> Vec<String> {
        let mut dirty = Vec::new();
        if let Some(manager) = self.incremental.as_mut() {
            for item in &program.items {
                if let Some(name) = item_name(item) {
                    let deps = self.dep_graph.dependencies(name);
                    manager.set_deps(name.to_string(), deps);
                    manager.set_fingerprint(name.to_string(), fingerprint_item(item));
                }
            }
            if !changed_nodes.is_empty() {
                use std::collections::HashSet;
                let mut aggregate: HashSet<String> = HashSet::new();
                for node in changed_nodes {
                    let set = manager.mark_modified(node.clone());
                    aggregate.extend(set.into_iter());
                }
                dirty = aggregate.into_iter().collect();
                dirty.sort();
            }
        }
        dirty
    }
}

/* ─────────────────────────── Trait solver interne ─────────────────────────── */

#[derive(Default)]
struct TraitSolver {
    impls: HashMap<String, Vec<ImplRule>>,
    resolutions: Vec<TraitResolution>,
}

#[derive(Clone, Debug)]
struct ImplRule {
    pattern: ImplPattern,
    origin: Option<String>,
}

#[derive(Clone, Debug, PartialEq, Eq)]
enum ImplPattern {
    Exact(String),
    #[allow(dead_code)]
    Blanket(String),
}

impl ImplPattern {
    fn matches(&self, canonical_ty: &str) -> bool {
        match self {
            ImplPattern::Exact(expected) => expected == canonical_ty,
            ImplPattern::Blanket(pattern) => wildcard_matches(pattern, canonical_ty),
        }
    }
}

impl TraitSolver {
    fn new() -> Self {
        Self::default()
    }

    fn reset(&mut self) {
        self.resolutions.clear();
    }

    fn observe_item(&mut self, item: &ast::Item) {
        match item {
            ast::Item::Struct(s) => self.register_exact_impl(
                "Resolved",
                &ast::Type::Custom(s.name.clone()),
                Some(s.name.clone()),
            ),
            ast::Item::Enum(e) => self.register_exact_impl(
                "Resolved",
                &ast::Type::Custom(e.name.clone()),
                Some(e.name.clone()),
            ),
            _ => {}
        }
    }

    fn register_exact_impl(&mut self, trait_name: &str, ty: &ast::Type, origin: Option<String>) {
        let canonical = canonical_type(ty);
        self.register_impl_rule(
            trait_name,
            ImplRule { pattern: ImplPattern::Exact(canonical.clone()), origin: origin.clone() },
            canonical,
        );
    }

    #[allow(dead_code)]
    fn register_blanket_impl(&mut self, trait_name: &str, pattern: &str, origin: Option<String>) {
        let compact: String = pattern.chars().filter(|c| !c.is_whitespace()).collect();
        let normalized = compact.to_ascii_lowercase();
        self.register_impl_rule(
            trait_name,
            ImplRule { pattern: ImplPattern::Blanket(normalized.clone()), origin },
            normalized,
        );
    }

    fn register_impl_rule(&mut self, trait_name: &str, rule: ImplRule, canonical_display: String) {
        let entry = self.impls.entry(trait_name.to_string()).or_default();
        // Vérifier conflits/cohérence
        if let Some(conflict) =
            entry.iter().find(|existing| match (&existing.pattern, &rule.pattern) {
                (ImplPattern::Exact(a), ImplPattern::Exact(b)) => a == b,
                (ImplPattern::Blanket(a), ImplPattern::Exact(b))
                | (ImplPattern::Exact(b), ImplPattern::Blanket(a)) => wildcard_matches(a, b),
                (ImplPattern::Blanket(a), ImplPattern::Blanket(b)) => {
                    wildcard_matches(a, b) || wildcard_matches(b, a)
                }
            })
        {
            let reason = match (&conflict.pattern, &rule.pattern) {
                (ImplPattern::Exact(_), ImplPattern::Exact(_)) => {
                    "duplicate impl for same type".to_string()
                }
                _ => "blanket impl overlaps with existing rule".to_string(),
            };
            self.resolutions.push(TraitResolution {
                trait_name: trait_name.to_string(),
                target: ast::Type::Custom(canonical_display.clone()),
                origin: rule.origin.clone(),
                status: TraitStatus::Conflict { reason },
            });
        } else {
            entry.push(rule);
        }
    }

    fn require(&mut self, trait_name: &str, ty: ast::Type, origin: Option<String>) {
        let canonical = canonical_type(&ty);
        let satisfied = self
            .impls
            .get(trait_name)
            .map(|impls| impls.iter().any(|rule| rule.pattern.matches(&canonical)))
            .unwrap_or(false);

        self.resolutions.push(TraitResolution {
            trait_name: trait_name.to_string(),
            target: ty,
            origin,
            status: if satisfied { TraitStatus::Satisfied } else { TraitStatus::Unsatisfied },
        });
    }

    fn take_resolutions(&mut self) -> Vec<TraitResolution> {
        core::mem::take(&mut self.resolutions)
    }
}

/* ─────────────────────────── Helpers ─────────────────────────── */

fn item_name(item: &ast::Item) -> Option<&str> {
    match item {
        ast::Item::Function(f) => Some(&f.name),
        ast::Item::Const(c) => Some(&c.name),
        ast::Item::Struct(s) => Some(&s.name),
        ast::Item::Enum(e) => Some(&e.name),
    }
}

fn canonical_type(ty: &ast::Type) -> String {
    match ty {
        ast::Type::Int => "int".into(),
        ast::Type::Float => "float".into(),
        ast::Type::Bool => "bool".into(),
        ast::Type::Str => "str".into(),
        ast::Type::Void => "void".into(),
        ast::Type::Custom(name) => canonicalize_type_string(name),
    }
}

fn canonicalize_type_string(raw: &str) -> String {
    let trimmed = raw.trim();
    let lower = trimmed.to_ascii_lowercase();
    match lower.as_str() {
        "int" => "int".into(),
        "float" => "float".into(),
        "bool" => "bool".into(),
        "str" | "string" => "str".into(),
        "void" | "unit" => "void".into(),
        _ => canonicalize_custom(trimmed),
    }
}

fn canonicalize_custom(raw: &str) -> String {
    let compact: String = raw.chars().filter(|c| !c.is_whitespace()).collect();
    if let Some(start) = compact.find('<') {
        if compact.ends_with('>') {
            let base = compact[..start].to_ascii_lowercase();
            let inner = &compact[start + 1..compact.len() - 1];
            let params =
                split_generics(inner).into_iter().map(canonicalize_type_string).collect::<Vec<_>>();
            format!("{base}<{}>", params.join(","))
        } else {
            compact.to_ascii_lowercase()
        }
    } else {
        compact.to_ascii_lowercase()
    }
}

fn split_generics(input: &str) -> Vec<&str> {
    let mut parts = Vec::new();
    let mut depth = 0usize;
    let mut start = 0usize;
    for (idx, ch) in input.char_indices() {
        match ch {
            '<' => depth += 1,
            '>' => {
                if depth > 0 {
                    depth -= 1;
                }
            }
            ',' if depth == 0 => {
                parts.push(input[start..idx].trim());
                start = idx + 1;
            }
            _ => {}
        }
    }
    if start < input.len() {
        parts.push(input[start..].trim());
    }
    parts.iter().filter(|s| !s.is_empty()).copied().collect()
}

fn wildcard_matches(pattern: &str, text: &str) -> bool {
    let p = pattern.as_bytes();
    let t = text.as_bytes();
    let mut i = 0usize;
    let mut j = 0usize;
    let mut star = None::<usize>;
    let mut match_idx = 0usize;

    while j < t.len() {
        if i < p.len() && (p[i] == b'?' || p[i] == t[j]) {
            i += 1;
            j += 1;
        } else if i < p.len() && p[i] == b'*' {
            star = Some(i);
            i += 1;
            match_idx = j;
        } else if let Some(star_idx) = star {
            i = star_idx + 1;
            match_idx += 1;
            j = match_idx;
        } else {
            return false;
        }
    }

    while i < p.len() && p[i] == b'*' {
        i += 1;
    }

    i == p.len()
}

#[cfg(feature = "incremental")]
fn fingerprint_item(item: &ast::Item) -> u64 {
    #[cfg(feature = "std")]
    {
        use std::collections::hash_map::DefaultHasher;
        use std::hash::Hasher;
        let mut hasher = DefaultHasher::new();
        // Debug est implémenté pour les items AST.
        let repr = format!("{:?}", item);
        hasher.write(repr.as_bytes());
        hasher.finish()
    }
    #[cfg(not(feature = "std"))]
    {
        0
    }
}

/* ─────────────────────────── Tests ─────────────────────────── */

#[cfg(test)]
mod tests {
    use super::*;
    use ast::{Block, ConstDecl, Expr, Function, Item, Program, Stmt, StructDecl, Type};

    #[test]
    fn redecl_fails() {
        let mut az = Analyzer::new();
        let prog = Program {
            items: vec![
                Item::Const(ConstDecl {
                    name: "X".into(),
                    ty: Some(Type::Int),
                    value: Expr::Literal(ast::Literal::Int(1)),
                    span: None,
                }),
                Item::Const(ConstDecl {
                    name: "X".into(),
                    ty: Some(Type::Int),
                    value: Expr::Literal(ast::Literal::Int(2)),
                    span: None,
                }),
            ],
        };
        let res = az.analyze(&prog);
        assert!(res.is_err());
    }

    #[test]
    fn fn_with_params_ok() {
        let mut az = Analyzer::new();
        let f = Function {
            name: "add".into(),
            params: vec![],
            return_type: Some(Type::Int),
            body: Block { stmts: vec![], span: None },
            span: None,
        };
        let prog = Program { items: vec![Item::Function(f)] };
        assert!(az.analyze(&prog).is_ok());
    }

    #[test]
    fn derive_pipeline_generates_items() {
        let mut az = Analyzer::new();
        let program = Program {
            items: vec![Item::Struct(StructDecl {
                name: "Point".into(),
                fields: vec![ast::Field { name: "x".into(), ty: Type::Int, span: None }],
                span: None,
            })],
        };
        let report = az
            .analyze_with(
                AnalysisInput::new(&program)
                    .with_macros(vec![MacroInvocation::derive("Point", "Debug")]),
            )
            .expect("analysis succeeds");
        assert!(report.expansion.program.items.len() > program.items.len());
        assert!(report.trait_resolutions.iter().any(
            |res| res.trait_name == "Resolved" && matches!(res.status, TraitStatus::Satisfied)
        ));
    }

    #[test]
    fn dependency_graph_tracks_custom_types() {
        let mut az = Analyzer::new();
        let program = Program {
            items: vec![
                Item::Struct(StructDecl {
                    name: "Foo".into(),
                    fields: vec![ast::Field { name: "value".into(), ty: Type::Int, span: None }],
                    span: None,
                }),
                Item::Function(Function {
                    name: "make".into(),
                    params: vec![ast::Param {
                        name: "x".into(),
                        ty: Type::Custom("Foo".into()),
                        span: None,
                    }],
                    return_type: Some(Type::Custom("Foo".into())),
                    body: Block {
                        stmts: vec![Stmt::Return(Some(Expr::Ident("Foo".into())), None)],
                        span: None,
                    },
                    span: None,
                }),
            ],
        };
        let report = az.analyze_with(AnalysisInput::new(&program)).expect("analysis succeeds");
        let deps = report.dependency_graph.dependencies("make");
        assert!(deps.contains(&"Foo".into()));
        assert!(report.dependency_dot.contains("digraph"));
    }

    #[test]
    fn trait_solver_matches_blanket_impl() {
        let mut solver = TraitSolver::new();
        solver.register_blanket_impl("Display", "Option<*>", Some("blanket".into()));
        solver.require("Display", Type::Custom("Option<Int>".into()), Some("usage".into()));
        let resolutions = solver.take_resolutions();
        assert!(
            resolutions
                .iter()
                .any(|res| res.trait_name == "Display"
                    && matches!(res.status, TraitStatus::Satisfied))
        );
    }

    #[test]
    fn trait_solver_detects_conflicts() {
        let mut solver = TraitSolver::new();
        solver.register_exact_impl("Resolved", &Type::Custom("Foo".into()), Some("impl1".into()));
        solver.register_exact_impl("Resolved", &Type::Custom("Foo".into()), Some("impl2".into()));
        let resolutions = solver.take_resolutions();
        assert!(
            resolutions.iter().any(|res| matches!(res.status, TraitStatus::Conflict { .. })),
            "expected conflict"
        );
    }

    #[cfg(feature = "incremental")]
    #[test]
    fn incremental_marks_dirty_nodes() {
        let mut az = Analyzer::new();
        az.enable_incremental();
        let program = Program {
            items: vec![
                Item::Struct(StructDecl {
                    name: "Foo".into(),
                    fields: vec![ast::Field { name: "value".into(), ty: Type::Int, span: None }],
                    span: None,
                }),
                Item::Function(Function {
                    name: "make".into(),
                    params: vec![ast::Param {
                        name: "x".into(),
                        ty: Type::Custom("Foo".into()),
                        span: None,
                    }],
                    return_type: Some(Type::Custom("Foo".into())),
                    body: Block { stmts: vec![], span: None },
                    span: None,
                }),
            ],
        };
        let report = az
            .analyze_with(AnalysisInput::new(&program).with_changed_nodes(["Foo".to_string()]))
            .expect("analysis succeeds");
        assert!(report.dirty_nodes.contains(&"make".into()));
    }

    #[test]
    fn trait_resolutions_record_unsatisfied() {
        let mut az = Analyzer::new();
        let program = Program {
            items: vec![Item::Function(Function {
                name: "main".into(),
                params: vec![ast::Param {
                    name: "x".into(),
                    ty: Type::Custom("Unknown".into()),
                    span: None,
                }],
                return_type: None,
                body: Block { stmts: vec![], span: None },
                span: None,
            })],
        };
        let report = az.analyze_with(AnalysisInput::new(&program)).expect("analysis ok");
        assert!(report.trait_resolutions.iter().any(|res| res.trait_name == "Resolved"
            && res.target == Type::Custom("Unknown".into())
            && matches!(res.status, TraitStatus::Unsatisfied)));
    }
}
