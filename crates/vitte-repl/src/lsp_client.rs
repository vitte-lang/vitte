//! Client léger pour `vitte-lsp` (mode JSON-RPC/WebSocket) utilisé par le REPL.

use serde::{Deserialize, Serialize};
use serde_json::json;
use std::fs;
use std::io;
use std::path::{Path, PathBuf};
use thiserror::Error;
use tungstenite::client::IntoClientRequest;
use tungstenite::protocol::Message;
use tungstenite::stream::MaybeTlsStream;
use tungstenite::{connect, WebSocket};
use url::Url;

type WsStream = WebSocket<MaybeTlsStream<std::net::TcpStream>>;

/// Erreurs potentielles du client LSP.
#[derive(Debug, Error)]
pub enum LspClientError {
    /// Aucun socket connu.
    #[error("no LSP session available")] 
    NoSession,
    /// Erreur d'E/S ou de fichier.
    #[error("io error: {0}")]
    Io(#[from] io::Error),
    /// Erreur JSON sérialisation/désérialisation.
    #[error("json error: {0}")]
    Json(#[from] serde_json::Error),
    /// Erreur réseau (connexion, handshake, etc.).
    #[error("network error: {0}")]
    Network(String),
    /// Réponse JSON-RPC invalide.
    #[error("invalid response from LSP: {0}")]
    InvalidResponse(String),
}

/// Informations extraites du fichier de sessions.
#[derive(Debug, Clone, Deserialize)]
pub struct SessionInfo {
    /// Identifiant de session côté LSP.
    #[serde(rename = "sessionId")]
    pub session_id: String,
    /// Racine du workspace.
    #[serde(rename = "workspaceRoot")]
    pub workspace_root: PathBuf,
    /// Endpoint WebSocket (ex: ws://127.0.0.1:9337/repl).
    pub endpoint: Url,
    /// Indique si c'est la session primaire.
    pub primary: bool,
}

/// Résultat d'une synchronisation.
#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct SyncResponse {
    /// Cellules manquantes.
    pub missing_cells: Vec<CellDigest>,
    /// Export global synchronisé.
    pub exports: Vec<ExportSymbol>,
}

/// Export de symbole simple.
#[derive(Debug, Clone, Deserialize, Serialize)]
pub struct ExportSymbol {
    pub name: String,
    pub kind: String,
    pub signature: Option<String>,
}

/// Empreinte de cellule.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CellDigest {
    #[serde(rename = "cellId")]
    pub cell_id: String,
    pub hash: String,
}

/// Client WebSocket connecté.
pub struct LspClient {
    session: SessionInfo,
    socket: WsStream,
    request_id: i64,
}

impl std::fmt::Debug for LspClient {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("LspClient")
            .field("session_id", &self.session.session_id)
            .field("endpoint", &self.session.endpoint)
            .finish()
    }
}

impl LspClient {
    /// Tente de se connecter à la première session disponible.
    pub fn connect_default() -> Result<Self, LspClientError> {
        let sessions = discover_sessions()?;
        let info = sessions.into_iter().next().ok_or(LspClientError::NoSession)?;
        Self::connect(info)
    }

    /// Connexion explicite à une session.
    pub fn connect(session: SessionInfo) -> Result<Self, LspClientError> {
        let request = session
            .endpoint
            .clone()
            .into_client_request()
            .map_err(|e| LspClientError::Network(e.to_string()))?;
        let (socket, _response) = connect(request).map_err(|e| LspClientError::Network(e.to_string()))?;

        let mut client = Self { session, socket, request_id: 0 };
        client.perform_handshake()?;
        Ok(client)
    }

    /// Connexion directe via un endpoint fourni.
    pub fn connect_endpoint(url: Url, workspace: Option<PathBuf>) -> Result<Self, LspClientError> {
        let info = SessionInfo {
            session_id: "external".into(),
            workspace_root: workspace.unwrap_or_else(|| std::env::current_dir().unwrap_or_else(|_| PathBuf::from("."))),
            endpoint: url,
            primary: true,
        };
        Self::connect(info)
    }

    /// Retourne l'identifiant de session.
    pub fn session_id(&self) -> &str {
        &self.session.session_id
    }

    /// Résumé textuel de l'état du client.
    pub fn status_line(&self) -> String {
        format!(
            "session {} @ {}",
            self.session.session_id,
            self.session.endpoint
        )
    }

    /// Synchronise l'état en envoyant `repl/syncState`.
    pub fn sync_state(&mut self, known: &[CellDigest]) -> Result<SyncResponse, LspClientError> {
        let id = self.next_request_id();
        let payload = json!({
            "jsonrpc": "2.0",
            "id": id,
            "method": "repl/syncState",
            "params": {
                "sessionId": self.session.session_id,
                "knownCells": known,
            }
        });
        self.socket
            .send(Message::Text(payload.to_string()))
            .map_err(|e| LspClientError::Network(e.to_string()))?;

        let response = self.read_text_message()?;
        let value: serde_json::Value = serde_json::from_str(&response)?;
        if value.get("id").and_then(|v| v.as_i64()) != Some(id) {
            return Err(LspClientError::InvalidResponse(response));
        }
        if let Some(result) = value.get("result") {
            Ok(serde_json::from_value(result.clone())?)
        } else if let Some(error) = value.get("error") {
            Err(LspClientError::InvalidResponse(error.to_string()))
        } else {
            Err(LspClientError::InvalidResponse(response))
        }
    }

