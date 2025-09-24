//! handler.rs — Cœur métier DAP (sans IO)
//!
//! - Sépare la logique protocolaire de la boucle stdin/stdout.
//! - Fournit un trait `DebugEngine` pour brancher la VM Vitte.
//! - `Handler` maintient l’état DAP (program, breakpoints, threads).
//!
//! Utilisation (depuis dap.rs par ex.) :
//!   let mut h = Handler::new(Box::new(MyEngine::new()));
//!   let out = h.handle(&incoming_json)?; // Vec<Outbound> (responses + events)
//!   for msg in out { send(msg); }

use std::{collections::HashMap, fmt};

use serde::{Deserialize, Serialize};
use serde_json::{json, Value};
use color_eyre::eyre::{Result, eyre};
use log::{debug, warn};

/// Pont générique vers ta VM / runtime.
/// Implémente ces méthodes dans ton moteur réel pour avoir un débogage fonctionnel.
pub trait DebugEngine: Send {
    /// Charge et prépare un programme.
    fn launch(&mut self, program: &str, args: &[String]) -> Result<()>;

    /// Pose des breakpoints sur une source. Retourne la liste réellement armée.
    fn set_breakpoints(&mut self, source: &str, lines: &[u32]) -> Result<Vec<u32>>;

    /// Continue l’exécution du thread principal (id 1 pour MVP).
    fn r#continue(&mut self) -> Result<()>;

    /// Pas à pas (step over).
    fn step_over(&mut self) -> Result<()>;

    /// Retourne la pile d’appels courante (thread 1).
    fn stack_trace(&self) -> Result<Vec<Frame>>;

    /// Retourne les variables d’une portée donnée.
    fn variables(&self, variables_ref: i64) -> Result<Vec<Variable>>;

    /// Optionnel : évaluation d’une expression dans le contexte courant.
    fn evaluate(&mut self, _expr: &str) -> Result<Option<String>> {
        Ok(None)
    }

    /// Arrête proprement (libère ressources).
    fn disconnect(&mut self) -> Result<()>;
}

/// Un cadre de pile (stack frame) minimal.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Frame {
    pub id: i64,
    pub name: String,
    pub source_path: String,
    pub line: u32,
    pub column: u32,
}

/// Variable DAP minimaliste.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Variable {
    pub name: String,
    pub value: String,
    #[serde(default)]
    pub r#type: Option<String>,
    /// Si > 0, référence vers un objet “expandable”
    #[serde(default)]
    pub variables_reference: i64,
}

/// État DAP côté adaptateur
pub struct Handler {
    engine: Box<dyn DebugEngine>,
    program: Option<String>,
    /// source_path -> lignes
    breakpoints: HashMap<String, Vec<u32>>,
    /// compteur pour attribuer des variablesReference
    next_varref: i64,
}

impl fmt::Debug for Handler {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Handler")
            .field("program", &self.program)
            .field("breakpoints", &self.breakpoints)
            .field("next_varref", &self.next_varref)
            .finish()
    }
}

impl Handler {
    pub fn new(engine: Box<dyn DebugEngine>) -> Self {
        Self {
            engine,
            program: None,
            breakpoints: HashMap::new(),
            next_varref: 2, // 1 = Locals dans ce MVP
        }
    }

