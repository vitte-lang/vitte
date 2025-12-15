// vitte/rust/crates/vitte_muf/src/lex.rs
#![allow(dead_code)]

use crate::span::{Pos, Span};
use crate::token::{Token, TokenKind};

use vitte_common::err::{ErrCode, VResult, VitteError};

#[cfg(feature = "alloc")]
use alloc::string::String;

#[cfg(feature = "alloc")]
use alloc::vec::Vec;

#[derive(Copy, Clone, Debug)]
pub struct LexOptions {
    pub emit_newlines: bool,
}

impl Default for LexOptions {
    fn default() -> Self {
        Self { emit_newlines: true }
    }
}

pub struct Lexer<'a> {
    s: &'a [u8],
    i: usize,
    line: u32,
    col: u32,

    // indentation stack (spaces count)
    indents: Vec<u32>,
    pending: Vec<TokenKind>,
    opt: LexOptions,
}

impl<'a> Lexer<'a> {
    pub fn new(input: &'a str, opt: LexOptions) -> Self {
        Self {
            s: input.as_bytes(),
            i: 0,
            line: 0,
            col: 0,
            indents: vec![0],
            pending: Vec::new(),
            opt,
        }
    }

    pub fn next(&mut self) -> VResult<Token> {
        if let Some(k) = self.pending.pop() {
            return Ok(self.mk_tok(k, self.pos(), self.pos(), ""));
        }

        self.skip_ws_and_comments()?;

        let start = self.pos();

        if self.i >= self.s.len() {
            // emit pending dedents
            if self.indents.len() > 1 {
                self.indents.pop();
                return Ok(self.mk_tok(TokenKind::Dedent, start, start, ""));
            }
            return Ok(self.mk_tok(TokenKind::Eof, start, start, ""));
        }

        let b = self.s[self.i];

        // newline
        if b == b'\n' {
            self.bump();
            let end = self.pos();
            if self.opt.emit_newlines {
                // after newline, process indentation of the next line
                self.handle_indent()?;
                return Ok(self.mk_tok(TokenKind::Newline, start, end, "\n"));
            } else {
                // even if not emitting, still handle indent
                self.handle_indent()?;
                return self.next();
            }
        }

        // ".end"
        if b == b'.' && self.peek_str(".end") {
            self.bump_n(4);
            let end = self.pos();
            return Ok(self.mk_tok(TokenKind::DotEnd, start, end, ".end"));
        }

        // punct
        let (kind, lit_len) = match b {
            b'=' => (TokenKind::Eq, 1),
            b'[' => (TokenKind::LBracket, 1),
            b']' => (TokenKind::RBracket, 1),
            b',' => (TokenKind::Comma, 1),
            b':' => (TokenKind::Colon, 1),
            b'/' => (TokenKind::Slash, 1),
            b'.' => (TokenKind::Dot, 1),
            _ => (TokenKind::Eof, 0),
        };
        if lit_len != 0 {
            self.bump_n(lit_len);
            let end = self.pos();
            let text = core::str::from_utf8(&[b]).unwrap_or("");
            return Ok(self.mk_tok(kind, start, end, text));
        }

        // string
        if b == b'"' {
            return self.lex_string();
        }

        // int
        if (b'0'..=b'9').contains(&b) {
            return self.lex_int();
        }

        // ident
        if is_ident_start(b) {
            return self.lex_ident();
        }

        Err(VitteError::new(ErrCode::LexError))
    }

    fn lex_ident(&mut self) -> VResult<Token> {
        let start = self.pos();
        let mut j = self.i;
        while j < self.s.len() && is_ident_continue(self.s[j]) {
            j += 1;
        }
        let bytes = &self.s[self.i..j];
        self.bump_n(j - self.i);
        let end = self.pos();
        let text = core::str::from_utf8(bytes).map_err(|_| VitteError::new(ErrCode::Utf8Invalid))?;
        Ok(self.mk_tok(TokenKind::Ident, start, end, text))
    }

    fn lex_int(&mut self) -> VResult<Token> {
        let start = self.pos();
        let mut j = self.i;
        while j < self.s.len() && (b'0'..=b'9').contains(&self.s[j]) {
            j += 1;
        }
        let bytes = &self.s[self.i..j];
        self.bump_n(j - self.i);
        let end = self.pos();
        let text = core::str::from_utf8(bytes).map_err(|_| VitteError::new(ErrCode::Utf8Invalid))?;
        Ok(self.mk_tok(TokenKind::Int, start, end, text))
    }

