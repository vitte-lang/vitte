//! vitte-dap — Adaptateur Debug Protocol (VSCode, LLDB frontend, etc.)
//!
//! Objectif : exposer la VM Vitte via le protocole DAP (Debug Adapter Protocol).
//! Implémente un serveur JSON-RPC 2.0 qui communique sur stdin/stdout.
//!
//! Usage :
//!   $ cargo run -p vitte-dap --bin vitte-dap -- --program examples/hello.vitte
//!   Puis, config "debug" côté VSCode en pointant vers ce binaire.
//!
//! Modules externes :
//! - `serde`, `serde_json` pour les messages DAP
//! - `tokio` (ou std sync) selon besoin (ici synchrone pour simplicité)

use std::io::{self, BufRead, Write};
use std::collections::HashMap;

use serde::{Deserialize, Serialize};
use serde_json::Value;
use color_eyre::eyre::{Result, eyre};
use log::{info, debug, warn, error};

/// Exemple de breakpoint géré en mémoire
#[derive(Debug, Default)]
struct Breakpoints {
    /// Map fichier -> numéros de lignes
    map: HashMap<String, Vec<u32>>,
}

/// Contexte runtime du DAP
#[derive(Debug, Default)]
pub struct DapServer {
    breakpoints: Breakpoints,
    program: Option<String>,
    running: bool,
}

impl DapServer {
    pub fn new() -> Self {
        Self::default()
    }

    /// Boucle principale : lit les messages DAP sur stdin
    pub fn run(&mut self) -> Result<()> {
        let stdin = io::stdin();
        let mut handle = stdin.lock();

        loop {
            let mut header = String::new();
            if handle.read_line(&mut header)? == 0 {
                break; // EOF
            }

            if header.trim().is_empty() {
                continue;
            }

            // Ex: "Content-Length: 123"
            if let Some(len) = parse_content_length(&header) {
                // Consommer ligne vide
                let mut empty = String::new();
                handle.read_line(&mut empty)?;

                // Lire le JSON
                let mut buf = vec![0; len];
                handle.read_exact(&mut buf)?;

                let msg: serde_json::Value = serde_json::from_slice(&buf)?;
                self.handle_message(msg)?;
            }
        }

        Ok(())
    }

    /// Dispatch un message JSON-RPC
    fn handle_message(&mut self, msg: Value) -> Result<()> {
        let method = msg.get("command").and_then(|v| v.as_str()).unwrap_or("");
        let seq = msg.get("seq").and_then(|v| v.as_i64()).unwrap_or(0);

        debug!("→ Reçu: {}", method);

        match method {
            "initialize" => {
                self.send_response(seq, json!({
                    "supportsConfigurationDoneRequest": true,
                    "supportsSetVariable": true,
                }));
            }
            "launch" => {
                let program = msg["arguments"]["program"].as_str().unwrap_or("unknown");
                self.program = Some(program.to_string());
                self.running = true;
                self.send_response(seq, json!({}));
                self.send_event("initialized", json!({}));
            }
            "setBreakpoints" => {
                let source = msg["arguments"]["source"]["path"].as_str().unwrap_or("unknown");
                let bps: Vec<u32> = msg["arguments"]["breakpoints"]
                    .as_array()
                    .unwrap_or(&vec![])
                    .iter()
                    .filter_map(|bp| bp["line"].as_u64().map(|l| l as u32))
                    .collect();

                self.breakpoints.map.insert(source.to_string(), bps.clone());
                self.send_response(seq, json!({ "breakpoints": bps.iter().map(|l| json!({"line": l, "verified": true})).collect::<Vec<_>>() }));
            }
            "configurationDone" => {
                self.send_response(seq, json!({}));
            }
            "continue" => {
                self.running = true;
                self.send_response(seq, json!({ "allThreadsContinued": true }));
                self.send_event("continued", json!({ "threadId": 1 }));
            }
            "next" => {
                self.send_response(seq, json!({}));
                self.send_event("stopped", json!({ "reason": "step", "threadId": 1 }));
            }
            "stackTrace" => {
                self.send_response(seq, json!({
                    "stackFrames": [{
                        "id": 1,
                        "name": "main",
                        "line": 1,
                        "column": 1,
                        "source": { "path": self.program.clone().unwrap_or_default() }
                    }],
                    "totalFrames": 1
                }));
            }
            "scopes" => {
                self.send_response(seq, json!({
                    "scopes": [{
                        "name": "Locals",
                        "variablesReference": 1,
                        "expensive": false
                    }]
                }));
            }
            "variables" => {
                self.send_response(seq, json!({
                    "variables": [{
                        "name": "x",
                        "value": "42",
                        "type": "i32",
                        "variablesReference": 0
                    }]
                }));
            }
            "disconnect" => {
                self.send_response(seq, json!({}));
                std::process::exit(0);
            }
            _ => {
                warn!("Commande non gérée: {}", method);
                self.send_response(seq, json!({}));
            }
        }

        Ok(())
    }

    fn send_response(&self, seq: i64, body: Value) {
        self.send("response", json!({
            "request_seq": seq,
            "success": true,
            "command": "",
            "body": body
        }));
    }

    fn send_event(&self, event: &str, body: Value) {
        self.send("event", json!({
            "event": event,
            "body": body
        }));
    }

    fn send(&self, typ: &str, mut msg: Value) {
        msg["type"] = Value::String(typ.to_string());

        let s = serde_json::to_string(&msg).unwrap();
        print!("Content-Length: {}\r\n\r\n{}", s.len(), s);
        io::stdout().flush().unwrap();
    }
}

/// Parse un header Content-Length
fn parse_content_length(header: &str) -> Option<usize> {
    header
        .trim()
        .strip_prefix("Content-Length:")
        .and_then(|v| v.trim().parse::<usize>().ok())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_len() {
        assert_eq!(parse_content_length("Content-Length: 42"), Some(42));
        assert_eq!(parse_content_length("foo"), None);
    }
}
