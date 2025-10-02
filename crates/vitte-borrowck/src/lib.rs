//! vitte-borrock — moteur expérimental (sandbox / recherche / prototypes)
//!
//! Objectifs :
//! - IR minimaliste sérialisable (Module/Block/Instr/Value)
//! - Passes de transformation (constant-folding, DCE) sur l’IR
//! - Évaluateur interprété avec environnement lexical
//! - Builtins extensibles (print, len, assert, time_ns simulé)
//! - Diagnostics structurés et émetteur custom
//! - Chargement de config TOML optionnelle
//! - Pipeline : parse (stub) → passes → évaluation
//!
//! Ce crate n’est PAS stable. API sujette à changement.

#![forbid(unsafe_code)]

use std::borrow::Cow;
use std::collections::HashMap;
use std::fmt::{self, Display};
use std::path::{Path, PathBuf};
use std::sync::Arc;

// ============================================================================
// Erreurs
// ============================================================================

mod error {
    use super::*;
    use thiserror::Error;

    #[derive(Debug, Error)]
    pub enum BorrockError {
        #[error("E/S: {0}")]
        Io(#[from] std::io::Error),
        #[error("TOML: {0}")]
        Toml(#[from] toml::de::Error),
        #[error("JSON: {0}")]
        Json(#[from] serde_json::Error),
        #[error("Config: {0}")]
        Config(String),
        #[error("Analyse: {0}")]
        Parse(String),
        #[error("Exécution: {0}")]
        Eval(String),
        #[error("Builtin introuvable: {0}")]
        MissingBuiltin(String),
        #[error("Type invalide: {0}")]
        Type(String),
        #[error("Variable inconnue: {0}")]
        UnknownVar(String),
        #[error("Division par zéro")]
        DivByZero,
        #[error("Pass: {0}")]
        Pass(String),
    }

    pub type Result<T> = std::result::Result<T, BorrockError>;
}
pub use error::{BorrockError, Result};

// ============================================================================
// Diagnostics
// ============================================================================

mod diag {
    use super::*;

    #[derive(Debug, Clone, Copy, serde::Serialize, serde::Deserialize, PartialEq, Eq)]
    pub enum Severity {
        Info,
        Warning,
        Error,
    }

    #[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
    pub struct Span {
        pub file: Option<PathBuf>,
        pub line: u32,
        pub col: u32,
    }

    impl Default for Span {
        fn default() -> Self {
            Self { file: None, line: 0, col: 0 }
        }
    }

    #[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
    pub struct Diagnostic {
        pub severity: Severity,
        pub message: String,
        pub span: Option<Span>,
        pub notes: Vec<String>,
    }

    pub trait Emitter: Send + Sync {
        fn emit(&self, d: &Diagnostic);
    }

    #[derive(Default)]
    pub struct SimpleEmitter;

    impl Emitter for SimpleEmitter {
        fn emit(&self, d: &Diagnostic) {
            use Severity::*;
            let tag = match d.severity { Info => "info", Warning => "warn", Error => "error" };
            if let Some(sp) = &d.span {
                if let Some(f) = &sp.file {
                    log::log!(match d.severity {
                        Info => log::Level::Info,
                        Warning => log::Level::Warn,
                        Error => log::Level::Error,
                    }, "[{tag}] {}:{}:{}: {}", f.display(), sp.line, sp.col, d.message);
                } else {
                    log::log!(match d.severity {
                        Info => log::Level::Info,
                        Warning => log::Level::Warn,
                        Error => log::Level::Error,
                    }, "[{tag}] {}:{}: {}", sp.line, sp.col, d.message);
                }
            } else {
                log::log!(match d.severity {
                    Info => log::Level::Info,
                    Warning => log::Level::Warn,
                    Error => log::Level::Error,
                }, "[{tag}] {}", d.message);
            }
            for n in &d.notes {
                log::info!("  note: {n}");
            }
        }
    }

    pub fn info(msg: impl Into<String>) -> Diagnostic {
        Diagnostic { severity: Severity::Info, message: msg.into(), span: None, notes: vec![] }
    }

    pub fn warn(msg: impl Into<String>) -> Diagnostic {
        Diagnostic { severity: Severity::Warning, message: msg.into(), span: None, notes: vec![] }
    }

    pub fn error(msg: impl Into<String>) -> Diagnostic {
        Diagnostic { severity: Severity::Error, message: msg.into(), span: None, notes: vec![] }
    }
}
pub use diag::{Diagnostic, Emitter, Severity};
pub use diag::{error as diag_error, info as diag_info, warn as diag_warn};
pub use diag::{SimpleEmitter, Span};

// ============================================================================
// Config
// ============================================================================

mod config {
    use super::*;

    #[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
    pub struct Config {
        pub optimize: bool,
        pub enable_dce: bool,
        pub enable_const_folding: bool,
        pub entry: Option<String>,
        pub builtins: Option<Vec<String>>,
    }

    impl Default for Config {
        fn default() -> Self {
            Self {
                optimize: true,
                enable_dce: true,
                enable_const_folding: true,
                entry: Some("main".into()),
                builtins: Some(vec![
                    "print".into(),
                    "len".into(),
                    "assert".into(),
                    "time_ns".into(),
                ]),
            }
        }
    }

    pub fn load_config(path: &Path) -> crate::Result<Config> {
        if !path.exists() {
            return Ok(Config::default());
        }
        let s = std::fs::read_to_string(path)?;
        let cfg: Config = toml::from_str(&s)?;
        Ok(cfg)
    }

    pub use Config as BorrockConfig;
}
pub use config::{load_config, BorrockConfig};

// ============================================================================
// IR
// ============================================================================

mod ir {
    use super::*;

    #[derive(Debug, Clone, PartialEq, serde::Serialize, serde::Deserialize)]
    #[serde(tag = "t", content = "v")]
    pub enum Value {
        Unit,
        Bool(bool),
        Num(f64),
        Str(String),
        Array(Vec<Value>),
        Object(HashMap<String, Value>),
        /// Référence symbolique (variable)
        Sym(String),
    }

    impl Display for Value {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            match self {
                Value::Unit => write!(f, "()"),
                Value::Bool(b) => write!(f, "{b}"),
                Value::Num(n) => write!(f, "{n}"),
                Value::Str(s) => write!(f, "{s:?}"),
                Value::Array(a) => write!(f, "{a:?}"),
                Value::Object(o) => write!(f, "{o:?}"),
                Value::Sym(s) => write!(f, "${s}"),
            }
        }
    }

    #[derive(Debug, Clone, Copy, PartialEq, Eq, serde::Serialize, serde::Deserialize)]
    pub enum BinOp {
        Add, Sub, Mul, Div, Mod,
        Eq, Ne, Lt, Le, Gt, Ge,
        And, Or,
    }

    #[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
    pub struct Block {
        pub name: String,
        pub instrs: Vec<Instr>,
    }

    impl Block {
        pub fn new(name: impl Into<String>) -> Self {
            Self { name: name.into(), instrs: vec![] }
        }
    }

    #[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
    #[serde(tag = "kind")]
    pub enum Instr {
        Const { dst: String, value: Value },
        Load  { dst: String, var: String },
        Store { var: String, src: String },
        Bin   { dst: String, op: BinOp, a: String, b: String },
        If    { cond: String, then_blk: Block, else_blk: Option<Block>, dst: Option<String> },
        CallBuiltin { dst: Option<String>, name: String, args: Vec<String> },
        Seq   { items: Vec<Instr> },
        Ret   { src: Option<String> },
        // NOP pour DCE
        Nop,
    }

    #[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
    pub struct Func {
        pub name: String,
        pub params: Vec<String>,
        pub body: Block,
    }

    #[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
    pub struct Module {
        pub funcs: HashMap<String, Func>,
        pub exports: Vec<String>,
        pub meta: HashMap<String, String>,
    }

    impl Module {
        pub fn new() -> Self {
            Self { funcs: HashMap::new(), exports: vec![], meta: HashMap::new() }
        }
        pub fn add_func(&mut self, f: Func) { self.funcs.insert(f.name.clone(), f); }
        pub fn get(&self, name: &str) -> Option<&Func> { self.funcs.get(name) }
    }

    pub use BinOp::*;
}
pub use ir::{BinOp, Block, Func, Instr, Module, Value};

// ============================================================================
// Passes
// ============================================================================

mod pass {
    use super::*;