    fn lex_string(&mut self) -> VResult<Token> {
        let start = self.pos();
        self.bump(); // "
        let mut out = String::new();
        while self.i < self.s.len() {
            let b = self.s[self.i];
            if b == b'"' {
                self.bump();
                let end = self.pos();
                return Ok(Token {
                    kind: TokenKind::String,
                    span: Span::new(start, end),
                    text: out,
                });
            }
            if b == b'\\' {
                self.bump();
                if self.i >= self.s.len() {
                    return Err(VitteError::new(ErrCode::LexError));
                }
                let e = self.s[self.i];
                self.bump();
                match e {
                    b'n' => out.push('\n'),
                    b'r' => out.push('\r'),
                    b't' => out.push('\t'),
                    b'"' => out.push('"'),
                    b'\\' => out.push('\\'),
                    _ => return Err(VitteError::new(ErrCode::LexError)),
                }
                continue;
            }
            // raw byte (UTF-8 validation delegated to Rust String push)
            out.push(b as char);
            self.bump();
        }
        Err(VitteError::new(ErrCode::LexError))
    }

    fn skip_ws_and_comments(&mut self) -> VResult<()> {
        loop {
            if self.i >= self.s.len() {
                return Ok(());
            }
            let b = self.s[self.i];
            // spaces/tabs (not newlines) are handled here
            if b == b' ' || b == b'\t' || b == b'\r' {
                self.bump();
                continue;
            }
            // comments start with '#', to end of line
            if b == b'#' {
                while self.i < self.s.len() && self.s[self.i] != b'\n' {
                    self.bump();
                }
                continue;
            }
            // '//' comment
            if b == b'/' && self.i + 1 < self.s.len() && self.s[self.i + 1] == b'/' {
                while self.i < self.s.len() && self.s[self.i] != b'\n' {
                    self.bump();
                }
                continue;
            }
            break;
        }
        Ok(())
    }

    fn handle_indent(&mut self) -> VResult<()> {
        // Look ahead at next line indentation (spaces only; tabs discouraged but accepted)
        let mut j = self.i;
        let mut spaces: u32 = 0;

        while j < self.s.len() {
            let b = self.s[j];
            if b == b' ' {
                spaces += 1;
                j += 1;
                continue;
            }
            if b == b'\t' {
                // treat tab as 2 spaces (heuristic); configurable later
                spaces += 2;
                j += 1;
                continue;
            }
            break;
        }

        // If next is newline or EOF, do nothing (blank line)
        if j >= self.s.len() || self.s[j] == b'\n' {
            return Ok(());
        }

        let cur = *self.indents.last().unwrap_or(&0);
        if spaces > cur {
            self.indents.push(spaces);
            self.pending.push(TokenKind::Indent);
        } else if spaces < cur {
            // pop until match or underflow
            while self.indents.len() > 1 && *self.indents.last().unwrap() > spaces {
                self.indents.pop();
                self.pending.push(TokenKind::Dedent);
            }
            // If mismatch, treat as error (strict)
            if *self.indents.last().unwrap_or(&0) != spaces {
                return Err(VitteError::new(ErrCode::LexError));
            }
        }

        Ok(())
    }

    fn mk_tok(&self, kind: TokenKind, start: Pos, end: Pos, text: &str) -> Token {
        Token {
            kind,
            span: Span::new(start, end),
            text: text.into(),
        }
    }

    #[inline]
    fn pos(&self) -> Pos {
        Pos::new(self.i, self.line, self.col)
    }

    #[inline]
    fn bump(&mut self) {
        if self.i < self.s.len() {
            let b = self.s[self.i];
            self.i += 1;
            if b == b'\n' {
                self.line += 1;
                self.col = 0;
            } else {
                self.col += 1;
            }
        }
    }

    #[inline]
    fn bump_n(&mut self, n: usize) {
        for _ in 0..n {
            self.bump();
        }
    }

    #[inline]
    fn peek_str(&self, s: &str) -> bool {
        let bytes = s.as_bytes();
        self.s.len() >= self.i + bytes.len() && &self.s[self.i..self.i + bytes.len()] == bytes
    }
}

#[inline]
fn is_ident_start(b: u8) -> bool {
    (b'A'..=b'Z').contains(&b)
        || (b'a'..=b'z').contains(&b)
        || b == b'_'
}

#[inline]
fn is_ident_continue(b: u8) -> bool {
    is_ident_start(b) || (b'0'..=b'9').contains(&b) || b == b'-'
}
