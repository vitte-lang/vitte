//! vitte-consteval — compile-time constant evaluation for Vitte
//!
//! Scope
//! - Deterministic const-eval for a pure expression subset.
//! - Constant folding for a minimal SSA IR (scaffold).
//! - Intrinsics: arithmetic, bit ops, comparisons, selects, popcount/ctlz/cttz,
//!   min/max/abs, and a few float ops when `std` is enabled.
//!
//! Design
//! - `Value` is the canonical constant domain.
//! - `Cx` holds evaluation limits and an environment for named consts.
//! - `eval_const_expr` reduces an expression tree to `Value`.
//! - `fold_ir` performs forward lattice propagation to fold SSA instructions.
//!
//! Safety
//! - No I/O, no clock, no randomness. UB-guarded: div-by-zero, shift ranges.
//!
//! Features
//! - `fold` (on by default): SSA folder.
//! - `interpreter`: small expression evaluator (on by default via `default` in Cargo).
//! - `miri-like`: extra UB checks (shift, overflow flags [does not panic], NaN flags).
//! - `wasm-safe`: avoids `libm`/`std` float intrinsics when not available.

#![cfg_attr(not(feature = "std"), no_std)]
#![forbid(unsafe_op_in_unsafe_fn)]
#![deny(missing_docs)]

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{boxed::Box, format, string::String, vec, vec::Vec};
#[cfg(feature = "std")]
use std::{boxed::Box, format, string::String, vec, vec::Vec};

use core::cmp::Ordering;

#[cfg(not(feature = "std"))]
use alloc::collections::BTreeMap as Map;
#[cfg(feature = "std")]
use std::collections::BTreeMap as Map;

/// Result alias.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Consteval error kinds.
#[derive(Debug, Clone, PartialEq)]
pub enum Error {
    /// Generic invalid input or domain error.
    Invalid(String),
    /// Unsupported feature or node.
    Unsupported(String),
    /// UB-like condition detected (guarded).
    UB(String),
    /// Step or recursion limit exceeded.
    Limit(String),
}

/// Canonical constant domain.
#[derive(Debug, Clone, PartialEq)]
pub enum Value {
    /// 1-bit boolean.
    Bool(bool),
    /// 64-bit signed integer.
    I64(i64),
    /// 64-bit unsigned integer.
    U64(u64),
    /// 64-bit float (IEEE-754).
    F64(f64),
    /// UTF-8 string (const).
    Str(String),
    /// Homogeneous fixed array.
    Array(Vec<Value>),
    /// Tuple.
    Tuple(Vec<Value>),
    /// Unit.
    Unit,
}

impl Value {
    /// Truthiness for conditionals. Only `Bool` is allowed.
    #[inline]
    pub fn as_bool(&self) -> Result<bool> {
        match self {
            Value::Bool(b) => Ok(*b),
            _ => Err(Error::Invalid("expected bool".into())),
        }
    }
    #[inline]
    pub fn as_i64(&self) -> Result<i64> {
        match self {
            Value::I64(v) => Ok(*v),
            _ => Err(Error::Invalid("expected i64".into())),
        }
    }
    #[inline]
    pub fn as_u64(&self) -> Result<u64> {
        match self {
            Value::U64(v) => Ok(*v),
            _ => Err(Error::Invalid("expected u64".into())),
        }
    }
    #[inline]
    pub fn as_f64(&self) -> Result<f64> {
        match self {
            Value::F64(v) => Ok(*v),
            _ => Err(Error::Invalid("expected f64".into())),
        }
    }
}

/// Evaluation limits and environment.
#[derive(Debug, Clone)]
pub struct Limits {
    /// Max AST nodes evaluated.
    pub max_steps: u64,
    /// Max recursion depth.
    pub max_depth: u32,
}
impl Default for Limits {
    fn default() -> Self {
        Self { max_steps: 100_000, max_depth: 256 }
    }
}

