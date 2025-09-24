//! vitte-lexer — analyse lexicale pour Vitte (version ++)
//!
//! Faits saillants :
//! - `Lexer` + `LexerOptions` : commentaires `//`, `/* */` (imbriqués), ident/keywords, nombres
//!   (2/8/10/16, `_`, floats + exposant), **chaînes** avec échappements, **raw strings** à la Rust
//!   (`r".."`, `r#".."#`, …), **char literal** `'a'`, `'\n'`, etc.
//! - `Span`/`Spanned`/`SourceId` + **LineMap** pour `(ligne, colonne)`
//! - Erreurs avec `Display` propre + spans précis
//!
//! Exemple éclair :
//! ```
//! use vitte_core::SourceId;
//! use vitte_lexer::{Lexer, LexerOptions, TokenKind};
//!
//! let src = r#"let s = r#"multi " raw"#; let c = '\n';"#;
//! let opts = LexerOptions::default();
//! let mut lx = Lexer::with_options(src, SourceId(0), opts);
//! while let Some(tok) = lx.next().transpose().unwrap() {
//!     if matches!(tok.value, TokenKind::Eof) { break; }
//!     // println!("{:?} @ {:?}", tok.value, tok.span);
//! }
//! ```

#![deny(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]

use core::fmt;

#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

#[cfg(not(feature = "std"))]
use alloc::{string::String, vec::Vec};
#[cfg(not(feature = "std"))]
extern crate alloc;

use vitte_core::{Pos, SourceId, Span, Spanned};

/* ─────────────────────────── Options & LineMap ─────────────────────────── */

/// Options du lexer.
#[derive(Debug, Clone, Copy)]
pub struct LexerOptions {
    /// Autoriser commentaires blocs imbriqués `/* ... /* .. */ ... */`.
    pub nested_block_comments: bool,
    /// Autoriser raw strings `r".."`, `r#".."#`, …
    pub raw_strings: bool,
    /// Nombre max de `#` autorisés autour d’un raw string (anti-DoS).
    pub max_raw_hashes: u8,
}

impl Default for LexerOptions {
    fn default() -> Self {
        Self {
            nested_block_comments: true,
            raw_strings: true,
            max_raw_hashes: 16,
        }
    }
}

/// Table des lignes pour (byte offset) → (ligne, colonne).
#[derive(Debug, Clone)]
pub struct LineMap {
    /// Offsets des débuts de lignes (toujours contient 0).
    pub line_starts: Vec<u32>,
}

impl LineMap {
    /// Construit la table à partir d’un `&str`.
    pub fn new(src: &str) -> Self {
        let mut ls = Vec::with_capacity(64);
        ls.push(0);
        for (i, b) in src.as_bytes().iter().enumerate() {
            if *b == b'\n' {
                ls.push((i as u32) + 1);
            }
        }
        Self { line_starts: ls }
    }

    /// Convertit un `Pos` en (ligne, colonne), 1-based.
    pub fn line_col(&self, pos: Pos) -> (u32, u32) {
        let off = pos.0;
        // binary search
        let idx = match self.line_starts.binary_search(&off) {
            Ok(i) => i,
            Err(i) => i.saturating_sub(1),
        };
        let line_start = self.line_starts[idx];
        let col = off.saturating_sub(line_start) + 1;
        ((idx as u32) + 1, col)
    }

    /// Convertit un `Span` en ((l1,c1), (l2,c2)).
    pub fn span_lines(&self, sp: Span) -> ((u32, u32), (u32, u32)) {
        (self.line_col(sp.start), self.line_col(sp.end))
    }
}

/* ─────────────────────────── Tokens ─────────────────────────── */

/// Mots-clés reconnus.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Keyword {
    /// `fn`
    Fn,
    /// `let`
    Let,
    /// `const`
    Const,
    /// `if`
    If,
    /// `else`
    Else,
    /// `while`
    While,
    /// `for`
    For,
    /// `return`
    Return,
    /// `struct`
    Struct,
    /// `enum`
    Enum,
    /// `true`
    True,
    /// `false`
    False,
    /// `null`
    Null,
}

