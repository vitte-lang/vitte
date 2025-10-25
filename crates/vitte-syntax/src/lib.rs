//! vitte-syntax — parsing Vitte (lexing + AST léger + abaissement optionnel)
//!
//! Cible: une grammaire pragmatique et un parseur robuste sans dépendance
//! externe par défaut. `feature = "grammar"` peut charger une EBNF via `pest`.
//!
//! Contenu principal:
//! - Lexeur: [`Token`], [`Lexer`]
//! - AST syntaxique léger: [`SyntaxModule`], [`Item`], [`Stmt`], [`Expr`], [`TypeExpr`]
//! - Parseur: [`Parser`], helpers [`parse_module`], [`parse_expr`]
//! - Pretty: [`pretty::to_string_module`]
//! - JSON optionnel (feature `json`)
//!
//! Intégrations:
//! - `vitte-ast` est référencé mais non exigé ici: on expose un stub
//!   d'abaissement conditionnel via [`lower::to_vitte_ast`] si l'API correspond.
//!
//! Cette bibliothèque vise la *résilience*: des erreurs de récupération sont
//! ajoutées (`Error` avec spans) et le parseur continue quand possible.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines,
    clippy::match_same_arms
)]

use anyhow::{Result, bail};
use std::fmt;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/* ===================================== Span ===================================== */

/// Position source [start, end) en bytes UTF-8.
#[derive(Clone, Copy, PartialEq, Eq, Debug, Default)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Span {
    pub start: u32,
    pub end: u32,
}
impl Span {
    pub fn new(start: usize, end: usize) -> Self {
        Self { start: start as u32, end: end as u32 }
    }
    pub fn merge(self, other: Span) -> Span {
        Span { start: self.start.min(other.start), end: self.end.max(other.end) }
    }
}

/* ===================================== Error ==================================== */

/// Erreur de syntaxe.
#[derive(Clone, Debug, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Error {
    pub message: String,
    pub span: Span,
    pub expected: Vec<String>,
    pub severity: Severity,
}
impl Error {
    fn new(message: impl Into<String>, span: Span) -> Self {
        Self { message: message.into(), span, expected: Vec::new(), severity: Severity::Error }
    }

    fn with_expected(mut self, items: Vec<String>) -> Self {
        self.expected = items;
        self
    }

    fn incomplete(mut self) -> Self {
        self.severity = Severity::Incomplete;
        self
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Severity {
    Error,
    Incomplete,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} at {}..{}", self.message, self.span.start, self.span.end)
    }
}

/* ===================================== Tokens =================================== */

/// Kinds minimaux (extensibles sans BC-break).
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum TokenKind {
    // punctuation
    LParen,
    RParen,
    LBrace,
    RBrace,
    Comma,
    Colon,
    Semicolon,
    At,
    Arrow,
    Assign,
    Plus,
    Minus,
    Star,
    Slash,
    // keywords
    Fn,
    Let,
    If,
    Else,
    Return,
    True,
    False,
    // literals/idents
    Ident(String),
    Int(String),
    Str(String),
    // special
    Eof,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Token {
    pub kind: TokenKind,
    pub span: Span,
}

pub struct Lexer<'a> {
    src: &'a str,
    bytes: &'a [u8],
    i: usize,
}
impl<'a> Lexer<'a> {
    pub fn new(src: &'a str) -> Self {
        Self { src, bytes: src.as_bytes(), i: 0 }
    }

    fn peek(&self) -> Option<u8> {
        self.bytes.get(self.i).copied()
    }
    fn bump(&mut self) -> Option<u8> {
        let b = self.peek()?;
        self.i += 1;
        Some(b)
    }
    fn span_from(&self, start: usize) -> Span {
        Span::new(start, self.i)
    }

    fn eat_while<F: Fn(u8) -> bool>(&mut self, f: F) {
        while let Some(b) = self.peek() {
            if f(b) {
                self.i += 1
            } else {
                break;
            }
        }
    }

    fn skip_ws_and_comments(&mut self) {
        loop {
            let before = self.i;
            // whitespace
            self.eat_while(|b| matches!(b, b' ' | b'\t' | b'\n' | b'\r'));
            // line comment //
            if self.peek() == Some(b'/') && self.bytes.get(self.i + 1) == Some(&b'/') {
                self.i += 2;
                self.eat_while(|b| b != b'\n');
            }
            // block comment /* ... */
            if self.peek() == Some(b'/') && self.bytes.get(self.i + 1) == Some(&b'*') {
                self.i += 2;
                while self.i + 1 < self.bytes.len()
                    && !(self.peek() == Some(b'*') && self.bytes.get(self.i + 1) == Some(&b'/'))
                {
                    self.i += 1;
                }
                if self.i + 1 < self.bytes.len() {
                    self.i += 2;
                }
            }
            if self.i == before {
                break;
            }
        }
    }

    fn ident_or_kw(&mut self, start: usize) -> Token {
        self.eat_while(|b| is_ident_continue(b));
        let s = &self.src[start..self.i];
        let kind = match s {
            "fn" => TokenKind::Fn,
            "let" => TokenKind::Let,
            "if" => TokenKind::If,
            "else" => TokenKind::Else,
            "return" => TokenKind::Return,
            "true" => TokenKind::True,
            "false" => TokenKind::False,
            _ => TokenKind::Ident(s.to_string()),
        };
        Token { kind, span: self.span_from(start) }
    }

