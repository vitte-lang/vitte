//! vitte-runtime — Pont FFI / services hôte pour Vitte
//!
//! Objectif : offrir une **boîte à outils runtime** indépendante de la VM,
//! pour brancher des **fonctions natives** (I/O, temps, OS…) appelables
//! depuis le bytecode / la VM. Le projet peut ainsi évoluer sans couplage fort.
//!
//! - `Value`   : type dynamique simple (Null/Bool/I64/F64/Str/Bytes)
//! - `Runtime` : registre de natives, environnement clé/valeur, stdout capturable
//! - `RuntimeCtx` : contexte passé aux natives (stdout/env, etc.)
//! - `register_fn`, `call`, `with_defaults()` pour I/O/temps/math basiques
//! - `vitte_native!` : macro pour écrire des natives en 2 lignes
//!
//! Tu pourras plus tard :
//! - exposer ces natives via une instruction VM (ex: `CallNative idx`),
//! - ou via un tableau de résolutions nom→idx préparé au chargement.
//!
//! ⚠️ Ce crate **n’exécute pas** le bytecode (c’est le rôle de `vitte-vm`).

#![forbid(unsafe_code)]
#![deny(rust_2018_idioms, unused_must_use)]
#![cfg_attr(not(debug_assertions), warn(missing_docs))]

use std::collections::HashMap;
use std::fmt;
use std::io::{self, Write};
use std::time::{SystemTime, UNIX_EPOCH};

use thiserror::Error;

/* --------------------------- Types de valeur --------------------------- */

/// Valeur dynamique échangée avec les natives.
///
/// Simple, mais suffisante pour 95% des FFI : on reste num/texte/bytes.
/// Étends plus tard avec `Array`, `Map`, `Func`, etc., si besoin.
#[derive(Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub enum Value {
    /// Null / absence de valeur.
    Null,
    /// Booléen.
    Bool(bool),
    /// Entier 64 bits signé.
    I64(i64),
    /// Flottant 64 bits.
    F64(f64),
    /// Chaîne UTF-8 possédée.
    Str(String),
    /// Blob binaire.
    Bytes(Vec<u8>),
}

impl fmt::Debug for Value {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Value::Null => write!(f, "Null"),
            Value::Bool(b) => write!(f, "Bool({b})"),
            Value::I64(i) => write!(f, "I64({i})"),
            Value::F64(x) => write!(f, "F64({x})"),
            Value::Str(s) => {
                if s.len() > 64 { write!(f, "Str({}…)", &s[..64]) } else { write!(f, "Str({s})") }
            }
            Value::Bytes(b) => write!(f, "Bytes(len={})", b.len()),
        }
    }
}

/* Conversions convivia-les */
impl From<()> for Value { fn from(_: ()) -> Self { Value::Null } }
impl From<bool> for Value { fn from(v: bool) -> Self { Value::Bool(v) } }
impl From<i64> for Value { fn from(v: i64) -> Self { Value::I64(v) } }
impl From<i32> for Value { fn from(v: i32) -> Self { Value::I64(v as i64) } }
impl From<f64> for Value { fn from(v: f64) -> Self { Value::F64(v) } }
impl From<f32> for Value { fn from(v: f32) -> Self { Value::F64(v as f64) } }
impl From<String> for Value { fn from(v: String) -> Self { Value::Str(v) } }
impl From<&str> for Value { fn from(v: &str) -> Self { Value::Str(v.to_owned()) } }
impl From<Vec<u8>> for Value { fn from(v: Vec<u8>) -> Self { Value::Bytes(v) } }

impl TryFrom<Value> for bool {
    type Error = Error;
    fn try_from(v: Value) -> std::result::Result<Self, Self::Error> {
        match v { Value::Bool(b) => Ok(b), _ => Err(Error::Type("bool".into())) }
    }
}
impl TryFrom<Value> for i64 {
    type Error = Error;
    fn try_from(v: Value) -> std::result::Result<Self, Self::Error> {
        match v {
            Value::I64(i) => Ok(i),
            Value::F64(x) => Ok(x as i64),
            _ => Err(Error::Type("i64".into())),
        }
    }
}
impl TryFrom<Value> for f64 {
    type Error = Error;
    fn try_from(v: Value) -> std::result::Result<Self, Self::Error> {
        match v {
            Value::F64(x) => Ok(x),
            Value::I64(i) => Ok(i as f64),
            _ => Err(Error::Type("f64".into())),
        }
    }
}
impl TryFrom<Value> for String {
    type Error = Error;
    fn try_from(v: Value) -> std::result::Result<Self, Self::Error> {
        match v { Value::Str(s) => Ok(s), _ => Err(Error::Type("string".into())) }
    }
}

