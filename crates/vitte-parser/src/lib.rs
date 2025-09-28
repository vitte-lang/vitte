//! vitte-parser — parseur du langage Vitte
//!
//! Branches :
//! - `vitte-lexer` pour la tokenisation
//! - `vitte-core` pour `Span/Spanned`
//! - `vitte-ast` pour l’AST cible
//!
//! Grammaire (essentiel):
//! ```text
//! program        := item*
//! item           := fn_item | const_item | struct_item | enum_item
//! fn_item        := "fn" ident "(" params? ")" ("->" type)? block
//! params         := param ("," param)*
//! param          := ident ":" type
//! const_item     := "const" ident ":" type "=" expr ";"
//! struct_item    := "struct" ident "{" (ident ":" type ";")* "}"
//! enum_item      := "enum" ident "{" (ident ( "(" type ("," type)* ")" )? "," )* "}"
//!
//! block          := "{" stmt* "}"
//! stmt           := "let" ident (":" type)? ("=" expr)? ";"
//!                 | "return" expr? ";"
//!                 | "while" "(" expr ")" block
//!                 | "for" ident "in" expr block
//!                 | "if" "(" expr ")" block ("else" block)?
//!                 | expr ";"
//!
//! expr           := pratt_expression
//! primary        := ident | literal | "(" expr ")" | call | field
//! call           := primary "(" args? ")"
//! field          := primary "." ident
//! literal        := INT | FLOAT | STRING | "true" | "false" | "null"
//! type           := prim_type | ident
//! prim_type      := "i64" | "f64" | "bool" | "str" | "void"
//! ```

#![deny(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]

/* ─────────────────────────── Imports / alloc ─────────────────────────── */

#[cfg(not(feature = "std"))]
extern crate alloc;

use core::fmt;

#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

#[cfg(not(feature = "std"))]
use alloc::{string::String, vec::Vec};

use vitte_ast as ast;
use vitte_core::{Pos, SourceId, Span};
use vitte_lexer::{Keyword, Lexer, LexerOptions, Token, TokenKind};

/* ─────────────────────────── Erreurs ─────────────────────────── */

/// Erreur de parsing avec span.
#[derive(Debug, Clone)]
pub struct ParseError {
    /// Localisation.
    pub span: Span,
    /// Message humain.
    pub message: String,
}

impl ParseError {
    fn new(span: Span, message: impl Into<String>) -> Self {
        Self {
            span,
            message: message.into(),
        }
    }
}

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} @{:?}", self.message, self.span)
    }
}

#[cfg(feature = "std")]
impl std::error::Error for ParseError {}

type PResult<T> = core::result::Result<T, ParseError>;

/* ─────────────────────────── Parser ─────────────────────────── */

/// Parser Vitte.
pub struct Parser<'a> {
    /// Lexer interne.
    lx: Lexer<'a>,
    /// Buffer 1-token d’anticipation.
    look: Option<Token<'a>>,
    /// Dernier span consommé (utile pour composer des spans).
    last_span: Option<Span>,
    /// Source id (propagé dans les spans composés).
    source: SourceId,
}

impl<'a> Parser<'a> {
    /// Crée un parser depuis une source.
    pub fn new(src: &'a str, source: SourceId) -> Self {
        Self::with_options(src, source, LexerOptions::default())
    }

    /// Crée avec options de lexer.
    pub fn with_options(src: &'a str, source: SourceId, opts: LexerOptions) -> Self {
        let mut lx = Lexer::with_options(src, source, opts);
        let look = next_tok(&mut lx).ok().flatten();
        Self {
            lx,
            look,
            last_span: None,
            source,
        }
    }

    /// Parse un programme complet.
    pub fn parse_program(&mut self) -> PResult<ast::Program> {
        let mut items = Vec::new();
        while !self.is(TokenKind::Eof) {
            items.push(self.parse_item()?);
        }
        Ok(ast::Program { items })
    }

    /* ─────────── Items ─────────── */

    fn parse_item(&mut self) -> PResult<ast::Item> {
        match &self.peek()?.value {
            TokenKind::Kw(Keyword::Fn) => self.parse_fn_item(),
            TokenKind::Kw(Keyword::Const) => self.parse_const_item(),
            TokenKind::Kw(Keyword::Struct) => self.parse_struct_item(),
            TokenKind::Kw(Keyword::Enum) => self.parse_enum_item(),
            _ => {
                let t = self.peek()?;
                Err(err_here(&t, "attendu: `fn`, `const`, `struct` ou `enum`"))
            }
        }
    }

