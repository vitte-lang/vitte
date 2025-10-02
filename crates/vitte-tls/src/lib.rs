

#![deny(missing_docs)]
//! vitte-tls — abstraction TLS pour Vitte
//!
//! Fournit :
//! - Client/serveur TLS via Rustls (par défaut).
//! - Intégration tokio/async-std.
//! - Fallback native-tls (macOS/Windows).
//! - Gestion certificats PEM, PKI.
//!
//! Exemple :
//! ```ignore
//! use vitte_tls::{TlsConnector, TlsError};
//! let cx = TlsConnector::rustls_client().unwrap();
//! ```

use thiserror::Error;

#[cfg(feature = "rustls")]
use tokio_rustls::rustls;

#[cfg(feature = "native-tls")]
use tokio_native_tls as tokio_tls;

/// Erreurs TLS
#[derive(Debug, Error)]
pub enum TlsError {
    #[error("io: {0}")]
    Io(#[from] std::io::Error),
    #[cfg(feature = "rustls")]
    #[error("rustls: {0}")]
    Rustls(#[from] rustls::Error),
    #[cfg(feature = "native-tls")]
    #[error("native-tls: {0}")]
    Native(#[from] native_tls::Error),
    #[error("autre: {0}")]
    Other(String),
}

/// Résultat spécialisé
pub type Result<T> = std::result::Result<T, TlsError>;

/// Connecteur TLS abstrait
pub struct TlsConnector {
    #[cfg(feature = "rustls")]
    rustls: Option<rustls::ClientConfig>,
    #[cfg(feature = "native-tls")]
    native: Option<native_tls::TlsConnector>,
}

impl TlsConnector {
    /// Constructeur Rustls
    #[cfg(feature = "rustls")]
    pub fn rustls_client() -> Result<Self> {
        let mut root = rustls::RootCertStore::empty();
        root.add_server_trust_anchors(webpki_roots::TLS_SERVER_ROOTS.iter().map(|ta| {
            rustls::OwnedTrustAnchor::from_subject_spki_name_constraints(
                ta.subject, ta.spki, ta.name_constraints,
            )
        }));
        let cfg = rustls::ClientConfig::builder()
            .with_safe_defaults()
            .with_root_certificates(root)
            .with_no_client_auth();
        Ok(Self { rustls: Some(cfg), #[cfg(feature="native-tls")] native: None })
    }

    /// Constructeur native-tls
    #[cfg(feature = "native-tls")]
    pub fn native_client() -> Result<Self> {
        let cx = native_tls::TlsConnector::new()?;
        Ok(Self { native: Some(cx), #[cfg(feature="rustls")] rustls: None })
    }
}

/// Charge un certificat PEM en rustls::Certificate
#[cfg(feature="rustls")]
pub fn load_cert_pem(data: &[u8]) -> Result<Vec<rustls::Certificate>> {
    use std::io::BufReader;
    let mut rd = BufReader::new(data);
    let certs = rustls_pemfile::certs(&mut rd)
        .map_err(|_| TlsError::Other("invalid pem".into()))?;
    Ok(certs.into_iter().map(rustls::Certificate).collect())
}

/// Charge une clé privée PEM en rustls::PrivateKey
#[cfg(feature="rustls")]
pub fn load_key_pem(data: &[u8]) -> Result<rustls::PrivateKey> {
    use std::io::BufReader;
    let mut rd = BufReader::new(data);
    let keys = rustls_pemfile::pkcs8_private_keys(&mut rd)
        .map_err(|_| TlsError::Other("invalid key".into()))?;
    keys.into_iter().next()
        .map(rustls::PrivateKey)
        .ok_or_else(|| TlsError::Other("no key".into()))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn construct_rustls() {
        #[cfg(feature="rustls")]
        {
            let cx = TlsConnector::rustls_client().unwrap();
            assert!(cx.rustls.is_some());
        }
    }
}