/// Genre de jeton lexical.
#[derive(Debug, Clone, PartialEq)]
pub enum TokenKind<'a> {
    /// Fin de fichier.
    Eof,
    /// Identifiant (ou mot-clé reclassé dans `Keyword`).
    Ident(&'a str),
    /// Mot-clé.
    Kw(Keyword),
    /// Littéral entier (i64).
    Int(i64),
    /// Littéral flottant (f64).
    Float(f64),
    /// Littéral char (un seul scalaire Unicode).
    Char(char),
    /// Littéral chaîne (décodée).
    Str(String),
    /// Symbole `(`
    LParen,
    /// Symbole `)`
    RParen,
    /// Symbole `{`
    LBrace,
    /// Symbole `}`
    RBrace,
    /// Symbole `[`
    LBracket,
    /// Symbole `]`
    RBracket,
    /// `,`
    Comma,
    /// `.`
    Dot,
    /// `;`
    Semi,
    /// `:`
    Colon,
    /// `::`
    PathSep,
    /// `->`
    Arrow,
    /// `=>`
    FatArrow,
    /// `+`
    Plus,
    /// `-`
    Minus,
    /// `*`
    Star,
    /// `/`
    Slash,
    /// `%`
    Percent,
    /// `=`
    Eq,
    /// `==`
    EqEq,
    /// `!=`
    Ne,
    /// `<`
    Lt,
    /// `<=`
    Le,
    /// `>`
    Gt,
    /// `>=`
    Ge,
    /// `&&`
    AndAnd,
    /// `||`
    OrOr,
    /// `!`
    Bang,
}

/// Jeton avec span.
pub type Token<'a> = Spanned<TokenKind<'a>>;

/* ─────────────────────────── Erreurs ─────────────────────────── */

/// Genre d’erreur lexicale.
#[derive(Debug, Clone, PartialEq)]
pub enum LexErrorKind {
    /// Caractère inattendu.
    UnexpectedChar(char),
    /// Commentaire bloc non terminé.
    UnterminatedBlockComment,
    /// Chaîne non terminée.
    UnterminatedString,
    /// Raw string non terminée (compte de `#` non respecté).
    UnterminatedRawString,
    /// Séquence d’échappement invalide.
    InvalidEscape,
    /// Littéral numérique invalide.
    InvalidNumber,
    /// Dépassement entier i64.
    IntOverflow,
    /// Littéral char invalide.
    InvalidCharLiteral,
    /// Trop de `#` dans un raw string (protège contre abus).
    TooManyRawHashes,
}

/// Erreur lexicale avec localisation.
#[derive(Debug, Clone, PartialEq)]
pub struct LexError {
    /// Localisation.
    pub span: Span,
    /// Genre d’erreur.
    pub kind: LexErrorKind,
}

impl fmt::Display for LexError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use LexErrorKind::*;
        match &self.kind {
            UnexpectedChar(c) => write!(f, "unexpected character: {c:?}"),
            UnterminatedBlockComment => write!(f, "unterminated block comment"),
            UnterminatedString => write!(f, "unterminated string literal"),
            UnterminatedRawString => write!(f, "unterminated raw string"),
            InvalidEscape => write!(f, "invalid escape sequence"),
            InvalidNumber => write!(f, "invalid number literal"),
            IntOverflow => write!(f, "integer literal overflows i64"),
            InvalidCharLiteral => write!(f, "invalid char literal"),
            TooManyRawHashes => write!(f, "too many raw string hashes"),
        }
    }
}
#[cfg(feature = "std")]
impl std::error::Error for LexError {}

/* ─────────────────────────── Lexer ─────────────────────────── */

/// Analyseur lexical (itératif).
pub struct Lexer<'a> {
    src: &'a str,
    bytes: &'a [u8],
    /// Position courante en bytes.
    off: usize,
    /// Id de la source.
    source: SourceId,
    /// Options.
    opts: LexerOptions,
    /// Table des lignes (exposée pour diagnostics).
    pub lines: LineMap,
}

impl<'a> Lexer<'a> {
    /// Crée un lexer avec options par défaut.
    pub fn new(src: &'a str, source: SourceId) -> Self {
        Self::with_options(src, source, LexerOptions::default())
    }

    /// Crée un lexer avec `LexerOptions`.
    pub fn with_options(src: &'a str, source: SourceId, opts: LexerOptions) -> Self {
        Self { src, bytes: src.as_bytes(), off: 0, source, opts, lines: LineMap::new(src) }
    }