    /// Traite un message DAP (objet JSON complet).
    /// Retourne une liste de messages sortants (responses et events).
    pub fn handle(&mut self, msg: &Value) -> Result<Vec<Outbound>> {
        let mut out = Vec::new();
        let seq = msg.get("seq").and_then(|v| v.as_i64()).unwrap_or(0);
        let cmd = msg.get("command").and_then(|v| v.as_str()).unwrap_or("");

        debug!("handle command={cmd}");

        match cmd {
            "initialize" => {
                out.push(Outbound::response(seq, "initialize", json!({
                    "supportsConfigurationDoneRequest": true,
                    "supportsSetVariable": false,
                    "supportsEvaluateForHovers": false,
                    "supportsStepInTargetsRequest": false,
                })));
            }
            "launch" => {
                let args = &msg["arguments"];
                let program = args.get("program").and_then(|v| v.as_str())
                    .ok_or_else(|| eyre!("launch: 'program' manquant"))?;
                let argv: Vec<String> = args.get("args")
                    .and_then(|a| a.as_array())
                    .map(|arr| arr.iter().filter_map(|v| v.as_str().map(|s| s.to_string())).collect())
                    .unwrap_or_default();

                self.engine.launch(program, &argv)?;
                self.program = Some(program.to_string());

                out.push(Outbound::response(seq, "launch", json!({})));
                out.push(Outbound::event("initialized", json!({})));
            }
            "setBreakpoints" => {
                let source_path = msg["arguments"]["source"]["path"]
                    .as_str()
                    .unwrap_or("unknown")
                    .to_string();

                let lines: Vec<u32> = msg["arguments"]["breakpoints"]
                    .as_array()
                    .unwrap_or(&vec![])
                    .iter()
                    .filter_map(|bp| bp.get("line").and_then(|l| l.as_u64()).map(|u| u as u32))
                    .collect();

                let armed = self.engine.set_breakpoints(&source_path, &lines)?;
                self.breakpoints.insert(source_path.clone(), armed.clone());

                let bps_json: Vec<Value> = armed
                    .into_iter()
                    .map(|l| json!({"verified": true, "line": l}))
                    .collect();

                out.push(Outbound::response(seq, "setBreakpoints", json!({ "breakpoints": bps_json })));
            }
            "configurationDone" => {
                out.push(Outbound::response(seq, "configurationDone", json!({})));
            }
            "continue" => {
                self.engine.r#continue()?;
                out.push(Outbound::response(seq, "continue", json!({ "allThreadsContinued": true })));
                out.push(Outbound::event("continued", json!({ "threadId": 1 })));
            }
            "next" => {
                self.engine.step_over()?;
                out.push(Outbound::response(seq, "next", json!({})));
                out.push(Outbound::event("stopped", json!({ "reason": "step", "threadId": 1 })));
            }
            "stackTrace" => {
                let frames = self.engine.stack_trace()?;
                let frames_json: Vec<Value> = frames
                    .into_iter()
                    .map(|f| {
                        json!({
                            "id": f.id,
                            "name": f.name,
                            "line": f.line,
                            "column": f.column,
                            "source": { "path": f.source_path }
                        })
                    })
                    .collect();
                out.push(Outbound::response(seq, "stackTrace", json!({
                    "stackFrames": frames_json,
                    "totalFrames": frames_json.len()
                })));
            }
            "scopes" => {
                // MVP : une seule portée "Locals" avec variablesReference=1
                out.push(Outbound::response(seq, "scopes", json!({
                    "scopes": [{
                        "name": "Locals",
                        "variablesReference": 1,
                        "expensive": false
                    }]
                })));
            }
            "variables" => {
                let vr = msg["arguments"]["variablesReference"].as_i64().unwrap_or(0);
                let vars = self.engine.variables(vr)?;
                let vars_json: Vec<Value> = vars.into_iter().map(|v| {
                    json!({
                        "name": v.name,
                        "value": v.value,
                        "type": v.r#type,
                        "variablesReference": v.variables_reference
                    })
                }).collect();

                out.push(Outbound::response(seq, "variables", json!({ "variables": vars_json })));
            }
            "evaluate" => {
                let expr = msg["arguments"]["expression"].as_str().unwrap_or("");
                match self.engine.evaluate(expr)? {
                    Some(val) => out.push(Outbound::response(seq, "evaluate", json!({
                        "result": val,
                        "variablesReference": 0
                    }))),
                    None => out.push(Outbound::response(seq, "evaluate", json!({
                        "result": "<not available>",
                        "variablesReference": 0
                    }))),
                }
            }
            "disconnect" => {
                self.engine.disconnect()?;
                out.push(Outbound::response(seq, "disconnect", json!({})));
                out.push(Outbound::event("terminated", json!({})));
            }
            other => {
                warn!("Commande non gérée: {other}");
                out.push(Outbound::response(seq, other, json!({})));
            }
        }

        Ok(out)
    }
}