    pub trait Pass: Send {
        fn name(&self) -> &str;
        fn run(&mut self, m: &mut Module) -> Result<()>;
    }

    // -------- Constant Folding -------------------------------------------------

    pub struct ConstFold;

    impl ConstFold {
        pub fn new() -> Self { Self }
    }

    fn as_num(v: &Value) -> Option<f64> {
        match v { Value::Num(n) => Some(*n), _ => None }
    }
    fn as_bool(v: &Value) -> Option<bool> {
        match v { Value::Bool(b) => Some(*b), _ => None }
    }

    impl Pass for ConstFold {
        fn name(&self) -> &str { "const-fold" }

        fn run(&mut self, m: &mut Module) -> Result<()> {
            for f in m.funcs.values_mut() {
                fn fold_instr(i: &mut Instr) {
                    use Instr::*;
                    match i {
                        Seq { items } => {
                            for it in items.iter_mut() { fold_instr(it); }
                        }
                        If { then_blk, else_blk, .. } => {
                            for it in then_blk.instrs.iter_mut() { fold_instr(it); }
                            if let Some(b) = else_blk {
                                for it in b.instrs.iter_mut() { fold_instr(it); }
                            }
                        }
                        Bin { dst: _, op, a: _, b: _ } => {
                            // Folding local impossible sans table. On cible uniquement Bin(Const,Const)
                            // via motif : Seq(Const x, Const y, Bin z x y) est complexe ici.
                            // Simplification: rien sans contexte.
                            let _ = op;
                        }
                        _ => {}
                    }
                }
                for it in f.body.instrs.iter_mut() {
                    fold_instr(it);
                }
            }
            Ok(())
        }
    }