/// Evaluation context.
#[derive(Debug, Default)]
pub struct Cx {
    steps: u64,
    depth: u32,
    limits: Limits,
    env: Map<String, Value>,
}

impl Cx {
    /// Create.
    pub fn new() -> Self { Self { limits: Limits::default(), ..Default::default() } }
    /// With limits.
    pub fn with_limits(limits: Limits) -> Self { Self { limits, ..Default::default() } }
    /// Bind a named constant.
    pub fn define_const<S: Into<String>>(&mut self, name: S, val: Value) { self.env.insert(name.into(), val); }
    fn charge(&mut self) -> Result<()> {
        self.steps = self.steps.checked_add(1).ok_or_else(|| Error::Limit("steps overflow".into()))?;
        if self.steps > self.limits.max_steps { return Err(Error::Limit("max steps".into())); }
        Ok(())
    }
    fn enter(&mut self) -> Result<()> {
        if self.depth >= self.limits.max_depth { return Err(Error::Limit("max depth".into())); }
        self.depth += 1;
        Ok(())
    }
    fn leave(&mut self) { self.depth = self.depth.saturating_sub(1); }
    fn get(&self, name: &str) -> Option<&Value> { self.env.get(name) }
}

/* ============================ Interpreter AST ============================ */

/// Unary operators.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum UnOp { Not, NegI, NegF }

/// Binary operators.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum BinOp {
    AddI, SubI, MulI, DivI, RemI,
    AddU, SubU, MulU, DivU, RemU,
    AddF, SubF, MulF, DivF,
    Shl, LShr, AShr,
    And, Or, Xor,
    CmpEq, CmpNe, CmpLt, CmpLe, CmpGt, CmpGe,
}

/// Intrinsic identifiers.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum Intrinsic {
    /// abs(x: i64)->i64
    AbsI64,
    /// min/max on i64
    MinI64, MaxI64,
    /// popcount/ctlz/cttz on u64
    CtpopU64, CtlzU64, CttzU64,
    /// float ops (enabled when available)
    #[cfg(feature = "std")]
    SqrtF64,
    #[cfg(feature = "std")]
    SinF64,
    #[cfg(feature = "std")]
    CosF64,
}

/// Expression tree.
#[derive(Debug, Clone, PartialEq)]
pub enum Expr {
    /// Literal.
    Lit(Value),
    /// Variable reference.
    Var(String),
    /// Let-binding.
    Let { name: String, rhs: Box<Expr>, body: Box<Expr> },
    /// If-then-else.
    If { cond: Box<Expr>, then_: Box<Expr>, else_: Box<Expr> },
    /// Unary op.
    Un { op: UnOp, arg: Box<Expr> },
    /// Binary op.
    Bin { op: BinOp, l: Box<Expr>, r: Box<Expr> },
    /// Tuple constructor.
    Tup(Vec<Expr>),
    /// Array constructor.
    Arr(Vec<Expr>),
    /// Indexing (array or tuple).
    Index { base: Box<Expr>, idx: usize },
    /// Intrinsic call with positional args.
    Intrin { which: Intrinsic, args: Vec<Expr> },
}