/// Message sortant prêt à sérialiser et à envoyer avec en-tête DAP.
#[derive(Debug, Clone, Serialize)]
#[serde(tag = "type", rename_all = "lowercase")]
pub enum Outbound {
    /// Réponse à une requête
    Response {
        request_seq: i64,
        success: bool,
        command: String,
        body: Value,
    },
    /// Événement spontané
    Event {
        event: String,
        body: Value,
    },
}

impl Outbound {
    pub fn response(seq: i64, command: impl Into<String>, body: Value) -> Self {
        Self::Response {
            request_seq: seq,
            success: true,
            command: command.into(),
            body,
        }
    }
    pub fn event(event: impl Into<String>, body: Value) -> Self {
        Self::Event {
            event: event.into(),
            body,
        }
    }

    /// Sérialise en JSON (sans en-tête DAP).
    pub fn to_json(&self) -> Result<String> {
        Ok(serde_json::to_string(self)?)
    }

    /// Rend `(len_bytes, payload_json)` ; utile pour écrire:
    ///   Content-Length: <len>\r\n\r\n<payload>
    pub fn to_payload(&self) -> Result<(usize, String)> {
        let s = self.to_json()?;
        Ok((s.len(), s))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    /// Moteur factice pour tests
    struct DummyEngine;
    impl DebugEngine for DummyEngine {
        fn launch(&mut self, _program: &str, _args: &[String]) -> Result<()> { Ok(()) }
        fn set_breakpoints(&mut self, _source: &str, lines: &[u32]) -> Result<Vec<u32>> { Ok(lines.to_vec()) }
        fn r#continue(&mut self) -> Result<()> { Ok(()) }
        fn step_over(&mut self) -> Result<()> { Ok(()) }
        fn stack_trace(&self) -> Result<Vec<Frame>> {
            Ok(vec![Frame{ id:1, name:"main".into(), source_path:"/tmp/x.vitte".into(), line:1, column:1 }])
        }
        fn variables(&self, _variables_ref: i64) -> Result<Vec<Variable>> {
            Ok(vec![Variable{ name:"x".into(), value:"42".into(), r#type:Some("i32".into()), variables_reference:0 }])
        }
        fn evaluate(&mut self, expr: &str) -> Result<Option<String>> { Ok(Some(format!("evaluated:{expr}"))) }
        fn disconnect(&mut self) -> Result<()> { Ok(()) }
    }

    #[test]
    fn initialize_ok() {
        let mut h = Handler::new(Box::new(DummyEngine));
        let msgs = h.handle(&json!({"seq":1,"type":"request","command":"initialize","arguments":{}})).unwrap();
        assert!(!msgs.is_empty());
        match &msgs[0] {
            Outbound::Response{ request_seq, .. } => assert_eq!(*request_seq, 1),
            _ => panic!("expected response"),
        }
    }

    #[test]
    fn set_bps_roundtrip() {
        let mut h = Handler::new(Box::new(DummyEngine));
        let req = json!({
            "seq": 2,
            "type": "request",
            "command": "setBreakpoints",
            "arguments": {
                "source": { "path": "/tmp/x.vitte" },
                "breakpoints": [{ "line": 3 }, { "line": 7 }]
            }
        });
        let msgs = h.handle(&req).unwrap();
        assert!(matches!(msgs[0], Outbound::Response{..}));
    }

    #[test]
    fn outbound_payload() {
        let ev = Outbound::event("stopped", json!({"reason":"step"}));
        let (len, body) = ev.to_payload().unwrap();
        assert!(len == body.len());
        assert!(body.contains("\"event\":\"stopped\""));
    }
}