/* ------------------------------ Erreurs ------------------------------ */

/// Erreurs runtime.
#[derive(Debug, Error)]
pub enum Error {
    /// Fonction native introuvable.
    #[error("fonction native introuvable: {0}")]
    NotFound(String),

    /// Arite invalide (n args attendus).
    #[error("mauvaise arité: attendu {expected}, reçu {got}")]
    Arity {
        /// Nombre d'arguments attendus par la native.
        expected: usize,
        /// Nombre d'arguments effectivement fournis lors de l'appel.
        got: usize,
    },

    /// Type inattendu (message court).
    #[error("type invalide: {0}")]
    Type(String),

    /// I/O hôte.
    #[error("io: {0}")]
    Io(#[from] io::Error),

    /// Message générique.
    #[error("{0}")]
    Msg(String),
}

/// Résultat du runtime.
pub type RResult<T> = std::result::Result<T, Error>;

/* -------------------------- Signature des natives -------------------------- */

/// Fonction native (FFI) : reçoit des **Values** et un **contexte** mut.
/// Retourne une Value (ou une erreur) — pas d’allocs exotiques.
pub type NativeFn = fn(&[Value], &mut RuntimeCtx) -> RResult<Value>;

/// Descripteur d’une native.
#[derive(Clone)]
pub struct NativeEntry {
    /// Nom complet `module.name`
    pub fqname: String,
    /// Arité (si connue). Si `None`, libre.
    pub arity: Option<usize>,
    /// Pointeur de fonction.
    pub func: NativeFn,
}

/* ------------------------------ Contexte ------------------------------ */

/// Contexte passé aux natives (stdout, env, …).
pub struct RuntimeCtx {
    /// Sortie vers laquelle `io.print/println` vont écrire (capturable en tests).
    pub stdout: Box<dyn Write + Send>,
    /// Petit KV store global (utile pour partager un état).
    pub env: HashMap<String, Value>,
}

impl RuntimeCtx {
    /// Écrit un texte brut dans `stdout`.
    pub fn write_str(&mut self, s: &str) -> io::Result<()> { self.stdout.write_all(s.as_bytes()) }
    /// Écrit une ligne terminée par `\n`.
    pub fn writeln_str(&mut self, s: &str) -> io::Result<()> {
        self.stdout.write_all(s.as_bytes())?;
        self.stdout.write_all(b"\n")
    }
}

/* ------------------------------ Runtime ------------------------------ */

/// Runtime: registre de natives + contexte partagé.
pub struct Runtime {
    registry: HashMap<String, NativeEntry>,
    ctx: RuntimeCtx,
}

impl Default for Runtime {
    fn default() -> Self { Self::new() }
}

impl Runtime {
    /// Crée un runtime avec `stdout` réel et env vide.
    pub fn new() -> Self {
        Self {
            registry: HashMap::new(),
            ctx: RuntimeCtx {
                stdout: Box::new(io::stdout()),
                env: HashMap::new(),
            },
        }
    }

    /// Variante utile pour tests: `stdout` capturé.
    pub fn with_captured_stdout() -> (Self, Captured) {
        let cap = Captured::default();
        let rt = Self {
            registry: HashMap::new(),
            ctx: RuntimeCtx {
                stdout: Box::new(cap.clone()),
                env: HashMap::new(),
            },
        };
        (rt, cap)
    }

    /// Permet d’injecter un writer custom (ex: buffer, fichier…).
    pub fn with_stdout<W: Write + Send + 'static>(mut self, w: W) -> Self {
        self.ctx.stdout = Box::new(w);
        self
    }

    /// Accès lecture/écriture à l’environnement global.
    pub fn env(&self) -> &HashMap<String, Value> { &self.ctx.env }
    /// Donne un accès mutable à l'environnement global partagé entre les natives.
    pub fn env_mut(&mut self) -> &mut HashMap<String, Value> { &mut self.ctx.env }

    /// Enregistre une fonction native sous `module` + `name`.
    pub fn register_fn(
        &mut self,
        module: &str,
        name: &str,
        arity: Option<usize>,
        func: NativeFn,
    ) {
        let fq = format!("{module}.{name}");
        self.registry.insert(fq.clone(), NativeEntry { fqname: fq, arity, func });
    }

