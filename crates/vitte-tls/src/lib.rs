//! vitte-tls — abstraction TLS (Rustls par défaut, fallback native-tls)

#[cfg(feature = "rustls")]
use rustls::pki_types::{CertificateDer, PrivateKeyDer};
#[cfg(feature = "rustls")]
use tokio_rustls::rustls;

#[derive(Debug)]
pub enum TlsError {
    Io(std::io::Error),
    #[cfg(feature = "rustls")]
    Rustls(rustls::Error),
    #[cfg(feature = "native-tls")]
    Native(native_tls::Error),
    Other(String),
}
impl core::fmt::Display for TlsError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            TlsError::Io(e) => write!(f, "io: {e}"),
            #[cfg(feature = "rustls")]
            TlsError::Rustls(e) => write!(f, "rustls: {e}"),
            #[cfg(feature = "native-tls")]
            TlsError::Native(e) => write!(f, "native-tls: {e}"),
            TlsError::Other(s) => write!(f, "{s}"),
        }
    }
}
#[cfg(feature = "std")]
impl std::error::Error for TlsError {}
impl From<std::io::Error> for TlsError {
    fn from(e: std::io::Error) -> Self {
        Self::Io(e)
    }
}
#[cfg(feature = "rustls")]
impl From<rustls::Error> for TlsError {
    fn from(e: rustls::Error) -> Self {
        Self::Rustls(e)
    }
}
#[cfg(feature = "native-tls")]
impl From<native_tls::Error> for TlsError {
    fn from(e: native_tls::Error) -> Self {
        Self::Native(e)
    }
}

pub type Result<T> = std::result::Result<T, TlsError>;

pub struct TlsConnector {
    #[cfg(feature = "rustls")]
    #[allow(dead_code)]
    rustls: Option<rustls::ClientConfig>,
    #[cfg(feature = "native-tls")]
    #[allow(dead_code)]
    native: Option<native_tls::TlsConnector>,
}
impl TlsConnector {
    #[cfg(feature = "rustls")]
    pub fn rustls_client() -> Result<Self> {
        let root = rustls::RootCertStore::empty();
        #[cfg(feature = "rustls-native-certs")]
        {
            let certs: Vec<CertificateDer<'static>> = rustls_native_certs::load_native_certs()
                .map_err(|e| TlsError::Other(e.to_string()))?
                .into_iter()
                .collect();
            for cert in certs {
                let _ = root.add(cert);
            }
        }
        #[cfg(not(feature = "rustls-native-certs"))]
        {
            // Fallback: empty store; callers may inject roots separately.
        }
        let cfg = rustls::ClientConfig::builder()
            .with_root_certificates(root)
            .with_no_client_auth();
        Ok(Self {
            rustls: Some(cfg),
            #[cfg(feature = "native-tls")]
            native: None,
        })
    }
    #[cfg(feature = "native-tls")]
    pub fn native_client() -> Result<Self> {
        Ok(Self {
            native: Some(native_tls::TlsConnector::new()?),
            #[cfg(feature = "rustls")]
            rustls: None,
        })
    }
}

#[cfg(feature = "rustls")]
pub fn load_cert_pem(data: &[u8]) -> Result<Vec<CertificateDer<'static>>> {
    use std::io::BufReader;
    let mut rd = BufReader::new(data);
    rustls_pemfile::certs(&mut rd)
        .collect::<std::result::Result<_, _>>()
        .map_err(|_| TlsError::Other("invalid pem".into()))
}
#[cfg(feature = "rustls")]
pub fn load_key_pem<'a>(data: &'a [u8]) -> Result<PrivateKeyDer<'a>> {
    use std::io::BufReader;
    let mut rd = BufReader::new(data);
    let mut keys = rustls_pemfile::pkcs8_private_keys(&mut rd)
        .collect::<std::result::Result<Vec<_>, _>>()
        .map_err(|_| TlsError::Other("invalid key".into()))?;
    let pk = keys.pop().ok_or_else(|| TlsError::Other("no key".into()))?;
    Ok(PrivateKeyDer::from(pk))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn rustls_build() {
        #[cfg(feature = "rustls")]
        {
            assert!(TlsConnector::rustls_client().unwrap().rustls.is_some());
        }
    }
}
