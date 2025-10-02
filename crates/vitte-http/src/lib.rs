//! vitte-http — Client/serveur HTTP unifiés pour Vitte
//!
//! Features:
//! - `client`  : client HTTP basé sur reqwest (GET/POST JSON, texte, download).
//! - `server`  : serveur HTTP basé sur hyper (router minimal, middlewares simples).
//!
//! Objectif: API propre, async, avec erreurs unifiées.

#![forbid(unsafe_code)]
#![deny(missing_docs)]

use thiserror::Error;

/// Résultat standard.
pub type HttpResult<T> = Result<T, HttpError>;

/// Erreurs unifiées.
#[derive(Error, Debug)]
pub enum HttpError {
    /// Erreur client (réseau, HTTP…).
    #[cfg(feature = "client")]
    #[error("reqwest: {0}")]
    Reqwest(#[from] reqwest::Error),

    /// Erreur serveur (hyper).
    #[cfg(feature = "server")]
    #[error("hyper: {0}")]
    Hyper(#[from] hyper::Error),

    /// Statut HTTP non-2xx.
    #[error("http status {status}: {body_preview}")]
    Status {
        /// Code HTTP.
        status: u16,
        /// Extrait corps pour debug (troncature).
        body_preview: String,
    },

    /// I/O local.
    #[error("io: {0}")]
    Io(#[from] std::io::Error),

    /// Sérialisation JSON.
    #[cfg(any(feature = "client", feature = "server"))]
    #[error("json: {0}")]
    Json(#[from] serde_json::Error),

    /// Argument invalide.
    #[error("invalid: {0}")]
    Invalid(&'static str),

    /// Divers.
    #[error("{0}")]
    Other(String),
}

fn preview(s: &str, max: usize) -> String {
    let s = s.replace('\n', "\\n");
    if s.len() <= max {
        s
    } else {
        format!("{}…(+{} chars)", &s[..max], s.len() - max)
    }
}

#[cfg(feature = "client")]
mod client {
    use super::*;
    use std::time::Duration;

    /// Client HTTP configuré.
    #[derive(Clone)]
    pub struct HttpClient {
        inner: reqwest::Client,
    }

    /// Options de construction.
    #[derive(Debug, Clone)]
    pub struct ClientOptions {
        /// Timeout total par requête.
        pub timeout: Duration,
        /// User-Agent.
        pub user_agent: Option<String>,
        /// Redirections max.
        pub redirects: usize,
        /// Gzip/Brotli activés par défaut via features reqwest.
    }
    impl Default for ClientOptions {
        fn default() -> Self {
            Self {
                timeout: Duration::from_secs(30),
                user_agent: Some(format!(
                    "vitte-http/{}",
                    option_env!("CARGO_PKG_VERSION").unwrap_or("0.0.0")
                )),
                redirects: 10,
            }
        }
    }

    impl HttpClient {
        /// Construit un client.
        pub fn new(opts: &ClientOptions) -> HttpResult<Self> {
            let mut b = reqwest::Client::builder()
                .timeout(opts.timeout)
                .connect_timeout(Duration::from_secs(10))
                .pool_idle_timeout(Some(Duration::from_secs(30)))
                .redirect(reqwest::redirect::Policy::limited(opts.redirects));
            if let Some(ua) = &opts.user_agent {
                b = b.user_agent(ua.clone());
            }
            Ok(Self { inner: b.build()? })
        }

        /// GET texte.
        pub async fn get_text(&self, url: &str) -> HttpResult<String> {
            let resp = self.inner.get(url).send().await?;
            let status = resp.status();
            let text = resp.text().await?;
            if !status.is_success() {
                return Err(HttpError::Status {
                    status: status.as_u16(),
                    body_preview: preview(&text, 512),
                });
            }
            Ok(text)
        }

        /// GET JSON → T.
        pub async fn get_json<T: serde::de::DeserializeOwned>(&self, url: &str) -> HttpResult<T> {
            let resp = self.inner.get(url).send().await?;
            let status = resp.status();
            let text = resp.text().await?;
            if !status.is_success() {
                return Err(HttpError::Status {
                    status: status.as_u16(),
                    body_preview: preview(&text, 512),
                });
            }
            Ok(serde_json::from_str(&text)?)
        }

        /// POST JSON, retourne JSON.
        pub async fn post_json<T: serde::Serialize, R: serde::de::DeserializeOwned>(
            &self,
            url: &str,
            body: &T,
        ) -> HttpResult<R> {
            let resp = self
                .inner
                .post(url)
                .json(body)
                .header(reqwest::header::CONTENT_TYPE, "application/json")
                .send()
                .await?;
            let status = resp.status();
            let text = resp.text().await?;
            if !status.is_success() {
                return Err(HttpError::Status {
                    status: status.as_u16(),
                    body_preview: preview(&text, 512),
                });
            }
            Ok(serde_json::from_str(&text)?)
        }

        /// Télécharge vers un fichier (atomique).
        pub async fn download_to_file(&self, url: &str, path: &std::path::Path) -> HttpResult<()> {
            let resp = self.inner.get(url).send().await?;
            let status = resp.status();
            let bytes = resp.bytes().await?;
            if !status.is_success() {
                let text = String::from_utf8_lossy(&bytes).to_string();
                return Err(HttpError::Status {
                    status: status.as_u16(),
                    body_preview: preview(&text, 256),
                });
            }
            if let Some(parent) = path.parent() {
                std::fs::create_dir_all(parent)?;
            }
            let tmp = path.with_extension("tmp.download");
            std::fs::write(&tmp, &bytes)?;
            std::fs::rename(tmp, path)?;
            Ok(())
        }

        /// Requête générique avec headers. Renvoie (status, headers, body bytes).
        pub async fn request_bytes(
            &self,
            method: reqwest::Method,
            url: &str,
            headers: Option<reqwest::header::HeaderMap>,
            body: Option<Vec<u8>>,
        ) -> HttpResult<(u16, reqwest::header::HeaderMap, bytes::Bytes)> {
            let mut req = self.inner.request(method, url);
            if let Some(h) = headers {
                req = req.headers(h);
            }
            if let Some(b) = body {
                req = req.body(b);
            }
            let resp = req.send().await?;
            let status = resp.status().as_u16();
            let h = resp.headers().clone();
            let b = resp.bytes().await?;
            Ok((status, h, b))
        }
    }

    pub use reqwest::{header, Method};
    pub use HttpClient as Client;
    pub use ClientOptions;
}

#[cfg(feature = "server")]
mod server {
    use super::*;
    use hyper::{
        body::to_bytes,
        header::HeaderValue,
        http::Response,
        service::Service,
        Body, Method, Request, Server, StatusCode,
    };
    use std::{convert::Infallible, net::SocketAddr, sync::Arc};
    use tokio::signal;

    /// Réponse HTTP compacte.
    #[derive(Debug, Clone)]
    pub struct HttpResponse {
        /// Code HTTP.
        pub status: StatusCode,
        /// En-têtes.
        pub headers: Vec<(String, String)>,
        /// Corps (bytes).
        pub body: Vec<u8>,
    }
    impl HttpResponse {
        /// Réponse JSON.
        pub fn json<T: serde::Serialize>(status: StatusCode, v: &T) -> HttpResult<Self> {
            let s = serde_json::to_string(v)?;
            Ok(Self {
                status,
                headers: vec![(
                    "content-type".into(),
                    "application/json; charset=utf-8".into(),
                )],
                body: s.into_bytes(),
            })
        }
        /// Réponse texte.
        pub fn text(status: StatusCode, s: impl Into<String>) -> Self {
            Self {
                status,
                headers: vec![("content-type".into(), "text/plain; charset=utf-8".into())],
                body: s.into().into_bytes(),
            }
        }
        /// Réponse vide 204.
        pub fn empty() -> Self {
            Self {
                status: StatusCode::NO_CONTENT,
                headers: vec![],
                body: vec![],
            }
        }
    }

    /// Handler async de route.
    pub type Handler =
        Arc<dyn Send + Sync + Fn(Request<Body>) -> BoxFuture<'static, HttpResult<HttpResponse>>>;

    use futures_util::future::BoxFuture;

    /// Router minimal: (method, path) -> Handler.
    #[derive(Default, Clone)]
    pub struct Router {
        routes: Arc<Vec<(Method, String, Handler)>>,
    }

    impl Router {
        /// Nouveau routeur.
        pub fn new() -> Self {
            Self {
                routes: Arc::new(Vec::new()),
            }
        }
        /// Ajoute un handler.
        pub fn route<F, Fut>(self, method: Method, path: &str, f: F) -> Self
        where
            F: Send + Sync + 'static + Fn(Request<Body>) -> Fut,
            Fut: std::future::Future<Output = HttpResult<HttpResponse>> + Send + 'static,
        {
            let mut v = (*self.routes).clone();
            let h: Handler = Arc::new(move |req| Box::pin(f(req)));
            v.push((method, path.to_string(), h));
            Self {
                routes: Arc::new(v),
            }
        }
        /// GET.
        pub fn get<F, Fut>(self, path: &str, f: F) -> Self
        where
            F: Send + Sync + 'static + Fn(Request<Body>) -> Fut,
            Fut: std::future::Future<Output = HttpResult<HttpResponse>> + Send + 'static,
        {
            self.route(Method::GET, path, f)
        }
        /// POST.
        pub fn post<F, Fut>(self, path: &str, f: F) -> Self
        where
            F: Send + Sync + 'static + Fn(Request<Body>) -> Fut,
            Fut: std::future::Future<Output = HttpResult<HttpResponse>> + Send + 'static,
        {
            self.route(Method::POST, path, f)
        }

        fn find(&self, m: &Method, p: &str) -> Option<Handler> {
            // égalité exacte. Pour patterns, remplacer ici.
            self.routes
                .iter()
                .find(|(mm, pp, _)| mm == m && pp == p)
                .map(|(_, _, h)| h.clone())
        }
    }

    /// Démarre un serveur sur `addr`, avec arrêt sur Ctrl-C.
    pub async fn serve(addr: SocketAddr, router: Router) -> HttpResult<()> {
        let make = hyper::service::make_service_fn(move |_| {
            let r = router.clone();
            async move {
                Ok::<_, Infallible>(hyper::service::service_fn(move |req| {
                    let r2 = r.clone();
                    async move { Ok::<_, Infallible>(handle_request(r2, req).await) }
                }))
            }
        });

        let srv = Server::bind(&addr).serve(make);
        let graceful = srv.with_graceful_shutdown(async {
            let _ = signal::ctrl_c().await;
        });

        graceful.await?;
        Ok(())
    }

    async fn handle_request(router: Router, req: Request<Body>) -> Response<Body> {
        match router.find(req.method(), req.uri().path()) {
            Some(h) => match (h)(req).await {
                Ok(resp) => to_hyper(resp),
                Err(e) => {
                    let (code, msg) = map_error(&e);
                    to_hyper(HttpResponse::text(code, msg))
                }
            },
            None => to_hyper(HttpResponse::text(StatusCode::NOT_FOUND, "not found")),
        }
    }

    fn to_hyper(resp: HttpResponse) -> Response<Body> {
        let mut b = Response::builder().status(resp.status);
        {
            let headers = b.headers_mut().unwrap();
            for (k, v) in resp.headers {
                if let Ok(hv) = HeaderValue::from_str(&v) {
                    headers.append(hyper::header::HeaderName::from_bytes(k.as_bytes()).unwrap_or(hyper::header::CONTENT_TYPE), hv);
                }
            }
        }
        b.body(Body::from(resp.body)).unwrap_or_else(|_| {
            Response::builder()
                .status(StatusCode::INTERNAL_SERVER_ERROR)
                .body(Body::from("build response error"))
                .unwrap()
        })
    }

    fn map_error(e: &HttpError) -> (StatusCode, String) {
        match e {
            HttpError::Status { status, body_preview } => {
                (StatusCode::from_u16(*status).unwrap_or(StatusCode::BAD_GATEWAY), body_preview.clone())
            }
            HttpError::Json(err) => (StatusCode::BAD_REQUEST, err.to_string()),
            HttpError::Invalid(msg) => (StatusCode::BAD_REQUEST, msg.to_string()),
            HttpError::Io(err) => (StatusCode::INTERNAL_SERVER_ERROR, err.to_string()),
            HttpError::Other(s) => (StatusCode::INTERNAL_SERVER_ERROR, s.clone()),
            #[cfg(feature = "server")]
            HttpError::Hyper(err) => (StatusCode::BAD_GATEWAY, err.to_string()),
            #[cfg(feature = "client")]
            HttpError::Reqwest(err) => (StatusCode::BAD_GATEWAY, err.to_string()),
        }
    }

    /// Helpers d’extraction.

    /// Lit le corps entier en bytes.
    pub async fn body_bytes(req: Request<Body>) -> HttpResult<(Request<Body>, bytes::Bytes)> {
        let (parts, body) = req.into_parts();
        let bytes = to_bytes(body).await?;
        Ok((Request::from_parts(parts, Body::empty()), bytes))
    }

    /// Parse JSON du corps dans `T`.
    pub async fn body_json<T: serde::de::DeserializeOwned>(
        req: Request<Body>,
    ) -> HttpResult<(Request<Body>, T)> {
        let (req, b) = body_bytes(req).await?;
        let v = serde_json::from_slice::<T>(&b)?;
        Ok((req, v))
    }

    pub use hyper::{Body, Method, Request, StatusCode};
    pub use Router;
    pub use HttpResponse;
    pub use serve;
}

#[cfg(feature = "client")]
pub use client::*;

#[cfg(feature = "server")]
pub use server::*;

// ===================== Tests =====================

#[cfg(all(test, feature = "client"))]
mod client_tests {
    use super::*;
    #[tokio::test]
    async fn builds_client() {
        let c = Client::new(&ClientOptions::default()).unwrap();
        let _ = c.request_bytes(Method::GET, "https://example.com", None, None).await.unwrap();
    }
}

#[cfg(all(test, feature = "server"))]
mod server_tests {
    use super::*;
    use std::net::{Ipv4Addr, SocketAddrV4};
    use tokio::task;

    #[tokio::test]
    async fn serve_200() {
        let addr = SocketAddrV4::new(Ipv4Addr::LOCALHOST, 0);
        let listener = tokio::net::TcpListener::bind(addr).await.unwrap();
        let local = listener.local_addr().unwrap();

        let router = Router::new().get("/", |_req| async {
            Ok(HttpResponse::text(server::StatusCode::OK, "ok"))
        });

        let srv = task::spawn(async move {
            hyper::Server::from_tcp(listener)
                .unwrap()
                .serve(hyper::service::make_service_fn(move |_| {
                    let r = router.clone();
                    async move {
                        Ok::<_, std::convert::Infallible>(hyper::service::service_fn(move |req| {
                            let r2 = r.clone();
                            async move { Ok::<_, std::convert::Infallible>(server::handle_request(r2, req).await) }
                        }))
                    }
                }))
                .await
                .unwrap();
        });

        // simple probe with reqwest if feature client also present
        #[cfg(feature = "client")]
        {
            let c = Client::new(&ClientOptions::default()).unwrap();
            let url = format!("http://{}", local);
            let (code, _, body) = c.request_bytes(reqwest::Method::GET, &url, None, None).await.unwrap();
            assert_eq!(code, 200);
            assert_eq!(&body[..], b"ok");
        }

        srv.abort();
    }
}