    fn parse_fn_item(&mut self) -> PResult<ast::Item> {
        let k = self.expect_kw(Keyword::Fn)?;
        let name = self.expect_ident()?.to_string();
        self.expect(TokenKind::LParen)?;

        let mut params = Vec::new();
        if !self.check(TokenKind::RParen) {
            loop {
                let pname = self.expect_ident()?.to_string();
                self.expect(TokenKind::Colon)?;
                let pty = self.parse_type()?;
                params.push(ast::Param {
                    name: pname,
                    ty: pty,
                    span: None,
                });
                if self.check(TokenKind::Comma) {
                    self.bump();
                    continue;
                }
                break;
            }
        }
        self.expect(TokenKind::RParen)?;
        let ret_ty = if self.check(TokenKind::Arrow) {
            self.bump();
            Some(self.parse_type()?)
        } else {
            None
        };

        let body = self.parse_block()?;
        let fn_span = self.join_span_to_ast(k.span, self.prev_span());
        Ok(ast::Item::Function(ast::Function {
            name,
            params,
            return_type: ret_ty,
            body,
            span: fn_span,
        }))
    }

    fn parse_const_item(&mut self) -> PResult<ast::Item> {
        let k = self.expect_kw(Keyword::Const)?;
        let name = self.expect_ident()?.to_string();
        self.expect(TokenKind::Colon)?;
        let ty = self.parse_type()?;
        self.expect(TokenKind::Eq)?;
        let expr = self.parse_expr()?;
        let semi = self.expect(TokenKind::Semi)?;
        let span = self.join_span_to_ast(k.span, semi.span);
        Ok(ast::Item::Const(ast::ConstDecl {
            name,
            ty: Some(ty),
            value: expr,
            span,
        }))
    }

    fn parse_struct_item(&mut self) -> PResult<ast::Item> {
        let k = self.expect_kw(Keyword::Struct)?;
        let name = self.expect_ident()?.to_string();
        self.expect(TokenKind::LBrace)?;
        let mut fields = Vec::new();
        while !self.check(TokenKind::RBrace) {
            let fname = self.expect_ident()?.to_string();
            self.expect(TokenKind::Colon)?;
            let fty = self.parse_type()?;
            self.expect(TokenKind::Semi)?;
            fields.push(ast::Field {
                name: fname,
                ty: fty,
                span: None,
            });
        }
        let rb = self.expect(TokenKind::RBrace)?;
        Ok(ast::Item::Struct(ast::StructDecl {
            name,
            fields,
            span: self.join_span_to_ast(k.span, rb.span),
        }))
    }

    fn parse_enum_item(&mut self) -> PResult<ast::Item> {
        let k = self.expect_kw(Keyword::Enum)?;
        let name = self.expect_ident()?.to_string();
        self.expect(TokenKind::LBrace)?;
        let mut variants = Vec::new();
        while !self.check(TokenKind::RBrace) {
            let vname = self.expect_ident()?.to_string();
            let mut fields = Vec::new();
            if self.check(TokenKind::LParen) {
                self.bump();
                if !self.check(TokenKind::RParen) {
                    loop {
                        fields.push(self.parse_type()?);
                        if self.check(TokenKind::Comma) {
                            self.bump();
                            continue;
                        }
                        break;
                    }
                }
                self.expect(TokenKind::RParen)?;
            }
            // virgule optionnelle
            if self.check(TokenKind::Comma) {
                self.bump();
            }
            variants.push(ast::EnumVariant {
                name: vname,
                fields,
                span: None,
            });
        }
        let rb = self.expect(TokenKind::RBrace)?;
        Ok(ast::Item::Enum(ast::EnumDecl {
            name,
            variants,
            span: self.join_span_to_ast(k.span, rb.span),
        }))
    }

    /* ─────────── Blocs & Stmts ─────────── */

    fn parse_block(&mut self) -> PResult<ast::Block> {
        let lb = self.expect(TokenKind::LBrace)?;
        let mut stmts = Vec::new();
        while !self.check(TokenKind::RBrace) {
            stmts.push(self.parse_stmt()?);
        }
        let rb = self.expect(TokenKind::RBrace)?;
        Ok(ast::Block {
            stmts,
            span: self.join_span_to_ast(lb.span, rb.span),
        })
    }