    /// Enregistre un lot (module complet).
    pub fn register_all(&mut self, module: &str, entries: &[(&str, Option<usize>, NativeFn)]) {
        for (name, arity, func) in entries {
            self.register_fn(module, name, *arity, *func);
        }
    }

    /// Appelle une native par `module.name` avec des arguments.
    pub fn call<S: AsRef<str>>(&mut self, fqname: S, args: &[Value]) -> RResult<Value> {
        let key = fqname.as_ref();
        let entry = self.registry.get(key).ok_or_else(|| Error::NotFound(key.to_string()))?;
        if let Some(exp) = entry.arity {
            if args.len() != exp {
                return Err(Error::Arity { expected: exp, got: args.len() });
            }
        }
        (entry.func)(args, &mut self.ctx)
    }

    /// Précharge un **set de natives par défaut** (`io`, `time`, `math`, `os`).
    pub fn with_defaults() -> Self {
        let mut rt = Self::new();

        // io
        rt.register_all("io", &[
            ("print",   Some(1), native_io_print as NativeFn),
            ("println", Some(1), native_io_println as NativeFn),
            ("read",    Some(1), native_io_read_file as NativeFn),
            ("write",   Some(2), native_io_write_file as NativeFn),
        ]);

        // time
        rt.register_all("time", &[
            ("now_ms",  Some(0), native_time_now_ms as NativeFn),
        ]);

        // math
        rt.register_all("math", &[
            ("abs",   Some(1), native_math_abs as NativeFn),
            ("floor", Some(1), native_math_floor as NativeFn),
            ("ceil",  Some(1), native_math_ceil as NativeFn),
        ]);

        // os (environnement minimal)
        rt.register_all("os", &[
            ("getenv", Some(1), native_os_getenv as NativeFn),
            ("setenv", Some(2), native_os_setenv as NativeFn),
        ]);

        rt
    }
}

/* --------------------------- Impl natives par défaut --------------------------- */

fn native_io_print(args: &[Value], ctx: &mut RuntimeCtx) -> RResult<Value> {
    let s: String = args[0].clone().try_into().unwrap_or_else(|_| format!("{:?}", args[0]));
    ctx.write_str(&s)?;
    Ok(Value::Null)
}

fn native_io_println(args: &[Value], ctx: &mut RuntimeCtx) -> RResult<Value> {
    let s: String = args[0].clone().try_into().unwrap_or_else(|_| format!("{:?}", args[0]));
    ctx.writeln_str(&s)?;
    Ok(Value::Null)
}

fn native_io_read_file(args: &[Value], _ctx: &mut RuntimeCtx) -> RResult<Value> {
    let path: String = args[0].clone().try_into()?;
    let s = std::fs::read_to_string(path)?;
    Ok(Value::Str(s))
}

fn native_io_write_file(args: &[Value], _ctx: &mut RuntimeCtx) -> RResult<Value> {
    let path: String = args[0].clone().try_into()?;
    let text: String = args[1].clone().try_into()?;
    if let Some(parent) = std::path::Path::new(&path).parent() {
        let _ = std::fs::create_dir_all(parent);
    }
    std::fs::write(path, text.as_bytes())?;
    Ok(Value::Bool(true))
}

fn native_time_now_ms(_args: &[Value], _ctx: &mut RuntimeCtx) -> RResult<Value> {
    let t = SystemTime::now().duration_since(UNIX_EPOCH)
        .map_err(|e| Error::Msg(format!("clock error: {e}")))?;
    Ok(Value::I64(t.as_millis() as i64))
}

fn native_math_abs(args: &[Value], _ctx: &mut RuntimeCtx) -> RResult<Value> {
    match &args[0] {
        Value::I64(i) => Ok(Value::I64(i.abs())),
        Value::F64(x) => Ok(Value::F64(x.abs())),
        _ => Err(Error::Type("number".into())),
    }
}
fn native_math_floor(args: &[Value], _ctx: &mut RuntimeCtx) -> RResult<Value> {
    match &args[0] {
        Value::I64(i) => Ok(Value::I64(*i)),
        Value::F64(x) => Ok(Value::F64(x.floor())),
        _ => Err(Error::Type("number".into())),
    }
}
fn native_math_ceil(args: &[Value], _ctx: &mut RuntimeCtx) -> RResult<Value> {
    match &args[0] {
        Value::I64(i) => Ok(Value::I64(*i)),
        Value::F64(x) => Ok(Value::F64(x.ceil())),
        _ => Err(Error::Type("number".into())),
    }
}

