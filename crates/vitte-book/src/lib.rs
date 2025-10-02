//! vitte-book — moteur de génération de documentation (MD → HTML/PDF/EPUB)
//!
//! Objectifs :
//! - Parcours d’un dossier `src/` rempli de `.md` et d’actifs statiques
//! - Conversion Markdown → HTML via `pulldown-cmark`
//! - Export optionnel en PDF (`printpdf`) et EPUB (`epub-builder`) via features
//! - Lecture d’un fichier `vitte-book.toml` pour la configuration
//! - Génération de la table des matières (depuis `SUMMARY.md` si présent) et métadonnées basiques
//!
//! API principale :
//! - [`build_all`], [`build_html`], [`build_pdf`], [`build_epub`]
//! - [`load_config`], [`discover_book`]

mod error {
    use std::{io, path::PathBuf};
    use thiserror::Error;

    #[derive(Debug, Error)]
    pub enum BookError {
        #[error("E/S: {0}")]
        Io(#[from] io::Error),
        #[error("TOML: {0}")]
        Toml(#[from] toml::de::Error),
        #[error("JSON: {0}")]
        Json(#[from] serde_json::Error),
        #[error("Chemin introuvable: {0:?}")]
        MissingPath(PathBuf),
        #[error("Aucun chapitre détecté")]
        NoChapters,
        #[error("PDF désactivé. Activez la feature `pdf`.")]
        PdfDisabled,
        #[error("EPUB désactivé. Activez la feature `epub`.")]
        EpubDisabled,
        #[cfg(feature = "pdf")]
        #[error("PDF: {0}")]
        Pdf(String),
        #[cfg(feature = "epub")]
        #[error("EPUB: {0}")]
        Epub(String),
        #[error("Config: {0}")]
        Config(String),
    }

    pub type Result<T> = std::result::Result<T, BookError>;
}
pub use error::{BookError, Result};

mod config {
    use serde::{Deserialize, Serialize};
    use std::{fs, path::Path};

    use crate::{BookError, Result};

    #[derive(Debug, Clone, Serialize, Deserialize)]
    pub struct Config {
        /// Dossier source contenant les .md et les assets (par défaut: "book")
        pub source_dir: String,
        /// Dossier de sortie (par défaut: "target/book")
        pub output_dir: String,
        /// Chemin CSS optionnel à injecter dans les pages HTML
        pub theme_css: Option<String>,
        /// Métadonnées
        pub title: Option<String>,
        pub author: Option<String>,
        pub language: Option<String>,
        /// Formats à produire par défaut: ["html"] | ["html","pdf","epub"]
        pub default_formats: Option<Vec<String>>,
        /// Inclure README.md comme page d’accueil si présent
        pub include_readme: Option<bool>,
    }

    impl Default for Config {
        fn default() -> Self {
            Self {
                source_dir: "book".into(),
                output_dir: "target/book".into(),
                theme_css: None,
                title: Some("Vitte — Documentation".into()),
                author: None,
                language: Some("fr".into()),
                default_formats: Some(vec!["html".into()]),
                include_readme: Some(true),
            }
        }
    }

    pub fn load_config(path: &Path) -> Result<Config> {
        if !path.exists() {
            return Ok(Config::default());
        }
        let s = fs::read_to_string(path)?;
        let cfg: Config = toml::from_str(&s)?;
        Ok(cfg)
    }

    pub use Config as BookConfig;
}
pub use config::{load_config, BookConfig};

mod model {
    use serde::{Deserialize, Serialize};
    use std::path::PathBuf;

    #[derive(Debug, Clone, Serialize, Deserialize)]
    pub struct Chapter {
        pub id: String,
        pub title: String,
        pub source: PathBuf,
        pub order: usize,
        pub html: String,
        pub raw_md: String,
    }

    #[derive(Debug, Clone, Serialize, Deserialize)]
    pub struct TocItem {
        pub title: String,
        pub id: String,
        pub order: usize,
    }

    #[derive(Debug, Clone, Serialize, Deserialize)]
    pub struct Book {
        pub title: String,
        pub author: Option<String>,
        pub language: Option<String>,
        pub chapters: Vec<Chapter>,
        pub toc: Vec<TocItem>,
        pub assets: Vec<PathBuf>,
    }
}
pub use model::{Book, Chapter, TocItem};

mod md {
    use pulldown_cmark::{html, Options, Parser};

    pub fn md_to_html(input: &str) -> String {
        let mut opts = Options::empty();
        opts.insert(Options::ENABLE_TABLES);
        opts.insert(Options::ENABLE_FOOTNOTES);
        opts.insert(Options::ENABLE_STRIKETHROUGH);
        opts.insert(Options::ENABLE_TASKLISTS);
        let parser = Parser::new_ext(input, opts);
        let mut out = String::with_capacity(input.len() * 2);
        html::push_html(&mut out, parser);
        out
    }

    /// Extrait un titre H1 comme titre de chapitre si possible, sinon fallback.
    pub fn infer_title(md: &str, fallback: &str) -> String {
        for line in md.lines() {
            let l = line.trim();
            if l.starts_with("# ") {
                return l.trim_start_matches("# ").trim().to_string();
            }
        }
        fallback.to_string()
    }

    /// Slug rudimentaire pour id d’ancre.
    pub fn slugify(s: &str) -> String {
        let mut out = String::with_capacity(s.len());
        for ch in s.chars() {
            if ch.is_ascii_alphanumeric() {
                out.push(ch.to_ascii_lowercase());
            } else if ch.is_whitespace() || ch == '-' || ch == '_' {
                out.push('-');
            }
        }
        while out.contains("--") {
            out = out.replace("--", "-");
        }
        out.trim_matches('-').to_string()
    }
}

mod fsops {
    use rayon::prelude::*;
    use std::{
        ffi::OsStr,
        fs,
        path::{Path, PathBuf},
    };
    use walkdir::WalkDir;

    use crate::md::{infer_title, md_to_html, slugify};
    use crate::{Book, BookConfig, Chapter, Result, TocItem};

    pub fn discover_book(cfg: &BookConfig) -> Result<Book> {
        let src = Path::new(&cfg.source_dir);
        if !src.exists() {
            return Err(crate::BookError::MissingPath(src.into()));
        }

        let mut md_files: Vec<PathBuf> = WalkDir::new(src)
            .into_iter()
            .filter_map(|e| e.ok())
            .filter(|e| {
                let p = e.path();
                p.is_file()
                    && p.extension()
                        .and_then(OsStr::to_str)
                        .map(|ext| ext.eq_ignore_ascii_case("md"))
                        .unwrap_or(false)
            })
            .map(|e| e.path().to_path_buf())
            .collect();

        // SUMMARY.md sert d’ordre si présent
        let summary = src.join("SUMMARY.md");
        let mut order_map: Vec<PathBuf> = Vec::new();
        if summary.exists() {
            let s = fs::read_to_string(&summary).unwrap_or_default();
            for line in s.lines() {
                // Format commun mdBook: "* [Titre](chap/path.md)"
                if let Some(start) = line.find('(') {
                    if let Some(end) = line.find(')') {
                        let rel = &line[start + 1..end];
                        let p = src.join(rel);
                        if p.exists() {
                            order_map.push(p);
                        }
                    }
                }
            }
        }

        // Optionnellement inclure README.md en premier si demandé
        if cfg.include_readme.unwrap_or(true) {
            let readme = src.join("README.md");
            if readme.exists() && !order_map.contains(&readme) {
                order_map.insert(0, readme);
            }
        }

        // Tri par SUMMARY si dispo, sinon tri lexicographique
        if !order_map.is_empty() {
            md_files.sort_by_key(|p| {
                order_map
                    .iter()
                    .position(|q| q == p)
                    .unwrap_or(usize::MAX - 1)
            });
        } else {
            md_files.sort();
        }

        // Assets = tout sauf .md
        let mut assets: Vec<PathBuf> = WalkDir::new(src)
            .into_iter()
            .filter_map(|e| e.ok())
            .map(|e| e.path().to_path_buf())
            .filter(|p| {
                p.is_file()
                    && p.extension()
                        .and_then(OsStr::to_str)
                        .map(|ext| !ext.eq_ignore_ascii_case("md"))
                        .unwrap_or(true)
            })
            .collect();

        assets.sort();
        assets.dedup();

        // Charger Markdown en parallèle
        let chapters: Vec<Chapter> = md_files
            .par_iter()
            .enumerate()
            .map(|(i, path)| {
                let raw = std::fs::read_to_string(path)?;
                let title_guess = path
                    .file_stem()
                    .and_then(OsStr::to_str)
                    .unwrap_or("Chapitre");
                let title = infer_title(&raw, title_guess);
                let html = md_to_html(&raw);
                let id = slugify(&title);
                Ok(Chapter {
                    id,
                    title,
                    source: path.clone(),
                    order: i,
                    html,
                    raw_md: raw,
                })
            })
            .collect::<Result<Vec<_>>>()?;

        if chapters.is_empty() {
            return Err(crate::BookError::NoChapters);
        }

        let toc: Vec<TocItem> = chapters
            .iter()
            .map(|c| TocItem {
                title: c.title.clone(),
                id: c.id.clone(),
                order: c.order,
            })
            .collect();

        let title = cfg
            .title
            .clone()
            .unwrap_or_else(|| "Vitte — Documentation".into());

        Ok(Book {
            title,
            author: cfg.author.clone(),
            language: cfg.language.clone(),
            chapters,
            toc,
            assets,
        })
    }

    pub fn ensure_clean_dir(dir: &Path) -> Result<()> {
        if dir.exists() {
            std::fs::remove_dir_all(dir)?;
        }
        std::fs::create_dir_all(dir)?;
        Ok(())
    }

    pub fn copy_assets(src_root: &Path, out_root: &Path, assets: &[PathBuf]) -> Result<()> {
        for a in assets {
            if !a.starts_with(src_root) {
                continue;
            }
            let rel = a.strip_prefix(src_root).unwrap();
            let dest = out_root.join(rel);
            if let Some(parent) = dest.parent() {
                std::fs::create_dir_all(parent)?;
            }
            std::fs::copy(a, &dest)?;
        }
        Ok(())
    }
}
pub use fsops::{discover_book, ensure_clean_dir};

mod html {
    use std::path::{Path, PathBuf};

    use crate::{Book, Result};

    fn base_html(title: &str, css_href: Option<&str>, body: &str) -> String {
        let css_link = css_href
            .map(|p| format!(r#"<link rel="stylesheet" href="{p}"/>"#))
            .unwrap_or_default();
        format!(
            r#"<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>{}</title>
{css}
<style>
body{{font-family:system-ui,-apple-system,Segoe UI,Roboto,Ubuntu,Cantarell,Noto Sans,sans-serif;max-width:920px;margin:0 auto;padding:24px;line-height:1.6}}
main{{display:block}}
nav.toc{{border-left:3px solid #ddd;padding-left:12px;margin:16px 0}}
article.chapter{{margin:40px 0}}
article.chapter h1{{border-bottom:1px solid #e5e5e5;padding-bottom:6px}}
pre,code{{font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace}}
pre{{overflow:auto;background:#f7f7f7;padding:12px;border-radius:6px}}
a{{color:#0b57d0;text-decoration:none}} a:hover{{text-decoration:underline}}
footer{{margin:48px 0;color:#666;font-size:.9em}}
</style>
</head>
<body>
{body}
</body>
</html>"#,
            html_escape::encode_text(title),
            css = css_link,
            body = body
        )
    }

    pub fn write_site(book: &Book, out_dir: &Path, css: Option<&Path>) -> Result<()> {
        std::fs::create_dir_all(out_dir)?;

        // TOC
        let mut nav = String::from(r#"<nav class="toc"><strong>Sommaire</strong><ol>"#);
        for t in &book.toc {
            nav.push_str(&format!(r#"<li><a href="{}.html">{}</a></li>"#, t.id, html_escape::encode_text(&t.title)));
        }
        nav.push_str("</ol></nav>");

        // Index
        let mut index_body = format!(
            r#"<header><h1>{}</h1><p>{}</p></header>{}"#,
            html_escape::encode_text(&book.title),
            html_escape::encode_text(book.author.as_deref().unwrap_or("")),
            nav
        );

        for c in &book.chapters {
            index_body.push_str(&format!(
                r#"<article class="chapter"><h2><a href="{id}.html">{title}</a></h2>
<p>Chapitre {order}</p></article>"#,
                id = c.id,
                title = html_escape::encode_text(&c.title),
                order = c.order + 1
            ));
        }

        let css_href = css.map(|p| p.file_name().unwrap().to_string_lossy().to_string());
        let index_html = base_html(&book.title, css_href.as_deref(), &format!(r#"<main>{}</main><footer>Généré par vitte-book</footer>"#, index_body));
        std::fs::write(out_dir.join("index.html"), index_html)?;

        // Chapitres
        for c in &book.chapters {
            let body = format!(
                r#"<main><article class="chapter"><h1 id="{id}">{title}</h1>{html}</article></main><footer>Chapitre {n}</footer>"#,
                id = c.id,
                title = html_escape::encode_text(&c.title),
                html = c.html,
                n = c.order + 1
            );
            let page = base_html(&format!("{} — {}", book.title, c.title), css_href.as_deref(), &body);
            std::fs::write(out_dir.join(format!("{}.html", c.id)), page)?;
        }

        Ok(())
    }

    pub fn maybe_copy_css(css_path: Option<&Path>, out_dir: &Path) -> Result<Option<PathBuf>> {
        if let Some(css) = css_path {
            let dest = out_dir.join(
                css.file_name()
                    .unwrap_or_else(|| std::ffi::OsStr::new("theme.css")),
            );
            std::fs::copy(css, &dest)?;
            return Ok(Some(dest));
        }
        Ok(None)
    }
}
pub use html::write_site;

#[cfg(feature = "pdf")]
mod pdf {
    use printpdf::*;
    use std::fs::File;
    use std::io::BufWriter;
    use std::path::Path;

    use crate::{Book, Result};

    /// Rendu PDF minimaliste: concatène les chapitres sous forme de texte brut.
    /// But: fournir une sortie simple, fiable, sans mise en page avancée.
    pub fn write_pdf(book: &Book, out_dir: &Path) -> Result<()> {
        std::fs::create_dir_all(out_dir)?;
        let (doc, page1, layer1) =
            PdfDocument::new(&book.title, Mm(210.0), Mm(297.0), "Layer 1");
        let current_layer = doc.get_page(page1).get_layer(layer1);

        let font = doc.add_external_font(include_bytes!("./DejaVuSans.ttf") as &[u8])
            .unwrap_or_else(|_| doc.add_builtin_font(BuiltinFont::Helvetica).unwrap());

        let mut y = Mm(280.0);

        // Titre
        current_layer.use_text(&book.title, 18.0, Mm(15.0), y, &font);
        y.0 -= 12.0;

        for (idx, c) in book.chapters.iter().enumerate() {
            y.0 -= 6.0;
            current_layer.use_text(
                format!("{}. {}", idx + 1, c.title),
                12.0,
                Mm(15.0),
                y,
                &font,
            );
            y.0 -= 8.0;

            // Texte brut à partir du Markdown source
            for line in c.raw_md.lines() {
                if line.trim().is_empty() {
                    y.0 -= 4.0;
                    continue;
                }
                current_layer.use_text(line.replace('\t', "    "), 9.0, Mm(15.0), y, &font);
                y.0 -= 4.0;
                if y.0 < 20.0 {
                    // Nouvelle page
                    let (p, l) = doc.add_page(Mm(210.0), Mm(297.0), "Layer");
                    let nl = doc.get_page(p).get_layer(l);
                    y = Mm(280.0);
                    nl.use_text(format!("Suite — {}", c.title), 10.0, Mm(15.0), y, &font);
                    y.0 -= 10.0;
                }
            }
            y.0 -= 6.0;
        }

        let mut buf = BufWriter::new(File::create(out_dir.join("book.pdf"))?);
        doc.save(&mut buf).map_err(|e| crate::BookError::Pdf(e.to_string()))?;
        Ok(())
    }
}
#[cfg(feature = "pdf")]
pub use pdf::write_pdf;

#[cfg(not(feature = "pdf"))]
mod pdf_stub {
    use std::path::Path;

    use crate::{Book, BookError, Result};

    pub fn write_pdf(_book: &Book, _out_dir: &Path) -> Result<()> {
        Err(BookError::PdfDisabled)
    }
}
#[cfg(not(feature = "pdf"))]
pub use pdf_stub::write_pdf;

#[cfg(feature = "epub")]
mod epubx {
    use epub_builder::{EpubBuilder, EpubContent, ReferenceType, ZipLibrary};
    use std::fs::File;
    use std::path::Path;

    use crate::{Book, Result};

    pub fn write_epub(book: &Book, out_dir: &Path) -> Result<()> {
        std::fs::create_dir_all(out_dir)?;
        let file = File::create(out_dir.join("book.epub"))?;
        let mut epub = EpubBuilder::new(ZipLibrary::new()?)
            .map_err(|e| crate::BookError::Epub(e.to_string()))?;

        epub.metadata("title", &book.title)
            .and_then(|_| {
                if let Some(a) = &book.author {
                    epub.metadata("creator", a)
                } else {
                    Ok(())
                }
            })
            .and_then(|_| epub.metadata("lang", book.language.as_deref().unwrap_or("fr")))
            .map_err(|e| crate::BookError::Epub(e.to_string()))?;

        // Sommaire
        let mut toc_html = String::from("<h1>Sommaire</h1><ol>");
        for t in &book.toc {
            toc_html.push_str(&format!(r#"<li><a href="{}.xhtml">{}</a></li>"#, t.id, html_escape::encode_text(&t.title)));
        }
        toc_html.push_str("</ol>");
        epub.add_content(
            EpubContent::new("index.xhtml", toc_html.as_bytes())
                .title("Sommaire")
                .reftype(ReferenceType::Text),
        )
        .map_err(|e| crate::BookError::Epub(e.to_string()))?;

        for c in &book.chapters {
            let page = format!(
                r#"<?xml version="1.0" encoding="utf-8"?>
<html xmlns="http://www.w3.org/1999/xhtml" lang="fr">
<head><meta charset="utf-8"/><title>{}</title></head>
<body><h1 id="{}">{}</h1>{}</body></html>"#,
                html_escape::encode_text(&c.title),
                c.id,
                html_escape::encode_text(&c.title),
                c.html
            );
            epub.add_content(
                EpubContent::new(format!("{}.xhtml", c.id), page.as_bytes())
                    .title(&c.title)
                    .reftype(ReferenceType::Text),
            )
            .map_err(|e| crate::BookError::Epub(e.to_string()))?;
        }

        epub.generate(file)
            .map_err(|e| crate::BookError::Epub(e.to_string()))?;
        Ok(())
    }
}
#[cfg(feature = "epub")]
pub use epubx::write_epub;

#[cfg(not(feature = "epub"))]
mod epub_stub {
    use std::path::Path;

    use crate::{Book, BookError, Result};

    pub fn write_epub(_book: &Book, _out_dir: &Path) -> Result<()> {
        Err(BookError::EpubDisabled)
    }
}
#[cfg(not(feature = "epub"))]
pub use epub_stub::write_epub;

mod orchestrator {
    use std::path::{Path, PathBuf};

    use crate::config::BookConfig;
    use crate::{discover_book, ensure_clean_dir, html, write_epub, write_pdf, Book, Result};

    pub struct BuildOutput {
        pub out_dir: PathBuf,
        pub html_dir: PathBuf,
        pub pdf_path: Option<PathBuf>,
        pub epub_path: Option<PathBuf>,
    }

    pub fn build_html(cfg: &BookConfig) -> Result<(Book, PathBuf)> {
        let book = discover_book(cfg)?;
        let out_dir = Path::new(&cfg.output_dir).join("html");
        ensure_clean_dir(&out_dir)?;
        let css_path = cfg.theme_css.as_ref().map(Path::new);
        let copied = html::maybe_copy_css(css_path, &out_dir)?;
        html::write_site(&book, &out_dir, copied.as_deref())?;
        // copier les assets
        let src_root = std::path::Path::new(&cfg.source_dir);
        crate::fsops::copy_assets(src_root, &out_dir, &book.assets)?;
        Ok((book, out_dir))
    }

    pub fn build_all(cfg: &BookConfig) -> Result<BuildOutput> {
        let (book, html_dir) = build_html(cfg)?;
        let out_dir = Path::new(&cfg.output_dir).to_path_buf();

        let mut pdf_path = None;
        let mut epub_path = None;

        if cfg
            .default_formats
            .as_ref()
            .map(|v| v.iter().any(|f| f.eq_ignore_ascii_case("pdf")))
            .unwrap_or(false)
        {
            let p = out_dir.join("pdf");
            ensure_clean_dir(&p)?;
            write_pdf(&book, &p)?;
            pdf_path = Some(p.join("book.pdf"));
        }

        if cfg
            .default_formats
            .as_ref()
            .map(|v| v.iter().any(|f| f.eq_ignore_ascii_case("epub")))
            .unwrap_or(false)
        {
            let e = out_dir.join("epub");
            ensure_clean_dir(&e)?;
            write_epub(&book, &e)?;
            epub_path = Some(e.join("book.epub"));
        }

        Ok(BuildOutput {
            out_dir,
            html_dir,
            pdf_path,
            epub_path,
        })
    }
}
pub use orchestrator::{build_all, build_html, BuildOutput};

/// Helpers de haut niveau

use std::path::Path;

/// Construit tout selon la config TOML si existante, sinon valeurs par défaut.
pub fn build_all_from_file(config_path: impl AsRef<Path>) -> Result<orchestrator::BuildOutput> {
    let cfg = load_config(config_path.as_ref())?;
    build_all(&cfg)
}

/// Construit uniquement le site HTML.
pub fn build_html_from_file(config_path: impl AsRef<Path>) -> Result<(Book, std::path::PathBuf)> {
    let cfg = load_config(config_path.as_ref())?;
    build_html(&cfg)
}