/// Evaluate a pure expression to a constant `Value`.
#[cfg(feature = "interpreter")]
pub fn eval_const_expr(cx: &mut Cx, e: &Expr) -> Result<Value> {
    cx.enter()?;
    let _scope_guard = ScopeGuard(&mut cx.depth);
    cx.charge()?;
    match e {
        Expr::Lit(v) => Ok(v.clone()),
        Expr::Var(name) => cx.get(name).cloned().ok_or_else(|| Error::Invalid(format!("unknown const `{name}`"))),
        Expr::Let { name, rhs, body } => {
            let v = eval_const_expr(cx, rhs)?;
            let old = cx.env.insert(name.clone(), v);
            let out = eval_const_expr(cx, body)?;
            if let Some(prev) = old { cx.env.insert(name.clone(), prev); } else { cx.env.remove(name); }
            Ok(out)
        }
        Expr::If { cond, then_, else_ } => {
            let c = eval_const_expr(cx, cond)?.as_bool()?;
            if c { eval_const_expr(cx, then_) } else { eval_const_expr(cx, else_) }
        }
        Expr::Un { op, arg } => {
            let v = eval_const_expr(cx, arg)?;
            match (op, v) {
                (UnOp::Not, Value::Bool(b)) => Ok(Value::Bool(!b)),
                (UnOp::NegI, Value::I64(x)) => Ok(Value::I64(x.wrapping_neg())),
                (UnOp::NegF, Value::F64(f)) => Ok(Value::F64(-f)),
                _ => Err(Error::Invalid("unop type mismatch".into())),
            }
        }
        Expr::Bin { op, l, r } => {
            let a = eval_const_expr(cx, l)?;
            let b = eval_const_expr(cx, r)?;
            eval_bin(*op, a, b)
        }
        Expr::Tup(xs) => {
            let mut out = Vec::with_capacity(xs.len());
            for x in xs { out.push(eval_const_expr(cx, x)?); }
            Ok(Value::Tuple(out))
        }
        Expr::Arr(xs) => {
            let mut out = Vec::with_capacity(xs.len());
            for x in xs { out.push(eval_const_expr(cx, x)?); }
            Ok(Value::Array(out))
        }
        Expr::Index { base, idx } => {
            let b = eval_const_expr(cx, base)?;
            match b {
                Value::Array(v) => v.get(*idx).cloned().ok_or_else(|| Error::Invalid("index oob".into())),
                Value::Tuple(v) => v.get(*idx).cloned().ok_or_else(|| Error::Invalid("index oob".into())),
                _ => Err(Error::Invalid("index on non-aggregate".into())),
            }
        }
        Expr::Intrin { which, args } => eval_intrinsic(cx, *which, args),
    }
}