fn native_os_getenv(args: &[Value], _ctx: &mut RuntimeCtx) -> RResult<Value> {
    let key: String = args[0].clone().try_into()?;
    match std::env::var(&key) {
        Ok(v) => Ok(Value::Str(v)),
        Err(_) => Ok(Value::Null),
    }
}
fn native_os_setenv(args: &[Value], _ctx: &mut RuntimeCtx) -> RResult<Value> {
    let key: String = args[0].clone().try_into()?;
    let val: String = args[1].clone().try_into()?;
    std::env::set_var(key, val);
    Ok(Value::Bool(true))
}

/* ----------------------------- Macro sucrée ----------------------------- */

/// Macro pour déclarer une native rapidement.
///
/// # Exemple
/// ```
/// use vitte_runtime::{vitte_native, Value, RuntimeCtx, RResult};
/// vitte_native!(hello |args, ctx| {
///     let name = args.get(0).cloned().unwrap_or_else(|| "world".into());
///     Ok(format!("Hello, {name:?}!").into())
/// });
/// ```
#[macro_export]
macro_rules! vitte_native {
    ($name:ident |$args:ident, $ctx:ident| $body:block) => {
        pub fn $name($args: &[ $crate::Value ], $ctx: &mut $crate::RuntimeCtx) -> $crate::RResult<$crate::Value> {
            $body
        }
    };
}

/* ------------------------ Outil de capture stdout ------------------------ */

/// Petit writer qui **capture** le stdout dans une String (utile en tests/REPL).
#[derive(Default, Clone)]
pub struct Captured(std::sync::Arc<std::sync::Mutex<String>>);

impl Captured {
    /// Récupère le buffer (copie).
    pub fn get(&self) -> String { self.0.lock().unwrap().clone() }
    /// Réinitialise le buffer.
    pub fn clear(&self) { self.0.lock().unwrap().clear(); }
}
impl Write for Captured {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        let s = String::from_utf8_lossy(buf);
        self.0.lock().unwrap().push_str(&s);
        Ok(buf.len())
    }
    fn flush(&mut self) -> io::Result<()> { Ok(()) }
}

/* -------------------------------- Prelude -------------------------------- */

/// Prelude pratique pour importer d’un coup.
pub mod prelude {
    pub use crate::{
        Runtime, RuntimeCtx, Value, NativeFn, NativeEntry, RResult, Error, Captured,
        vitte_native,
    };
}

/* --------------------------------- Tests --------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn default_natives_work() {
        let (mut rt, cap) = Runtime::with_captured_stdout();
        // enregistrer defaults
        rt = Runtime::with_defaults().with_stdout(cap.clone());

        // println
        let _ = rt.call("io.println", &[Value::from("yo")]).unwrap();
        let out = cap.get();
        assert!(out.contains("yo"));

        // time
        let v = rt.call("time.now_ms", &[]).unwrap();
        match v { Value::I64(i) => assert!(i > 0), _ => panic!("now_ms doit renvoyer I64") }

        // math
        let v = rt.call("math.abs", &[Value::from(-42i64)]).unwrap();
        assert_eq!(v, Value::I64(42));
    }

    #[test]
    fn register_and_call_custom() {
        vitte_native!(plus_one |args, _ctx| {
            let x: i64 = args.get(0).cloned().ok_or_else(|| Error::Arity{expected:1, got:0})?.try_into()?;
            Ok((x + 1).into())
        });

        let mut rt = Runtime::new();
        rt.register_fn("demo", "plus_one", Some(1), plus_one);

        let out = rt.call("demo.plus_one", &[Value::from(9i64)]).unwrap();
        assert_eq!(out, Value::I64(10));
    }

    #[test]
    fn arity_and_types() {
        let mut rt = Runtime::new();
        rt.register_fn("t", "id", Some(1), |a, _| Ok(a[0].clone()));

        let err = rt.call("t.id", &[]).unwrap_err();
        match err { Error::Arity{..} => {}, _ => panic!("devrait être Arity") }

        let (mut rt, _) = Runtime::with_captured_stdout();
        rt.register_fn("io", "print", Some(1), native_io_print);
        // OK: imprime debug d’un type non-string
        let _ = rt.call("io.print", &[Value::Bool(true)]).unwrap();
    }
}