    fn parse_stmt(&mut self) -> PResult<ast::Stmt> {
        if self.is_kw(Keyword::Let) {
            let k = self.bump().unwrap();
            let name = self.expect_ident()?.to_string();
            let mut ty = None;
            if self.check(TokenKind::Colon) {
                self.bump();
                ty = Some(self.parse_type()?);
            }
            let mut value = None;
            if self.check(TokenKind::Eq) {
                self.bump();
                value = Some(self.parse_expr()?);
            }
            let semi = self.expect(TokenKind::Semi)?;
            return Ok(ast::Stmt::Let {
                name,
                ty,
                value,
                span: self.join_span_to_ast(k.span, semi.span),
            });
        }
        if self.is_kw(Keyword::Return) {
            let k = self.bump().unwrap();
            let value = if !self.check(TokenKind::Semi) {
                Some(self.parse_expr()?)
            } else {
                None
            };
            let semi = self.expect(TokenKind::Semi)?;
            return Ok(ast::Stmt::Return(value, self.join_span_to_ast(k.span, semi.span)));
        }
        if self.is_kw(Keyword::While) {
            let k = self.bump().unwrap();
            self.expect(TokenKind::LParen)?;
            let cond = self.parse_expr()?;
            self.expect(TokenKind::RParen)?;
            let body = self.parse_block()?;
            return Ok(ast::Stmt::While {
                condition: cond,
                body,
                span: self.join_span_to_ast(k.span, self.prev_span()),
            });
        }
        if self.is_kw(Keyword::For) {
            let k = self.bump().unwrap();
            let var = self.expect_ident()?.to_string();
            // on autorise 'in' comme ident simple ; pour propreté, ajoutons un mot-clé un jour.
            let _in_span = self.expect_ident_text("in")?;
            let iter = self.parse_expr()?;
            let body = self.parse_block()?;
            return Ok(ast::Stmt::For {
                var,
                iter,
                body,
                span: self.join_span_to_ast(k.span, self.prev_span()),
            });
        }
        if self.is_kw(Keyword::If) {
            let k = self.bump().unwrap();
            self.expect(TokenKind::LParen)?;
            let cond = self.parse_expr()?;
            self.expect(TokenKind::RParen)?;
            let then_block = self.parse_block()?;
            let else_block = if self.is_kw(Keyword::Else) {
                self.bump();
                Some(self.parse_block()?)
            } else {
                None
            };
            return Ok(ast::Stmt::If {
                condition: cond,
                then_block,
                else_block,
                span: self.join_span_to_ast(k.span, self.prev_span()),
            });
        }
        // Expression statement
        let e = self.parse_expr()?;
        let semi = self.expect(TokenKind::Semi)?;
        Ok(ast::Stmt::Expr(e).with_span(semi.span))
    }

    /* ─────────── Expressions (Pratt) ─────────── */

    fn parse_expr(&mut self) -> PResult<ast::Expr> {
        self.parse_prec(0)
    }

    // Priorités (0..=7)
    fn parse_prec(&mut self, min_bp: u8) -> PResult<ast::Expr> {
        let mut lhs = self.parse_unary()?;

        loop {
            let op = match self.peek()?.value.clone() {
                TokenKind::OrOr => Bin::Or,
                TokenKind::AndAnd => Bin::And,
                TokenKind::EqEq => Bin::Eq,
                TokenKind::Ne => Bin::Ne,
                TokenKind::Lt => Bin::Lt,
                TokenKind::Le => Bin::Le,
                TokenKind::Gt => Bin::Gt,
                TokenKind::Ge => Bin::Ge,
                TokenKind::Plus => Bin::Add,
                TokenKind::Minus => Bin::Sub,
                TokenKind::Star => Bin::Mul,
                TokenKind::Slash => Bin::Div,
                TokenKind::Percent => Bin::Mod,
                _ => break,
            };
            let (lbp, rbp) = precedence(&op);
            if lbp < min_bp {
                break;
            }
            // consomme l’op
            self.bump();

            let rhs = self.parse_prec(rbp)?;
            let span = span_join(lhs.span(), rhs.span());
            lhs = ast::Expr::Binary {
                left: Box::new(lhs),
                op: op.into(),
                right: Box::new(rhs),
            }
            .with_span(span);
        }

        Ok(lhs)
    }