    /// Prochain jeton (ou `None` si **vraiment** rien — ici on émet toujours `Eof`).
    pub fn next(&mut self) -> Result<Option<Token<'a>>, LexError> {
        self.skip_ws_and_comments()?;
        if self.is_eof() {
            let span = self.span_here(0);
            return Ok(Some(Spanned { value: TokenKind::Eof, span }));
        }
        let start = self.off;
        let c = self.bump_char().unwrap(); // safe: non EOF

        let kind = match c {
            ch if is_ident_start(ch) => {
                self.consume_while(|b| is_ident_continue(b as char));
                let s = &self.src[start..self.off];
                match keyword_of(s) {
                    Some(kw) => TokenKind::Kw(kw),
                    None => TokenKind::Ident(s),
                }
            }
            ch if ch.is_ascii_digit() => self.lex_number(start, c)?,
            '"' => TokenKind::Str(self.lex_string(start)?),
            'r' if self.opts.raw_strings && self.peek_char() == Some('"') || self.peek_char() == Some('#') => {
                // 'r' déjà consommé ; on tente raw string
                self.lex_raw_string(start)?
            }
            '\'' => TokenKind::Char(self.lex_char(start)?),

            ':' => if self.eat(':') { TokenKind::PathSep } else { TokenKind::Colon },
            '-' => if self.eat('>') { TokenKind::Arrow } else { TokenKind::Minus },
            '=' => if self.eat('>') { TokenKind::FatArrow } else if self.eat('=') { TokenKind::EqEq } else { TokenKind::Eq },
            '&' => if self.eat('&') { TokenKind::AndAnd } else { return Err(self.err_here(LexErrorKind::UnexpectedChar('&'))); },
            '|' => if self.eat('|') { TokenKind::OrOr } else { return Err(self.err_here(LexErrorKind::UnexpectedChar('|'))); },
            '!' => if self.eat('=') { TokenKind::Ne } else { TokenKind::Bang },
            '<' => if self.eat('=') { TokenKind::Le } else { TokenKind::Lt },
            '>' => if self.eat('=') { TokenKind::Ge } else { TokenKind::Gt },

            '+' => TokenKind::Plus,
            '*' => TokenKind::Star,
            '/' => TokenKind::Slash,
            '%' => TokenKind::Percent,
            '(' => TokenKind::LParen,
            ')' => TokenKind::RParen,
            '{' => TokenKind::LBrace,
            '}' => TokenKind::RBrace,
            '[' => TokenKind::LBracket,
            ']' => TokenKind::RBracket,
            ',' => TokenKind::Comma,
            ';' => TokenKind::Semi,
            '.' => TokenKind::Dot,

            other => return Err(self.err_spanned(start, LexErrorKind::UnexpectedChar(other))),
        };