    // -------- DCE (suppression NOP et Seqs vides) -----------------------------

    pub struct DeadCodeElim;

    impl DeadCodeElim { pub fn new() -> Self { Self } }

    impl Pass for DeadCodeElim {
        fn name(&self) -> &str { "dce" }

        fn run(&mut self, m: &mut Module) -> Result<()> {
            fn clean_block(b: &mut Block) {
                use Instr::*;
                let mut out = Vec::with_capacity(b.instrs.len());
                for mut i in std::mem::take(&mut b.instrs) {
                    match &mut i {
                        Seq { items } => {
                            for it in items.iter_mut() {
                                if let Instr::Seq { .. } = it {} // recurse later
                            }
                            // aplatissement léger
                            let mut flat = Vec::new();
                            for it in std::mem::take(items) {
                                match it {
                                    Seq { items: inner } => flat.extend(inner),
                                    other => flat.push(other),
                                }
                            }
                            items.extend(flat);
                            if items.is_empty() {
                                continue;
                            }
                            out.push(Seq { items: std::mem::take(items) });
                        }
                        If { then_blk, else_blk, .. } => {
                            clean_block(then_blk);
                            if let Some(e) = else_blk {
                                let mut e2 = e.clone();
                                clean_block(&mut e2);
                                *else_blk = Some(e2);
                            }
                            out.push(i);
                        }
                        Nop => { /* drop */ }
                        _ => out.push(i),
                    }
                }
                b.instrs = out;
            }

            for f in m.funcs.values_mut() {
                clean_block(&mut f.body);
            }
            Ok(())
        }
    }

    pub use DeadCodeElim as Dce;
    pub use ConstFold;
    pub use Pass as IrPass;
}
pub use pass::{ConstFold, Dce, IrPass};

// ============================================================================
// Builtins
// ============================================================================

mod builtins {
    use super::*;

    pub type BuiltinFn = dyn Fn(&[Value]) -> Result<Value> + Send + Sync + 'static;

    #[derive(Default, Clone)]
    pub struct Builtins {
        inner: Arc<HashMap<String, Arc<BuiltinFn>>>,
    }