    fn number(&mut self, start: usize) -> Token {
        self.eat_while(|b| (b as char).is_ascii_digit());
        Token {
            kind: TokenKind::Int(self.src[start..self.i].to_string()),
            span: self.span_from(start),
        }
    }

    fn string(&mut self, start: usize) -> Token {
        // bump starting quote already consumed
        while let Some(b) = self.bump() {
            if b == b'\"' {
                break;
            }
            if b == b'\\' {
                self.bump();
            } // skip escaped
        }
        Token {
            kind: TokenKind::Str(self.src[start..self.i].to_string()),
            span: self.span_from(start),
        }
    }

    pub fn next_token(&mut self) -> Token {
        self.skip_ws_and_comments();
        let start = self.i;
        match self.bump() {
            None => Token { kind: TokenKind::Eof, span: Span::new(self.i, self.i) },
            Some(b'(') => tok(TokenKind::LParen, self, start),
            Some(b')') => tok(TokenKind::RParen, self, start),
            Some(b'{') => tok(TokenKind::LBrace, self, start),
            Some(b'}') => tok(TokenKind::RBrace, self, start),
            Some(b',') => tok(TokenKind::Comma, self, start),
            Some(b':') => tok(TokenKind::Colon, self, start),
            Some(b';') => tok(TokenKind::Semicolon, self, start),
            Some(b'@') => tok(TokenKind::At, self, start),
            Some(b'=') => tok(TokenKind::Assign, self, start),
            Some(b'+') => tok(TokenKind::Plus, self, start),
            Some(b'-') => {
                if self.peek() == Some(b'>') {
                    self.bump();
                    Token { kind: TokenKind::Arrow, span: self.span_from(start) }
                } else {
                    tok(TokenKind::Minus, self, start)
                }
            }
            Some(b'*') => tok(TokenKind::Star, self, start),
            Some(b'/') => tok(TokenKind::Slash, self, start),
            Some(b'"') => self.string(start),
            Some(b) if is_ident_start(b) => self.ident_or_kw(start),
            Some(b) if (b as char).is_ascii_digit() => self.number(start),
            Some(_) => Token { kind: TokenKind::Eof, span: self.span_from(start) },
        }
    }

    pub fn tokenize(mut self) -> Vec<Token> {
        let mut v = Vec::new();
        loop {
            let t = self.next_token();
            let is_eof = matches!(t.kind, TokenKind::Eof);
            v.push(t);
            if is_eof {
                break;
            }
        }
        v
    }
}
fn tok(kind: TokenKind, lx: &Lexer, start: usize) -> Token {
    Token { kind, span: lx.span_from(start) }
}
fn is_ident_start(b: u8) -> bool {
    (b as char).is_ascii_alphabetic() || b == b'_'
}
fn is_ident_continue(b: u8) -> bool {
    is_ident_start(b) || (b as char).is_ascii_digit()
}

/* ===================================== AST ====================================== */

#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct SyntaxModule {
    pub items: Vec<Item>,
    pub span: Span,
    pub errors: Vec<Error>,
}

#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Item {
    Fn(FnItem),
}

#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct FnItem {
    pub attributes: Vec<Attribute>,
    pub name: String,
    pub params: Vec<Param>,
    pub ret: Option<TypeExpr>,
    pub body: Block,
    pub span: Span,
}

#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Param {
    pub name: String,
    pub ty: Option<TypeExpr>,
    pub span: Span,
}

#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum TypeExpr {
    Named(String, Span),
    Tuple(Vec<TypeExpr>, Span),
    Unit(Span),
}

impl TypeExpr {
    pub fn span(&self) -> Span {
        match self {
            TypeExpr::Named(_, s) | TypeExpr::Tuple(_, s) | TypeExpr::Unit(s) => *s,
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Block {
    pub stmts: Vec<Stmt>,
    pub span: Span,
}

#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Stmt {
    Let {
        attributes: Vec<Attribute>,
        name: String,
        ty: Option<TypeExpr>,
        init: Option<Expr>,
        span: Span,
    },
    Expr(Expr),
    Return(Option<Expr>, Span),
}

#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Expr {
    Unit(Span),
    Int(i64, Span),
    Bool(bool, Span),
    Str(String, Span),
    Var(String, Span),
    Call { callee: Box<Expr>, args: Vec<Expr>, span: Span },
    Binary { op: BinOp, lhs: Box<Expr>, rhs: Box<Expr>, span: Span },
    Unary { op: UnOp, expr: Box<Expr>, span: Span },
    If { cond: Box<Expr>, then_blk: Block, else_blk: Option<Block>, span: Span },
    Block(Block),
}

#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Attribute {
    pub path: String,
    pub args: Option<Vec<Expr>>,
    pub span: Span,
}

