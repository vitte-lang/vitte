#![cfg_attr(not(feature = "std"), no_std)]
//! vitte-resolver — **résolution de noms / tables de symboles** pour Vitte
//!
//! Buts :
//! - Construire des **tables de définitions** et **références** à partir d'événements de parcours (agnostique de l'AST).
//! - Gérer **scopes** imbriqués (module, fonction, bloc, type…), **imports**, **prélude**, **visibilité** et **ombrage**.
//! - Détecter : symboles dupliqués, imports non résolus / cycliques, usages non résolus.
//! - `no_std`-friendly via `alloc`; [`serde`] optionnel (export diagnostic/graphes).
//!
//! Design : un **moteur par événements**. Tu « pousses »/« pops » les scopes, déclares, importes, puis demandes
//! à résoudre un identifiant / chemin. Pas de dépendance dure à `vitte-ast`.
//!
//! ```ignore
//! use vitte_resolver as vr;
//! use vitte_mm::{StrInterner, Symbol};
//!
//! let mut interner = StrInterner::new();
//! let (foo, bar) = (interner.intern("foo"), interner.intern("bar"));
//! let mut r = vr::Resolver::new(vr::Options::default());
//! let root = r.open_module(foo, vr::NodeId(1));
//! r.declare(foo, vr::DefKind::Function, vr::Visibility::Pub, vr::NodeId(2));
//! let use_site = vr::NodeId(3);
//! let def = r.resolve_name(foo, use_site);
//! assert!(def.is_ok());
//! r.close_module(root);
//! ```

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{
    string::{String, ToString},
    vec::Vec,
};
#[cfg(feature = "std")]
use std::{
    string::{String, ToString},
    vec::Vec,
};

#[cfg(not(feature = "std"))]
use alloc::collections::BTreeMap as HashMap;
#[cfg(feature = "std")]
use std::collections::HashMap;

// On réutilise l'interneur de vitte-mm si dispo
pub use vitte_mm::{StrInterner, Symbol};

// ============================== Identifiants ==============================

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug)]
pub struct NodeId(pub u32);

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, Default)]
pub struct ModuleId(pub u32);

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, Default)]
pub struct ScopeId(pub u32);

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, PartialEq, Eq, Hash, Debug, Default)]
pub struct DefId(pub u32);

// ============================== Chemins ==================================

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq, Hash, Default)]
pub struct Path {
    pub segments: Vec<Symbol>,
    pub is_absolute: bool,
}
impl Path {
    pub fn new<I: IntoIterator<Item = Symbol>>(segs: I) -> Self {
        Self { segments: segs.into_iter().collect(), is_absolute: false }
    }
    pub fn absolute(mut self) -> Self {
        self.is_absolute = true;
        self
    }
    pub fn push(&mut self, s: Symbol) {
        self.segments.push(s);
    }
}

// ============================== Défs / Scopes ============================

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum ScopeKind {
    Module,
    Function,
    Block,
    Type,
    Impl,
}

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum DefKind {
    Module,
    Function,
    Var,
    Const,
    Type,
    Struct,
    Enum,
    Field,
    Param,
    Import,
    Macro,
}

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum Visibility {
    Pub,
    PubCrate,
    Private,
}

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug)]
pub struct Def {
    pub id: DefId,
    pub name: Symbol,
    pub kind: DefKind,
    pub scope: ScopeId,
    pub module: ModuleId,
    pub vis: Visibility,
    pub node: NodeId,
    /// Pour un import `use path as name` : cible résolue
    pub import_target: Option<DefId>,
}

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug)]
pub struct Scope {
    pub id: ScopeId,
    pub kind: ScopeKind,
    pub parent: Option<ScopeId>,
    pub module: ModuleId,
    pub symbols: HashMap<Symbol, DefId>,
}

// ============================== Diagnostics ==============================

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum Severity {
    Error,
    Warning,
    Note,
}

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug)]
pub struct Diagnostic {
    pub severity: Severity,
    pub message: String,
    pub node: NodeId,
}

