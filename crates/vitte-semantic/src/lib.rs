//! vitte-semantic — semantic NLP primitives for Vitte
//!
//! Features:
//! - `hf-tokenizers`: HuggingFace tokenizers
//! - `lang`: language detection via whatlang
//! - `serde`: serialize embeddings
//!
//! Example:
//! ```
//! use vitte_semantic as vs;
//! let e1 = vs::embed("hello world", 128);
//! let e2 = vs::embed("hello there", 128);
//! let sim = vs::cosine(&e1.vector, &e2.vector);
//! assert!((0.0..=1.0).contains(&sim));
//! ```

#![forbid(unsafe_code)]
#![deny(missing_docs)]

use std::collections::hash_map::DefaultHasher;
use std::hash::{Hash, Hasher};


#[cfg(feature = "lang")]
use whatlang::detect;

/// NLP errors.
#[derive(Debug)]
pub enum NlpError {
    /// Tokenization error.
    Tok(String),
    /// Generic error.
    Other(String),
}

impl core::fmt::Display for NlpError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            NlpError::Tok(s) => write!(f, "tokenization error: {s}"),
            NlpError::Other(s) => write!(f, "other: {s}"),
        }
    }
}

impl std::error::Error for NlpError {}

/// Result alias.
pub type Result<T> = core::result::Result<T, NlpError>;

/// Tokenize input text.
pub fn tokenize(text: &str) -> Vec<String> {
    #[cfg(feature = "hf-tokenizers")]
    {
        use tokenizers::{
            models::wordpiece::WordPiece,
            normalizers::bert::BertNormalizer,
            pre_tokenizers::whitespace::Whitespace,
            Tokenizer,
        };
        let wp = WordPiece::new(Default::default())
            .unk_token("[UNK]".into())
            .build()
            .map_err(|e| NlpError::Tok(format!("{e}")))
            .unwrap();
        let mut tok = Tokenizer::new(wp);
        tok.with_normalizer(BertNormalizer::new(false, true, true, false));
        tok.with_pre_tokenizer(Whitespace);
        let out = tok.encode(text, false).map_err(|e| NlpError::Tok(format!("{e}"))).unwrap();
        return out.get_tokens().iter().map(|s| s.to_string()).collect();
    }
    #[cfg(not(feature = "hf-tokenizers"))]
    {
        simple_tokens(text)
    }
}

/// Fallback tokenizer.
fn simple_tokens(text: &str) -> Vec<String> {
    text.split(|c: char| c.is_whitespace() || is_punct(c))
        .filter(|s| !s.is_empty())
        .map(|s| s.to_lowercase())
        .collect()
}

/// Detect punctuation.
fn is_punct(c: char) -> bool {
    matches!(
        c,
        '\u{0021}'..='\u{002F}'
            | '\u{003A}'..='\u{0040}'
            | '\u{005B}'..='\u{0060}'
            | '\u{007B}'..='\u{007E}'
    )
}

/// Language detection (ISO code).
#[cfg(feature = "lang")]
pub fn detect_lang(text: &str) -> Option<&'static str> {
    detect(text).map(|info| info.lang().code())
}

/// No-op lang detection when feature off.
#[cfg(not(feature = "lang"))]
pub fn detect_lang(_text: &str) -> Option<&'static str> {
    None
}

/// Embedding container.
#[derive(Debug, Clone, PartialEq)]
pub struct Embedding {
    /// Dense vector.
    pub vector: Vec<f32>,
}

impl Embedding {
    /// Norm (L2).
    pub fn norm2(&self) -> f32 {
        l2(&self.vector)
    }
}

/// Hashing-trick embedder.
pub fn embed(text: &str, dim: usize) -> Embedding {
    let toks = tokenize(text);
    let mut v = vec![0f32; dim.max(1)];
    for t in toks {
        let idx = (hash_u64(&t) as usize) % v.len();
        v[idx] += 1.0;
    }
    let n = l2(&v);
    if n > 0.0 {
        for x in &mut v {
            *x /= n;
        }
    }
    Embedding { vector: v }
}

/// Cosine similarity.
pub fn cosine(a: &[f32], b: &[f32]) -> f32 {
    if a.is_empty() || b.is_empty() || a.len() != b.len() {
        return 0.0;
    }
    let (mut dot, mut na, mut nb) = (0.0, 0.0, 0.0);
    for i in 0..a.len() {
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    if na == 0.0 || nb == 0.0 {
        0.0
    } else {
        dot / (na.sqrt() * nb.sqrt())
    }
}

/// L2 norm helper.
fn l2(v: &[f32]) -> f32 {
    v.iter().map(|x| x * x).sum::<f32>().sqrt()
}

/// Hash helper.
fn hash_u64<T: Hash>(x: &T) -> u64 {
    let mut h = DefaultHasher::new();
    x.hash(&mut h);
    h.finish()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn simple_tokenization() {
        let t = tokenize("Hello, world! This is Vitte.");
        assert!(!t.is_empty());
    }

    #[test]
    fn embedding_and_cosine() {
        let e1 = embed("hello world", 64);
        let e2 = embed("hello there", 64);
        let sim = cosine(&e1.vector, &e2.vector);
        assert!((0.0..=1.0).contains(&sim));
    }

    #[cfg(feature = "lang")]
    #[test]
    fn language_detection() {
        let l = detect_lang("Bonjour, ceci est un test.");
        assert!(l.is_some());
    }
}