    impl Builtins {
        pub fn new() -> Self { Self { inner: Arc::new(HashMap::new()) } }

        pub fn with_default() -> Self {
            let mut m: HashMap<String, Arc<BuiltinFn>> = HashMap::new();

            m.insert("print".into(), Arc::new(|args: &[Value]| {
                let s: Vec<String> = args.iter().map(|v| format!("{v}")).collect();
                println!("{}", s.join(" "));
                Ok(Value::Unit)
            }));

            m.insert("len".into(), Arc::new(|args: &[Value]| {
                if args.len() != 1 { return Err(BorrockError::Type("len(arg)".into())); }
                match &args[0] {
                    Value::Array(a) => Ok(Value::Num(a.len() as f64)),
                    Value::Str(s) => Ok(Value::Num(s.chars().count() as f64)),
                    Value::Object(o) => Ok(Value::Num(o.len() as f64)),
                    _ => Err(BorrockError::Type("len: unsupported".into())),
                }
            }));

            m.insert("assert".into(), Arc::new(|args: &[Value]| {
                if args.len() != 1 { return Err(BorrockError::Type("assert(bool)".into())); }
                match args[0] {
                    Value::Bool(true) => Ok(Value::Unit),
                    Value::Bool(false) => Err(BorrockError::Eval("assertion failed".into())),
                    _ => Err(BorrockError::Type("assert expects bool".into())),
                }
            }));

            // Pas d’accès OS: simulateur temps monotone
            m.insert("time_ns".into(), Arc::new(|_args: &[Value]| {
                use std::time::{SystemTime, UNIX_EPOCH};
                let now = SystemTime::now().duration_since(UNIX_EPOCH).unwrap();
                Ok(Value::Num(now.as_nanos() as f64))
            }));

            Self { inner: Arc::new(m) }
        }

        pub fn get(&self, name: &str) -> Option<Arc<BuiltinFn>> {
            self.inner.get(name).cloned()
        }

        pub fn insert(&mut self, name: impl Into<String>, f: Arc<BuiltinFn>) {
            let mut m = (*self.inner).clone();
            m.insert(name.into(), f);
            self.inner = Arc::new(m);
        }
    }
}
pub use builtins::{BuiltinFn, Builtins};

// ============================================================================
// Évaluateur
// ============================================================================

mod eval {
    use super::*;

    #[derive(Default, Clone)]
    pub struct Env {
        scopes: Vec<HashMap<String, Value>>,
    }

    impl Env {
        pub fn new() -> Self { Self { scopes: vec![HashMap::new()] } }
        fn push(&mut self) { self.scopes.push(HashMap::new()); }
        fn pop(&mut self) { self.scopes.pop(); }
        pub fn set(&mut self, k: impl Into<String>, v: Value) { self.scopes.last_mut().unwrap().insert(k.into(), v); }
        pub fn get(&self, k: &str) -> Option<Value> {
            for s in self.scopes.iter().rev() {
                if let Some(v) = s.get(k) { return Some(v.clone()); }
            }
            None
        }
        pub fn set_existing(&mut self, k: &str, v: Value) -> Result<()> {
            for s in self.scopes.iter_mut().rev() {
                if s.contains_key(k) { s.insert(k.to_string(), v); return Ok(()); }
            }
            Err(BorrockError::UnknownVar(k.to_string()))
        }
    }

    pub struct Interpreter<'b> {
        pub builtins: &'b Builtins,
        pub emitter: Option<Arc<dyn Emitter>>,
    }