// ============================== Options ==================================

#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
#[derive(Clone, Debug)]
pub struct Options {
    pub allow_shadowing: bool,
    pub case_sensitive: bool,
    pub prelude: Vec<(Symbol, DefKind)>,
}
impl Default for Options {
    fn default() -> Self {
        Self { allow_shadowing: true, case_sensitive: true, prelude: Vec::new() }
    }
}

// ============================== Résolveur =================================

#[derive(Default)]
pub struct Tables {
    /// Défs par NodeId (sites de déclaration)
    pub def_of: HashMap<NodeId, DefId>,
    /// Référence → Définition
    pub ref_of: HashMap<NodeId, DefId>,
    /// Scope qui s'ouvre à un NodeId (ex: un bloc)
    pub scope_of: HashMap<NodeId, ScopeId>,
    /// Export de module : nom → DefId
    pub exports: HashMap<ModuleId, HashMap<Symbol, DefId>>,
}

pub struct Resolver {
    opts: Options,
    next_scope: u32,
    next_def: u32,
    next_module: u32,

    // graphes
    pub scopes: Vec<Scope>,
    pub defs: Vec<Def>,

    // piles
    stack: Vec<ScopeId>,
    module_stack: Vec<ModuleId>,
    current_module: ModuleId,

    // diagnostics
    pub diagnostics: Vec<Diagnostic>,

    // tables de sortie
    pub tables: Tables,
}

impl Resolver {
    pub fn new(opts: Options) -> Self {
        let mut r = Self {
            opts,
            next_scope: 0,
            next_def: 0,
            next_module: 0,
            scopes: Vec::new(),
            defs: Vec::new(),
            stack: Vec::new(),
            module_stack: Vec::new(),
            current_module: ModuleId(0),
            diagnostics: Vec::new(),
            tables: Tables::default(),
        };
        // module racine + scope racine
        let root = r.alloc_module();
        r.current_module = root;
        r.module_stack.push(root);
        let _root_scope = r.push_scope(ScopeKind::Module, NodeId(0));
        // injecte le prélude dans le scope racine
        for (name, kind) in r.opts.prelude.clone() {
            let _ = r.define(name, kind, Visibility::Pub, NodeId(0));
        }
        r
    }

    // ----------------------- allocation IDs -----------------------
    fn alloc_scope(
        &mut self,
        kind: ScopeKind,
        parent: Option<ScopeId>,
        module: ModuleId,
    ) -> ScopeId {
        let id = ScopeId(self.next_scope);
        self.next_scope += 1;
        self.scopes
            .push(Scope { id, kind, parent, module, symbols: HashMap::default() });
        id
    }
    fn alloc_def(
        &mut self,
        name: Symbol,
        kind: DefKind,
        scope: ScopeId,
        vis: Visibility,
        node: NodeId,
    ) -> DefId {
        let id = DefId(self.next_def);
        self.next_def += 1;
        self.defs.push(Def {
            id,
            name,
            kind,
            scope,
            module: self.current_module,
            vis,
            node,
            import_target: None,
        });
        id
    }
    fn alloc_module(&mut self) -> ModuleId {
        let id = ModuleId(self.next_module);
        self.next_module += 1;
        id
    }

    // ----------------------- scopes & modules ---------------------
    pub fn push_scope(&mut self, kind: ScopeKind, node: NodeId) -> ScopeId {
        let parent = self.stack.last().copied();
        let id = self.alloc_scope(kind, parent, self.current_module);
        self.stack.push(id);
        self.tables.scope_of.insert(node, id);
        id
    }
    pub fn pop_scope(&mut self) {
        let _ = self.stack.pop();
    }