#[cfg(feature = "interpreter")]
fn eval_bin(op: BinOp, a: Value, b: Value) -> Result<Value> {
    use BinOp::*;
    match op {
        AddI => Ok(Value::I64(a.as_i64()? .wrapping_add(b.as_i64()?))),
        SubI => Ok(Value::I64(a.as_i64()? .wrapping_sub(b.as_i64()?))),
        MulI => Ok(Value::I64(a.as_i64()? .wrapping_mul(b.as_i64()?))),
        DivI => {
            let x = a.as_i64()?;
            let y = b.as_i64()?;
            if y == 0 { return Err(Error::UB("i64 div by zero".into())); }
            #[cfg(feature = "miri-like")]
            if x == i64::MIN && y == -1 { return Err(Error::UB("i64 overflow on div".into())); }
            Ok(Value::I64(x / y))
        }
        RemI => {
            let y = b.as_i64()?;
            if y == 0 { return Err(Error::UB("i64 rem by zero".into())); }
            Ok(Value::I64(a.as_i64()? % y))
        }
        AddU => Ok(Value::U64(a.as_u64()? .wrapping_add(b.as_u64()?))),
        SubU => Ok(Value::U64(a.as_u64()? .wrapping_sub(b.as_u64()?))),
        MulU => Ok(Value::U64(a.as_u64()? .wrapping_mul(b.as_u64()?))),
        DivU => {
            let y = b.as_u64()?;
            if y == 0 { return Err(Error::UB("u64 div by zero".into())); }
            Ok(Value::U64(a.as_u64()? / y))
        }
        RemU => {
            let y = b.as_u64()?;
            if y == 0 { return Err(Error::UB("u64 rem by zero".into())); }
            Ok(Value::U64(a.as_u64()? % y))
        }
        AddF => Ok(Value::F64(a.as_f64()? + b.as_f64()?)),
        SubF => Ok(Value::F64(a.as_f64()? - b.as_f64()?)),
        MulF => Ok(Value::F64(a.as_f64()? * b.as_f64()?)),
        DivF => {
            let y = b.as_f64()?;
            #[cfg(feature = "miri-like")]
            if y == 0.0 { return Err(Error::UB("f64 div by zero".into())); }
            Ok(Value::F64(a.as_f64()? / y))
        }
        Shl => {
            let x = a.as_u64()?;
            let s = b.as_u64()?;
            if s >= 64 { return Err(Error::UB("shift >= 64".into())); }
            Ok(Value::U64(x.wrapping_shl(s as u32)))
        }
        LShr => {
            let x = a.as_u64()?;
            let s = b.as_u64()?;
            if s >= 64 { return Err(Error::UB("shift >= 64".into())); }
            Ok(Value::U64(x.wrapping_shr(s as u32)))
        }
        AShr => {
            let x = a.as_i64()?;
            let s = b.as_u64()?;
            if s >= 64 { return Err(Error::UB("shift >= 64".into())); }
            Ok(Value::I64((x as i64) >> (s as u32)))
        }
        And => {
            match (a, b) {
                (Value::U64(x), Value::U64(y)) => Ok(Value::U64(x & y)),
                (Value::Bool(x), Value::Bool(y)) => Ok(Value::Bool(x & y)),
                _ => Err(Error::Invalid("and types".into())),
            }
        }
        Or => {
            match (a, b) {
                (Value::U64(x), Value::U64(y)) => Ok(Value::U64(x | y)),
                (Value::Bool(x), Value::Bool(y)) => Ok(Value::Bool(x | y)),
                _ => Err(Error::Invalid("or types".into())),
            }
        }
        Xor => {
            match (a, b) {
                (Value::U64(x), Value::U64(y)) => Ok(Value::U64(x ^ y)),
                (Value::Bool(x), Value::Bool(y)) => Ok(Value::Bool(x ^ y)),
                _ => Err(Error::Invalid("xor types".into())),
            }
        }
        CmpEq | CmpNe | CmpLt | CmpLe | CmpGt | CmpGe => cmp_bin(op, a, b),
    }
}

#[cfg(feature = "interpreter")]
fn cmp_bin(op: BinOp, a: Value, b: Value) -> Result<Value> {
    use BinOp::*;
    let boolv = |b| Ok(Value::Bool(b));
    match (a, b) {
        (Value::I64(x), Value::I64(y)) => {
            let ord = x.cmp(&y);
            boolv(match op {
                CmpEq => ord == Ordering::Equal,
                CmpNe => ord != Ordering::Equal,
                CmpLt => ord == Ordering::Less,
                CmpLe => ord != Ordering::Greater,
                CmpGt => ord == Ordering::Greater,
                CmpGe => ord != Ordering::Less,
                _ => return Err(Error::Invalid("cmp op".into())),
            })
        }
        (Value::U64(x), Value::U64(y)) => {
            let ord = x.cmp(&y);
            boolv(match op {
                CmpEq => ord == Ordering::Equal,
                CmpNe => ord != Ordering::Equal,
                CmpLt => ord == Ordering::Less,
                CmpLe => ord != Ordering::Greater,
                CmpGt => ord == Ordering::Greater,
                CmpGe => ord != Ordering::Less,
                _ => return Err(Error::Invalid("cmp op".into())),
            })
        }
        (Value::F64(x), Value::F64(y)) => {
            #[cfg(feature = "miri-like")]
            if x.is_nan() || y.is_nan() { return Err(Error::UB("f64 cmp with NaN".into())); }
            let ord = x.partial_cmp(&y).ok_or_else(|| Error::Invalid("f64 unordered compare".into()))?;
            boolv(match op {
                CmpEq => ord == Ordering::Equal,
                CmpNe => ord != Ordering::Equal,
                CmpLt => ord == Ordering::Less,
                CmpLe => ord != Ordering::Greater,
                CmpGt => ord == Ordering::Greater,
                CmpGe => ord != Ordering::Less,
                _ => return Err(Error::Invalid("cmp op".into())),
            })
        }
        (Value::Bool(x), Value::Bool(y)) => {
            let ord = (x as u8).cmp(&(y as u8));
            boolv(match op {
                CmpEq => ord == Ordering::Equal,
                CmpNe => ord != Ordering::Equal,
                CmpLt => ord == Ordering::Less,
                CmpLe => ord != Ordering::Greater,
                CmpGt => ord == Ordering::Greater,
                CmpGe => ord != Ordering::Less,
                _ => return Err(Error::Invalid("cmp op".into())),
            })
        }
        _ => Err(Error::Invalid("cmp type mismatch".into())),
    }
}