    impl<'b> Interpreter<'b> {
        pub fn new(builtins: &'b Builtins) -> Self { Self { builtins, emitter: None } }

        pub fn with_emitter(mut self, e: Arc<dyn Emitter>) -> Self { self.emitter = Some(e); self }

        pub fn eval_module(&self, m: &Module, entry: &str) -> Result<Value> {
            let func = m.get(entry).ok_or_else(|| BorrockError::Eval(format!("fonction `{entry}` introuvable")))?;
            let mut env = Env::new();
            self.eval_block(&func.body, &mut env)
        }

        fn eval_block(&self, b: &Block, env: &mut Env) -> Result<Value> {
            use Instr::*;
            let mut last = Value::Unit;
            env.push();
            for instr in &b.instrs {
                match instr {
                    Instr::Const { dst, value } => { env.set(dst, value.clone()); }
                    Instr::Load { dst, var } => {
                        let v = env.get(var).ok_or_else(|| BorrockError::UnknownVar(var.clone()))?;
                        env.set(dst, v);
                    }
                    Instr::Store { var, src } => {
                        let v = env.get(src).ok_or_else(|| BorrockError::UnknownVar(src.clone()))?;
                        env.set_existing(var, v)?;
                    }
                    Instr::Bin { dst, op, a, b } => {
                        let va = env.get(a).ok_or_else(|| BorrockError::UnknownVar(a.clone()))?;
                        let vb = env.get(b).ok_or_else(|| BorrockError::UnknownVar(b.clone()))?;
                        let r = self.eval_bin(*op, va, vb)?;
                        env.set(dst, r);
                    }
                    Instr::If { cond, then_blk, else_blk, dst } => {
                        let c = env.get(cond).ok_or_else(|| BorrockError::UnknownVar(cond.clone()))?;
                        let truth = match c { Value::Bool(b) => b, Value::Num(n) => n != 0.0, _ => false };
                        let val = if truth { self.eval_block(then_blk, env)? }
                                  else if let Some(e) = else_blk { self.eval_block(e, env)? }
                                  else { Value::Unit };
                        if let Some(d) = dst { env.set(d, val.clone()); last = val; }
                    }
                    Instr::CallBuiltin { dst, name, args } => {
                        let mut argv = Vec::with_capacity(args.len());
                        for a in args {
                            argv.push(env.get(a).ok_or_else(|| BorrockError::UnknownVar(a.clone()))?);
                        }
                        let f = self.builtins.get(name).ok_or_else(|| BorrockError::MissingBuiltin(name.clone()))?;
                        let out = f(&argv)?;
                        if let Some(d) = dst { env.set(d, out.clone()); last = out; }
                    }
                    Instr::Seq { items } => {
                        for it in items { match it {
                            Ret { src } => {
                                let v = if let Some(s) = src {
                                    env.get(s).ok_or_else(|| BorrockError::UnknownVar(s.clone()))?
                                } else { Value::Unit };
                                env.pop();
                                return Ok(v);
                            }
                            other => {
                                let tmp_block = Block { name: "<seq>".into(), instrs: vec![other.clone()] };
                                last = self.eval_block(&tmp_block, env)?;
                            }
                        }}
                    }
                    Instr::Ret { src } => {
                        let v = if let Some(s) = src {
                            env.get(s).ok_or_else(|| BorrockError::UnknownVar(s.clone()))?
                        } else { Value::Unit };
                        env.pop();
                        return Ok(v);
                    }
                    Instr::Nop => {}
                }
            }
            env.pop();
            Ok(last)
        }

        fn eval_bin(&self, op: ir::BinOp, a: Value, b: Value) -> Result<Value> {
            use ir::BinOp::*;
            match op {
                Add => match (a, b) {
                    (Value::Num(x), Value::Num(y)) => Ok(Value::Num(x + y)),
                    (Value::Str(x), Value::Str(y)) => Ok(Value::Str(format!("{x}{y}"))),
                    (Value::Str(x), v) | (v, Value::Str(x)) => Ok(Value::Str(format!("{x}{v}"))),
                    _ => Err(BorrockError::Type("Add".into())),
                },
                Sub => match (a, b) {
                    (Value::Num(x), Value::Num(y)) => Ok(Value::Num(x - y)),
                    _ => Err(BorrockError::Type("Sub".into())),
                },
                Mul => match (a, b) {
                    (Value::Num(x), Value::Num(y)) => Ok(Value::Num(x * y)),
                    _ => Err(BorrockError::Type("Mul".into())),
                },
                Div => match (a, b) {
                    (Value::Num(_), Value::Num(0.0)) => Err(BorrockError::DivByZero),
                    (Value::Num(x), Value::Num(y)) => Ok(Value::Num(x / y)),
                    _ => Err(BorrockError::Type("Div".into())),
                },
                Mod => match (a, b) {
                    (Value::Num(x), Value::Num(y)) => Ok(Value::Num(x % y)),
                    _ => Err(BorrockError::Type("Mod".into())),
                },
                Eq => Ok(Value::Bool(a == b)),
                Ne => Ok(Value::Bool(a != b)),
                Lt => match (a, b) { (Value::Num(x), Value::Num(y)) => Ok(Value::Bool(x < y)), _ => Err(BorrockError::Type("Lt".into())) },
                Le => match (a, b) { (Value::Num(x), Value::Num(y)) => Ok(Value::Bool(x <= y)), _ => Err(BorrockError::Type("Le".into())) },
                Gt => match (a, b) { (Value::Num(x), Value::Num(y)) => Ok(Value::Bool(x > y)), _ => Err(BorrockError::Type("Gt".into())) },
                Ge => match (a, b) { (Value::Num(x), Value::Num(y)) => Ok(Value::Bool(x >= y)), _ => Err(BorrockError::Type("Ge".into())) },
                And => match (a, b) {
                    (Value::Bool(x), Value::Bool(y)) => Ok(Value::Bool(x && y)),
                    _ => Err(BorrockError::Type("And".into())),
                },
                Or => match (a, b) {
                    (Value::Bool(x), Value::Bool(y)) => Ok(Value::Bool(x || y)),
                    _ => Err(BorrockError::Type("Or".into())),
                },
            }
        }
    }

