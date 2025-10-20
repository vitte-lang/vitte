//! vitte-net — réseau minimal

#[derive(Debug)]
pub enum NetError {
    Io(std::io::Error),
    #[cfg(feature = "http")]
    Http(hyper::Error),
    Tls(String),
    Dns(String),
    Other(String),
}

pub type Result<T> = std::result::Result<T, NetError>;

impl From<std::io::Error> for NetError {
    fn from(e: std::io::Error) -> Self {
        NetError::Io(e)
    }
}
#[cfg(feature = "http")]
impl From<hyper::Error> for NetError {
    fn from(e: hyper::Error) -> Self {
        NetError::Http(e)
    }
}

impl core::fmt::Display for NetError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            NetError::Io(e) => write!(f, "I/O error: {e}"),
            #[cfg(feature = "http")]
            NetError::Http(e) => write!(f, "HTTP error: {e}"),
            NetError::Tls(e) => write!(f, "TLS error: {e}"),
            NetError::Dns(e) => write!(f, "DNS resolution failed: {e}"),
            NetError::Other(e) => write!(f, "Other: {e}"),
        }
    }
}
#[cfg(feature = "std")]
impl std::error::Error for NetError {}

#[cfg(feature = "rt-tokio")]
pub mod tcp {
    use super::*;
    use tokio::io::{AsyncReadExt, AsyncWriteExt};
    use tokio::net::{TcpListener, TcpStream};
    pub async fn connect(addr: &str) -> Result<TcpStream> {
        Ok(TcpStream::connect(addr).await?)
    }
    pub async fn listen(addr: &str) -> Result<TcpListener> {
        Ok(TcpListener::bind(addr).await?)
    }
    pub async fn send_all(s: &mut TcpStream, d: &[u8]) -> Result<()> {
        s.write_all(d).await?;
        Ok(())
    }
    pub async fn recv_all(s: &mut TcpStream) -> Result<Vec<u8>> {
        let mut b = Vec::new();
        s.read_to_end(&mut b).await?;
        Ok(b)
    }
}

#[cfg(all(feature = "http", feature = "rt-tokio"))]
pub mod http_client {
    use super::*;
    use hyper::client::HttpConnector;
    use hyper::{Body, Client, Request, Response};
    pub async fn get(url: &str) -> Result<Response<Body>> {
        let c = Client::new();
        let r = Request::builder().method("GET").uri(url).body(Body::empty()).unwrap();
        Ok(c.request(r).await?)
    }
    pub async fn post(url: &str, body: Vec<u8>) -> Result<Response<Body>> {
        let c = Client::new();
        let r = Request::builder().method("POST").uri(url).body(Body::from(body)).unwrap();
        Ok(c.request(r).await?)
    }
    pub fn client() -> Client<HttpConnector> {
        Client::new()
    }
}

#[cfg(all(feature = "ws", feature = "rt-tokio"))]
pub mod ws {
    use super::*;
    use futures_util::{SinkExt, StreamExt};
    use tokio_tungstenite::connect_async;
    use tungstenite::protocol::Message;
    pub async fn connect(
        url: &str,
    ) -> Result<(tokio_tungstenite::WebSocketStream<tokio::net::TcpStream>, http::Response<()>)>
    {
        let (ws, resp) = connect_async(url).await.map_err(|e| NetError::Other(e.to_string()))?;
        Ok((ws, resp))
    }
    pub async fn echo(url: &str, msg: &str) -> Result<String> {
        let (mut ws, _) = connect(url).await?;
        ws.send(Message::Text(msg.to_string()))
            .await
            .map_err(|e| NetError::Other(e.to_string()))?;
        if let Some(Ok(Message::Text(r))) = ws.next().await {
            Ok(r)
        } else {
            Err(NetError::Other("no reply".into()))
        }
    }
}

pub mod dns {
    use super::*;
    use std::net::ToSocketAddrs;
    pub fn resolve(host: &str, port: u16) -> Result<Vec<std::net::SocketAddr>> {
        Ok((host, port).to_socket_addrs().map_err(|e| NetError::Dns(e.to_string()))?.collect())
    }
}

#[cfg(feature = "tls-rustls")]
pub mod tls {
    use super::*;
    use rustls::pki_types::CertificateDer;
    use rustls::{ClientConfig, RootCertStore};
    pub fn client_config() -> Result<ClientConfig> {
        let mut roots = RootCertStore::empty();
        let certs: Vec<CertificateDer<'static>> = rustls_native_certs::load_native_certs()
            .map_err(|e| NetError::Tls(e.to_string()))?
            .into_iter()
            .collect();
        for c in certs {
            let _ = roots.add(c);
        }
        Ok(ClientConfig::builder().with_root_certificates(roots).with_no_client_auth())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn dns_resolve_localhost() {
        assert!(!dns::resolve("localhost", 80).unwrap().is_empty());
    }
}
