//! vitte-docgen — outil CLI de génération de documentation pour Vitte
//!
//! Ce crate est le moteur du binaire `vitte-docgen`.
//! Il s’appuie sur `vitte-doc` pour :
//! - parcourir un projet Vitte
//! - extraire la documentation
//! - générer des sorties Markdown, HTML ou JSON
//!
//! API publique :
//! - [`DocGenOptions`] : options passées en CLI
//! - [`run`] : lance la génération

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use anyhow::{Context, Result};
use std::fs;
use std::path::{Path, PathBuf};
use walkdir::WalkDir;

use vitte_doc::{build_book_from_sources, render_markdown, DocBook};

#[cfg(feature = "html")]
use vitte_doc::render_html;

#[cfg(feature = "json")]
use vitte_doc::render_json;

/// Options de génération.
#[derive(Debug, Clone)]
pub struct DocGenOptions {
    /// Racine du projet à scanner.
    pub root: PathBuf,
    /// Format de sortie : "md", "html", "json".
    pub format: String,
    /// Fichier de sortie (stdout si None).
    pub output: Option<PathBuf>,
    /// Extensions acceptées (par défaut .vitte).
    pub exts: Vec<String>,
}

impl Default for DocGenOptions {
    fn default() -> Self {
        Self {
            root: PathBuf::from("."),
            format: "md".into(),
            output: None,
            exts: vec!["vitte".into()],
        }
    }
}

/// Point d’entrée principal.
pub fn run(opts: &DocGenOptions) -> Result<()> {
    let sources = collect_sources(&opts.root, &opts.exts)?;
    let book = build_book_from_sources(sources.iter().map(|(p, s)| (p.as_str(), s.as_str())));

    let rendered = match opts.format.as_str() {
        "md" | "markdown" => render_markdown(&book),
        #[cfg(feature = "html")]
        "html" => render_html(&book),
        #[cfg(feature = "json")]
        "json" => render_json(&book)?,
        other => {
            anyhow::bail!("format non supporté: {}", other);
        }
    };

    if let Some(out) = &opts.output {
        fs::write(out, rendered).with_context(|| format!("write {}", out.display()))?;
    } else {
        println!("{}", rendered);
    }
    Ok(())
}

/// Collecte toutes les sources Vitte dans un projet.
fn collect_sources(root: &Path, exts: &[String]) -> Result<Vec<(String, String)>> {
    let mut out = Vec::new();
    for entry in WalkDir::new(root).into_iter().filter_map(|e| e.ok()) {
        let path = entry.path();
        if path.is_file() {
            if let Some(ext) = path.extension().and_then(|e| e.to_str()) {
                if exts.iter().any(|x| x == ext) {
                    let data = fs::read_to_string(path)
                        .with_context(|| format!("read {}", path.display()))?;
                    out.push((path.display().to_string(), data));
                }
            }
        }
    }
    Ok(out)
}

// --- Tests ---------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::tempdir;

    #[test]
    fn smoke_markdown() {
        let dir = tempdir().unwrap();
        let f = dir.path().join("foo.vitte");
        fs::write(&f, "/// test\nfn foo() {}").unwrap();

        let opts = DocGenOptions {
            root: dir.path().into(),
            format: "md".into(),
            output: None,
            exts: vec!["vitte".into()],
        };
        run(&opts).unwrap();
    }
}