impl Expr {
    pub fn span(&self) -> Span {
        match self {
            Expr::Unit(s)
            | Expr::Int(_, s)
            | Expr::Bool(_, s)
            | Expr::Str(_, s)
            | Expr::Var(_, s) => *s,
            Expr::Call { span, .. }
            | Expr::Binary { span, .. }
            | Expr::Unary { span, .. }
            | Expr::If { span, .. } => *span,
            Expr::Block(b) => b.span,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum BinOp {
    Add,
    Sub,
    Mul,
    Div,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum UnOp {
    Neg,
}

/* ===================================== Parser =================================== */

/// Parseur par descente récursive avec précédence (Pratt simplifié).
pub struct Parser<'a> {
    toks: Vec<Token>,
    i: usize,
    errors: Vec<Error>,
    src_len: usize,
    _src: &'a str,
}

impl<'a> Parser<'a> {
    pub fn new(src: &'a str) -> Self {
        let toks = Lexer::new(src).tokenize();
        Self { toks, i: 0, errors: Vec::new(), src_len: src.len(), _src: src }
    }

    fn peek(&self) -> &Token {
        self.toks.get(self.i).unwrap()
    }
    fn at(&self, k: &TokenKind) -> bool {
        &self.peek().kind == k
    }
    fn bump(&mut self) -> &Token {
        let t = self.toks.get(self.i).unwrap();
        self.i += 1;
        t
    }
    fn expect(&mut self, k: TokenKind, msg: &str) -> Span {
        if self.peek().kind == k {
            return self.bump().span;
        }
        let sp = self.peek().span;
        self.errors.push(
            Error::new(format!("attendu {k:?}: {msg}"), sp)
                .with_expected(vec![format!("{k:?}")])
                .incomplete(),
        );
        sp
    }
    fn eat(&mut self, k: TokenKind) -> bool {
        if self.peek().kind == k {
            self.bump();
            true
        } else {
            false
        }
    }

    pub fn parse_module(mut self) -> SyntaxModule {
        let start = 0;
        let mut items = Vec::new();
        while !matches!(self.peek().kind, TokenKind::Eof) {
            let attrs = self.collect_attributes();
            match self.parse_item(attrs) {
                Ok(it) => items.push(it),
                Err(e) => {
                    self.errors.push(e);
                    // sync: skip to next ';' or '}'
                    while !matches!(
                        self.peek().kind,
                        TokenKind::Semicolon | TokenKind::RBrace | TokenKind::Eof
                    ) {
                        self.bump();
                    }
                    if matches!(self.peek().kind, TokenKind::Semicolon) {
                        self.bump();
                    }
                    if matches!(self.peek().kind, TokenKind::RBrace) {
                        self.bump();
                    }
                }
            }
        }
        SyntaxModule { items, span: Span::new(start, self.src_len), errors: self.errors }
    }

    fn parse_item(&mut self, attrs: Vec<Attribute>) -> std::result::Result<Item, Error> {
        match self.peek().kind {
            TokenKind::Fn => {
                let fn_item = self.parse_fn(attrs)?;
                Ok(Item::Fn(fn_item))
            }
            _ => {
                let sp = self.peek().span;
                if !attrs.is_empty() {
                    for attr in attrs {
                        self.errors.push(
                            Error::new("attribut non supporté sur cet item", attr.span)
                                .incomplete(),
                        );
                    }
                }
                Err(Error::new("item inconnu, 'fn' attendu", sp)
                    .with_expected(vec!["fn".into()])
                    .incomplete())
            }
        }
    }

    fn collect_attributes(&mut self) -> Vec<Attribute> {
        let mut attrs = Vec::new();
        while matches!(self.peek().kind, TokenKind::At) {
            let at_span = self.bump().span;
            let name_tok = self.bump().clone();
            let (name, mut last_span) = match name_tok.kind {
                TokenKind::Ident(s) => (s, name_tok.span),
                _ => {
                    self.errors.push(
                        Error::new("ident d'attribut attendu", name_tok.span)
                            .with_expected(vec!["ident".into()])
                            .incomplete(),
                    );
                    attrs.push(Attribute { path: "_attr".into(), args: None, span: at_span });
                    continue;
                }
            };
            let mut args = None;
            if self.eat(TokenKind::LParen) {
                let mut exprs = Vec::new();
                let mut last_expr_span = last_span;
                loop {
                    if matches!(self.peek().kind, TokenKind::RParen | TokenKind::Eof) {
                        break;
                    }
                    match self.parse_expr_bp(0) {
                        Ok(expr) => {
                            last_expr_span = expr.span();
                            exprs.push(expr);
                        }
                        Err(err) => {
                            self.errors.push(err.incomplete());
                            break;
                        }
                    }
                    if !self.eat(TokenKind::Comma) {
                        break;
                    }
                }
                last_span = if matches!(self.peek().kind, TokenKind::RParen) {
                    self.bump().span
                } else {
                    self.errors.push(
                        Error::new(")' attendu dans l'attribut", self.peek().span)
                            .with_expected(vec![")".into()])
                            .incomplete(),
                    );
                    last_expr_span
                };
                args = Some(exprs);
            }
            let span = at_span.merge(last_span);
            attrs.push(Attribute { path: name, args, span });
        }
        attrs
    }

    fn parse_fn(&mut self, attrs: Vec<Attribute>) -> std::result::Result<FnItem, Error> {
        let start = self.expect(TokenKind::Fn, "début de fonction");
        let name = match &self.bump().kind {
            TokenKind::Ident(s) => s.clone(),
            _ => {
                let sp = self.peek().span;
                return Err(Error::new("nom de fonction attendu", sp));
            }
        };
        self.expect(TokenKind::LParen, "(");
        let mut params = Vec::new();
        if !self.at(&TokenKind::RParen) {
            loop {
                let p_start = self.peek().span;
                let pname = match &self.bump().kind {
                    TokenKind::Ident(s) => s.clone(),
                    _ => return Err(Error::new("nom de paramètre attendu", p_start)),
                };
                let mut pty = None;
                if self.eat(TokenKind::Colon) {
                    pty = Some(self.parse_type()?);
                }
                params.push(Param { name: pname, ty: pty, span: p_start });
                if self.eat(TokenKind::Comma) {
                    continue;
                }
                break;
            }
        }
        self.expect(TokenKind::RParen, ")");
        let mut ret = None;
        if self.eat(TokenKind::Arrow) {
            ret = Some(self.parse_type()?);
        }
        let body = self.parse_block()?;
        let mut span = start.merge(body.span);
        if let Some(first_attr) = attrs.first() {
            span = first_attr.span.merge(span);
        }
        Ok(FnItem { attributes: attrs, name, params, ret, body, span })
    }

    fn parse_type(&mut self) -> std::result::Result<TypeExpr, Error> {
        // Named | () | (T, U, ...)
        let t = self.bump().clone();
        match t.kind {
            TokenKind::Ident(s) => Ok(TypeExpr::Named(s, t.span)),
            TokenKind::LParen => {
                if self.eat(TokenKind::RParen) {
                    Ok(TypeExpr::Unit(Span::new(
                        t.span.start as usize,
                        self.peek().span.end as usize,
                    )))
                } else {
                    let mut ts = Vec::new();
                    loop {
                        ts.push(self.parse_type()?);
                        if self.eat(TokenKind::Comma) {
                            continue;
                        }
                        break;
                    }
                    let end = self.expect(TokenKind::RParen, ")");
                    Ok(TypeExpr::Tuple(ts, t.span.merge(end)))
                }
            }
            _ => Err(Error::new("type attendu", t.span)
                .with_expected(vec!["ident".into(), "(".into()])
                .incomplete()),
        }
    }

    fn parse_block(&mut self) -> std::result::Result<Block, Error> {
        let l = self.expect(TokenKind::LBrace, "{");
        let mut stmts = Vec::new();
        while !matches!(self.peek().kind, TokenKind::RBrace | TokenKind::Eof) {
            if self.at(&TokenKind::Semicolon) {
                self.bump();
                continue;
            }
            let attrs = self.collect_attributes();
            if matches!(self.peek().kind, TokenKind::RBrace | TokenKind::Eof) && !attrs.is_empty() {
                for attr in attrs {
                    self.errors
                        .push(Error::new("attribut sans déclaration", attr.span).incomplete());
                }
                continue;
            }
            match self.parse_stmt(attrs) {
                Ok(stmt) => stmts.push(stmt),
                Err(mut err) => {
                    if matches!(self.peek().kind, TokenKind::Eof) {
                        err = err.incomplete();
                    }
                    self.errors.push(err);
                    self.synchronize_stmt();
                    continue;
                }
            }
            let _ = self.eat(TokenKind::Semicolon);
        }
        let r = if self.peek().kind == TokenKind::RBrace {
            self.bump().span
        } else {
            self.errors.push(
                Error::new("'}' attendu pour fermer le bloc", self.peek().span)
                    .with_expected(vec!["}".into()])
                    .incomplete(),
            );
            Span::new(l.start as usize, self.peek().span.end as usize)
        };
        Ok(Block { stmts, span: l.merge(r) })
    }

    fn synchronize_stmt(&mut self) {
        while !matches!(self.peek().kind, TokenKind::Semicolon | TokenKind::RBrace | TokenKind::Eof)
        {
            self.bump();
        }
        if matches!(self.peek().kind, TokenKind::Semicolon) {
            self.bump();
        }
    }

    fn parse_stmt(&mut self, attrs: Vec<Attribute>) -> std::result::Result<Stmt, Error> {
        match self.peek().kind {
            TokenKind::Let => {
                let start = self.bump().span;
                let name = match &self.bump().kind {
                    TokenKind::Ident(s) => s.clone(),
                    _ => {
                        let sp = self.peek().span;
                        return Err(Error::new("ident attendu après `let`", sp)
                            .with_expected(vec!["ident".into()])
                            .incomplete());
                    }
                };
                let mut ty = None;
                if self.eat(TokenKind::Colon) {
                    ty = Some(self.parse_type()?);
                }
                let mut init = None;
                if self.eat(TokenKind::Assign) {
                    init = Some(self.parse_expr_bp(0)?);
                }
                let mut span = start;
                if let Some(first_attr) = attrs.first() {
                    span = first_attr.span.merge(span);
                }
                Ok(Stmt::Let { attributes: attrs, name, ty, init, span })
            }
            TokenKind::Return => {
                if !attrs.is_empty() {
                    for attr in attrs {
                        self.errors.push(
                            Error::new("attribut non supporté sur `return`", attr.span)
                                .incomplete(),
                        );
                    }
                }
                let s = self.bump().span;
                if matches!(
                    self.peek().kind,
                    TokenKind::Semicolon | TokenKind::RBrace | TokenKind::Eof
                ) {
                    Ok(Stmt::Return(None, s))
                } else {
                    let e = self.parse_expr_bp(0)?;
                    Ok(Stmt::Return(Some(e), s))
                }
            }
            TokenKind::Eof => Err(Error::new("déclaration interrompue", self.peek().span)
                .incomplete()
                .with_expected(vec!["expression".into()])),
            _ => {
                if !attrs.is_empty() {
                    for attr in attrs {
                        self.errors.push(
                            Error::new("attribut non supporté sur cette instruction", attr.span)
                                .incomplete(),
                        );
                    }
                }
                let e = self.parse_expr_bp(0)?;
                Ok(Stmt::Expr(e))
            }
        }
    }

    // Pratt parser
    fn parse_expr_bp(&mut self, min_bp: u8) -> std::result::Result<Expr, Error> {
        let mut lhs = self.parse_prefix()?;

        loop {
            let op = match self.peek().kind {
                TokenKind::Plus => BinOp::Add,
                TokenKind::Minus => BinOp::Sub,
                TokenKind::Star => BinOp::Mul,
                TokenKind::Slash => BinOp::Div,
                TokenKind::LParen => {
                    if matches!(self.peek().kind, TokenKind::Eof) {
                        self.errors.push(
                            Error::new("parenthèse fermante manquante", lhs.span())
                                .with_expected(vec![")".into()])
                                .incomplete(),
                        );
                        break;
                    }
                    // call
                    let _ = self.bump().span;
                    let mut args = Vec::new();
                    if !self.at(&TokenKind::RParen) {
                        loop {
                            args.push(self.parse_expr_bp(0)?);
                            if self.eat(TokenKind::Comma) {
                                continue;
                            }
                            break;
                        }
                    }
                    let r = if self.peek().kind == TokenKind::RParen {
                        self.bump().span
                    } else {
                        self.errors.push(
                            Error::new("parenthèse fermante manquante", self.peek().span)
                                .with_expected(vec![")".into()])
                                .incomplete(),
                        );
                        lhs.span()
                    };
                    let sp = lhs.span().merge(r);
                    lhs = Expr::Call { callee: Box::new(lhs), args, span: sp };
                    continue;
                }
                _ => break,
            };
            let (lbp, rbp) = infix_binding_power(op);
            if lbp < min_bp {
                break;
            }
            let _op_tok = self.bump();
            let rhs = self.parse_expr_bp(rbp)?;
            let span = lhs.span().merge(rhs.span());
            lhs = Expr::Binary { op, lhs: Box::new(lhs), rhs: Box::new(rhs), span };
        }

        Ok(lhs)
    }

    fn parse_prefix(&mut self) -> std::result::Result<Expr, Error> {
        let t = self.bump().clone();
        match t.kind {
            TokenKind::Int(s) => {
                let v: i64 = s.parse().unwrap_or(0);
                Ok(Expr::Int(v, t.span))
            }
            TokenKind::True => Ok(Expr::Bool(true, t.span)),
            TokenKind::False => Ok(Expr::Bool(false, t.span)),
            TokenKind::Str(s) => Ok(Expr::Str(s, t.span)),
            TokenKind::Ident(s) => Ok(Expr::Var(s, t.span)),
            TokenKind::Minus => {
                let e = self.parse_expr_bp(100)?; // haute précédence
                let span = t.span.merge(e.span());
                Ok(Expr::Unary { op: UnOp::Neg, expr: Box::new(e), span })
            }
            TokenKind::LParen => {
                if self.peek().kind == TokenKind::Eof {
                    return Err(Error::new("parenthèse fermante manquante", t.span)
                        .with_expected(vec![")".into()])
                        .incomplete());
                }
                if self.eat(TokenKind::RParen) {
                    Ok(Expr::Unit(t.span))
                } else {
                    let e = self.parse_expr_bp(0)?;
                    let r = if self.peek().kind == TokenKind::RParen {
                        self.bump().span
                    } else {
                        self.errors.push(
                            Error::new("parenthèse fermante manquante", self.peek().span)
                                .with_expected(vec![")".into()])
                                .incomplete(),
                        );
                        t.span
                    };
                    Ok(with_span(e, t.span.merge(r)))
                }
            }
            TokenKind::If => self.parse_if(t.span),
            TokenKind::LBrace => {
                // already consumed: back up one to reuse parse_block
                self.i -= 1;
                Ok(Expr::Block(self.parse_block()?))
            }
            _ => Err(Error::new("expression attendue", t.span)
                .with_expected(vec![
                    "ident".into(),
                    "nombre".into(),
                    "-".into(),
                    "(".into(),
                    "if".into(),
                    "{".into(),
                ])
                .incomplete()),
        }
    }

    fn parse_if(&mut self, kw_span: Span) -> std::result::Result<Expr, Error> {
        let cond = self.parse_expr_bp(0)?;
        let then_blk = self.parse_block()?;
        let else_blk = if self.eat(TokenKind::Else) {
            if self.eat(TokenKind::If) {
                // else if ...
                let nested = self.parse_if(kw_span)?;
                let span = kw_span.merge(nested.span());
                let blk = Block { stmts: vec![Stmt::Expr(nested)], span };
                Some(blk)
            } else {
                Some(self.parse_block()?)
            }
        } else {
            None
        };
        let span = kw_span.merge(then_blk.span);
        Ok(Expr::If { cond: Box::new(cond), then_blk, else_blk, span })
    }
}

fn with_span(mut e: Expr, sp: Span) -> Expr {
    match &mut e {
        Expr::Unit(s) | Expr::Int(_, s) | Expr::Bool(_, s) | Expr::Str(_, s) | Expr::Var(_, s) => {
            *s = sp
        }
        Expr::Call { span, .. }
        | Expr::Binary { span, .. }
        | Expr::Unary { span, .. }
        | Expr::If { span, .. } => *span = sp,
        Expr::Block(b) => b.span = sp,
    }
    e
}

fn infix_binding_power(op: BinOp) -> (u8, u8) {
    match op {
        BinOp::Mul | BinOp::Div => (60, 61),
        BinOp::Add | BinOp::Sub => (40, 41),
    }
}

/* ============================== Facade helpers ============================== */

/// Parse un module complet.
pub fn parse_module(src: &str) -> SyntaxModule {
    Parser::new(src).parse_module()
}

/// Parse une expression isolée.
pub fn parse_expr(src: &str) -> Result<Expr> {
    let mut p = Parser::new(src);
    let e = p.parse_expr_bp(0).map_err(|e| anyhow::anyhow!(e.to_string()))?;
    if !matches!(p.peek().kind, TokenKind::Eof | TokenKind::Semicolon) {
        let sp = p.peek().span;
        p.errors.push(
            Error::new("tokens restants après expression", sp)
                .with_expected(vec!["fin d'expression".into()])
                .incomplete(),
        );
    }
    if p.errors.is_empty() { Ok(e) } else { bail!(p.errors[0].to_string()) }
}

/* ============================ Incremental parsing ============================ */

pub mod incremental;
pub use incremental::{IncrementalParser, ParseDelta, TextEdit};
pub use tree::ParseTree;

/* ================================= Pretty ================================== */

pub mod pretty {
    use super::*;

    pub fn to_string_module(m: &SyntaxModule) -> String {
        let mut s = String::new();
        for it in &m.items {
            match it {
                Item::Fn(f) => {
                    s.push_str("fn ");
                    s.push_str(&f.name);
                    s.push('(');
                    for (i, p) in f.params.iter().enumerate() {
                        if i > 0 {
                            s.push_str(", ");
                        }
                        s.push_str(&p.name);
                        if let Some(t) = &p.ty {
                            s.push_str(": ");
                            s.push_str(&ty_to_string(t));
                        }
                    }
                    s.push(')');
                    if let Some(t) = &f.ret {
                        s.push_str(" -> ");
                        s.push_str(&ty_to_string(t));
                    }
                    s.push_str(" { ... }\n");
                }
            }
        }
        s
    }

    fn ty_to_string(t: &TypeExpr) -> String {
        match t {
            TypeExpr::Named(n, _) => n.clone(),
            TypeExpr::Unit(_) => "()".into(),
            TypeExpr::Tuple(ts, _) => {
                let parts: Vec<_> = ts.iter().map(ty_to_string).collect();
                format!("({})", parts.join(", "))
            }
        }
    }
}

/* ================================= Parse Tree =============================== */

pub mod tree {
    use super::*;

    #[derive(Clone, Debug, PartialEq)]
    #[cfg_attr(feature = "serde", derive(Serialize))]
    pub struct ParseTree {
        pub kind: &'static str,
        pub span: Span,
        pub value: Option<String>,
        pub children: Vec<ParseTree>,
    }

    impl ParseTree {
        fn new(kind: &'static str, span: Span) -> Self {
            Self { kind, span, value: None, children: Vec::new() }
        }

        fn with_value(mut self, value: impl Into<String>) -> Self {
            self.value = Some(value.into());
            self
        }

        fn with_children(mut self, children: Vec<ParseTree>) -> Self {
            self.children = children;
            self
        }
    }

    pub fn build_parse_tree(module: &SyntaxModule) -> ParseTree {
        let mut root = ParseTree::new("Module", module.span);
        root.children = module.items.iter().map(item_to_node).collect();
        root
    }

    #[cfg(feature = "serde")]
    pub fn to_json(module: &SyntaxModule) -> serde_json::Value {
        serde_json::to_value(build_parse_tree(module)).expect("serialize parse tree")
    }

    /// Représentation S-expression inspirée de `tree-sitter parse`.
    pub fn to_sexpr(module: &SyntaxModule) -> String {
        let tree = build_parse_tree(module);
        format_sexpr(&tree)
    }

    fn item_to_node(item: &Item) -> ParseTree {
        match item {
            Item::Fn(f) => fn_to_node(f),
        }
    }

    fn fn_to_node(f: &FnItem) -> ParseTree {
        let mut node = ParseTree::new("Fn", f.span).with_value(&f.name);
        if !f.attributes.is_empty() {
            node.children.push(attributes_node(&f.attributes));
        }
        let params = f.params.iter().map(param_to_node).collect::<Vec<_>>();
        let params_span = if f.params.is_empty() { f.span } else { merge_param_spans(&f.params) };
        node.children.push(ParseTree::new("Params", params_span).with_children(params));
        if let Some(ret) = &f.ret {
            node.children.push(type_to_node("ReturnType", ret));
        }
        node.children.push(block_to_node("Body", &f.body));
        node
    }

    fn param_to_node(p: &Param) -> ParseTree {
        let mut node = ParseTree::new("Param", p.span).with_value(&p.name);
        if let Some(ty) = &p.ty {
            node.children.push(type_to_node("Type", ty));
        }
        node
    }

    fn block_to_node(kind: &'static str, block: &Block) -> ParseTree {
        let mut node = ParseTree::new(kind, block.span);
        node.children = block.stmts.iter().map(stmt_to_node).collect();
        node
    }

    fn stmt_to_node(stmt: &Stmt) -> ParseTree {
        match stmt {
            Stmt::Let { attributes, name, ty, init, span } => {
                let mut node = ParseTree::new("Let", *span).with_value(name);
                if !attributes.is_empty() {
                    node.children.push(attributes_node(attributes));
                }
                if let Some(t) = ty {
                    node.children.push(type_to_node("Type", t));
                }
                if let Some(expr) = init {
                    node.children.push(expr_to_node("Init", expr));
                }
                node
            }
            Stmt::Expr(expr) => expr_to_node("Expr", expr),
            Stmt::Return(expr, span) => {
                let mut node = ParseTree::new("Return", *span);
                if let Some(e) = expr {
                    node.children.push(expr_to_node("Value", e));
                }
                node
            }
        }
    }

    fn expr_to_node(label: &'static str, expr: &Expr) -> ParseTree {
        match expr {
            Expr::Unit(span) => ParseTree::new(label, *span).with_value("()"),
            Expr::Int(v, span) => ParseTree::new(label, *span).with_value(v.to_string()),
            Expr::Bool(v, span) => ParseTree::new(label, *span).with_value(v.to_string()),
            Expr::Str(s, span) => ParseTree::new(label, *span).with_value(s.clone()),
            Expr::Var(name, span) => ParseTree::new(label, *span).with_value(name.clone()),
            Expr::Call { callee, args, span } => {
                let mut node = ParseTree::new("Call", *span);
                node.children.push(expr_to_node("Callee", callee));
                let args_span = if args.is_empty() { *span } else { merge_expr_spans(args) };
                let args_node = ParseTree::new("Args", args_span)
                    .with_children(args.iter().map(|a| expr_to_node("Arg", a)).collect());
                node.children.push(args_node);
                node
            }
            Expr::Binary { op, lhs, rhs, span } => {
                let mut node = ParseTree::new("BinaryExpr", *span).with_value(format!("{op:?}"));
                node.children.push(expr_to_node("Lhs", lhs));
                node.children.push(expr_to_node("Rhs", rhs));
                node
            }
            Expr::Unary { op, expr, span } => {
                let mut node = ParseTree::new("UnaryExpr", *span).with_value(format!("{op:?}"));
                node.children.push(expr_to_node("Expr", expr));
                node
            }
            Expr::If { cond, then_blk, else_blk, span } => {
                let mut node = ParseTree::new("If", *span);
                node.children.push(expr_to_node("Cond", cond));
                node.children.push(block_to_node("Then", then_blk));
                if let Some(else_block) = else_blk {
                    node.children.push(block_to_node("Else", else_block));
                }
                node
            }
            Expr::Block(block) => block_to_node("BlockExpr", block),
        }
    }

    fn type_to_node(label: &'static str, ty: &TypeExpr) -> ParseTree {
        match ty {
            TypeExpr::Named(name, span) => ParseTree::new(label, *span).with_value(name.clone()),
            TypeExpr::Unit(span) => ParseTree::new(label, *span).with_value("()"),
            TypeExpr::Tuple(items, span) => {
                let mut node = ParseTree::new(label, *span);
                node.children = items.iter().map(|t| type_to_node("Element", t)).collect();
                node
            }
        }
    }

    fn attributes_node(attrs: &[Attribute]) -> ParseTree {
        let span = merge_attribute_spans(attrs);
        let mut node = ParseTree::new("Attributes", span);
        node.children = attrs.iter().map(attribute_to_node).collect();
        node
    }

    fn attribute_to_node(attr: &Attribute) -> ParseTree {
        let mut node = ParseTree::new("Attribute", attr.span).with_value(attr.path.clone());
        if let Some(args) = &attr.args {
            node.children = args.iter().map(|arg| expr_to_node("Arg", arg)).collect();
        }
        node
    }

    fn format_sexpr(node: &ParseTree) -> String {
        let mut out = String::new();
        write_sexpr(node, 0, &mut out);
        out
    }

    fn write_sexpr(node: &ParseTree, depth: usize, out: &mut String) {
        let indent = "  ".repeat(depth);
        out.push_str(&indent);
        out.push('(');
        out.push_str(node.kind);
        if let Some(value) = &node.value {
            out.push(' ');
            out.push_str(&escape_atom(value));
        }
        if node.children.is_empty() {
            out.push(')');
            return;
        }
        out.push('\n');
        for (i, child) in node.children.iter().enumerate() {
            write_sexpr(child, depth + 1, out);
            if i + 1 < node.children.len() {
                out.push('\n');
            }
        }
        out.push('\n');
        out.push_str(&indent);
        out.push(')');
    }

    fn escape_atom(value: &str) -> String {
        if value.chars().all(|c| c.is_alphanumeric() || "_-".contains(c)) {
            value.to_string()
        } else {
            format!("\"{}\"", value.replace('"', "\\\""))
        }
    }

    fn merge_param_spans(params: &[Param]) -> Span {
        merge_spans(params.iter().map(|p| p.span))
    }

    fn merge_expr_spans(exprs: &[Expr]) -> Span {
        merge_spans(exprs.iter().map(|e| e.span()))
    }

    fn merge_attribute_spans(attrs: &[Attribute]) -> Span {
        merge_spans(attrs.iter().map(|a| a.span))
    }

    fn merge_spans<I>(mut it: I) -> Span
    where
        I: Iterator<Item = Span>,
    {
        match it.next() {
            None => Span::default(),
            Some(mut acc) => {
                for span in it {
                    acc = acc.merge(span);
                }
                acc
            }
        }
    }
}

/* =========================== Abaissement optionnel =========================== */

/// Intégration optionnelle avec `vitte-ast`. Adaptez les types si l'API réelle diffère.
#[cfg(feature = "ast")]
pub mod lower {
    use super::*;
    use vitte_ast as ast;

    /// Démonstration: reconstruit un `ast::Module` minimal si compatible.
    pub fn to_vitte_ast(sm: &SyntaxModule) -> Result<ast::Module> {
        // Cette fonction suppose l’existence d’API:
        //  - ast::Module { items: Vec<ast::Item> }
        //  - ast::Item::Fn(ast::Fn)
        //  - ast::Fn { name: String, params: Vec<ast::Param>, body: ast::Block, ret: Option<ast::Type> }
        //  - ast::Block { stmts: Vec<ast::Stmt> }, etc.
        // Si l’API diffère, adaptez l’implémentation.
        let mut am = ast::Module::new("module");
        for it in &sm.items {
            if let Item::Fn(f) = it {
                let mut af = ast::Fn::new(&f.name);
                for p in &f.params {
                    let at = p.ty.as_ref().map(|t| ast_type(t));
                    af = af.param(ast::Param::new(&p.name, at));
                }
                if let Some(r) = &f.ret {
                    af = af.ret(ast_type(r));
                }
                // corps vide en attendant un vrai lower d’Expr→AST
                let blk = ast::Block::new();
                af = af.body(blk);
                am.push(ast::Item::Fn(af));
            }
        }
        Ok(am)
    }

    fn ast_type(t: &TypeExpr) -> ast::Type {
        match t {
            TypeExpr::Named(n, _) => ast::Type::named(n),
            TypeExpr::Unit(_) => ast::Type::unit(),
            TypeExpr::Tuple(ts, _) => ast::Type::tuple(ts.iter().map(ast_type).collect()),
        }
    }
}

#[cfg(not(feature = "ast"))]
pub mod lower {
    use super::*;
    /// Non-disponible sans la feature `ast`.
    pub fn to_vitte_ast(_sm: &SyntaxModule) -> anyhow::Result<()> {
        anyhow::bail!("abaissement vers `vitte-ast` désactivé (feature `ast`)");
    }
}

/* ================================== Tests =================================== */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lexes_basics() {
        let v = Lexer::new("fn add(x: i32, y: i32) -> i32 { x + y }").tokenize();
        assert!(v.len() > 5);
        assert!(matches!(v.last().unwrap().kind, TokenKind::Eof));
    }

    #[test]
    fn parses_fn_and_exprs() {
        let m = parse_module(
            r#"
            fn add(x: i32, y: i32) -> i32 { x + y }
            fn main() { let z = add(1, 2); return z }
        "#,
        );
        assert!(m.errors.is_empty(), "{:?}", m.errors);
        assert_eq!(m.items.len(), 2);
        let Item::Fn(f) = &m.items[0];
        assert!(f.attributes.is_empty());
        assert_eq!(f.name, "add");
        assert_eq!(f.params.len(), 2);
    }

    #[test]
    fn parses_if_and_block_expr() {
        let e = parse_expr("if true { 1 } else { 2 }").unwrap();
        match e {
            Expr::If { .. } => {}
            _ => panic!("if attendu"),
        }
    }

    #[test]
    fn binop_precedence() {
        // 1 + 2 * 3 = 1 + (2*3)
        let e = parse_expr("1 + 2 * 3").unwrap();
        let s = match e {
            Expr::Binary { op: BinOp::Add, rhs, .. } => match *rhs {
                Expr::Binary { op: BinOp::Mul, .. } => "ok",
                _ => "bad",
            },
            _ => "bad",
        };
        assert_eq!(s, "ok");
    }

    #[test]
    fn pretty_shows_fn() {
        let m = parse_module("fn f() -> () { }");
        let s = crate::pretty::to_string_module(&m);
        assert!(s.contains("fn f()"));
    }

    #[test]
    fn parses_fn_attributes() {
        let m = parse_module("@inline @repr(linear) fn foo() {}");
        let Item::Fn(f) = &m.items[0];
        assert_eq!(f.attributes.len(), 2);
        assert_eq!(f.attributes[0].path, "inline");
        assert_eq!(f.attributes[1].path, "repr");
    }

    #[test]
    fn parses_let_attributes_inside_block() {
        let m = parse_module("fn f() { @trace let x = 1; }");
        let Item::Fn(f) = &m.items[0];
        let Stmt::Let { attributes, name, .. } = &f.body.stmts[0] else {
            panic!("let attendu");
        };
        assert_eq!(name, "x");
        assert_eq!(attributes.len(), 1);
        assert_eq!(attributes[0].path, "trace");
    }

    #[test]
    fn parse_tree_includes_attributes() {
        let module = parse_module("@inline fn foo(a: int) { @trace let b = a; }");
        let tree = crate::tree::build_parse_tree(&module);
        assert_eq!(tree.kind, "Module");
        let fn_node = &tree.children[0];
        assert_eq!(fn_node.kind, "Fn");
        let attrs_node =
            fn_node.children.iter().find(|n| n.kind == "Attributes").expect("fn attrs");
        assert_eq!(attrs_node.children[0].value.as_deref(), Some("inline"));
        let body_node = fn_node.children.iter().find(|n| n.kind == "Body").expect("fn body");
        let let_node = &body_node.children[0];
        let let_attrs =
            let_node.children.iter().find(|n| n.kind == "Attributes").expect("let attrs");
        assert_eq!(let_attrs.children[0].value.as_deref(), Some("trace"));
    }

    #[test]
    fn incomplete_function_body_reports_missing_brace() {
        let m = parse_module("fn main() { let x = 1;");
        assert!(!m.errors.is_empty());
        let err = &m.errors[0];
        assert!(matches!(err.severity, Severity::Incomplete));
        assert!(err.expected.iter().any(|e| e == "}"));
    }

    #[test]
    fn incomplete_param_type_is_flagged() {
        let m = parse_module("fn main(x: ) {}");
        assert!(!m.errors.is_empty());
        let err = &m.errors[0];
        assert!(matches!(err.severity, Severity::Incomplete));
        assert!(!err.expected.is_empty());
    }
}
