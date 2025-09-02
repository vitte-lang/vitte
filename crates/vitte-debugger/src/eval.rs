//! eval.rs — Évaluateur d’expressions pour débogage & logpoints
//!
//! - Grammaire (précédence standard) :
//!     expr        := or
//!     or          := and ("||" and)*
//!     and         := equality ("&&" equality)*
//!     equality    := comparison (("==" | "!=") comparison)*
//!     comparison  := term (("<" | "<=" | ">" | ">=") term)*
//!     term        := factor (("+" | "-") factor)*
//!     factor      := unary (("*" | "/" | "%") unary)*
//!     unary       := ("!" | "+" | "-") unary | call
//!     call        := primary ( "(" args? ")" | "." ident | "[" expr "]" )*
//!     primary     := IDENT | NUMBER | STRING | "true" | "false" | "null" | "(" expr ")"
//!     args        := expr ("," expr)*
//!
//! - Variables depuis un environnement : `EvalEnv`
//! - Types supportés : Null, Bool, Int(i64), Float(f64), Str, Array, Map
//! - Appels de fonctions : délégués à l’environnement
//! - Interpolation de logpoints : `format_log_message("x={x} s={a+b}", &env)`
//!
//! Intégration typique :
//!     struct VmEnv<'a> { /* refs vers tes frames/vars */ }
//!     impl<'a> EvalEnv for VmEnv<'a> { /* get_var, get_field, index, call */ }
//!     let val = eval_expr("a+b*2", &env)?;
//!
//! Sécurité : évaluation "read-only", pas d’effets de bord ici (sauf via `call` côté env).
//!
//! Dépendances (Cargo.toml) :
//!     color-eyre = "0.6"
//!     serde = { version = "1", features = ["derive"] }

use std::collections::HashMap;
use std::fmt;

use color_eyre::eyre::{eyre, Result};
use serde::{Deserialize, Serialize};

/* ----------------------------- Valeurs & Env ------------------------------ */

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
pub enum Value {
    Null,
    Bool(bool),
    Int(i64),
    Float(f64),
    Str(String),
    Array(Vec<Value>),
    Map(HashMap<String, Value>),
}

impl Value {
    pub fn as_bool_strict(&self) -> Result<bool> {
        match self {
            Value::Bool(b) => Ok(*b),
            _ => Err(eyre!("attendu bool, trouvé {}", self.type_name())),
        }
    }
    pub fn as_number(&self) -> Result<Number> {
        match self {
            Value::Int(i) => Ok(Number::Int(*i)),
            Value::Float(f) => Ok(Number::Float(*f)),
            _ => Err(eyre!("attendu nombre, trouvé {}", self.type_name())),
        }
    }
    pub fn truthy(&self) -> bool {
        match self {
            Value::Null => false,
            Value::Bool(b) => *b,
            Value::Int(i) => *i != 0,
            Value::Float(f) => *f != 0.0,
            Value::Str(s) => !s.is_empty(),
            Value::Array(v) => !v.is_empty(),
            Value::Map(m) => !m.is_empty(),
        }
    }
    pub fn type_name(&self) -> &'static str {
        match self {
            Value::Null => "null",
            Value::Bool(_) => "bool",
            Value::Int(_) => "int",
            Value::Float(_) => "float",
            Value::Str(_) => "string",
            Value::Array(_) => "array",
            Value::Map(_) => "map",
        }
    }
}

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Value::Null => write!(f, "null"),
            Value::Bool(b) => write!(f, "{}", b),
            Value::Int(i) => write!(f, "{}", i),
            Value::Float(x) => {
                // joli rendu sans superflus
                if x.fract() == 0.0 { write!(f, "{:.0}", x) } else { write!(f, "{}", x) }
            }
            Value::Str(s) => write!(f, "{}", s),
            Value::Array(v) => {
                write!(f, "[")?;
                for (i, el) in v.iter().enumerate() {
                    if i > 0 { write!(f, ", ")?; }
                    write!(f, "{}", el)?;
                }
                write!(f, "]")
            }
            Value::Map(m) => {
                write!(f, "{{")?;
                let mut it = m.iter().peekable();
                while let Some((k, v)) = it.next() {
                    write!(f, "{}: {}", k, v)?;
                    if it.peek().is_some() { write!(f, ", ")?; }
                }
                write!(f, "}}")
            }
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum Number {
    Int(i64),
    Float(f64),
}
impl Number {
    fn to_value(self) -> Value {
        match self {
            Number::Int(i) => Value::Int(i),
            Number::Float(f) => Value::Float(f),
        }
    }
}

