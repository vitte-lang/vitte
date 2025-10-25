//! Gestion du buffer multi-ligne pour le REPL.

use std::string::String;
use std::vec::Vec;

/// Buffer intermédiaire regroupant les lignes d’un bloc.
#[derive(Debug, Default)]
pub struct SessionBuffer {
    lines: Vec<String>,
    brace_depth: i32,
    paren_depth: i32,
    bracket_depth: i32,
    in_string: bool,
    string_delim: Option<char>,
}

impl SessionBuffer {
    /// Ajoute une ligne et retourne `true` si le bloc peut être évalué.
    pub fn push_line(&mut self, line: &str) -> bool {
        self.update_depths(line);
        self.lines.push(line.to_string());
        self.is_complete()
    }

    /// Indique si aucune ligne n’est accumulée.
    pub fn is_empty(&self) -> bool {
        self.lines.is_empty()
    }

    /// Retourne `true` si le bloc est complet (tous les compteurs à zéro).
    pub fn is_complete(&self) -> bool {
        if self.lines.is_empty() {
            return false;
        }
        self.brace_depth == 0
            && self.paren_depth == 0
            && self.bracket_depth == 0
            && !self.in_string
            && Self::looks_terminated(self.lines.last().unwrap())
    }

    /// Vide le buffer et renvoie le bloc formatté.
    pub fn flush(&mut self) -> String {
        let mut chunk = String::new();
        for (idx, line) in self.lines.iter().enumerate() {
            if idx > 0 {
                chunk.push('\n');
            }
            chunk.push_str(line);
        }
        self.lines.clear();
        chunk
    }

    fn update_depths(&mut self, line: &str) {
        let mut chars = line.chars().peekable();
        while let Some(ch) = chars.next() {
            if self.in_string {
                if ch == '\\' {
                    // Sauter le caractère échappé.
                    chars.next();
                    continue;
                }
                if Some(ch) == self.string_delim {
                    self.in_string = false;
                    self.string_delim = None;
                }
                continue;
            }
            match ch {
                '"' | '\'' => {
                    self.in_string = true;
                    self.string_delim = Some(ch);
                }
                '{' => self.brace_depth += 1,
                '}' => self.brace_depth -= 1,
                '(' => self.paren_depth += 1,
                ')' => self.paren_depth -= 1,
                '[' => self.bracket_depth += 1,
                ']' => self.bracket_depth -= 1,
                _ => {}
            }
        }
    }

    fn looks_terminated(line: &str) -> bool {
        let trimmed = line.trim_end();
        if trimmed.is_empty() {
            return true;
        }
        matches!(trimmed.chars().last(), Some(';' | '}' | ')' | ']' | '"' | '\''))
    }
}