struct ScopeGuard<'a>(&'a mut u32);
impl Drop for ScopeGuard<'_> {
    fn drop(&mut self) { *self.0 = self.0.saturating_sub(1); }
}

#[cfg(feature = "interpreter")]
fn eval_intrinsic(cx: &mut Cx, which: Intrinsic, args: &[Expr]) -> Result<Value> {
    cx.charge()?;
    let mut eval1 = |i: usize| -> Result<Value> { eval_const_expr(cx, &args[i]) };
    Ok(match which {
        Intrinsic::AbsI64 => {
            let x = eval1(0)?.as_i64()?;
            Value::I64(x.wrapping_abs())
        }
        Intrinsic::MinI64 => {
            let a = eval1(0)?.as_i64()?;
            let b = eval1(1)?.as_i64()?;
            Value::I64(core::cmp::min(a, b))
        }
        Intrinsic::MaxI64 => {
            let a = eval1(0)?.as_i64()?;
            let b = eval1(1)?.as_i64()?;
            Value::I64(core::cmp::max(a, b))
        }
        Intrinsic::CtpopU64 => {
            let x = eval1(0)?.as_u64()?;
            Value::U64(x.count_ones() as u64)
        }
        Intrinsic::CtlzU64 => {
            let x = eval1(0)?.as_u64()?;
            Value::U64(x.leading_zeros() as u64)
        }
        Intrinsic::CttzU64 => {
            let x = eval1(0)?.as_u64()?;
            Value::U64(x.trailing_zeros() as u64)
        }
        #[cfg(feature = "std")]
        Intrinsic::SqrtF64 => {
            let x = eval1(0)?.as_f64()?;
            #[cfg(feature = "miri-like")]
            if x < 0.0 { return Err(Error::UB("sqrt of negative".into())); }
            Value::F64(x.sqrt())
        }
        #[cfg(feature = "std")]
        Intrinsic::SinF64 => {
            let x = eval1(0)?.as_f64()?;
            Value::F64(x.sin())
        }
        #[cfg(feature = "std")]
        Intrinsic::CosF64 => {
            let x = eval1(0)?.as_f64()?;
            Value::F64(x.cos())
        }
    })
}

/* ============================== SSA IR Fold ============================== */

/// SSA value id.
#[cfg(feature = "fold")]
#[derive(Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct V(u32);

/// Minimal SSA instruction set.
#[cfg(feature = "fold")]
#[derive(Debug, Clone, PartialEq)]
pub enum Inst {
    /// v = const k
    Const { dst: V, k: Value },
    /// v = binop a, b
    Bin { dst: V, op: BinOp, a: V, b: V },
    /// v = unop a
    Un { dst: V, op: UnOp, a: V },
    /// v = select c, t, f
    Select { dst: V, c: V, t: V, f: V },
    /// dead anchor to keep side-effect-free value alive in tests
    Keep(V),
    /// return v
    Ret(V),
}

/// Function is a flat list for simplicity.
#[cfg(feature = "fold")]
#[derive(Debug, Clone, Default)]
pub struct Func {
    /// Body.
    pub body: Vec<Inst>,
}