/* Environnement d’éval : fourni par la VM / debugger */
pub trait EvalEnv {
    /// Cherche une variable par nom (dans les scopes visibles).
    fn get_var(&self, name: &str) -> Option<Value>;

    /// Accès champ : `base.field`
    fn get_field(&self, base: &Value, field: &str) -> Option<Value> {
        match base {
            Value::Map(m) => m.get(field).cloned(),
            _ => None,
        }
    }

    /// Indexation : `base[idx]` (array ou map)
    fn index(&self, base: &Value, idx: &Value) -> Option<Value> {
        match (base, idx) {
            (Value::Array(v), Value::Int(i)) if *i >= 0 => v.get(*i as usize).cloned(),
            (Value::Map(m), Value::Str(s)) => m.get(s).cloned(),
            _ => None,
        }
    }

    /// Appels de fonctions/fonctions natives : `fn(a,b)` (optionnel).
    fn call(&self, _func: &str, _args: &[Value]) -> Result<Value> {
        Err(eyre!("appel de fonction non supporté pour '{}'", _func))
    }
}

/* ------------------------------- Lexer ------------------------------------ */

#[derive(Debug, Clone, PartialEq)]
enum Tok {
    Ident(String),
    Str(String),
    Int(i64),
    Float(f64),
    True,
    False,
    Null,
    // opérateurs / ponctuation
    LParen, RParen, LBracket, RBracket, Dot, Comma,
    Plus, Minus, Star, Slash, Percent,
    Bang,
    EqEq, NotEq,
    Lt, Le, Gt, Ge,
    AndAnd, OrOr,
    Eof,
}