        Ok(Some(Spanned { value: kind, span: self.span_from(start) }))
    }

    /// Tokenise toute la source (ajoute `Eof` final).
    pub fn tokenize(mut self) -> Result<Vec<Token<'a>>, LexError> {
        let mut out = Vec::new();
        loop {
            match self.next()? {
                Some(t) => {
                    let is_eof = matches!(t.value, TokenKind::Eof);
                    out.push(t);
                    if is_eof { break; }
                }
                None => break,
            }
        }
        Ok(out)
    }

    /* ────────── Primitives internes ────────── */

    #[inline] fn is_eof(&self) -> bool { self.off >= self.bytes.len() }
    #[inline] fn peek(&self) -> Option<u8> { self.bytes.get(self.off).copied() }
    #[inline] fn peek_char(&self) -> Option<char> { self.peek().map(|b| b as char) }
    #[inline] fn peek2(&self) -> Option<u8> { self.bytes.get(self.off + 1).copied() }
    #[inline] fn bump(&mut self) -> Option<u8> { let b = self.peek(); if b.is_some(){ self.off+=1; } b }
    #[inline] fn bump_char(&mut self) -> Option<char> { self.bump().map(|b| b as char) }
    #[inline] fn eat(&mut self, ch: char) -> bool { if self.peek_char()==Some(ch){ self.off+=1; true } else { false } }

    fn consume_while(&mut self, mut p: impl FnMut(u8) -> bool) {
        while let Some(b) = self.peek() {
            if p(b) { self.off += 1; } else { break; }
        }
    }

    fn skip_ws_and_comments(&mut self) -> Result<(), LexError> {
        loop {
            while let Some(c) = self.peek_char() {
                if c.is_whitespace() { self.off += 1; } else { break; }
            }
            if self.peek_char() == Some('/') && self.peek2() == Some(b'/') {
                self.off += 2;
                while let Some(c) = self.peek_char() {
                    self.off += 1;
                    if c == '\n' { break; }
                }
                continue;
            }
            if self.peek_char() == Some('/') && self.peek2() == Some(b'*') {
                self.off += 2;
                let start = self.off.saturating_sub(2);
                let mut depth = 1u32;
                loop {
                    if self.is_eof() { return Err(self.err_from(start, LexErrorKind::UnterminatedBlockComment)); }
                    if self.opts.nested_block_comments && self.peek_char() == Some('/') && self.peek2() == Some(b'*') {
                        self.off += 2; depth += 1; continue;
                    }
                    if self.peek_char() == Some('*') && self.peek2() == Some(b'/') {
                        self.off += 2; depth -= 1;
                        if depth == 0 { break; }
                        continue;
                    }
                    self.off += 1;
                }
                continue;
            }
            break;
        }
        Ok(())
    }

    fn lex_string(&mut self, start_quote: usize) -> Result<String, LexError> {
        let mut out = String::new();
        loop {
            let c = self.bump_char().ok_or_else(|| self.err_from(start_quote, LexErrorKind::UnterminatedString))?;
            match c {
                '"' => break,
                '\\' => {
                    let esc = self.bump_char().ok_or_else(|| self.err_here(LexErrorKind::UnterminatedString))?;
                    match esc {
                        '"' => out.push('"'),
                        '\\' => out.push('\\'),
                        'n' => out.push('\n'),
                        'r' => out.push('\r'),
                        't' => out.push('\t'),
                        '0' => out.push('\0'),
                        'x' => {
                            let h1 = self.bump_char().ok_or_else(|| self.err_here(LexErrorKind::InvalidEscape))?;
                            let h2 = self.bump_char().ok_or_else(|| self.err_here(LexErrorKind::InvalidEscape))?;
                            let v = (hex_val(h1).ok_or_else(|| self.err_here(LexErrorKind::InvalidEscape))? << 4)
                                | hex_val(h2).ok_or_else(|| self.err_here(LexErrorKind::InvalidEscape))?;
                            out.push(v as char);
                        }
                        'u' => out.push(self.read_unicode_escape()?),
                        _ => return Err(self.err_here(LexErrorKind::InvalidEscape)),
                    }
                }
                other => out.push(other),
            }
        }
        Ok(out)
    }

    fn lex_raw_string(&mut self, start_r: usize) -> Result<TokenKind<'a>, LexError> {
        // On a déjà lu 'r'. Lit 0..=N '#' puis un '"', jusqu’au même nombre de '#' et un '"'.
        if !self.opts.raw_strings {
            return Err(self.err_from(start_r, LexErrorKind::UnexpectedChar('r')));
        }
        // Compter les '#'
        let mut hashes: u8 = 0;
        while self.peek_char() == Some('#') {
            hashes = hashes.saturating_add(1);
            if hashes > self.opts.max_raw_hashes { return Err(self.err_from(start_r, LexErrorKind::TooManyRawHashes)); }
            self.off += 1;
        }
        if !self.eat('"') {
            // ce n’était pas un raw string, c’était juste un ident "r..."
            // on recule ? trop tard. On choisit l’erreur claire :
            return Err(self.err_from(start_r, LexErrorKind::InvalidNumber)); // faute bénigne : “rX” inattendu
        }
        let content_start = self.off;
        // Chercher la fin : un `"` suivi de `hashes` fois '#'
        loop {
            if self.is_eof() { return Err(self.err_from(start_r, LexErrorKind::UnterminatedRawString)); }
            if self.peek_char() == Some('"') {
                // check hashes
                let mut ok = true;
                let mut i = 0;
                while i < hashes {
                    if self.bytes.get(self.off + 1 + (i as usize)) != Some(&b'#') { ok = false; break; }
                    i += 1;
                }
                if ok {
                    // consume closing '"' and hashes
                    self.off += 1 + (hashes as usize);
                    break;
                }
            }
            self.off += 1;
        }
        let s = &self.src[content_start..self.off - (hashes as usize) - 1];
        Ok(TokenKind::Str(s.to_string()))
    }

    fn lex_char(&mut self, start_quote: usize) -> Result<char, LexError> {
        // Lit un littéral char : `'a'`, `'\n'`, `'\x41'`, `'\u{1F600}'`
        let ch = match self.bump_char().ok_or_else(|| self.err_from(start_quote, LexErrorKind::InvalidCharLiteral))? {
            '\\' => {
                let e = self.bump_char().ok_or_else(|| self.err_here(LexErrorKind::InvalidCharLiteral))?;
                match e {
                    '\'' => '\'',
                    '"'  => '"',
                    '\\' => '\\',
                    'n'  => '\n',
                    'r'  => '\r',
                    't'  => '\t',
                    '0'  => '\0',
                    'x'  => {
                        let h1 = self.bump_char().ok_or_else(|| self.err_here(LexErrorKind::InvalidEscape))?;
                        let h2 = self.bump_char().ok_or_else(|| self.err_here(LexErrorKind::InvalidEscape))?;
                        let v = (hex_val(h1).ok_or_else(|| self.err_here(LexErrorKind::InvalidEscape))? << 4)
                            | hex_val(h2).ok_or_else(|| self.err_here(LexErrorKind::InvalidEscape))?;
                        v as char
                    }
                    'u'  => self.read_unicode_escape()?,
                    _    => return Err(self.err_here(LexErrorKind::InvalidCharLiteral)),
                }
            }
            c => c,
        };
        if !self.eat('\'') { return Err(self.err_here(LexErrorKind::InvalidCharLiteral)); }
        Ok(ch)
    }

    fn read_unicode_escape(&mut self) -> Result<char, LexError> {
        if !self.eat('{') { return Err(self.err_here(LexErrorKind::InvalidEscape)); }
        let start = self.off;
        while let Some(c) = self.peek_char() {
            if c == '}' { break; }
            if !(c.is_ascii_hexdigit() || c == '_') { return Err(self.err_here(LexErrorKind::InvalidEscape)); }
            self.off += 1;
        }
        if !self.eat('}') { return Err(self.err_here(LexErrorKind::InvalidEscape)); }
        let raw = &self.src[start..self.off].replace('_', "");
        let v = u32::from_str_radix(raw, 16).map_err(|_| self.err_here(LexErrorKind::InvalidEscape))?;
        core::char::from_u32(v).ok_or_else(|| self.err_here(LexErrorKind::InvalidEscape))
    }

    fn lex_number(&mut self, start: usize, first: char) -> Result<TokenKind<'a>, LexError> {
        if first == '0' {
            if self.peek_char().map(|c| c == 'x' || c == 'X').unwrap_or(false) {
                self.off += 1;
                self.consume_while(|b| is_digit_base(b as char, 16) || b == b'_');
                let raw = self.src[start + 2..self.off].replace('_', "");
                let v = i64::from_str_radix(&raw, 16).map_err(|_| self.err_from(start, LexErrorKind::IntOverflow))?;
                return Ok(TokenKind::Int(v));
            } else if self.peek_char().map(|c| c == 'b' || c == 'B').unwrap_or(false) {
                self.off += 1;
                self.consume_while(|b| is_digit_base(b as char, 2) || b == b'_');
                let raw = self.src[start + 2..self.off].replace('_', "");
                let v = i64::from_str_radix(&raw, 2).map_err(|_| self.err_from(start, LexErrorKind::IntOverflow))?;
                return Ok(TokenKind::Int(v));
            } else if self.peek_char().map(|c| c == 'o' || c == 'O').unwrap_or(false) {
                self.off += 1;
                self.consume_while(|b| is_digit_base(b as char, 8) || b == b'_');
                let raw = self.src[start + 2..self.off].replace('_', "");
                let v = i64::from_str_radix(&raw, 8).map_err(|_| self.err_from(start, LexErrorKind::IntOverflow))?;
                return Ok(TokenKind::Int(v));
            }
        }

        // Décimal / flottant
        self.consume_while(|b| (b as char).is_ascii_digit() || b == b'_');
        let mut saw_dot = false;
        let mut saw_exp = false;
        if self.peek_char() == Some('.') && self.peek2().map(|d| (d as char).is_ascii_digit()).unwrap_or(false) {
            saw_dot = true;
            self.off += 1;
            self.consume_while(|b| (b as char).is_ascii_digit() || b == b'_');
        }
        if matches!(self.peek_char(), Some('e' | 'E')) {
            saw_exp = true;
            self.off += 1;
            if matches!(self.peek_char(), Some('+' | '-')) { self.off += 1; }
            self.consume_while(|b| (b as char).is_ascii_digit() || b == b'_');
        }

        let raw = self.src[start..self.off].replace('_', "");
        if saw_dot || saw_exp {
            let v = raw.parse::<f64>().map_err(|_| self.err_from(start, LexErrorKind::InvalidNumber))?;
            Ok(TokenKind::Float(v))
        } else {
            raw.parse::<i64>()
               .map(TokenKind::Int)
               .map_err(|_| self.err_from(start, LexErrorKind::IntOverflow))
        }
    }

    /* ────────── Spans / erreurs ────────── */

    #[inline] fn span_here(&self, width: usize) -> Span {
        Span { source: self.source, start: Pos(self.off as u32), end: Pos((self.off + width) as u32) }
    }
    #[inline] fn span_from(&self, start: usize) -> Span {
        Span { source: self.source, start: Pos(start as u32), end: Pos(self.off as u32) }
    }
    #[inline] fn err_here(&self, kind: LexErrorKind) -> LexError { LexError { span: self.span_here(1), kind } }
    #[inline] fn err_from(&self, start: usize, kind: LexErrorKind) -> LexError { LexError { span: self.span_from(start), kind } }
    #[inline] fn err_spanned(&self, start: usize, kind: LexErrorKind) -> LexError { LexError { span: self.span_from(start), kind } }
}