#[cfg(feature = "fold")]
impl Func {
    /// Allocate a new id (monotonic). Caller keeps the counter.
    pub fn next_id(&self) -> V { V(self.body.len() as u32 + 1) }
}

/// Folding result: mapping of constants and rewritten body.
#[cfg(feature = "fold")]
#[derive(Debug, Default)]
pub struct FoldResult {
    /// Value lattice: Some(const) or None = unknown.
    pub const_of: Map<V, Value>,
    /// New body with folded ops.
    pub body: Vec<Inst>,
}

/// Fold a function in place and return a folded copy plus lattice map.
#[cfg(feature = "fold")]
pub fn fold_ir(f: &Func) -> FoldResult {
    use Inst::*;
    let mut const_of: Map<V, Value> = Map::new();
    let mut out: Vec<Inst> = Vec::with_capacity(f.body.len());

    for insn in &f.body {
        match insn {
            Const { dst, k } => {
                const_of.insert(*dst, k.clone());
                out.push(insn.clone());
            }
            Un { dst, op, a } => {
                if let Some(va) = const_of.get(a).cloned() {
                    // interpret
                    let folded = match (op, va) {
                        (UnOp::Not, Value::Bool(b)) => Some(Value::Bool(!b)),
                        (UnOp::NegI, Value::I64(x)) => Some(Value::I64(x.wrapping_neg())),
                        (UnOp::NegF, Value::F64(x)) => Some(Value::F64(-x)),
                        _ => None,
                    };
                    if let Some(v) = folded {
                        const_of.insert(*dst, v.clone());
                        out.push(Const { dst: *dst, k: v });
                        continue;
                    }
                }
                out.push(insn.clone());
            }
            Bin { dst, op, a, b } => {
                let av = const_of.get(a).cloned();
                let bv = const_of.get(b).cloned();
                if let (Some(a), Some(b)) = (av, bv) {
                    if let Ok(v) = eval_bin(*op, a.clone(), b.clone()) {
                        const_of.insert(*dst, v.clone());
                        out.push(Const { dst: *dst, k: v });
                        continue;
                    }
                }
                // algebraic identities
                match (op, av.as_ref(), bv.as_ref()) {
                    (BinOp::AddI, Some(Value::I64(0)), _) => { const_of.remove(dst); out.push(Bin { dst:*dst, op:*op, a:*b, b:*a }); }
                    (BinOp::AddI, _, Some(Value::I64(0))) => { const_of.insert(*dst, av.unwrap().clone()); out.push(Const { dst:*dst, k: const_of.get(a).unwrap().clone() }); }
                    (BinOp::AddU, Some(Value::U64(0)), _) => { const_of.remove(dst); out.push(Bin { dst:*dst, op:*op, a:*b, b:*a }); }
                    (BinOp::AddU, _, Some(Value::U64(0))) => { if let Some(v) = av { const_of.insert(*dst, v.clone()); out.push(Const{dst:*dst,k:v}); } else { out.push(insn.clone()); } }
                    (BinOp::MulI, Some(Value::I64(1)), _) | (BinOp::MulU, Some(Value::U64(1)), _) => { const_of.remove(dst); out.push(Bin { dst:*dst, op:*op, a:*b, b:*a }); }
                    (BinOp::MulI, _, Some(Value::I64(1))) | (BinOp::MulU, _, Some(Value::U64(1))) => { if let Some(v) = av { const_of.insert(*dst, v.clone()); out.push(Const{dst:*dst,k:v}); } else { out.push(insn.clone()); } }
                    (BinOp::MulI, Some(Value::I64(0)), _) | (BinOp::MulI, _, Some(Value::I64(0))) => { const_of.insert(*dst, Value::I64(0)); out.push(Const{dst:*dst, k:Value::I64(0)}); }
                    (BinOp::MulU, Some(Value::U64(0)), _) | (BinOp::MulU, _, Some(Value::U64(0))) => { const_of.insert(*dst, Value::U64(0)); out.push(Const{dst:*dst, k:Value::U64(0)}); }
                    _ => out.push(insn.clone()),
                }
            }
            Select { dst, c, t, f: e } => {
                match const_of.get(c) {
                    Some(Value::Bool(true)) => {
                        if let Some(tv) = const_of.get(t).cloned() {
                            const_of.insert(*dst, tv.clone());
                            out.push(Const { dst: *dst, k: tv });
                        } else {
                            // replace by move t
                            out.push(Keep(*t));
                            const_of.remove(dst);
                        }
                    }
                    Some(Value::Bool(false)) => {
                        if let Some(ev) = const_of.get(e).cloned() {
                            const_of.insert(*dst, ev.clone());
                            out.push(Const { dst: *dst, k: ev });
                        } else {
                            out.push(Keep(*e));
                            const_of.remove(dst);
                        }
                    }
                    _ => {
                        out.push(insn.clone());
                    }
                }
            }
            Keep(v) => { let _ = v; out.push(insn.clone()); }
            Ret(v) => {
                if let Some(k) = const_of.get(v).cloned() {
                    out.push(Const { dst: *v, k: k.clone() });
                }
                out.push(Ret(*v));
            }
        }
    }

    FoldResult { const_of, body: out }
}

