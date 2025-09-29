//! vitte-fmt — formateur de code Vitte
//!
//! Objectifs pragmatiques :
//! - Indentation cohérente basée sur `{}`
//! - Espaces autour des opérateurs
//! - Sauts de ligne après `;` et `}`
//! - Respect des commentaires `//` et `/* ... */`
//! - Chaînes préservées telles quelles
//! - Mode `pretty` optionnel (quelques blancs contextuels)
//!
//! API :
//! - [`FormatOptions`]  : paramètres
//! - [`Formatter`]      : instance réutilisable
//! - [`format_str`]     : fonction simple
//!
//! Notes : implémentation basée sur un tokenizer léger indépendant de l'AST.
//! Les dépendances `vitte-ast` et `vitte-parser` restent disponibles
//! mais ne sont pas requises pour formatter du texte.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use anyhow::{bail, Result};
use std::fmt::Write as _;
use std::mem;

#[cfg(feature = "json")]
use serde::{Deserialize, Serialize};

/// Options de mise en forme.
#[derive(Clone, Debug)]
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
pub struct FormatOptions {
    /// Largeur d'indentation en espaces si `use_tabs = false`.
    pub indent_width: usize,
    /// Utiliser des tabulations au lieu d'espaces.
    pub use_tabs: bool,
    /// Longueur maximale souhaitée d'une ligne (non stricte).
    pub max_line_len: usize,
    /// Mode pretty : insertions de lignes vides contextuelles.
    pub pretty: bool,
}

impl Default for FormatOptions {
    fn default() -> Self {
        Self {
            indent_width: 4,
            use_tabs: false,
            max_line_len: 100,
            pretty: false,
        }
    }
}

/// Formateur réutilisable.
pub struct Formatter {
    opts: FormatOptions,
}

impl Formatter {
    pub fn new(opts: FormatOptions) -> Self {
        Self { opts }
    }

    /// Formatte une chaîne source.
    pub fn format(&self, src: &str) -> Result<String> {
        let tokens = tokenize(src)?;
        Ok(layout(&tokens, &self.opts))
    }
}

/// Raccourci statique.
pub fn format_str(src: &str) -> Result<String> {
    Formatter::new(FormatOptions::default()).format(src)
}

/* =========================== Tokenizer ================================== */

#[derive(Debug, Clone, PartialEq)]
enum Tok {
    Word(String),        // ident, mot-clé, nombre
    Sym(char),           // symbole ponctuel: (){}[],;=+-*/%<>!&|.^:?
    StringLit(String),   // "...." avec échappements
    LineComment(String), // //...
    BlockComment(String),// /* ... */
    Newline,             // \n
    Space,               // séquence d'espaces/tabs
}

fn is_word_start(c: char) -> bool {
    c == '_' || c.is_ascii_alphabetic()
}
fn is_word_continue(c: char) -> bool {
    c == '_' || c.is_ascii_alphanumeric()
}

/// Symboles traités comme opérateurs binaires pour entourer d'espaces.
fn is_op_char(c: char) -> bool {
    matches!(c, '+'|'-'|'*'|'/'|'%'|'='|'!'|'<'|'>'|'&'|'|'|'^')
}

/// Symboles ponctuation collés à gauche (jamais d'espace avant).
fn is_right_punct(c: char) -> bool {
    matches!(c, ','|';'|')'|']'|'}'|':')
}

/// Symboles ponctuation collés à droite (jamais d'espace après).
fn is_left_punct(c: char) -> bool {
    matches!(c, '('|'['|'{' )
}

fn tokenize(src: &str) -> Result<Vec<Tok>> {
    let mut out = Vec::new();
    let mut it = src.chars().peekable();

    while let Some(c) = it.next() {
        match c {
            // newlines
            '\n' => out.push(Tok::Newline),
            // space runs (spaces or tabs)
            ' ' | '\t' => {
                while matches!(it.peek(), Some(' ' | '\t')) {
                    it.next();
                }
                out.push(Tok::Space);
            }
            // strings
            '"' => {
                let mut s = String::from("\"");
                let mut escaped = false;
                while let Some(&nc) = it.peek() {
                    s.push(nc);
                    it.next();
                    if escaped {
                        escaped = false;
                    } else if nc == '\\' {
                        escaped = true;
                    } else if nc == '"' {
                        break;
                    }
                }
                out.push(Tok::StringLit(s));
            }
            // comments or divide
            '/' => {
                match it.peek().copied() {
                    Some('/') => {
                        it.next();
                        let mut s = String::from("//");
                        while let Some(&nc) = it.peek() {
                            if nc == '\n' { break; }
                            s.push(nc);
                            it.next();
                        }
                        out.push(Tok::LineComment(s));
                    }
                    Some('*') => {
                        it.next();
                        let mut s = String::from("/*");
                        let mut prev = '\0';
                        while let Some(nc) = it.next() {
                            s.push(nc);
                            if prev == '*' && nc == '/' { break; }
                            prev = nc;
                        }
                        out.push(Tok::BlockComment(s));
                    }
                    _ => out.push(Tok::Sym('/')),
                }
            }
            c if is_word_start(c) => {
                let mut s = String::new();
                s.push(c);
                while let Some(&nc) = it.peek() {
                    if is_word_continue(nc) {
                        s.push(nc);
                        it.next();
                    } else {
                        break;
                    }
                }
                out.push(Tok::Word(s));
            }
            c if is_op_char(c) => {
                // agglomère opérateurs à 2 caractères (==,!=,<=,>=,&&,||,->,::)
                if let Some(&n) = it.peek() {
                    let two = format!("{c}{n}");
                    if matches!(two.as_str(), "==" | "!=" | "<=" | ">=" | "&&" | "||" | "->" | "::") {
                        it.next();
                        for ch in two.chars() {
                            out.push(Tok::Sym(ch));
                        }
                        continue;
                    }
                }
                out.push(Tok::Sym(c));
            }
            c if is_left_punct(c) || is_right_punct(c) || matches!(c, '.' ) => {
                out.push(Tok::Sym(c));
            }
            // other single chars
            other => out.push(Tok::Sym(other)),
        }
    }

    Ok(out)
}