struct Lexer<'a> {
    s: &'a [u8],
    i: usize,
}
impl<'a> Lexer<'a> {
    fn new(src: &'a str) -> Self { Self { s: src.as_bytes(), i: 0 } }
    fn next_token(&mut self) -> Result<Tok> {
        self.skip_ws();
        if self.eof() { return Ok(Tok::Eof); }
        let c = self.peek();

        // Ident / keyword
        if is_ident_start(c) {
            let start = self.i;
            self.i += 1;
            while !self.eof() && is_ident_part(self.peek()) { self.i += 1; }
            let s = std::str::from_utf8(&self.s[start..self.i]).unwrap().to_string();
            return Ok(match s.as_str() {
                "true" => Tok::True,
                "false" => Tok::False,
                "null" => Tok::Null,
                _ => Tok::Ident(s),
            });
        }

        // Number
        if c.is_ascii_digit() || (c == b'.' && self.peek2().map(|d| d.is_ascii_digit()).unwrap_or(false)) {
            return self.lex_number();
        }

        // String (double quotes)
        if c == b'"' {
            return self.lex_string();
        }

        // Two-char ops
        if let Some(op) = self.try_two_char_op() {
            return Ok(op);
        }

        // Single-char
        self.i += 1;
        Ok(match c {
            b'(' => Tok::LParen,
            b')' => Tok::RParen,
            b'[' => Tok::LBracket,
            b']' => Tok::RBracket,
            b'.' => Tok::Dot,
            b',' => Tok::Comma,
            b'+' => Tok::Plus,
            b'-' => Tok::Minus,
            b'*' => Tok::Star,
            b'/' => Tok::Slash,
            b'%' => Tok::Percent,
            b'!' => Tok::Bang,
            b'<' => Tok::Lt,
            b'>' => Tok::Gt,
            _ => return Err(eyre!("caractère inattendu: {}", c as char)),
        })
    }
    fn skip_ws(&mut self) {
        while !self.eof() {
            let c = self.peek();
            if c == b' ' || c == b'\t' || c == b'\n' || c == b'\r' {
                self.i += 1;
            } else if c == b'/' && self.peek2() == Some(b'/') {
                // comment // ...
                self.i += 2;
                while !self.eof() && self.peek() != b'\n' { self.i += 1; }
            } else { break; }
        }
    }
    fn eof(&self) -> bool { self.i >= self.s.len() }
    fn peek(&self) -> u8 { self.s[self.i] }
    fn peek2(&self) -> Option<u8> { if self.i+1 < self.s.len() { Some(self.s[self.i+1]) } else { None } }
    fn lex_number(&mut self) -> Result<Tok> {
        let start = self.i;
        let mut saw_dot = false;
        while !self.eof() {
            let c = self.peek();
            if c.is_ascii_digit() {
                self.i += 1;
                continue;
            }
            if c == b'.' && !saw_dot {
                saw_dot = true;
                self.i += 1;
                continue;
            }
            break;
        }
        let s = std::str::from_utf8(&self.s[start..self.i]).unwrap();
        if saw_dot {
            let v: f64 = s.parse().map_err(|_| eyre!("float invalide: {s}"))?;
            Ok(Tok::Float(v))
        } else {
            let v: i64 = s.parse().map_err(|_| eyre!("int invalide: {s}"))?;
            Ok(Tok::Int(v))
        }
    }
    fn lex_string(&mut self) -> Result<Tok> {
        // assume opening "
        self.i += 1;
        let mut out = String::new();
        while !self.eof() {
            let c = self.peek();
            self.i += 1;
            match c {
                b'"' => return Ok(Tok::Str(out)),
                b'\\' => {
                    let e = self.peek(); self.i += 1;
                    match e {
                        b'"' => out.push('"'),
                        b'\\' => out.push('\\'),
                        b'n' => out.push('\n'),
                        b'r' => out.push('\r'),
                        b't' => out.push('\t'),
                        b'0' => out.push('\0'),
                        other => return Err(eyre!("échappement invalide: \\{}", other as char)),
                    }
                }
                _ => out.push(c as char),
            }
        }
        Err(eyre!("string non terminée"))
    }
    fn try_two_char_op(&mut self) -> Option<Tok> {
        let two = if self.i + 1 < self.s.len() { &self.s[self.i..self.i+2] } else { return None; };
        let t = match two {
            b"&&" => Tok::AndAnd,
            b"||" => Tok::OrOr,
            b"==" => Tok::EqEq,
            b"!=" => Tok::NotEq,
            b"<=" => Tok::Le,
            b">=" => Tok::Ge,
            _ => return None,
        };
        self.i += 2;
        Some(t)
    }
}
fn is_ident_start(c: u8) -> bool { (c == b'_') || (c as char).is_ascii_alphabetic() }
fn is_ident_part(c: u8) -> bool { is_ident_start(c) || (c as char).is_ascii_digit() }

/* ------------------------------ Parser ------------------------------------ */

struct Parser<'a> {
    lx: Lexer<'a>,
    look: Tok,
}
impl<'a> Parser<'a> {
    fn new(src: &'a str) -> Result<Self> {
        let mut lx = Lexer::new(src);
        let look = lx.next_token()?;
        Ok(Self { lx, look })
    }
    fn bump(&mut self) -> Result<()> { self.look = self.lx.next_token()?; Ok(()) }
    fn expect(&mut self, t: &Tok) -> Result<()> {
        if &self.look == t { self.bump() } else { Err(eyre!("attendu {:?}, trouvé {:?}", t, self.look)) }
    }

    fn parse_expr(&mut self) -> Result<Expr> { self.parse_or() }

