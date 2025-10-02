

#![deny(missing_docs)]
//! vitte-string — utilitaires chaînes pour Vitte
//!
//! Objectifs:
//! - Fonctions pures, sans allocation inutile.
//! - Support Unicode (chars). Graphemes disponibles si feature `graphemes`.
//! - Outils pratiques: `is_blank`, normalisation d'espaces, `slugify`, coupe sûre,
//!   indentation/désindentation, split/lines universel.
//!
//! Exemple:
//! ```
//! use vitte_string as vs;
//! assert!(vs::is_blank("  \t\n"));
//! assert_eq!(vs::collapse_ws("a\n\t  b"), "a b");
//! assert_eq!(vs::slugify("Hello, World!"), "hello-world");
//! ```

use thiserror::Error;

/// Erreurs liées aux chaînes
#[derive(Debug, Error)]
pub enum StrError {
    #[error("index hors limites")] OutOfBounds,
    #[error("opération invalide: {0}")] Invalid(String),
}

/// Résultat spécialisé
pub type Result<T> = std::result::Result<T, StrError>;

/// Vrai si la chaîne ne contient que des blancs Unicode
pub fn is_blank(s: &str) -> bool { s.chars().all(char::is_whitespace) }

/// Compte les lignes (tous séparateurs: `\n`, `\r\n`, `\r`).
pub fn line_count(s: &str) -> usize { split_lines(s).count() }

/// Itérateur sur lignes, tolérant `\n`, `\r\n`, `\r`.
pub fn split_lines(s: &str) -> impl Iterator<Item=&str> { s.split_inclusive(['\n','\r']).map(|seg| seg.trim_end_matches(['\n','\r'])) }

/// Normalise tout espace/blanc en un simple espace ASCII. Supprime répétitions et bords.
pub fn collapse_ws(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    let mut inspace = false;
    for ch in s.chars() {
        if ch.is_whitespace() {
            inspace = true;
        } else {
            if inspace && !out.is_empty() { out.push(' '); }
            inspace = false;
            out.push(ch);
        }
    }
    out.trim().to_string()
}

/// Coupe en nombre de *caractères* Unicode (pas d'octets). Ajoute `…` si tronqué.
pub fn truncate_chars(s: &str, max_chars: usize) -> String {
    if s.chars().count() <= max_chars { return s.to_string(); }
    let mut it = s.chars();
    let mut out = String::new();
    for _ in 0..max_chars.saturating_sub(1) { if let Some(c) = it.next() { out.push(c) } }
    out.push('…');
    out
}

/// Coupe en nombre de *graphemes* si la feature `graphemes` est active. Sinon alias de `truncate_chars`.
#[cfg(feature = "graphemes")]
pub fn truncate_graphemes(s: &str, max_g: usize) -> String {
    use unicode_segmentation::UnicodeSegmentation;
    let gs: Vec<&str> = UnicodeSegmentation::graphemes(s, true).collect();
    if gs.len() <= max_g { return s.to_string(); }
    let mut out = String::new();
    for g in gs.into_iter().take(max_g.saturating_sub(1)) { out.push_str(g); }
    out.push('…');
    out
}

#[cfg(not(feature = "graphemes"))]
/// Fallback sans `graphemes`.
pub fn truncate_graphemes(s: &str, max_g: usize) -> String { truncate_chars(s, max_g) }

/// Slug simple: minuscule, remplace tout non-alphanum par `-`, compresse, retire bords.
pub fn slugify(input: &str) -> String {
    let mut out = String::with_capacity(input.len());
    let mut prev_dash = false;
    for ch in input.chars().flat_map(|c| c.to_lowercase()) {
        if ch.is_alphanumeric() {
            out.push(ch);
            prev_dash = false;
        } else {
            if !prev_dash { out.push('-'); prev_dash = true; }
        }
    }
    // trim '-'
    while out.ends_with('-') { out.pop(); }
    while out.starts_with('-') { out.remove(0); }
    out
}

/// Indente chaque ligne avec `prefix`.
pub fn indent(s: &str, prefix: &str) -> String {
    split_lines(s).map(|l| format!("{prefix}{l}")).collect::<Vec<_>>().join("\n")
}

/// Dé-indente en retirant `n` espaces au début si présents.
pub fn dedent(s: &str, n: usize) -> String {
    split_lines(s)
        .map(|l| {
            let take = l.chars().take_while(|c| *c==' ').count().min(n);
            l.chars().skip(take).collect::<String>()
        })
        .collect::<Vec<_>>()
        .join("\n")
}

/// Joint des segments avec séparateur, en ignorant segments vides si `skip_empty`.
pub fn join<I>(iter: I, sep: &str, skip_empty: bool) -> String
where I: IntoIterator, I::Item: AsRef<str> {
    let mut out = String::new();
    for part in iter.into_iter() {
        let p = part.as_ref();
        if skip_empty && p.is_empty() { continue; }
        if !out.is_empty() { out.push_str(sep); }
        out.push_str(p);
    }
    out
}

/// Trim chaque ligne.
pub fn trim_lines(s: &str) -> String { split_lines(s).map(|l| l.trim()).collect::<Vec<_>>().join("\n") }

/// Remplace CRLF/CR par LF.
pub fn normalize_newlines(s: &str) -> String { s.replace("\r\n", "\n").replace('\r', "\n") }

/// Vérifie si `s` contient uniquement ASCII imprimable (32..126) ou LF/TAB.
pub fn is_printable_ascii(s: &str) -> bool { s.chars().all(|c| c == '\n' || c == '\t' || ((' '..='~').contains(&c))) }

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn blank_and_collapse() {
        assert!(is_blank("\t \n"));
        assert_eq!(collapse_ws("a\n\t  b"), "a b");
    }

    #[test]
    fn slug_and_truncate() {
        assert_eq!(slugify("Hello, World!"), "hello-world");
        assert_eq!(truncate_chars("abcdef", 4), "abc…");
    }

    #[test]
    fn indent_dedent() {
        let s = "x\ny";
        assert_eq!(indent(s, "  "), "  x\n  y");
        assert_eq!(dedent("  x\n    y", 2), "x\n  y");
    }

    #[test]
    fn split_and_join() {
        let v: Vec<&str> = split_lines("a\r\nb\nc\r").collect();
        assert_eq!(v, vec!["a","b","c"]);
        assert_eq!(join(["a", "", "b"], ",", true), "a,b");
    }
}