

#![deny(missing_docs)]
//! vitte-regex — moteur regex pour Vitte
//!
//! Fournit :
//! - Wrappers autour de `regex` Rust.
//! - API simple pour recherche, remplacement, captures.
//! - Option `unicode` pour support Unicode.
//! - Intégration prévue avec l’AST Vitte (feature `syntax`).
//!
//! Objectif : exposer une API stable, sûre, et facilement testable.

use thiserror::Error;
use regex::Regex;

/// Erreurs liées au moteur regex.
#[derive(Debug, Error)]
pub enum RegexError {
    #[error("compilation regex: {0}")]
    Compile(#[from] regex::Error),
    #[error("aucune correspondance trouvée")]
    NoMatch,
    #[error("autre: {0}")]
    Other(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, RegexError>;

/// Compile un motif regex.
pub fn compile(pattern: &str) -> Result<Regex> {
    Regex::new(pattern).map_err(RegexError::from)
}

/// Recherche une occurrence et renvoie le match complet.
pub fn find(text: &str, pattern: &str) -> Result<String> {
    let re = compile(pattern)?;
    if let Some(m) = re.find(text) {
        Ok(m.as_str().to_owned())
    } else {
        Err(RegexError::NoMatch)
    }
}

/// Trouve toutes les occurrences et renvoie sous forme de vecteur.
pub fn find_all(text: &str, pattern: &str) -> Result<Vec<String>> {
    let re = compile(pattern)?;
    let out: Vec<String> = re.find_iter(text).map(|m| m.as_str().to_owned()).collect();
    if out.is_empty() {
        Err(RegexError::NoMatch)
    } else {
        Ok(out)
    }
}

/// Remplace la première occurrence.
pub fn replace_first(text: &str, pattern: &str, replacement: &str) -> Result<String> {
    let re = compile(pattern)?;
    Ok(re.replace(text, replacement).to_string())
}

/// Remplace toutes les occurrences.
pub fn replace_all(text: &str, pattern: &str, replacement: &str) -> Result<String> {
    let re = compile(pattern)?;
    Ok(re.replace_all(text, replacement).to_string())
}

/// Capture les groupes d’une première correspondance.
pub fn captures(text: &str, pattern: &str) -> Result<Vec<String>> {
    let re = compile(pattern)?;
    if let Some(caps) = re.captures(text) {
        Ok(caps.iter().filter_map(|m| m.map(|x| x.as_str().to_owned())).collect())
    } else {
        Err(RegexError::NoMatch)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_find() {
        let res = find("abc123", r"\d+").unwrap();
        assert_eq!(res, "123");
    }

    #[test]
    fn test_find_all() {
        let res = find_all("a1b22c333", r"\d+").unwrap();
        assert_eq!(res, vec!["1","22","333"]);
    }

    #[test]
    fn test_replace() {
        let r = replace_first("foo foo", "foo", "bar").unwrap();
        assert_eq!(r, "bar foo");
        let r2 = replace_all("foo foo", "foo", "bar").unwrap();
        assert_eq!(r2, "bar bar");
    }

    #[test]
    fn test_captures() {
        let caps = captures("abc123", r"(\w+)(\d+)").unwrap();
        assert_eq!(caps[0], "abc123");
        assert_eq!(caps[1], "abc");
        assert_eq!(caps[2], "123");
    }

    #[test]
    fn test_no_match() {
        let err = find("abc", r"\d+").unwrap_err();
        assert!(matches!(err, RegexError::NoMatch));
    }
}