    fn parse_or(&mut self) -> Result<Expr> {
        let mut e = self.parse_and()?;
        while matches!(self.look, Tok::OrOr) {
            self.bump()?;
            let r = self.parse_and()?;
            e = Expr::Binary(Box::new(e), BinOp::Or, Box::new(r));
        }
        Ok(e)
    }
    fn parse_and(&mut self) -> Result<Expr> {
        let mut e = self.parse_equality()?;
        while matches!(self.look, Tok::AndAnd) {
            self.bump()?;
            let r = self.parse_equality()?;
            e = Expr::Binary(Box::new(e), BinOp::And, Box::new(r));
        }
        Ok(e)
    }
    fn parse_equality(&mut self) -> Result<Expr> {
        let mut e = self.parse_comparison()?;
        loop {
            match self.look {
                Tok::EqEq => { self.bump()?; let r = self.parse_comparison()?; e = Expr::Binary(Box::new(e), BinOp::Eq, Box::new(r)); }
                Tok::NotEq => { self.bump()?; let r = self.parse_comparison()?; e = Expr::Binary(Box::new(e), BinOp::Ne, Box::new(r)); }
                _ => break,
            }
        }
        Ok(e)
    }
    fn parse_comparison(&mut self) -> Result<Expr> {
        let mut e = self.parse_term()?;
        loop {
            match self.look {
                Tok::Lt => { self.bump()?; let r = self.parse_term()?; e = Expr::Binary(Box::new(e), BinOp::Lt, Box::new(r)); }
                Tok::Le => { self.bump()?; let r = self.parse_term()?; e = Expr::Binary(Box::new(e), BinOp::Le, Box::new(r)); }
                Tok::Gt => { self.bump()?; let r = self.parse_term()?; e = Expr::Binary(Box::new(e), BinOp::Gt, Box::new(r)); }
                Tok::Ge => { self.bump()?; let r = self.parse_term()?; e = Expr::Binary(Box::new(e), BinOp::Ge, Box::new(r)); }
                _ => break,
            }
        }
        Ok(e)
    }
    fn parse_term(&mut self) -> Result<Expr> {
        let mut e = self.parse_factor()?;
        loop {
            match self.look {
                Tok::Plus => { self.bump()?; let r = self.parse_factor()?; e = Expr::Binary(Box::new(e), BinOp::Add, Box::new(r)); }
                Tok::Minus => { self.bump()?; let r = self.parse_factor()?; e = Expr::Binary(Box::new(e), BinOp::Sub, Box::new(r)); }
                _ => break,
            }
        }
        Ok(e)
    }
    fn parse_factor(&mut self) -> Result<Expr> {
        let mut e = self.parse_unary()?;
        loop {
            match self.look {
                Tok::Star => { self.bump()?; let r = self.parse_unary()?; e = Expr::Binary(Box::new(e), BinOp::Mul, Box::new(r)); }
                Tok::Slash => { self.bump()?; let r = self.parse_unary()?; e = Expr::Binary(Box::new(e), BinOp::Div, Box::new(r)); }
                Tok::Percent => { self.bump()?; let r = self.parse_unary()?; e = Expr::Binary(Box::new(e), BinOp::Rem, Box::new(r)); }
                _ => break,
            }
        }
        Ok(e)
    }
    fn parse_unary(&mut self) -> Result<Expr> {
        match self.look {
            Tok::Bang => { self.bump()?; Ok(Expr::Unary(UnOp::Not, Box::new(self.parse_unary()?))) }
            Tok::Plus => { self.bump()?; Ok(Expr::Unary(UnOp::Plus, Box::new(self.parse_unary()?))) }
            Tok::Minus => { self.bump()?; Ok(Expr::Unary(UnOp::Neg, Box::new(self.parse_unary()?))) }
            _ => self.parse_call(),
        }
    }
    fn parse_call(&mut self) -> Result<Expr> {
        let mut e = self.parse_primary()?;
        loop {
            match self.look {
                Tok::LParen => {
                    self.bump()?;
                    let mut args = Vec::new();
                    if !matches!(self.look, Tok::RParen) {
                        loop {
                            args.push(self.parse_expr()?);
                            if matches!(self.look, Tok::Comma) { self.bump()?; continue; }
                            break;
                        }
                    }
                    self.expect(&Tok::RParen)?;
                    e = Expr::Call(Box::new(e), args);
                }
                Tok::Dot => {
                    self.bump()?;
                    let ident = match &self.look {
                        Tok::Ident(s) => { let s2 = s.clone(); self.bump()?; s2 }
                        _ => return Err(eyre!("attendu ident après '.'")),
                    };
                    e = Expr::Get(Box::new(e), ident);
                }
                Tok::LBracket => {
                    self.bump()?;
                    let idx = self.parse_expr()?;
                    self.expect(&Tok::RBracket)?;
                    e = Expr::Index(Box::new(e), Box::new(idx));
                }
                _ => break,
            }
        }
        Ok(e)
    }
    fn parse_primary(&mut self) -> Result<Expr> {
        let e = match self.look.clone() {
            Tok::True => { self.bump()?; Expr::Literal(Value::Bool(true)) }
            Tok::False => { self.bump()?; Expr::Literal(Value::Bool(false)) }
            Tok::Null => { self.bump()?; Expr::Literal(Value::Null) }
            Tok::Int(i) => { self.bump()?; Expr::Literal(Value::Int(i)) }
            Tok::Float(x) => { self.bump()?; Expr::Literal(Value::Float(x)) }
            Tok::Str(s) => { self.bump()?; Expr::Literal(Value::Str(s)) }
            Tok::Ident(name) => { self.bump()?; Expr::Var(name) }
            Tok::LParen => {
                self.bump()?;
                let e = self.parse_expr()?;
                self.expect(&Tok::RParen)?;
                e
            }
            other => return Err(eyre!("token inattendu: {:?}", other)),
        };
        Ok(e)
    }
}

