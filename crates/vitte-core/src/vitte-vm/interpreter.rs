//! vitte-vm/interpreter.rs
//!
//! Interpréteur minimal pour le bytecode Vitte (pile + locals).
//! - Pile dynamique (Vec<Value>)
//! - Locals auto-agrandis à l’écriture/lecture
//! - Sauts relatifs (pc := pc + 1 + off)
//! - Arithmétique i64/f64, comparaisons, (dé)booléens
//! - `Print` écrit sur stdout
//!
//! ⚠️ Opcodes non encore supportés → erreur explicite:
//!   - Call / TailCall
//!   - MakeClosure / LoadUpvalue / StoreUpvalue
//!
//! API:
//!   let mut vm = Vm::new();
//!   vm.run(&chunk)?;
//!
//! Ajoute au besoin un budget d’instructions avec `set_max_steps`.

use std::fmt;

use vitte_core::bytecode::{
    chunk::{Chunk, ConstValue, ConstPool},
    op::Op,
};

/// Valeurs manipulées par la VM.
#[derive(Clone, Debug, PartialEq)]
pub enum Value {
    Null,
    Bool(bool),
    I64(i64),
    F64(f64),
    Str(String),
    Bytes(Vec<u8>),
}

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Value::Null => write!(f, "null"),
            Value::Bool(b) => write!(f, "{b}"),
            Value::I64(i) => write!(f, "{i}"),
            Value::F64(x) => {
                // affichage sympa: 1.0 -> 1
                if x.fract() == 0.0 {
                    write!(f, "{:.0}", x)
                } else {
                    write!(f, "{x}")
                }
            }
            Value::Str(s) => write!(f, "{s}"),
            Value::Bytes(b) => write!(f, "bytes[len={}]", b.len()),
        }
    }
}

impl From<&ConstValue> for Value {
    fn from(c: &ConstValue) -> Self {
        match c {
            ConstValue::Null => Value::Null,
            ConstValue::Bool(b) => Value::Bool(*b),
            ConstValue::I64(i) => Value::I64(*i),
            ConstValue::F64(x) => Value::F64(*x),
            ConstValue::Str(s) => Value::Str(s.clone()),
            ConstValue::Bytes(b) => Value::Bytes(b.clone()),
        }
    }
}

/// Erreurs d’exécution.
#[derive(Debug)]
pub enum VmError {
    StackUnderflow(&'static str),
    Type(String),
    DivByZero,
    ConstOutOfRange(u32),
    JumpOutOfRange(i64),
    Unsupported(&'static str),
}

impl fmt::Display for VmError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use VmError::*;
        match self {
            StackUnderflow(w) => write!(f, "stack underflow (op: {w})"),
            Type(s) => write!(f, "type error: {s}"),
            DivByZero => write!(f, "division by zero"),
            ConstOutOfRange(ix) => write!(f, "const index out of range: {ix}"),
            JumpOutOfRange(n) => write!(f, "jump target out of range: {n}"),
            Unsupported(op) => write!(f, "unsupported opcode: {op}"),
        }
    }
}

impl std::error::Error for VmError {}

/// Interpréteur.
pub struct Vm {
    stack: Vec<Value>,
    locals: Vec<Value>,
    max_steps: Option<u64>,
}

impl Default for Vm {
    fn default() -> Self {
        Self {
            stack: Vec::with_capacity(64),
            locals: Vec::new(),
            max_steps: None,
        }
    }
}

impl Vm {
    pub fn new() -> Self {
        Self::default()
    }

    /// Optionnel: budget d’instructions (anti-boucle infinie)
    pub fn set_max_steps(&mut self, n: Option<u64>) {
        self.max_steps = n;
    }

    pub fn reset(&mut self) {
        self.stack.clear();
        self.locals.clear();
    }

    #[inline]
    fn truthy(v: &Value) -> bool {
        match v {
            Value::Null => false,
            Value::Bool(b) => *b,
            Value::I64(i) => *i != 0,
            Value::F64(x) => *x != 0.0,
            Value::Str(s) => !s.is_empty(),
            Value::Bytes(b) => !b.is_empty(),
        }
    }

    #[inline]
    fn ensure_local(&mut self, ix: usize) {
        if ix >= self.locals.len() {
            self.locals.resize(ix + 1, Value::Null);
        }
    }

    #[inline]
    fn pop(&mut self, op: &'static str) -> Result<Value, VmError> {
        self.stack.pop().ok_or(VmError::StackUnderflow(op))
    }