/* =========================== Layout ===================================== */

fn indent_str(level: usize, opts: &FormatOptions) -> String {
    if opts.use_tabs {
        "\t".repeat(level)
    } else {
        " ".repeat(level.saturating_mul(opts.indent_width))
    }
}

fn layout(tokens: &[Tok], opts: &FormatOptions) -> String {
    let mut out = String::new();
    let mut indent = 0usize;
    let mut need_space = false;
    let mut at_line_start = true;
    let mut last_was_newline = true;

    // petits helpers
    let mut push_newline = |out: &mut String| {
        if !last_was_newline {
            out.push('\n');
            last_was_newline = true;
            at_line_start = true;
        }
    };
    let mut push_indent_if_needed = |out: &mut String| {
        if at_line_start {
            let s = indent_str(indent, opts);
            out.push_str(&s);
            at_line_start = false;
        }
    };
    let mut push_space = |out: &mut String| {
        if !at_line_start && !last_was_newline {
            out.push(' ');
        }
    };

    // état pour `pretty`
    let mut last_token_was_closing_brace = false;

    let mut i = 0usize;
    while i < tokens.len() {
        let tok = &tokens[i];

        match tok {
            Tok::Newline => {
                push_newline(&mut out);
                need_space = false;
            }
            Tok::Space => {
                // ignoré : l'espacement est déterminé par les règles
            }
            Tok::LineComment(text) => {
                push_indent_if_needed(&mut out);
                // commentaire de ligne : flush jusqu'au \n.
                out.push_str(text);
                push_newline(&mut out);
                need_space = false;
            }
            Tok::BlockComment(text) => {
                // conserve indentation courante, imprime tel quel.
                push_indent_if_needed(&mut out);
                out.push_str(text);
                need_space = true;
            }
            Tok::StringLit(s) => {
                push_indent_if_needed(&mut out);
                if need_space {
                    push_space(&mut out);
                }
                out.push_str(s);
                need_space = true;
            }
            Tok::Word(w) => {
                // mots-clés pouvant influer sur espaces/braces
                push_indent_if_needed(&mut out);
                if need_space {
                    push_space(&mut out);
                }
                out.push_str(w);
                need_space = true;
            }
            Tok::Sym('{') => {
                // espace avant { si précédé d'un mot ou ')'
                push_indent_if_needed(&mut out);
                if !last_was_newline && !at_line_start {
                    push_space(&mut out);
                }
                out.push('{');
                // newline + indent++
                push_newline(&mut out);
                indent = indent.saturating_add(1);
                need_space = false;
                last_token_was_closing_brace = false;
            }
            Tok::Sym('}') => {
                // dedent avant impression
                indent = indent.saturating_sub(1);
                if !last_was_newline {
                    push_newline(&mut out);
                }
                push_indent_if_needed(&mut out);
                out.push('}');
                need_space = true;
                last_token_was_closing_brace = true;
            }
            Tok::Sym(';') => {
                out.push(';');
                // fin d'instruction
                push_newline(&mut out);
                need_space = false;
            }
            Tok::Sym(',') => {
                out.push(',');
                out.push(' ');
                need_space = false;
            }
            Tok::Sym('(') | Tok::Sym('[') => {
                if need_space {
                    // pas d'espace avant parenthèse d'appel si mot avant immédiatement
                    if matches!(tokens.get(i.wrapping_sub(1)), Some(Tok::Word(_)) ) {
                        // rien
                    } else {
                        push_space(&mut out);
                    }
                } else {
                    push_indent_if_needed(&mut out);
                }
                out.push(match tok { Tok::Sym(c) => *c, _ => unreachable!() });
                need_space = false;
            }
            Tok::Sym(')') | Tok::Sym(']') => {
                out.push(match tok { Tok::Sym(c) => *c, _ => unreachable!() });
                need_space = true;
            }
            Tok::Sym('.') => {
                // pas d'espaces pour l'accès membre
                out.push('.');
                need_space = false;
            }
            Tok::Sym(':') => {
                // gestion de `::` déjà tokenisé char-par-char ; évitons l'espace au milieu
                if matches!(tokens.get(i.wrapping_sub(1)), Some(Tok::Sym(':')))
                    || matches!(tokens.get(i + 1), Some(Tok::Sym(':')))
                {
                    out.push(':');
                    need_space = false;
                } else {
                    out.push(':');
                    out.push(' ');
                    need_space = false;
                }
            }
            Tok::Sym(op) if is_op_char(*op) => {
                // regrouper opérateurs doubles déjà tokenisés char à char
                // on espace autour sauf cas unaires simples (+ - devant nombre/mot)
                // pour simplicité: toujours espaces autour sauf si collé à ( ou ) ou ,
                if !last_was_newline && !at_line_start {
                    out.push(' ');
                } else {
                    push_indent_if_needed(&mut out);
                }
                out.push(*op);
                // pas d'espace pour "->" entre '-' et '>' car traité char à char
                if tokens.get(i + 1).map_or(false, |t| matches!(t, Tok::Sym('>'))) && *op == '-' {
                    // ne rien ajouter ici; '>' ajoutera l'espace suivant
                } else {
                    out.push(' ');
                }
                need_space = false;
            }
            Tok::Sym(other) => {
                // défaut : symbole brut
                push_indent_if_needed(&mut out);
                if need_space && !is_right_punct(*other) {
                    push_space(&mut out);
                }
                out.push(*other);
                need_space = !is_left_punct(*other);
            }
        }

        // gestion états ligne
        if matches!(tok, Tok::Newline) {
            last_was_newline = true;
            at_line_start = true;
        } else {
            if last_was_newline {
                // au vrai début de ligne, injecter indentation immédiate on-demand
                // (déjà géré par push_indent_if_needed)
            }
            last_was_newline = false;
            at_line_start = false;
        }

        // pretty: ligne vide après un bloc fermé si prochain token démarre un item
        if opts.pretty && last_token_was_closing_brace {
            if let Some(next) = tokens.get(i + 1) {
                if matches!(next, Tok::Word(_)) {
                    // insère une ligne vide si la précédente n'était pas déjà vide
                    if !out.ends_with("\n\n") {
                        out.push('\n');
                    }
                }
            }
            last_token_was_closing_brace = false;
        }

        i += 1;
    }

    // trim final
    while out.ends_with('\n') && out.matches('\n').count() > 1 && out.ends_with("\n\n") {
        out.pop();
    }
    out
}