    fn parse_unary(&mut self) -> PResult<ast::Expr> {
        if self.check(TokenKind::Minus) {
            let op_tok = self.bump().unwrap();
            let e = self.parse_unary()?;
            let span = span_join(op_tok.span, e.span());
            return Ok(ast::Expr::Unary {
                op: ast::UnaryOp::Neg,
                expr: Box::new(e),
            }
            .with_span(span));
        }
        if self.check(TokenKind::Bang) {
            let op_tok = self.bump().unwrap();
            let e = self.parse_unary()?;
            let span = span_join(op_tok.span, e.span());
            return Ok(ast::Expr::Unary {
                op: ast::UnaryOp::Not,
                expr: Box::new(e),
            }
            .with_span(span));
        }
        self.parse_postfix()
    }

    fn parse_postfix(&mut self) -> PResult<ast::Expr> {
        let mut e = self.parse_primary()?;
        loop {
            if self.check(TokenKind::LParen) {
                // appel
                let _lp = self.bump().unwrap();
                let mut args = Vec::new();
                if !self.check(TokenKind::RParen) {
                    loop {
                        args.push(self.parse_expr()?);
                        if self.check(TokenKind::Comma) {
                            self.bump();
                            continue;
                        }
                        break;
                    }
                }
                let rp = self.expect(TokenKind::RParen)?;
                let span = span_join(e.span(), rp.span);
                e = ast::Expr::Call {
                    func: Box::new(e),
                    args,
                }
                .with_span(span);
                continue;
            }
            if self.check(TokenKind::Dot) {
                let _dot = self.bump().unwrap();
                let field = self.expect_ident()?.to_string();
                let span = span_join(e.span(), self.prev_span());
                e = ast::Expr::Field {
                    expr: Box::new(e),
                    field,
                }
                .with_span(span);
                continue;
            }
            break;
        }
        Ok(e)
    }

    fn parse_primary(&mut self) -> PResult<ast::Expr> {
        let t = self.peek()?;
        match &t.value {
            TokenKind::Ident(s) => {
                self.bump();
                Ok(ast::Expr::Ident((*s).to_string()).with_span(t.span))
            }
            TokenKind::Int(i) => {
                self.bump();
                Ok(ast::Expr::Literal(ast::Literal::Int(*i)).with_span(t.span))
            }
            TokenKind::Float(f) => {
                self.bump();
                Ok(ast::Expr::Literal(ast::Literal::Float(*f)).with_span(t.span))
            }
            TokenKind::Str(s) => {
                self.bump();
                Ok(ast::Expr::Literal(ast::Literal::Str(s.clone())).with_span(t.span))
            }
            TokenKind::Kw(Keyword::True) => {
                self.bump();
                Ok(ast::Expr::Literal(ast::Literal::Bool(true)).with_span(t.span))
            }
            TokenKind::Kw(Keyword::False) => {
                self.bump();
                Ok(ast::Expr::Literal(ast::Literal::Bool(false)).with_span(t.span))
            }
            TokenKind::Kw(Keyword::Null) => {
                self.bump();
                Ok(ast::Expr::Literal(ast::Literal::Null).with_span(t.span))
            }
            TokenKind::LParen => {
                let lp = self.bump().unwrap();
                let e = self.parse_expr()?;
                let rp = self.expect(TokenKind::RParen)?;
                Ok(e.with_span(span_join(lp.span, rp.span)))
            }
            _ => Err(err_here(&t, "expression attendue")),
        }
    }

    /* ─────────── Types ─────────── */

    fn parse_type(&mut self) -> PResult<ast::Type> {
        // type = prim | ident
        if let Ok(t) = self.peek() {
            // ident sous-jacent
            match &t.value {
                TokenKind::Ident(s) => {
                    let name = *s;
                    self.bump();
                    return Ok(match name {
                        "i64" | "int" => ast::Type::Int,
                        "f64" | "float" => ast::Type::Float,
                        "bool" => ast::Type::Bool,
                        "str" | "string" => ast::Type::Str,
                        "void" => ast::Type::Void,
                        other => ast::Type::Custom(other.to_string()),
                    });
                }
                _ => {}
            }
        }
        let t = self.peek()?;
        Err(err_here(&t, "type attendu"))
    }

    /* ─────────── Utilitaires ─────────── */