/* ─────────────────────────── Helpers ─────────────────────────── */

#[inline]
fn is_ident_start(c: char) -> bool { c == '_' || c.is_ascii_alphabetic() }

#[inline]
fn is_ident_continue(c: char) -> bool { c == '_' || c.is_ascii_alphanumeric() }

#[inline]
fn is_digit_base(c: char, base: u32) -> bool {
    match base {
        2 => matches!(c, '0' | '1'),
        8 => c.is_ascii_digit() && c < '8',
        10 => c.is_ascii_digit(),
        16 => c.is_ascii_hexdigit(),
        _ => false,
    }
}

#[inline]
fn keyword_of(s: &str) -> Option<Keyword> {
    use Keyword::*;
    Some(match s {
        "fn" => Fn,
        "let" => Let,
        "const" => Const,
        "if" => If,
        "else" => Else,
        "while" => While,
        "for" => For,
        "return" => Return,
        "struct" => Struct,
        "enum" => Enum,
        "true" => True,
        "false" => False,
        "null" => Null,
        _ => return None,
    })
}

#[inline]
fn hex_val(c: char) -> Option<u8> {
    match c {
        '0'..='9' => Some((c as u8) - b'0'),
        'a'..='f' => Some((c as u8) - b'a' + 10),
        'A'..='F' => Some((c as u8) - b'A' + 10),
        _ => None,
    }
}