/* ------------------------------- AST & Eval ------------------------------- */

#[derive(Debug, Clone)]
enum Expr {
    Literal(Value),
    Var(String),
    Unary(UnOp, Box<Expr>),
    Binary(Box<Expr>, BinOp, Box<Expr>),
    Call(Box<Expr>, Vec<Expr>),
    Get(Box<Expr>, String),
    Index(Box<Expr>, Box<Expr>),
}

#[derive(Debug, Clone, Copy)]
enum UnOp { Not, Neg, Plus }

#[derive(Debug, Clone, Copy)]
enum BinOp { Add, Sub, Mul, Div, Rem, Eq, Ne, Lt, Le, Gt, Ge, And, Or }

pub fn eval_expr(src: &str, env: &dyn EvalEnv) -> Result<Value> {
    let mut p = Parser::new(src)?;
    let ast = p.parse_expr()?;
    // Consommer Eof (optionnel)
    // On ne force pas, pour tolérer whitespace en fin.
    eval(&ast, env)
}

pub fn eval_bool(src: &str, env: &dyn EvalEnv) -> Result<bool> {
    Ok(eval_expr(src, env)?.truthy())
}

fn eval(ast: &Expr, env: &dyn EvalEnv) -> Result<Value> {
    use Expr::*;
    match ast {
        Literal(v) => Ok(v.clone()),
        Var(name) => env.get_var(name).ok_or_else(|| eyre!("variable inconnue: {name}")),
        Unary(op, e) => {
            let v = eval(e, env)?;
            match op {
                UnOp::Not => Ok(Value::Bool(!v.truthy())),
                UnOp::Neg => match v.as_number()? {
                    Number::Int(i) => Ok(Value::Int(-i)),
                    Number::Float(f) => Ok(Value::Float(-f)),
                },
                UnOp::Plus => match v.as_number()? {
                    Number::Int(i) => Ok(Value::Int(i)),
                    Number::Float(f) => Ok(Value::Float(f)),
                },
            }
        }
        Binary(a, op, b) => {
            match op {
                BinOp::And => {
                    let va = eval(a, env)?;
                    if !va.truthy() { return Ok(Value::Bool(false)); }
                    let vb = eval(b, env)?;
                    return Ok(Value::Bool(vb.truthy()));
                }
                BinOp::Or => {
                    let va = eval(a, env)?;
                    if va.truthy() { return Ok(Value::Bool(true)); }
                    let vb = eval(b, env)?;
                    return Ok(Value::Bool(vb.truthy()));
                }
                _ => { /* suite dessous */ }
            }
            let va = eval(a, env)?;
            let vb = eval(b, env)?;
            match op {
                BinOp::Add => num_bin(va, vb, |x,y| x+y, |x,y| x+y, |s,t| Ok(Value::Str(format!("{s}{t}")))),
                BinOp::Sub => num_bin(va, vb, |x,y| x-y, |x,y| x-y, |_s,_t| Err(eyre!("soustraction non définie pour strings"))),
                BinOp::Mul => num_bin(va, vb, |x,y| x*y, |x,y| x*y, |_s,_t| Err(eyre!("multiplication non définie pour strings"))),
                BinOp::Div => num_bin(va, vb, |x,y| x / y, |x,y| x / y, |_s,_t| Err(eyre!("division non définie pour strings"))),
                BinOp::Rem => num_bin(va, vb, |x,y| x % y, |x,y| x % y, |_s,_t| Err(eyre!("modulo non défini pour strings"))),
                BinOp::Eq  => Ok(Value::Bool(eq_value(&va, &vb))),
                BinOp::Ne  => Ok(Value::Bool(!eq_value(&va, &vb))),
                BinOp::Lt  => Ok(Value::Bool(cmp_value(&va, &vb, |o| o.is_lt())?)),
                BinOp::Le  => Ok(Value::Bool(cmp_value(&va, &vb, |o| !o.is_gt())?)),
                BinOp::Gt  => Ok(Value::Bool(cmp_value(&va, &vb, |o| o.is_gt())?)),
                BinOp::Ge  => Ok(Value::Bool(cmp_value(&va, &vb, |o| !o.is_lt())?)),
                BinOp::And | BinOp::Or => unreachable!(),
            }
        }
        Call(callee, args) => {
            // callee doit être un ident (fonction globale) pour ce MVP
            match callee.as_ref() {
                Expr::Var(name) => {
                    let mut vals = Vec::with_capacity(args.len());
                    for a in args { vals.push(eval(a, env)?); }
                    env.call(name, &vals)
                }
                _ => Err(eyre!("appel sur non-fonction (seul `fn(...)` supporté ici)")),
            }
        }
        Get(base, field) => {
            let b = eval(base, env)?;
            env.get_field(&b, field)
                .ok_or_else(|| eyre!("champ introuvable: {}.{}", display_value_type(&b), field))
        }
        Index(base, idx) => {
            let b = eval(base, env)?;
            let i = eval(idx, env)?;
            env.index(&b, &i)
                .ok_or_else(|| eyre!("indexation invalide: {}[{}]", display_value_type(&b), i))
        }
    }
}

