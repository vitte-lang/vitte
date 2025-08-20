//! interpreter.rs — Boucle d’exécution (VM Vitte)
//!
//! MVP lisible et extensible :
//! - Stack VM simple (Value minimaliste ici — adapte à ton `vitte-runtime`)
//! - Cadres d’appel (frames) basiques (BP/IP/nom de fonction)
//! - Hooks de debug (`VmHook`) invoqués *avant* chaque instruction
//! - Exemples d’opcodes usuels (CONST, ADD/SUB/..., JUMP, CALL/RET, PRINT, HALT)
//!
//! ⚠️ Les enums `Op`/`Chunk` sont supposés venir de `vitte-core`.
//!    Si tes variants diffèrent, ajuste les `match` et les helpers.
//!
//! Intégration debug : vois `debug.rs` (VmHook, VmDebugger).
//!   - Appelle `hook.on_instruction(&ctx)` à chaque pas,
//!   - Construit un `ExecContext` (thread, frame, pc) ; source/ligne optionnels.

use std::fmt;

use color_eyre::eyre::{eyre, Result};
use log::{debug, trace};

use crate::debug::VmHook;
use vitte_debugger::hook::ExecContext;

// Adapte ces imports selon ton `vitte-core`.
use crate::bytecode::{Chunk, Op};

/// Valeur VM minimaliste pour le MVP.
/// Si tu as déjà un `Value` dans `vitte-runtime`, ré-exporte-le ici.
#[derive(Clone)]
pub enum Value {
    Nil,
    Bool(bool),
    Int(i64),
    Float(f64),
    Str(String),
}

impl fmt::Debug for Value {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Value::Nil => write!(f, "nil"),
            Value::Bool(b) => write!(f, "{b}"),
            Value::Int(i) => write!(f, "{i}"),
            Value::Float(x) => write!(f, "{x}"),
            Value::Str(s) => write!(f, "\"{s}\""),
        }
    }
}

impl Value {
    fn as_i64(&self) -> Result<i64> {
        match self {
            Value::Int(i) => Ok(*i),
            Value::Float(f) => Ok(*f as i64),
            _ => Err(eyre!("attendu nombre entier, trouvé {:?}", self)),
        }
    }
    fn as_f64(&self) -> Result<f64> {
        match self {
            Value::Int(i) => Ok(*i as f64),
            Value::Float(f) => Ok(*f),
            _ => Err(eyre!("attendu nombre, trouvé {:?}", self)),
        }
    }
}

/// Un cadre d’appel simple
#[derive(Debug, Clone)]
struct Frame {
    name: String,
    ip: usize,   // index de l’instruction courante dans le chunk (sera mis à jour)
    bp: usize,   // base pointer dans la pile
}

impl Frame {
    fn new(name: impl Into<String>, ip: usize, bp: usize) -> Self {
        Self { name: name.into(), ip, bp }
    }
}

/// Interpréteur
pub struct Interpreter {
    pub thread_id: i64,
    chunk: Chunk,
    ip: usize,
    stack: Vec<Value>,
    frames: Vec<Frame>,
    hook: Option<Box<dyn VmHook>>,
}

impl Interpreter {
    pub fn new(thread_id: i64, chunk: Chunk) -> Self {
        Self {
            thread_id,
            ip: 0,
            stack: Vec::with_capacity(256),
            frames: vec![Frame::new("main", 0, 0)],
            chunk,
            hook: None,
        }
    }

    /// Optionnel : installer un hook de debug (voir `debug.rs`)
    pub fn set_debug_hook(&mut self, hook: Box<dyn VmHook>) {
        self.hook = Some(hook);
    }

    /// Lance l’exécution jusqu’à HALT ou erreur.
    pub fn run(&mut self) -> Result<()> {
        while self.ip < self.chunk.ops.len() {
            // Hook de debug (avant exécution)
            if let Some(h) = self.hook.as_mut() {
                let ctx = self.exec_context();
                h.on_instruction(&ctx)?; // Le hook peut décider de stopper via les couches supérieures
            }

            let op = self.fetch()?;
            trace!("ip={} op={:?}", self.ip.saturating_sub(1), short(&op));
            self.exec(op)?;
        }
        Ok(())
    }

    /* -------------------------- Boucle principale -------------------------- */