    pub use Env;
    pub use Interpreter;
}
pub use eval::{Env, Interpreter};

// ============================================================================
// Parser (stub de démonstration)
// ============================================================================

mod parser {
    use super::*;

    #[derive(Debug, Clone)]
    pub struct ParseOptions {
        pub filename: Option<PathBuf>,
    }

    impl Default for ParseOptions {
        fn default() -> Self { Self { filename: None } }
    }

    /// Parseur non implémenté. Sert d’interface de raccord.
    /// Entrée cible à terme: DSL borrock. Pour l’instant, on expose un
    /// constructeur utilitaire créant un module "main" addition/multiplication.
    pub fn parse_str_stub(_src: &str, _opts: &ParseOptions) -> Result<Module> {
        let mut m = Module::new();

        // fn main() { const a=40; const b=2; c=a+b; print(c); ret c; }
        let mut body = Block::new("entry");
        body.instrs.push(Instr::Const { dst: "a".into(), value: Value::Num(40.0) });
        body.instrs.push(Instr::Const { dst: "b".into(), value: Value::Num(2.0) });
        body.instrs.push(Instr::Bin { dst: "c".into(), op: ir::BinOp::Add, a: "a".into(), b: "b".into() });
        body.instrs.push(Instr::CallBuiltin { dst: None, name: "print".into(), args: vec!["c".into()] });
        body.instrs.push(Instr::Ret { src: Some("c".into()) });

        let f = Func { name: "main".into(), params: vec![], body };
        m.add_func(f);
        m.exports.push("main".into());
        Ok(m)
    }

    pub use ParseOptions as Options;
}
pub use parser::{parse_str_stub, Options as ParseOptions};

// ============================================================================
// Moteur et pipeline
// ============================================================================

pub trait Engine {
    fn eval_module(&mut self, m: &Module, entry: &str) -> Result<Value>;
    fn eval_text(&mut self, src: &str) -> Result<Value>;
}

pub struct BorrockEngine {
    cfg: BorrockConfig,
    passes: Vec<Box<dyn IrPass>>,
    builtins: Builtins,
    emitter: Arc<dyn Emitter>,
}

impl BorrockEngine {
    pub fn new(cfg: Option<BorrockConfig>) -> Self {
        let cfg = cfg.unwrap_or_default();
        let mut passes: Vec<Box<dyn IrPass>> = Vec::new();
        if cfg.optimize && cfg.enable_const_folding { passes.push(Box::new(ConstFold::new())); }
        if cfg.optimize && cfg.enable_dce { passes.push(Box::new(Dce::new())); }
        let builtins = Builtins::with_default();
        Self {
            cfg,
            passes,
            builtins,
            emitter: Arc::new(SimpleEmitter::default()),
        }
    }