    #[inline]
    fn peek(&mut self) -> PResult<Token<'a>> {
        if let Some(t) = &self.look {
            return Ok(t.clone());
        }
        let nt = next_tok(&mut self.lx).map_err(|e| ParseError::new(e.span, e.to_string()))?;
        self.look = nt.clone();
        Ok(nt.ok_or_else(|| {
            ParseError::new(
                Span {
                    source: self.source,
                    start: Pos(0),
                    end: Pos(0),
                },
                "EOF",
            )
        })?)
    }

    #[inline]
    fn bump(&mut self) -> Option<Token<'a>> {
        let cur = self.look.take();
        if let Some(tok) = cur.as_ref() {
            self.last_span = Some(tok.span);
        }
        self.look = next_tok(&mut self.lx).ok().flatten();
        cur
    }

    #[inline]
    fn check(&mut self, kind: TokenKind<'_>) -> bool {
        matches!(self.look.as_ref().map(|t| &t.value), Some(k) if token_eq(k, &kind))
    }

    #[inline]
    fn is(&mut self, kind: TokenKind<'_>) -> bool {
        self.check(kind)
    }

    #[inline]
    fn is_kw(&mut self, kw: Keyword) -> bool {
        matches!(self.look.as_ref().map(|t| &t.value), Some(TokenKind::Kw(k)) if *k == kw)
    }

    fn expect(&mut self, kind: TokenKind<'_>) -> PResult<Token<'a>> {
        let t = self.peek()?;
        if token_eq(&t.value, &kind) {
            return Ok(self.bump().unwrap());
        }
        Err(err_here(&t, format!("attendu: {:?}", kind)))
    }

    fn expect_kw(&mut self, kw: Keyword) -> PResult<Token<'a>> {
        let t = self.peek()?;
        if matches!(t.value, TokenKind::Kw(k) if k == kw) {
            return Ok(self.bump().unwrap());
        }
        Err(err_here(&t, format!("mot-clé attendu: {:?}", kw)))
    }

    fn expect_ident(&mut self) -> PResult<&'a str> {
        let t = self.peek()?;
        if let TokenKind::Ident(s) = t.value {
            self.bump();
            Ok(s)
        } else {
            Err(err_here(&t, "identifiant attendu"))
        }
    }

    fn expect_ident_text(&mut self, s: &str) -> PResult<Span> {
        let t = self.peek()?;
        if let TokenKind::Ident(got) = t.value {
            if got == s {
                self.bump();
                return Ok(t.span);
            }
        }
        Err(err_here(&t, format!("identifiant `{}` attendu", s)))
    }

    #[inline]
    fn prev_span(&self) -> Span {
        self.last_span.unwrap_or(Span {
            source: self.source,
            start: Pos(0),
            end: Pos(0),
        })
    }

    fn join_span_to_ast(&self, start: Span, end: Span) -> Option<ast::Span> {
        if start.source != end.source {
            return None;
        }
        let joined = span_join(start, end);
        Some(self.ast_span(joined))
    }

    fn ast_span(&self, span: Span) -> ast::Span {
        let (line, column) = self.lx.lines.line_col(span.start);
        ast::Span::new(line, column, span.start.0)
    }
}

/* ─────────────────────────── Opérateurs & helpers ─────────────────────────── */

#[derive(Debug, Clone, Copy)]
enum Bin {
    Or,
    And,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    Add,
    Sub,
    Mul,
    Div,
    Mod,
}

impl From<Bin> for ast::BinaryOp {
    fn from(b: Bin) -> Self {
        use ast::BinaryOp::*;
        match b {
            Bin::Or => Or,
            Bin::And => And,
            Bin::Eq => Eq,
            Bin::Ne => Ne,
            Bin::Lt => Lt,
            Bin::Le => Le,
            Bin::Gt => Gt,
            Bin::Ge => Ge,
            Bin::Add => Add,
            Bin::Sub => Sub,
            Bin::Mul => Mul,
            Bin::Div => Div,
            Bin::Mod => Mod,
        }
    }
}

fn precedence(op: &Bin) -> (u8, u8) {
    // Pratt binding power (gauche-associatif)
    match op {
        Bin::Or => (1, 2),
        Bin::And => (3, 4),
        Bin::Eq | Bin::Ne => (5, 6),
        Bin::Lt | Bin::Le | Bin::Gt | Bin::Ge => (7, 8),
        Bin::Add | Bin::Sub => (9, 10),
        Bin::Mul | Bin::Div | Bin::Mod => (11, 12),
    }
}