    fn fetch(&mut self) -> Result<Op> {
        let op = self.chunk.ops.get(self.ip)
            .ok_or_else(|| eyre!("IP hors borne: {}", self.ip))?
            .clone();
        self.ip += 1;
        Ok(op)
    }

    fn exec(&mut self, op: Op) -> Result<()> {
        use Op::*;
        match op {
            // ---- Constantes / chargements ----
            // Adapte au format de ton pool de constantes
            ConstI64(i) => self.push(Value::Int(i)),
            ConstF64(x) => self.push(Value::Float(x)),
            ConstStr(ix) => {
                let s = self
                    .chunk
                    .strings
                    .get(ix as usize)
                    .ok_or_else(|| eyre!("string index invalide: {}", ix))?
                    .clone();
                self.push(Value::Str(s));
            }

            // ---- Arithmétique ----
            Add => {
                let b = self.pop()?;
                let a = self.pop()?;
                self.push(Value::Float(a.as_f64()? + b.as_f64()?));
            }
            Sub => {
                let b = self.pop()?;
                let a = self.pop()?;
                self.push(Value::Float(a.as_f64()? - b.as_f64()?));
            }
            Mul => {
                let b = self.pop()?;
                let a = self.pop()?;
                self.push(Value::Float(a.as_f64()? * b.as_f64()?));
            }
            Div => {
                let b = self.pop()?;
                let a = self.pop()?;
                self.push(Value::Float(a.as_f64()? / b.as_f64()?));
            }

            // ---- Comparaisons (bool) ----
            Eq => {
                let b = self.pop()?;
                let a = self.pop()?;
                self.push(Value::Bool(eq(&a, &b)));
            }
            Ne => {
                let b = self.pop()?;
                let a = self.pop()?;
                self.push(Value::Bool(!eq(&a, &b)));
            }
            Lt => {
                let b = self.pop()?.as_f64()?;
                let a = self.pop()?.as_f64()?;
                self.push(Value::Bool(a < b));
            }
            Le => {
                let b = self.pop()?.as_f64()?;
                let a = self.pop()?.as_f64()?;
                self.push(Value::Bool(a <= b));
            }
            Gt => {
                let b = self.pop()?.as_f64()?;
                let a = self.pop()?.as_f64()?;
                self.push(Value::Bool(a > b));
            }
            Ge => {
                let b = self.pop()?.as_f64()?;
                let a = self.pop()?.as_f64()?;
                self.push(Value::Bool(a >= b));
            }

            // ---- Sauts ----
            Jump(target) => self.jump(target)?,
            JumpIfFalse(target) => {
                let c = self.pop()?;
                if !truthy(&c) {
                    self.jump(target)?;
                }
            }

            // ---- Appels ----
            // Ici on suppose un appel par adresse (ip target) et un `argc`.
            // Adapte selon ton ABI (registres, closures, upvalues…).
            Call { target, argc, name_ix } => {
                let base = self.stack.len().saturating_sub(argc as usize);
                let fname = name_ix
                    .and_then(|ix| self.chunk.strings.get(ix as usize).cloned())
                    .unwrap_or_else(|| "<fn>".into());
                // Empiler un frame (l’IP actuel pointe déjà sur l’op suivant)
                let ret_ip = self.ip;
                self.frames.push(Frame::new(fname, target as usize, base));
                self.ip = target as usize;
                // (si tu veux poser des registres, c’est le bon moment)
                debug!("CALL → ip={} argc={} base={}", self.ip, argc, base);
                // On pourrait aussi vérifier que la fonction callee a la bonne arité.
            }
            Ret => {
                // Valeur de retour (optionnelle) → convention: top de pile
                let ret = self.pop().ok().unwrap_or(Value::Nil);

                let frame = self.frames.pop().ok_or_else(|| eyre!("RET sans frame"))?;
                // Nettoyer la pile au niveau de base
                while self.stack.len() > frame.bp {
                    self.stack.pop();
                }
                // Remettre la valeur de retour
                self.push(ret);

                // Reprendre à l’IP du call-site
                let caller = self.frames.last().ok_or_else(|| eyre!("RET sans caller"))?;
                self.ip = caller.ip_after(); // helper ci-dessous
            }

            // ---- IO / debug ----
            Print => {
                let v = self.pop()?;
                println!("{:?}", v);
            }

            // ---- Fin ----
            Halt => {
                // On force ip = len pour quitter proprement
                self.ip = self.chunk.ops.len();
            }

            // ---- Placeholders : adapte aux autres variants d’Op chez toi ----
            other => {
                // Tu peux soit `todo!()`, soit renvoyer une erreur
                return Err(eyre!("opcode non géré dans l'interpréteur: {:?}", other));
            }
        }
        Ok(())
    }

