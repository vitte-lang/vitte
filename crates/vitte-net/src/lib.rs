

#![deny(missing_docs)]
//! vitte-net — abstractions réseau pour Vitte
//!
//! Objectifs :
//! - Sockets TCP/UDP async unifiées sur plusieurs runtimes (tokio/async-std/smol).
//! - Clients HTTP/1.1 et HTTP/2 (via hyper).
//! - WebSockets (tokio-tungstenite / tungstenite).
//! - TLS optionnel (rustls ou native-tls).
//! - Résolution DNS simple.
//!
//! Features :
//! - `rt-tokio`, `rt-async-std`, `rt-smol` pour choisir le runtime.
//! - `http` pour activer Hyper.
//! - `ws` pour activer WebSockets.
//! - `tls-rustls` ou `tls-native` pour TLS.
//! - `serde` pour (dé)serialisation JSON.

use thiserror::Error;

/// Erreurs réseau unifiées.
#[derive(Debug, Error)]
pub enum NetError {
    #[error("I/O error: {0}")]
    Io(#[from] std::io::Error),

    #[error("HTTP error: {0}")]
    #[cfg(feature="http")]
    Http(#[from] hyper::Error),

    #[error("TLS error: {0}")]
    Tls(String),

    #[error("DNS resolution failed: {0}")]
    Dns(String),

    #[error("Other: {0}")]
    Other(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, NetError>;

// ================================================================================================
// TCP / UDP sockets
// ================================================================================================
#[cfg(feature="rt-tokio")]
pub mod tcp {
    use super::*;
    use tokio::net::{TcpStream, TcpListener};
    use tokio::io::{AsyncReadExt, AsyncWriteExt};

    pub async fn connect(addr: &str) -> Result<TcpStream> {
        Ok(TcpStream::connect(addr).await?)
    }

    pub async fn listen(addr: &str) -> Result<TcpListener> {
        Ok(TcpListener::bind(addr).await?)
    }

    pub async fn send_all(stream: &mut TcpStream, data: &[u8]) -> Result<()> {
        stream.write_all(data).await?;
        Ok(())
    }

    pub async fn recv_all(stream: &mut TcpStream) -> Result<Vec<u8>> {
        let mut buf = Vec::new();
        stream.read_to_end(&mut buf).await?;
        Ok(buf)
    }
}

// ================================================================================================
// HTTP client (hyper)
// ================================================================================================
#[cfg(feature="http")]
pub mod http_client {
    use super::*;
    use hyper::{Client, Request, Body, Response};
    use hyper::client::HttpConnector;

    pub async fn get(url: &str) -> Result<Response<Body>> {
        let client = Client::new();
        let req = Request::builder().method("GET").uri(url).body(Body::empty()).unwrap();
        Ok(client.request(req).await?)
    }

    pub async fn post(url: &str, body: Vec<u8>) -> Result<Response<Body>> {
        let client = Client::new();
        let req = Request::builder().method("POST").uri(url).body(Body::from(body)).unwrap();
        Ok(client.request(req).await?)
    }

    pub fn client() -> Client<HttpConnector> {
        Client::new()
    }
}

// ================================================================================================
// WebSockets
// ================================================================================================
#[cfg(feature="ws")]
pub mod ws {
    use super::*;
    use futures_util::{SinkExt, StreamExt};
    use tokio_tungstenite::connect_async;
    use tungstenite::protocol::Message;

    pub async fn connect(url: &str) -> Result<(tokio_tungstenite::WebSocketStream<tokio::net::TcpStream>, http::Response<()>)> {
        let (ws, resp) = connect_async(url).await.map_err(|e| NetError::Other(e.to_string()))?;
        Ok((ws, resp))
    }

    pub async fn echo(url: &str, msg: &str) -> Result<String> {
        let (mut ws, _) = connect(url).await?;
        ws.send(Message::Text(msg.to_string())).await.map_err(|e| NetError::Other(e.to_string()))?;
        if let Some(Ok(Message::Text(reply))) = ws.next().await {
            return Ok(reply);
        }
        Err(NetError::Other("no reply".into()))
    }
}

// ================================================================================================
// DNS utilities
// ================================================================================================
pub mod dns {
    use super::*;
    use std::net::ToSocketAddrs;

    pub fn resolve(host: &str, port: u16) -> Result<Vec<std::net::SocketAddr>> {
        let addrs: Vec<_> = (host, port).to_socket_addrs().map_err(|e| NetError::Dns(e.to_string()))?.collect();
        Ok(addrs)
    }
}

// ================================================================================================
// TLS helpers (stubs)
// ================================================================================================
#[cfg(feature="tls-rustls")]
pub mod tls {
    use super::*;
    use rustls::{ClientConfig, RootCertStore};
    use std::sync::Arc;

    pub fn client_config() -> Result<ClientConfig> {
        let mut roots = RootCertStore::empty();
        rustls_native_certs::load_native_certs().map_err(|e| NetError::Tls(e.to_string()))?
            .into_iter().for_each(|c| { let _ = roots.add(&c); });
        let config = ClientConfig::builder().with_root_certificates(roots).with_no_client_auth();
        Ok(config)
    }
}

// ================================================================================================
// Tests
// ================================================================================================
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn dns_resolve_localhost() {
        let r = dns::resolve("localhost", 80).unwrap();
        assert!(!r.is_empty());
    }
}