/* =========================== JSON output ================================ */

/// Vue simple en lignes.
#[cfg(feature = "json")]
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FormattedLines {
    pub lines: Vec<String>,
}

#[cfg(feature = "json")]
pub fn format_to_json(src: &str, opts: &FormatOptions) -> Result<String> {
    let s = Formatter::new(opts.clone()).format(src)?;
    let lines = s.lines().map(|l| l.to_string()).collect::<Vec<_>>();
    Ok(serde_json::to_string_pretty(&FormattedLines { lines })?)
}

/* =============================== Tests ================================== */

#[cfg(test)]
mod tests {
    use super::*;

    fn fmt(input: &str) -> String {
        format_str(input).unwrap()
    }

    #[test]
    fn basics_indent_and_semicolons() {
        let src = r#"
fn main(){let x=1+2; if(x>0){x=x+1;}}"#;
        let out = fmt(src);
        let expected = "\
fn main() {
    let x = 1 + 2;
    if (x > 0) {
        x = x + 1;
    }
}";
        assert_eq!(out, expected);
    }

    #[test]
    fn comments_and_strings() {
        let src = r#"
// header
fn say(){ println("a//b/*c*/d"); /* block
multi */ let x=1; // end
}"#;
        let out = fmt(src);
        assert!(out.contains("// header\n"));
        assert!(out.contains(r#"println("a//b/*c*/d")"#));
        assert!(out.contains("/* block\nmulti */"));
    }

    #[test]
    fn operators_spacing() {
        let src = "fn f(){let a=1;let b=2;let c=a+b* (b-1);}";
        let out = fmt(src);
        assert!(out.contains("let a = 1;"));
        assert!(out.contains("let b = 2;"));
        assert!(out.contains("let c = a + b * (b - 1);"));
    }

    #[test]
    fn arrays_and_calls() {
        let src = "fn f(){push(arr,1,2);a[ i+1 ]=42;}";
        let out = fmt(src);
        assert!(out.contains("push(arr, 1, 2);"));
        assert!(out.contains("a[i + 1] = 42;"));
    }

    #[test]
    fn pretty_blank_between_items() {
        let mut opts = FormatOptions::default();
        opts.pretty = true;
        let out = Formatter::new(opts).format("fn a(){}\nfn b(){}").unwrap();
        assert!(out.contains("}\n\nfn b"));
    }

    #[cfg(feature = "json")]
    #[test]
    fn json_lines() {
        let s = format_to_json("fn x(){let a=0;}", &FormatOptions::default()).unwrap();
        assert!(s.contains("\"lines\""));
    }
}