/* ─────────────────────────── Tests ─────────────────────────── */

#[cfg(test)]
mod tests {
    use super::*;

    fn toks(src: &str) -> Vec<TokenKind<'_>> {
        let mut lx = Lexer::new(src, SourceId(0));
        let mut out = Vec::new();
        loop {
            let t = lx.next().unwrap().unwrap();
            let end = matches!(t.value, TokenKind::Eof);
            out.push(t.value);
            if end { break; }
        }
        out
    }

    #[test]
    fn idents_keywords() {
        use Keyword::*;
        use TokenKind::*;
        let v = toks("fn let const if else while for return struct enum true false null ident _x x1");
        assert!(matches!(v[0], Kw(Fn)));
        assert!(matches!(v[1], Kw(Let)));
        assert!(matches!(v[2], Kw(Const)));
        assert!(matches!(v[3], Kw(If)));
        assert!(matches!(v[4], Kw(Else)));
        assert!(matches!(v[5], Kw(While)));
        assert!(matches!(v[6], Kw(For)));
        assert!(matches!(v[7], Kw(Return)));
        assert!(matches!(v[8], Kw(Struct)));
        assert!(matches!(v[9], Kw(Enum)));
        assert!(matches!(v[10], Kw(True)));
        assert!(matches!(v[11], Kw(False)));
        assert!(matches!(v[12], Kw(Null)));
        assert!(matches!(v[13], Ident("ident")));
        assert!(matches!(v[14], Ident("_x")));
        assert!(matches!(v[15], Ident("x1")));
    }

    #[test]
    fn numbers_and_floats() {
        use TokenKind::*;
        let v = toks("0xFF 0o77 0b1010 123 1_234 12.34 1e3 2.5e-2");
        assert_eq!(v[0], Int(255));
        assert_eq!(v[1], Int(63));
        assert_eq!(v[2], Int(10));
        assert_eq!(v[3], Int(123));
        assert_eq!(v[4], Int(1234));
        if let Float(f) = v[5] { assert!((f - 12.34).abs() < 1e-9) } else { panic!() }
        if let Float(f) = v[6] { assert!((f - 1000.0).abs() < 1e-9) } else { panic!() }
        if let Float(f) = v[7] { assert!((f - 0.025).abs() < 1e-9) } else { panic!() }
    }

    #[test]
    fn strings_escapes_chars() {
        use TokenKind::*;
        let v = toks(r#""hi" "\n" "\x41" "\u{1F600}" 'a' '\n' '\x41' '\u{61}'"#);
        assert_eq!(v[0], Str("hi".into()));
        assert_eq!(v[1], Str("\n".into()));
        assert_eq!(v[2], Str("A".into()));
        assert_eq!(v[3], Str("😀".into()));
        assert_eq!(v[4], Char('a'));
        assert_eq!(v[5], Char('\n'));
        assert_eq!(v[6], Char('A'));
        assert_eq!(v[7], Char('a'));
    }

    #[test]
    fn raw_strings() {
        use TokenKind::*;
        let v = toks(r###"r"ok" r#"multi " hash"# r##"## inside "##"##"###);
        assert_eq!(v[0], Str("ok".into()));
        assert_eq!(v[1], Str("multi \" hash".into()));
        assert_eq!(v[2], Str("## inside \"##".into()));
    }

    #[test]
    fn comments_ws() {
        use TokenKind::*;
        let v = toks("/* a /* nested */ still */ + // line\n 1");
        assert!(matches!(v[0], Plus));
        assert_eq!(v[1], Int(1));
    }

    #[test]
    fn ops_punct() {
        use TokenKind::*;
        let v = toks(":: -> => == != <= >= && || ! + - * / % ( ) { } [ ] , . ; : < > =");
        assert!(matches!(v[0], PathSep));
        assert!(matches!(v[1], Arrow));
        assert!(matches!(v[2], FatArrow));
        assert!(matches!(v[3], EqEq));
        assert!(matches!(v[4], Ne));
        assert!(matches!(v[5], Le));
        assert!(matches!(v[6], Ge));
        assert!(matches!(v[7], AndAnd));
        assert!(matches!(v[8], OrOr));
        assert!(matches!(v[9], Bang));
        assert!(matches!(v[10], Plus));
        assert!(matches!(v[11], Minus));
        assert!(matches!(v[12], Star));
        assert!(matches!(v[13], Slash));
        assert!(matches!(v[14], Percent));
        assert!(matches!(v[15], LParen));
        assert!(matches!(v[16], RParen));
        assert!(matches!(v[17], LBrace));
        assert!(matches!(v[18], RBrace));
        assert!(matches!(v[19], LBracket));
        assert!(matches!(v[20], RBracket));
        assert!(matches!(v[21], Comma));
        assert!(matches!(v[22], Dot));
        assert!(matches!(v[23], Semi));
        assert!(matches!(v[24], Colon));
        assert!(matches!(v[25], Lt));
        assert!(matches!(v[26], Gt));
        assert!(matches!(v[27], Eq));
    }

    #[test]
    fn linemap_basic() {
        let src = "a\nbb\nccc";
        let lm = LineMap::new(src);
        assert_eq!(lm.line_col(Pos(0)), (1,1));
        assert_eq!(lm.line_col(Pos(2)), (2,1));
        assert_eq!(lm.line_col(Pos(4)), (3,1));
        assert_eq!(lm.line_col(Pos(6)), (3,3));
    }
}