fn token_eq(a: &TokenKind<'_>, b: &TokenKind<'_>) -> bool {
    use TokenKind::*;
    match (a, b) {
        (Eof, Eof)
        | (LParen, LParen)
        | (RParen, RParen)
        | (LBrace, LBrace)
        | (RBrace, RBrace)
        | (Comma, Comma)
        | (Dot, Dot)
        | (Semi, Semi)
        | (Colon, Colon)
        | (PathSep, PathSep)
        | (Arrow, Arrow)
        | (FatArrow, FatArrow)
        | (Plus, Plus)
        | (Minus, Minus)
        | (Star, Star)
        | (Slash, Slash)
        | (Percent, Percent)
        | (Eq, Eq)
        | (EqEq, EqEq)
        | (Ne, Ne)
        | (Lt, Lt)
        | (Le, Le)
        | (Gt, Gt)
        | (Ge, Ge)
        | (AndAnd, AndAnd)
        | (OrOr, OrOr)
        | (Bang, Bang)
        | (LBracket, LBracket)
        | (RBracket, RBracket) => true,
        (Kw(ka), Kw(kb)) => ka == kb,
        _ => false,
    }
}

fn err_here(tok: &Token<'_>, message: impl Into<String>) -> ParseError {
    ParseError::new(tok.span, message)
}

fn span_join(a: Span, b: Span) -> Span {
    Span {
        source: a.source,
        start: a.start,
        end: b.end,
    }
}

fn next_tok<'a>(lx: &mut Lexer<'a>) -> Result<Option<Token<'a>>, vitte_lexer::LexError> {
    lx.next()
}

/* ─────────────────────────── Trait util pour .with_span() ─────────────────────────── */

trait WithSpan {
    fn with_span(self, span: Span) -> Self;
    fn span(&self) -> Span;
}

impl WithSpan for ast::Expr {
    fn with_span(mut self, _span: Span) -> Self {
        match &mut self {
            ast::Expr::Ident(_) => {}
            ast::Expr::Literal(_) => {}
            ast::Expr::Call { .. } => {}
            ast::Expr::Binary { .. } => {}
            ast::Expr::Unary { .. } => {}
            ast::Expr::Field { .. } => {}
        }
        // si ton ast::Expr a un champ span, adapte ici ; sinon on ignore (non destructif)
        self
    }
    fn span(&self) -> Span {
        // si ton ast::Expr stocke un span, renvoie-le ; sinon span nul
        Span {
            source: SourceId(0),
            start: Pos(0),
            end: Pos(0),
        }
    }
}

impl WithSpan for ast::Stmt {
    fn with_span(self, _span: Span) -> Self {
        self
    }
    fn span(&self) -> Span {
        Span {
            source: SourceId(0),
            start: Pos(0),
            end: Pos(0),
        }
    }
}

/* ─────────────────────────── Tests ─────────────────────────── */

#[cfg(test)]
mod tests {
    use super::*;

    fn parse_ok(src: &str) -> ast::Program {
        let mut p = Parser::new(src, SourceId(0));
        p.parse_program().expect("parse ok")
    }

    #[test]
    fn fn_simple() {
        let prg = parse_ok(
            r#"
            fn add(a: i64, b: i64) -> i64 { return a + b; }
        "#,
        );
        assert!(!prg.items.is_empty());
    }

    #[test]
    fn const_struct_enum() {
        let _ = parse_ok(
            r#"
            const N: i64 = 42;
            struct P { x: i64; y: i64; }
            enum E { A, B(i64, f64), C, }
            fn main() { let x: i64 = N; }
        "#,
        );
    }

    #[test]
    fn control_flow() {
        let _ = parse_ok(
            r#"
            fn f() -> i64 {
                let mutx: i64 = 0
                ; // on tolère le ; libre
                let x = 1 + 2 * 3;
                if (x > 0) { return x; } else { return 0; }
                while (x > 10) { return x; }
                for i in 0 { return 1; }
            }
        "#,
        );
    }

    #[test]
    fn calls_and_fields() {
        let _ = parse_ok(
            r#"
            fn f() {
                foo(1, 2 + 3).bar.baz(4);
            }
        "#,
        );
    }
}