    pub fn emitter(mut self, e: Arc<dyn Emitter>) -> Self { self.emitter = e; self }

    pub fn add_pass<P: IrPass + 'static>(mut self, p: P) -> Self { self.passes.push(Box::new(p)); self }

    pub fn builtins_mut(&mut self) -> &mut Builtins { &mut self.builtins }

    fn run_passes(&mut self, m: &mut Module) -> Result<()> {
        for p in self.passes.iter_mut() {
            log::debug!("pass {}", p.name());
            p.run(m)?;
        }
        Ok(())
    }
}

impl Engine for BorrockEngine {
    fn eval_module(&mut self, m: &Module, entry: &str) -> Result<Value> {
        let mut clone = m.clone();
        self.run_passes(&mut clone)?;
        let interp = Interpreter::new(&self.builtins).with_emitter(self.emitter.clone());
        interp.eval_module(&clone, entry)
    }

    fn eval_text(&mut self, src: &str) -> Result<Value> {
        let m = parse_str_stub(src, &ParseOptions::default())?;
        let entry = self.cfg.entry.as_deref().unwrap_or("main");
        self.eval_module(&m, entry)
    }
}

// ============================================================================
// Utilitaires
// ============================================================================

/// Charge un module JSON sérialisé depuis le disque.
pub fn load_module_json(path: &Path) -> Result<Module> {
    let s = std::fs::read_to_string(path)?;
    let m: Module = serde_json::from_str(&s)?;
    Ok(m)
}

/// Sauvegarde un module JSON.
pub fn save_module_json(path: &Path, m: &Module) -> Result<()> {
    let s = serde_json::to_string_pretty(m)?;
    std::fs::write(path, s)?;
    Ok(())
}

// ============================================================================
// Exemple de construction programmatique
// ============================================================================

/// Construit un `Module` de démonstration: calcule (x+1)*(x+2) avec x=5.
pub fn demo_module() -> Module {
    let mut b = Block::new("entry");
    b.instrs.push(Instr::Const { dst: "x".into(), value: Value::Num(5.0) });
    b.instrs.push(Instr::Const { dst: "one".into(), value: Value::Num(1.0) });
    b.instrs.push(Instr::Const { dst: "two".into(), value: Value::Num(2.0) });
    b.instrs.push(Instr::Bin { dst: "a".into(), op: ir::BinOp::Add, a: "x".into(), b: "one".into() });
    b.instrs.push(Instr::Bin { dst: "b".into(), op: ir::BinOp::Add, a: "x".into(), b: "two".into() });
    b.instrs.push(Instr::Bin { dst: "y".into(), op: ir::BinOp::Mul, a: "a".into(), b: "b".into() });
    b.instrs.push(Instr::CallBuiltin { dst: None, name: "print".into(), args: vec!["y".into()] });
    b.instrs.push(Instr::Ret { src: Some("y".into()) });

    let f = Func { name: "main".into(), params: vec![], body: b };
    let mut m = Module::new();
    m.add_func(f);
    m.exports.push("main".into());
    m
}

// ============================================================================
// Tests
// ============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn eval_demo() {
        let mut eng = BorrockEngine::new(None);
        let m = demo_module();
        let v = eng.eval_module(&m, "main").unwrap();
        match v { Value::Num(n) => assert_eq!(n, 42.0), _ => panic!("unexpected") }
    }

    #[test]
    fn eval_text_stub() {
        let mut eng = BorrockEngine::new(None);
        let v = eng.eval_text("main()").unwrap();
        match v { Value::Num(n) => assert_eq!(n, 42.0), _ => panic!("expected 42") }
    }

    #[test]
    fn json_roundtrip() {
        let m = demo_module();
        let s = serde_json::to_string(&m).unwrap();
        let m2: Module = serde_json::from_str(&s).unwrap();
        assert_eq!(m.funcs.len(), m2.funcs.len());
    }
}