    /// Ferme la connexion proprement.
    pub fn close(mut self) {
        let _ = self.socket.close(None);
    }

    fn perform_handshake(&mut self) -> Result<(), LspClientError> {
        let id = self.next_request_id();
        let payload = json!({
            "jsonrpc": "2.0",
            "id": id,
            "method": "repl/attach",
            "params": {
                "clientName": "vitte-repl",
                "clientVersion": env!("CARGO_PKG_VERSION"),
                "workspaceRoot": self.session.workspace_root.to_string_lossy(),
                "sessionId": self.session.session_id,
                "capabilities": {
                    "diagnostics": true,
                    "completion": true,
                    "hover": true,
                    "navigation": true,
                }
            }
        });
        self.socket
            .send(Message::Text(payload.to_string()))
            .map_err(|e| LspClientError::Network(e.to_string()))?;

        let response = self.read_text_message()?;
        let value: serde_json::Value = serde_json::from_str(&response)?;
        if value.get("id").and_then(|v| v.as_i64()) != Some(id) {
            return Err(LspClientError::InvalidResponse(response));
        }
        if value.get("result").is_some() {
            Ok(())
        } else if let Some(error) = value.get("error") {
            Err(LspClientError::InvalidResponse(error.to_string()))
        } else {
            Err(LspClientError::InvalidResponse(response))
        }
    }

    fn read_text_message(&mut self) -> Result<String, LspClientError> {
        let msg = self
            .socket
            .read()
            .map_err(|e| LspClientError::Network(e.to_string()))?;
        match msg {
            Message::Text(text) => Ok(text),
            Message::Binary(bytes) => String::from_utf8(bytes)
                .map_err(|e| LspClientError::InvalidResponse(e.to_string())),
            Message::Ping(p) => {
                let _ = self.socket.send(Message::Pong(p));
                self.read_text_message()
            }
            Message::Pong(_) => self.read_text_message(),
            Message::Close(_) => Err(LspClientError::Network("connection closed".into())),
            Message::Frame(_) => self.read_text_message(),
        }
    }

    fn next_request_id(&mut self) -> i64 {
        self.request_id += 1;
        self.request_id
    }
}

/// Recherche des sessions actives via fichier/variables d'environnement.
pub fn discover_sessions() -> Result<Vec<SessionInfo>, LspClientError> {
    if let Ok(endpoint) = std::env::var("VITTE_LSP_ENDPOINT") {
        if let Ok(url) = Url::parse(&endpoint) {
            return Ok(vec![SessionInfo {
                session_id: "external".into(),
                workspace_root: std::env::current_dir().unwrap_or_else(|_| PathBuf::from(".")),
                endpoint: url,
                primary: true,
            }]);
        }
    }

    let path = default_sessions_path();
    if !path.exists() {
        return Ok(Vec::new());
    }
    let json = fs::read_to_string(&path)?;
    let mut sessions: Vec<SessionInfoSerialized> = serde_json::from_str(&json)?;
    // Convert endpoints en Url, gestion des erreurs individuelles.
    let mut result = Vec::new();
    for sess in sessions.drain(..) {
        if let Ok(url) = Url::parse(&sess.endpoint) {
            result.push(SessionInfo {
                session_id: sess.session_id,
                workspace_root: sess.workspace_root.into(),
                endpoint: url,
                primary: sess.primary,
            });
        }
    }
    Ok(result)
}

fn default_sessions_path() -> PathBuf {
    if let Some(home) = std::env::var_os("VITTE_HOME") {
        Path::new(&home).join("lsp-sessions.json")
    } else if let Some(home) = std::env::var_os("HOME") {
        Path::new(&home).join(".vitte/lsp-sessions.json")
    } else {
        PathBuf::from("lsp-sessions.json")
    }
}

#[derive(Debug, Deserialize)]
struct SessionInfoSerialized {
    #[serde(rename = "sessionId")]
    session_id: String,
    #[serde(rename = "workspaceRoot")]
    workspace_root: PathBuf,
    endpoint: String,
    primary: bool,
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;

    #[test]
    fn discover_sessions_missing_file() {
        let dir = tempfile::tempdir().unwrap();
        std::env::set_var("VITTE_HOME", dir.path());
        let sessions = discover_sessions().unwrap();
        assert!(sessions.is_empty());
        std::env::remove_var("VITTE_HOME");
    }

    #[test]
    fn discover_sessions_from_file() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("lsp-sessions.json");
        std::env::set_var("VITTE_HOME", dir.path());
        let payload = r#"[
          {
            "sessionId": "sess-1",
            "workspaceRoot": "/tmp/project",
            "endpoint": "ws://127.0.0.1:9337/repl",
            "primary": true
          }
        ]"#;
        let mut file = fs::File::create(&path).unwrap();
        file.write_all(payload.as_bytes()).unwrap();

        let sessions = discover_sessions().unwrap();
        assert_eq!(sessions.len(), 1);
        assert_eq!(sessions[0].session_id, "sess-1");
        assert_eq!(sessions[0].endpoint, Url::parse("ws://127.0.0.1:9337/repl").unwrap());

        std::env::remove_var("VITTE_HOME");
    }

    #[test]
    fn connect_default_without_server() {
        std::env::set_var("VITTE_LSP_ENDPOINT", "ws://127.0.0.1:65530/repl");
        let result = LspClient::connect_default();
        assert!(matches!(result, Err(LspClientError::Network(_))));
        std::env::remove_var("VITTE_LSP_ENDPOINT");
    }
}