    pub fn open_module(&mut self, name: Symbol, node: NodeId) -> ModuleId {
        // Déclare le module courant dans le scope courant puis descend dans un nouveau module
        let def = self.define(name, DefKind::Module, Visibility::Pub, node);
        let parent_module = self.current_module;
        let child = self.alloc_module();
        self.module_stack.push(child);
        self.current_module = child;
        // Crée un scope module imbriqué
        let _mscope = self.push_scope(ScopeKind::Module, node);
        // Enregistre un espace d'exports pour le nouveau module
        self.tables.exports.entry(self.current_module).or_default();
        self.tables
            .exports
            .entry(parent_module)
            .or_default()
            .insert(name, def);
        child
    }
    pub fn close_module(&mut self, _m: ModuleId) {
        self.pop_scope(); /* remonte au scope parent */
        self.module_stack.pop();
        if let Some(prev) = self.module_stack.last().copied() {
            self.current_module = prev;
        }
    }

    // ----------------------- déclarations -------------------------
    pub fn declare(&mut self, name: Symbol, kind: DefKind, vis: Visibility, node: NodeId) -> DefId {
        self.define(name, kind, vis, node)
    }

    fn define(&mut self, name: Symbol, kind: DefKind, vis: Visibility, node: NodeId) -> DefId {
        let scope = *self.stack.last().expect("scope stack vide");
        // collision ?
        if let Some(_existing) = self.scopes[scope.0 as usize].symbols.get(&name).copied() {
            if !self.opts.allow_shadowing {
                self.diagnostics.push(Diagnostic {
                    severity: Severity::Error,
                    message: format!("symbole déjà défini"),
                    node,
                });
            } else {
                self.diagnostics.push(Diagnostic {
                    severity: Severity::Warning,
                    message: format!("ombrage d'un symbole existant"),
                    node,
                });
            }
            // on ombre quand même :
        }
        let id = self.alloc_def(name, kind, scope, vis, node);
        self.scopes[scope.0 as usize].symbols.insert(name, id);
        self.tables.def_of.insert(node, id);
        id
    }

    /// `use path as alias` — crée une **définition d'import** dont la cible sera résolue.
    pub fn import(
        &mut self,
        path: &Path,
        alias: Option<Symbol>,
        vis: Visibility,
        node: NodeId,
    ) -> DefId {
        let target = self.resolve_path_internal(path, node);
        let name = alias.unwrap_or_else(|| *path.segments.last().expect("chemin vide"));
        let id = self.define(name, DefKind::Import, vis, node);
        if let Ok(def) = target {
            self.defs[id.0 as usize].import_target = Some(def);
        }
        id
    }

    // ----------------------- résolution ---------------------------
    pub fn resolve_name(&mut self, name: Symbol, node: NodeId) -> core::result::Result<DefId, ()> {
        // remonte les scopes
        let mut cur = self.stack.last().copied();
        while let Some(sid) = cur {
            let scope = &self.scopes[sid.0 as usize];
            if let Some(&d) = scope.symbols.get(&name) {
                self.tables.ref_of.insert(node, d);
                return Ok(d);
            }
            cur = scope.parent;
        }
        self.diagnostics.push(Diagnostic {
            severity: Severity::Error,
            message: format!("identifiant non résolu"),
            node,
        });
        Err(())
    }

    pub fn resolve_path(&mut self, path: &Path, node: NodeId) -> core::result::Result<DefId, ()> {
        let d = self.resolve_path_internal(path, node)?;
        self.tables.ref_of.insert(node, d);
        Ok(d)
    }