    #[inline]
    fn bin_num<F64, I64>(&mut self, op_name: &'static str, f_f64: F64, f_i64: I64) -> Result<(), VmError>
    where
        F64: FnOnce(f64, f64) -> Result<f64, VmError>,
        I64: FnOnce(i64, i64) -> Result<i64, VmError>,
    {
        let b = self.pop(op_name)?;
        let a = self.pop(op_name)?;
        match (a, b) {
            (Value::I64(x), Value::I64(y)) => {
                let r = f_i64(x, y)?;
                self.stack.push(Value::I64(r));
                Ok(())
            }
            (Value::I64(x), Value::F64(y)) => {
                let r = f_f64(x as f64, y)?;
                self.stack.push(Value::F64(r));
                Ok(())
            }
            (Value::F64(x), Value::I64(y)) => {
                let r = f_f64(x, y as f64)?;
                self.stack.push(Value::F64(r));
                Ok(())
            }
            (Value::F64(x), Value::F64(y)) => {
                let r = f_f64(x, y)?;
                self.stack.push(Value::F64(r));
                Ok(())
            }
            (va, vb) => Err(VmError::Type(format!("{} expects numbers, got {va:?} and {vb:?}", op_name))),
        }
    }

    #[inline]
    fn cmp_num<F64, I64>(&mut self, op_name: &'static str, f_f64: F64, f_i64: I64) -> Result<(), VmError>
    where
        F64: FnOnce(f64, f64) -> bool,
        I64: FnOnce(i64, i64) -> bool,
    {
        let b = self.pop(op_name)?;
        let a = self.pop(op_name)?;
        let res = match (a, b) {
            (Value::I64(x), Value::I64(y)) => f_i64(x, y),
            (Value::I64(x), Value::F64(y)) => f_f64(x as f64, y),
            (Value::F64(x), Value::I64(y)) => f_f64(x, y as f64),
            (Value::F64(x), Value::F64(y)) => f_f64(x, y),
            (va, vb) => return Err(VmError::Type(format!("{} expects numbers, got {va:?} and {vb:?}", op_name))),
        };
        self.stack.push(Value::Bool(res));
        Ok(())
    }

    fn const_get<'a>(pool: &'a ConstPool, ix: u32) -> Option<&'a ConstValue> {
        // API neutre: on se rabat sur .iter() pour rester compatible.
        for (i, c) in pool.iter() {
            if i == ix { return Some(c); }
        }
        None
    }