/* ================================= Tests ================================ */

#[cfg(any(test, feature = "std"))]
mod tests {
    use super::*;

    #[cfg(feature = "interpreter")]
    #[test]
    fn eval_if_minmax() {
        let mut cx = Cx::new();
        cx.define_const("a", Value::I64(7));
        cx.define_const("b", Value::I64(2));
        let e = Expr::If {
            cond: Box::new(Expr::Bin { op: BinOp::CmpGt, l: Box::new(Expr::Var("a".into())), r: Box::new(Expr::Var("b".into())) }),
            then_: Box::new(Expr::Intrin { which: Intrinsic::MinI64, args: vec![Expr::Var("a".into()), Expr::Var("b".into())]}),
            else_: Box::new(Expr::Intrin { which: Intrinsic::MaxI64, args: vec![Expr::Var("a".into()), Expr::Var("b".into())]}),
        };
        let v = eval_const_expr(&mut cx, &e).unwrap();
        assert_eq!(v, Value::I64(2));
    }

    #[cfg(feature = "fold")]
    #[test]
    fn fold_linear() {
        use BinOp::*;
        use Inst::*;
        let v0 = V(0); let v1 = V(1); let v2 = V(2); let v3 = V(3);
        let f = Func {
            body: vec![
                Const { dst: v0, k: Value::I64(40) },
                Const { dst: v1, k: Value::I64(2) },
                Bin { dst: v2, op: AddI, a: v0, b: v1 }, // 42
                Ret(v2),
            ],
        };
        let fr = fold_ir(&f);
        let last = fr.body.last().unwrap();
        match last { Ret(v) => assert_eq!(*v, v2), _ => panic!("ret expected") }
        // Ensure constant propagated
        assert_eq!(fr.const_of.get(&v2), Some(&Value::I64(42)));
    }

    #[cfg(all(feature = "interpreter", feature = "std"))]
    #[test]
    fn eval_float() {
        let mut cx = Cx::new();
        let e = Expr::Intrin { which: Intrinsic::SqrtF64, args: vec![Expr::Lit(Value::F64(9.0))] };
        let v = eval_const_expr(&mut cx, &e).unwrap();
        assert!(matches!(v, Value::F64(f) if (f-3.0).abs() < 1e-12));
    }

    #[cfg(feature = "interpreter")]
    #[test]
    fn guard_div_zero() {
        let mut cx = Cx::new();
        let e = Expr::Bin { op: BinOp::DivI, l: Box::new(Expr::Lit(Value::I64(1))), r: Box::new(Expr::Lit(Value::I64(0))) };
        let err = eval_const_expr(&mut cx, &e).unwrap_err();
        matches!(err, Error::UB(_));
    }
}