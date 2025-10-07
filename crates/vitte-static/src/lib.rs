

#![deny(missing_docs)]
//! vitte-static — gestion d’actifs statiques pour Vitte
//!
//! Fournit :
//! - Embedding compile-time (`rust-embed` ou `include_dir`).
//! - Chargement runtime, recherche MIME (`mime_guess`).
//! - Intégrations HTTP (`axum`, `hyper`).
//! - Manifestes JSON optionnels (`serde`).
//!
//! Exemple (embed):
//! ```ignore
//! #[cfg(feature="embed")]
//! #[derive(rust_embed::RustEmbed)]
//! #[folder = "static/"]
//! struct Assets;
//! ```

use thiserror::Error;

#[cfg(feature = "serde")]
use serde::{Serialize, Deserialize};

#[cfg(feature = "mime")]
use mime_guess::MimeGuess;

/// Erreurs liées aux assets
#[derive(Debug, Error)]
pub enum StaticError {
    /// Asset introuvable — contient le chemin/nome recherché.
    #[error("asset not found: {0}")]
    NotFound(String),
    /// Erreur d'entrée/sortie sous-jacente (lecture de fichier, etc.).
    #[error("io: {0}")]
    Io(#[from] std::io::Error),
    /// Autre erreur liée aux assets (message libre).
    #[error("other: {0}")]
    Other(String),
}

/// Résultat spécialisé
pub type Result<T> = std::result::Result<T, StaticError>;

/// Asset binaire en mémoire
#[cfg_attr(feature="serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone)]
pub struct Asset {
    /// Nom / chemin
    pub name: String,
    /// Contenu binaire
    pub data: Vec<u8>,
    /// MIME détecté
    #[cfg(feature = "mime")]
    pub mime: String,
}

impl Asset {
    /// Construit un asset depuis nom + data
    #[cfg(not(feature = "mime"))]
    pub fn new(name: &str, data: Vec<u8>) -> Self {
        Self { name: name.into(), data }
    }

    /// Construit un asset depuis nom + data, détecte MIME
    #[cfg(feature = "mime")]
    pub fn new(name: &str, data: Vec<u8>) -> Self {
        let mime = MimeGuess::from_path(name).first_or_octet_stream().essence_str().to_string();
        Self { name: name.into(), data, mime }
    }
}

#[cfg(feature = "embed")]
/// Retourne un asset intégré via rust-embed
pub fn get_embed<A: rust_embed::RustEmbed>(name: &str) -> Result<Asset> {
    if let Some(c) = A::get(name) {
        let bytes = c.data.into_owned();
        Ok(Asset::new(name, bytes))
    } else {
        Err(StaticError::NotFound(name.into()))
    }
}

#[cfg(feature = "include-dir")]
/// Retourne un asset intégré via include_dir
pub fn get_include(d: &include_dir::Dir, name: &str) -> Result<Asset> {
    if let Some(f) = d.get_file(name) {
        Ok(Asset::new(name, f.contents().to_vec()))
    } else {
        Err(StaticError::NotFound(name.into()))
    }
}

#[cfg(feature = "axum")]
/// Handler Axum qui sert un asset (GET)
pub async fn axum_handler<A: rust_embed::RustEmbed>(
    axum::extract::Path(path): axum::extract::Path<String>,
) -> std::result::Result<axum::response::Response, axum::http::StatusCode> {
    match get_embed::<A>(&path) {
        Ok(asset) => {
            let body = axum::body::Body::from(asset.data);
            let mut resp = axum::response::Response::new(body);
            #[cfg(feature = "mime")]
            {
                resp.headers_mut().insert(
                    axum::http::header::CONTENT_TYPE,
                    axum::http::HeaderValue::from_str(&asset.mime).unwrap(),
                );
            }
            Ok(resp)
        }
        Err(_) => Err(axum::http::StatusCode::NOT_FOUND),
    }
}

#[cfg(feature = "hyper")]
/// Service Hyper qui sert un asset unique
pub fn hyper_service<A: rust_embed::RustEmbed>(
    name: &'static str,
) -> impl hyper::service::Service<hyper::Request<hyper::body::Incoming>, Response=hyper::Response<hyper::Body>, Error=std::convert::Infallible> + Clone {
    use hyper::{Response, Body};
    let asset = get_embed::<A>(name).ok();
    tower::service_fn(move |_req| {
        let resp = if let Some(ref a) = asset {
            let mut r = Response::new(Body::from(a.data.clone()));
            #[cfg(feature="mime")]
            {
                r.headers_mut().insert(hyper::header::CONTENT_TYPE, hyper::header::HeaderValue::from_str(&a.mime).unwrap());
            }
            r
        } else {
            Response::builder().status(404).body(Body::empty()).unwrap()
        };
        async move { Ok::<_, std::convert::Infallible>(resp) }
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn asset_new_basic() {
        let a = Asset::new("foo.txt", b"hello".to_vec());
        assert_eq!(a.name, "foo.txt");
        assert_eq!(a.data, b"hello".to_vec());
    }
}