#![deny(missing_docs)]
//! vitte-remote-cache — cache distant pour Vitte
//!
//! Fournit :
//! - Backend HTTP simple (PUT/GET).
//! - Support optionnel S3 (aws-sdk-s3).
//! - Support optionnel Redis.
//! - Clés de contenu (sha256 + base64).
//! - Compression optionnelle via zstd.
//! - Intégration asynchrone (tokio).
//!
//! Usage typique :
//! ```no_run
//! use vitte_remote_cache as cache;
//! # #[tokio::main] async fn main() -> cache::Result<()> {
//! let c = cache::RemoteCache::new_http("http://localhost:8080")?;
//! let key = cache::make_key(b"hello");
//! c.put(&key, b"world").await?;
//! let val = c.get(&key).await?;
//! assert_eq!(val, Some(b"world".to_vec()));
//! # Ok(()) }
//! ```

use base64::{engine::general_purpose::URL_SAFE_NO_PAD, Engine as _};
use bytes::Bytes;
use sha2::{Digest, Sha256};
use thiserror::Error;

#[cfg(feature = "http")]
use reqwest::Client;

#[cfg(feature = "zstd")]
use zstd::stream::{decode_all, encode_all};

/// Erreurs du cache distant.
#[derive(Debug, Error)]
pub enum CacheError {
    #[error("http error: {0}")]
    #[cfg(feature = "http")]
    /// Erreur HTTP (activée avec la feature `http`).
    Http(#[from] reqwest::Error),
    #[error("io error: {0}")]
    /// Erreur d'entrée/sortie du système de fichiers.
    Io(#[from] std::io::Error),
    #[error("redis error: {0}")]
    #[cfg(feature = "redis")]
    /// Erreur Redis (activée avec la feature `redis`).
    Redis(#[from] redis::RedisError),
    #[error("aws error: {0}")]
    #[cfg(feature = "s3")]
    /// Erreur du SDK AWS S3 (activée avec la feature `s3`).
    Aws(String),
    #[error("compression error: {0}")]
    /// Erreur de compression ou décompression (zstd).
    Compression(String),
    #[error("other: {0}")]
    /// Autre erreur générique.
    Other(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, CacheError>;

/// Génère une clé de contenu basée sur SHA256 base64-url.
pub fn make_key(data: &[u8]) -> String {
    let mut hasher = Sha256::new();
    hasher.update(data);
    let digest = hasher.finalize();
    URL_SAFE_NO_PAD.encode(digest)
}

/// Compression optionnelle.
fn maybe_compress(data: &[u8]) -> Result<Vec<u8>> {
    #[cfg(feature = "zstd")]
    {
        let v = encode_all(data, 0).map_err(|e| CacheError::Compression(e.to_string()))?;
        return Ok(v);
    }
    #[cfg(not(feature = "zstd"))]
    {
        Ok(data.to_vec())
    }
}

/// Décompression optionnelle.
fn maybe_decompress(data: &[u8]) -> Result<Vec<u8>> {
    #[cfg(feature = "zstd")]
    {
        let v = decode_all(data).map_err(|e| CacheError::Compression(e.to_string()))?;
        return Ok(v);
    }
    #[cfg(not(feature = "zstd"))]
    {
        Ok(data.to_vec())
    }
}

/// Backend de cache distant.
pub struct RemoteCache {
    #[cfg(feature = "http")]
    http: Option<(Client, String)>,
}

impl RemoteCache {
    /// Nouveau backend HTTP.
    #[cfg(feature = "http")]
    pub fn new_http(base: &str) -> Result<Self> {
        Ok(Self { http: Some((Client::new(), base.to_string())) })
    }

    /// Put (stocke) une valeur.
    pub async fn put(&self, key: &str, val: &[u8]) -> Result<()> {
        let data = maybe_compress(val)?;
        #[cfg(feature = "http")]
        if let Some((cli, base)) = &self.http {
            let url = format!("{base}/{key}");
            cli.put(url).body(Bytes::from(data)).send().await?.error_for_status()?;
            return Ok(());
        }
        Err(CacheError::Other("no backend enabled".into()))
    }

    /// Get (charge) une valeur.
    pub async fn get(&self, key: &str) -> Result<Option<Vec<u8>>> {
        #[cfg(feature = "http")]
        if let Some((cli, base)) = &self.http {
            let url = format!("{base}/{key}");
            let resp = cli.get(url).send().await?;
            if resp.status().is_success() {
                let b = resp.bytes().await?;
                return Ok(Some(maybe_decompress(&b)?));
            } else if resp.status().as_u16() == 404 {
                return Ok(None);
            } else {
                return Err(CacheError::Http(resp.error_for_status().unwrap_err()));
            }
        }
        Err(CacheError::Other("no backend enabled".into()))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn key_generation() {
        let k = make_key(b"abc");
        assert!(!k.is_empty());
    }

    #[tokio::test]
    #[cfg(feature = "http")]
    async fn http_backend_put_get() {
        use httpmock::MockServer;
        let server = MockServer::start();
        let key = make_key(b"xyz");
        let path = format!("/{key}");
        let m1 = server.mock(|when, then| {
            when.method("PUT").path(path.clone());
            then.status(200);
        });
        let m2 = server.mock(|when, then| {
            when.method("GET").path(path.clone());
            then.status(200).body("hello");
        });
        let c = RemoteCache::new_http(&server.base_url()).unwrap();
        c.put(&key, b"hello").await.unwrap();
        let val = c.get(&key).await.unwrap();
        assert_eq!(val, Some(b"hello".to_vec()));
        m1.assert();
        m2.assert();
    }
}