    fn resolve_path_internal(
        &mut self,
        path: &Path,
        node: NodeId,
    ) -> core::result::Result<DefId, ()> {
        if path.segments.is_empty() {
            self.diagnostics.push(Diagnostic {
                severity: Severity::Error,
                message: "chemin vide".to_string(),
                node,
            });
            return Err(());
        }
        // point de départ : scope courant (ou racine si absolu)
        let cur_scope = if path.is_absolute {
            self.scopes.iter().find(|s| s.parent.is_none()).unwrap().id
        } else {
            *self.stack.last().unwrap()
        };
        // On résout le premier segment dans le scope de départ
        let mut it = path.segments.iter();
        let first = *it.next().unwrap();
        let mut cur_def = match self.lookup_in_scope_chain(cur_scope, first) {
            Some(d) => d,
            None => {
                self.diagnostics.push(Diagnostic {
                    severity: Severity::Error,
                    message: format!("segment initial introuvable"),
                    node,
                });
                return Err(());
            },
        };
        for &seg in it {
            // navigue seulement à travers modules / types qui portent un espace de noms
            match self.defs[cur_def.0 as usize].kind {
                DefKind::Module | DefKind::Type | DefKind::Struct | DefKind::Enum => {
                    // trouve le scope attaché à cette def (même module pour Module, sinon: type namespace = scope de déclaration)
                    let scope_of_def = self.defs[cur_def.0 as usize].scope;
                    if let Some(d) = self.scopes[scope_of_def.0 as usize].symbols.get(&seg).copied()
                    {
                        cur_def = d;
                    } else {
                        self.diagnostics.push(Diagnostic {
                            severity: Severity::Error,
                            message: format!("segment introuvable: {:?}", seg),
                            node,
                        });
                        return Err(());
                    }
                },
                _ => {
                    self.diagnostics.push(Diagnostic {
                        severity: Severity::Error,
                        message: "chemin traverse un symbole non namespace".to_string(),
                        node,
                    });
                    return Err(());
                },
            }
        }
        Ok(cur_def)
    }

    fn lookup_in_scope_chain(&self, scope: ScopeId, name: Symbol) -> Option<DefId> {
        let mut cur = Some(scope);
        while let Some(sid) = cur {
            let sc = &self.scopes[sid.0 as usize];
            if let Some(&d) = sc.symbols.get(&name) {
                return Some(d);
            }
            cur = sc.parent;
        }
        None
    }
}

// ============================== Tests ====================================

#[cfg(test)]
mod tests {
    use super::*;
    use vitte_mm::StrInterner;

    fn syms() -> (StrInterner, Symbol, Symbol, Symbol) {
        let mut i = StrInterner::new();
        let a = i.intern("a");
        let b = i.intern("b");
        let c = i.intern("c");
        (i, a, b, c)
    }

    #[test]
    fn declare_and_resolve_in_block() {
        let (_i, a, _, _) = syms();
        let mut r = Resolver::new(Options::default());
        let root = r.push_scope(ScopeKind::Block, NodeId(1));
        r.declare(a, DefKind::Var, Visibility::Private, NodeId(2));
        let got = r.resolve_name(a, NodeId(3)).unwrap();
        assert_eq!(r.defs[got.0 as usize].name, a);
        r.pop_scope();
    }

    #[test]
    fn shadowing_warns() {
        let (_i, a, _, _) = syms();
        let mut r = Resolver::new(Options { allow_shadowing: true, ..Default::default() });
        let _b = r.push_scope(ScopeKind::Block, NodeId(10));
        r.declare(a, DefKind::Var, Visibility::Private, NodeId(11));
        r.declare(a, DefKind::Var, Visibility::Private, NodeId(12));
        assert!(r.diagnostics.iter().any(|d| matches!(d.severity, Severity::Warning)));
    }

    #[test]
    fn imports_and_paths() {
        let (mut inter, a, b, c) = syms();
        let mut r = Resolver::new(Options::default());
        let _m = r.open_module(a, NodeId(1)); // module a
        let _s = r.push_scope(ScopeKind::Module, NodeId(2));
        let ty = r.declare(b, DefKind::Type, Visibility::Pub, NodeId(3));
        // use a::b as c
        let path = Path { segments: vec![a, b], is_absolute: false };
        r.import(&path, Some(c), Visibility::Pub, NodeId(4));
        // résout c → b
        let got = r.resolve_name(c, NodeId(5)).unwrap();
        assert_eq!(r.defs[got.0 as usize].name, b);
    }
}