    /* ------------------------------ Helpers VM ----------------------------- */

    fn push(&mut self, v: Value) {
        self.stack.push(v);
    }
    fn pop(&mut self) -> Result<Value> {
        self.stack.pop().ok_or_else(|| eyre!("pile vide"))
    }

    fn jump(&mut self, target: u32) -> Result<()> {
        let t = target as usize;
        if t >= self.chunk.ops.len() {
            return Err(eyre!("jump cible hors borne: {}", t));
        }
        self.ip = t;
        Ok(())
    }

    /// Construit un `ExecContext` pour les hooks de debug.
    /// Si tu as des *debug infos* (source path/line par IP), renseigne-les ici.
    fn exec_context(&self) -> ExecContext {
        let top = self.frames.last();
        ExecContext {
            thread_id: self.thread_id,
            frame_id: top.map(|_| 1), // identifiant symbolique MVP ; remplace par un vrai id si besoin
            pc: self.ip as u64,
            function: top.map(|f| f.name.clone()),
            source_path: self.source_path_for_ip(self.ip),
            line: self.source_line_for_ip(self.ip),
        }
    }

    /// Donne un chemin source pour une IP (si debug info dispo).
    fn source_path_for_ip(&self, _ip: usize) -> Option<String> {
        // TODO: si ton `Chunk` a une table `dbg_ip_to_file`, branche ici.
        None
    }
    /// Donne une ligne source pour une IP (si debug info dispo).
    fn source_line_for_ip(&self, _ip: usize) -> Option<u32> {
        // TODO: si ton `Chunk` a une table `dbg_ip_to_line`, branche ici.
        None
    }
}

/* ---------------------------- Utilitaires --------------------------------- */

fn truthy(v: &Value) -> bool {
    match v {
        Value::Nil => false,
        Value::Bool(b) => *b,
        Value::Int(i) => *i != 0,
        Value::Float(x) => *x != 0.0,
        Value::Str(s) => !s.is_empty(),
    }
}

fn eq(a: &Value, b: &Value) -> bool {
    use Value::*;
    match (a, b) {
        (Nil, Nil) => true,
        (Bool(x), Bool(y)) => x == y,
        (Int(x), Int(y)) => x == y,
        (Float(x), Float(y)) => x == y,
        (Int(x), Float(y)) => (*x as f64) == *y,
        (Float(x), Int(y)) => *x == (*y as f64),
        (Str(x), Str(y)) => x == y,
        _ => false,
    }
}

/* ---------------------------- Extensions Frame ---------------------------- */

impl Frame {
    /// IP de reprise pour le caller (ici on suppose que `ip` du caller pointe déjà
    /// sur l’instruction suivante — si tu stockes explicitement `ret_ip`, adapte).
    fn ip_after(&self) -> usize {
        // Dans ce MVP, on ne garde pas `ret_ip` sur le frame callee.
        // On se repose sur l’IP courant (déjà avancé lors du CALL).
        // Si tu ranges `ret_ip` dans le frame, renvoie-le ici.
        self.ip
    }
}

/* ------------------------------- Debug view ------------------------------- */

fn short(op: &Op) -> String {
    // Mini pretty-printer d’opcode pour le tracing
    use Op::*;
    match op {
        ConstI64(i) => format!("ConstI64({i})"),
        ConstF64(x) => format!("ConstF64({x})"),
        ConstStr(ix) => format!("ConstStr({ix})"),
        Add => "Add".into(),
        Sub => "Sub".into(),
        Mul => "Mul".into(),
        Div => "Div".into(),
        Eq => "Eq".into(),
        Ne => "Ne".into(),
        Lt => "Lt".into(),
        Le => "Le".into(),
        Gt => "Gt".into(),
        Ge => "Ge".into(),
        Jump(t) => format!("Jump({t})"),
        JumpIfFalse(t) => format!("JumpIfFalse({t})"),
        Call { target, argc, .. } => format!("Call{{target:{target}, argc:{argc}}}"),
        Ret => "Ret".into(),
        Print => "Print".into(),
        Halt => "Halt".into(),
        _ => format!("{op:?}"),
    }
}