    pub fn run(&mut self, chunk: &Chunk) -> Result<(), VmError> {
        let ops = &chunk.ops;
        let mut pc: i64 = 0;
        let end = ops.len() as i64;

        let mut steps: u64 = 0;
        while pc >= 0 && pc < end {
            if let Some(limit) = self.max_steps {
                if steps >= limit {
                    return Err(VmError::Unsupported("max_steps reached"));
                }
            }
            steps += 1;

            let op = ops[pc as usize];
            // par défaut: avance d’une instruction
            let mut jumped = false;

            match op {
                Op::Nop => {}
                Op::Return | Op::ReturnVoid => {
                    // Dans un futur: on pourrait renvoyer la valeur du top pour Return.
                    return Ok(());
                }

                // Constantes & littéraux
                Op::LoadConst(ix) => {
                    let c = Self::const_get(&chunk.consts, ix).ok_or(VmError::ConstOutOfRange(ix))?;
                    self.stack.push(Value::from(c));
                }
                Op::LoadTrue => self.stack.push(Value::Bool(true)),
                Op::LoadFalse => self.stack.push(Value::Bool(false)),
                Op::LoadNull => self.stack.push(Value::Null),

                // Locals
                Op::LoadLocal(ix) => {
                    let i = ix as usize;
                    self.ensure_local(i);
                    self.stack.push(self.locals[i].clone());
                }
                Op::StoreLocal(ix) => {
                    let v = self.pop("StoreLocal")?;
                    let i = ix as usize;
                    self.ensure_local(i);
                    self.locals[i] = v;
                }

                // Arith
                Op::Add => self.bin_num("Add", |a, b| Ok(a + b), |a, b| Ok(a + b))?,
                Op::Sub => self.bin_num("Sub", |a, b| Ok(a - b), |a, b| Ok(a - b))?,
                Op::Mul => self.bin_num("Mul", |a, b| Ok(a * b), |a, b| Ok(a * b))?,
                Op::Div => self.bin_num("Div", |a, b| {
                    if b == 0.0 { return Err(VmError::DivByZero); }
                    Ok(a / b)
                }, |a, b| {
                    if b == 0 { return Err(VmError::DivByZero); }
                    // division entière “euclidienne” simple
                    Ok(a / b)
                })?,
                Op::Mod => self.bin_num("Mod", |a, b| {
                    if b == 0.0 { return Err(VmError::DivByZero); }
                    Ok(a % b)
                }, |a, b| {
                    if b == 0 { return Err(VmError::DivByZero); }
                    Ok(a % b)
                })?,
                Op::Neg => {
                    let v = self.pop("Neg")?;
                    match v {
                        Value::I64(x) => self.stack.push(Value::I64(-x)),
                        Value::F64(x) => self.stack.push(Value::F64(-x)),
                        other => return Err(VmError::Type(format!("Neg expects number, got {other:?}"))),
                    }
                }
                Op::Not => {
                    let v = self.pop("Not")?;
                    self.stack.push(Value::Bool(!Self::truthy(&v)));
                }

                // Comparaisons
                Op::Eq => {
                    let b = self.pop("Eq")?;
                    let a = self.pop("Eq")?;
                    // numéricité tolérante
                    let res = match (a, b) {
                        (Value::I64(x), Value::I64(y)) => x == y,
                        (Value::F64(x), Value::F64(y)) => x == y,
                        (Value::I64(x), Value::F64(y)) => (x as f64) == y,
                        (Value::F64(x), Value::I64(y)) => x == (y as f64),
                        (Value::Null, Value::Null) => true,
                        (Value::Bool(x), Value::Bool(y)) => x == y,
                        (Value::Str(x), Value::Str(y)) => x == y,
                        (Value::Bytes(x), Value::Bytes(y)) => x == y,
                        _ => false,
                    };
                    self.stack.push(Value::Bool(res));
                }
                Op::Ne => {
                    let b = self.pop("Ne")?;
                    let a = self.pop("Ne")?;
                    // utilise Eq logique précédente
                    let res = match (a, b) {
                        (Value::I64(x), Value::I64(y)) => x != y,
                        (Value::F64(x), Value::F64(y)) => x != y,
                        (Value::I64(x), Value::F64(y)) => (x as f64) != y,
                        (Value::F64(x), Value::I64(y)) => x != (y as f64),
                        (Value::Null, Value::Null) => false,
                        (Value::Bool(x), Value::Bool(y)) => x != y,
                        (Value::Str(x), Value::Str(y)) => x != y,
                        (Value::Bytes(x), Value::Bytes(y)) => x != y,
                        _ => true,
                    };
                    self.stack.push(Value::Bool(res));
                }
                Op::Lt => self.cmp_num("Lt", |a, b| a < b, |a, b| a < b)?,
                Op::Le => self.cmp_num("Le", |a, b| a <= b, |a, b| a <= b)?,
                Op::Gt => self.cmp_num("Gt", |a, b| a > b, |a, b| a > b)?,
                Op::Ge => self.cmp_num("Ge", |a, b| a >= b, |a, b| a >= b)?,

                // Contrôle
                Op::Jump(off) => {
                    let target = pc + 1 + (off as i64);
                    if target < 0 || target >= end { return Err(VmError::JumpOutOfRange(target)); }
                    pc = target;
                    jumped = true;
                }
                Op::JumpIfFalse(off) => {
                    let cond = self.pop("JumpIfFalse")?;
                    if !Self::truthy(&cond) {
                        let target = pc + 1 + (off as i64);
                        if target < 0 || target >= end { return Err(VmError::JumpOutOfRange(target)); }
                        pc = target;
                        jumped = true;
                    }
                }
                Op::Pop => { let _ = self.pop("Pop")?; }

                // Appels / fermetures (non impl)
                Op::Call(_argc)      => return Err(VmError::Unsupported("Call")),
                Op::TailCall(_argc)  => return Err(VmError::Unsupported("TailCall")),
                Op::MakeClosure(_,_) => return Err(VmError::Unsupported("MakeClosure")),
                Op::LoadUpvalue(_)   => return Err(VmError::Unsupported("LoadUpvalue")),
                Op::StoreUpvalue(_)  => return Err(VmError::Unsupported("StoreUpvalue")),

                // I/O debug
                Op::Print => {
                    let v = self.pop("Print")?;
                    println!("{v}");
                }
            }

            if !jumped {
                pc += 1;
            }
        }

        Ok(())
    }
}