fn display_value_type(v: &Value) -> &'static str { v.type_name() }

fn num_bin(va: Value, vb: Value,
           iop: fn(i64,i64)->i64,
           fop: fn(f64,f64)->f64,
           sop: fn(String,String)->Result<Value>) -> Result<Value> {
    match (va, vb) {
        (Value::Int(a), Value::Int(b)) => Ok(Value::Int(iop(a,b))),
        (Value::Float(a), Value::Float(b)) => Ok(Value::Float(fop(a,b))),
        (Value::Int(a), Value::Float(b)) => Ok(Value::Float(fop(a as f64, b))),
        (Value::Float(a), Value::Int(b)) => Ok(Value::Float(fop(a, b as f64))),
        (Value::Str(a), Value::Str(b)) => sop(a,b),
        (Value::Str(a), b) => sop(a, b.to_string()),
        (a, Value::Str(b)) => sop(a.to_string(), b),
        (a, b) => Err(eyre!("opération numérique invalide entre {} et {}", a.type_name(), b.type_name())),
    }
}

fn eq_value(a: &Value, b: &Value) -> bool {
    use Value::*;
    match (a, b) {
        (Bool(x), Bool(y)) => x == y,
        (Int(x), Int(y)) => x == y,
        (Float(x), Float(y)) => x == y,
        (Int(x), Float(y)) => (*x as f64) == *y,
        (Float(x), Int(y)) => *x == (*y as f64),
        (Str(x), Str(y)) => x == y,
        (Null, Null) => true,
        _ => false, // arrays/maps: stricte, pas d’égalité profonde ici
    }
}

fn cmp_value<F>(a: &Value, b: &Value, pred: F) -> Result<bool>
where F: Fn(std::cmp::Ordering) -> bool {
    use Value::*;
    let ord = match (a, b) {
        (Int(x), Int(y)) => x.cmp(y),
        (Float(x), Float(y)) => x.partial_cmp(y).ok_or_else(|| eyre!("NaN incomparable"))?,
        (Int(x), Float(y)) => (*x as f64).partial_cmp(y).ok_or_else(|| eyre!("NaN incomparable"))?,
        (Float(x), Int(y)) => x.partial_cmp(&(*y as f64)).ok_or_else(|| eyre!("NaN incomparable"))?,
        (Str(x), Str(y)) => x.cmp(y),
        _ => return Err(eyre!("comparaison non supportée entre {} et {}", a.type_name(), b.type_name())),
    };
    Ok(pred(ord))
}

/* --------------------------- Interpolation -------------------------------- */

/// Interpole un template avec `{expr}` évaluées dans `env`.
/// Ex: "x={x} y={a+b}" → "x=42 y=50"
pub fn format_log_message(template: &str, env: &dyn EvalEnv) -> Result<String> {
    let mut out = String::with_capacity(template.len());
    let mut chars = template.chars().peekable();
    while let Some(c) = chars.next() {
        if c == '{' {
            if let Some('{' ) = chars.peek().copied() {
                chars.next(); out.push('{'); continue; // "{{" -> "{"
            }
            // lire jusqu'à '}'
            let mut expr = String::new();
            let mut closed = false;
            while let Some(d) = chars.next() {
                if d == '}' {
                    closed = true; break;
                }
                expr.push(d);
            }
            if !closed { return Err(eyre!("accolade fermante manquante dans l'interpolation")); }
            let val = eval_expr(expr.trim(), env)?;
            out.push_str(&val.to_string());
        } else if c == '}' {
            if let Some('}') = chars.peek().copied() {
                chars.next(); out.push('}'); // "}}" -> "}"
            } else {
                return Err(eyre!("accolade fermante isolée '}}'"));
            }
        } else {
            out.push(c);
        }
    }
    Ok(out)
}

/* --------------------------------- Tests ---------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;

    struct MapEnv { vars: HashMap<String, Value> }
    impl EvalEnv for MapEnv {
        fn get_var(&self, name: &str) -> Option<Value> { self.vars.get(name).cloned() }
        fn call(&self, func: &str, args: &[Value]) -> Result<Value> {
            match func {
                "len" => {
                    if let Some(v) = args.get(0) {
                        match v {
                            Value::Str(s) => Ok(Value::Int(s.len() as i64)),
                            Value::Array(a) => Ok(Value::Int(a.len() as i64)),
                            _ => Err(eyre!("len() non défini pour {}", v.type_name())),
                        }
                    } else { Err(eyre!("len() attend 1 arg")) }
                }
                _ => Err(eyre!("fonction inconnue: {func}")),
            }
        }
    }

    fn env(vars: &[(&str, Value)]) -> MapEnv {
        MapEnv { vars: vars.iter().map(|(k,v)| (k.to_string(), v.clone())).collect() }
    }

    #[test]
    fn arithmetic_bool() {
        let e = env(&[("a", Value::Int(10)), ("b", Value::Int(5))]);
        assert_eq!(eval_expr("a + b*2", &e).unwrap(), Value::Int(20));
        assert_eq!(eval_bool("a > b && 1 < 2", &e).unwrap(), true);
        assert_eq!(eval_bool("false || true && false", &e).unwrap(), false);
    }

    #[test]
    fn strings_and_calls() {
        let e = env(&[("s", Value::Str("hi".into()))]);
        assert_eq!(eval_expr(r#""x=" + s"#, &e).unwrap(), Value::Str("x=hi".into()));
        assert_eq!(eval_expr(r#"len(s) == 2"#, &e).unwrap(), Value::Bool(true));
    }

    #[test]
    fn fields_and_index() {
        let mut m = HashMap::new();
        m.insert("x".into(), Value::Int(7));
        let arr = Value::Array(vec![Value::Int(1), Value::Int(2)]);
        let e = env(&[("obj", Value::Map(m)), ("arr", arr.clone())]);
        assert_eq!(eval_expr("obj.x + arr[1]", &e).unwrap(), Value::Int(9));
    }

    #[test]
    fn interpolation() {
        let e = env(&[("x", Value::Int(42)), ("y", Value::Int(8))]);
        let s = format_log_message("value={x} sum={x+y}", &e).unwrap();
        assert_eq!(s, "value=42 sum=50");
        let s2 = format_log_message("{{ok}} {x}", &e).unwrap();
        assert_eq!(s2, "{ok} 42");
    }
}
