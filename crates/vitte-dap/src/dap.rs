//! vitte-dap: impl minimale du Debug Adapter Protocol (DAP) via stdio.
//!
//! - Transport: STDIO avec entêtes `Content-Length: <n>\r\n\r\n<body>`
//! - JSON-RPC 2.0 (style DAP) ; "seq", "type", "command", "arguments", etc.
//! - Requêtes supportées (subset pragmatique):
//!   initialize, launch, setBreakpoints, configurationDone,
//!   threads, stackTrace, scopes, variables,
//!   continue, next (stepOver), pause, disconnect, evaluate
//! - Événements émis: initialized, stopped, continued, terminated, output
//!
//! Intégration runtime: implémentez le trait [`Backend`] pour brancher votre VM.

use serde::{Deserialize, Serialize};
use serde_json::{json, Value};
use std::collections::{BTreeMap, HashMap};
use std::io::{self, BufRead, Write};
use std::fmt;

/// Résultat interne simplifié.
type DapResult<T> = Result<T, DapError>;

/// Erreurs internes du DAP.
#[derive(Debug)]
pub enum DapError {
    Io(io::Error),
    Json(serde_json::Error),
    Protocol(&'static str),
    Backend(String),
}
impl fmt::Display for DapError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            DapError::Io(e) => write!(f, "io: {e}"),
            DapError::Json(e) => write!(f, "json: {e}"),
            DapError::Protocol(m) => write!(f, "protocol: {m}"),
            DapError::Backend(m) => write!(f, "backend: {m}"),
        }
    }
}
impl std::error::Error for DapError {}
impl From<io::Error> for DapError {
    fn from(e: io::Error) -> Self {
        DapError::Io(e)
    }
}
impl From<serde_json::Error> for DapError {
    fn from(e: serde_json::Error) -> Self {
        DapError::Json(e)
    }
}

/// Backend à implémenter par votre VM/runner.
pub trait Backend: Send {
    /// Démarre la session (chargement programme, etc.).
    fn launch(&mut self, program: String, args: Vec<String>, cwd: Option<String>) -> DapResult<()>;
    /// Liste des threads actifs (ici, un seul thread par défaut).
    fn threads(&self) -> DapResult<Vec<Thread>>;
    /// Déclenche l’exécution (continue).
    fn r#continue(&mut self, thread_id: i64) -> DapResult<ContinueOutcome>;
    /// Pause le thread (ex: via signal/flag).
    fn pause(&mut self, thread_id: i64) -> DapResult<()>;
    /// Step over.
    fn step_over(&mut self, thread_id: i64) -> DapResult<()>;
    /// Stack trace du thread.
    fn stack_trace(&self, thread_id: i64, start: i64, levels: i64) -> DapResult<Vec<StackFrame>>;
    /// Scopes pour un frame.
    fn scopes(&self, frame_id: i64) -> DapResult<Vec<Scope>>;
    /// Variables pour un handle donné.
    fn variables(&self, variables_reference: i64) -> DapResult<Vec<Variable>>;
    /// Fixe les breakpoints d’un fichier, renvoie leur statut.
    fn set_breakpoints(&mut self, file: String, lines: Vec<i64>) -> DapResult<Vec<Breakpoint>>;
    /// Évalue une expression (dans le frame courant).
    fn evaluate(&self, expression: String, frame_id: Option<i64>) -> DapResult<EvalResult>;
    /// Déconnexion/arrêt propre.
    fn disconnect(&mut self) -> DapResult<()>;
}

/// Résultat d’un continue (DAP veut savoir si allThreadsContinued…).
#[derive(Debug, Clone, Serialize)]
pub struct ContinueOutcome {
    pub all_threads_continued: bool,
    /// Optionnel: thread qui a continué (sinon -1)
    #[serde(skip_serializing_if = "Option::is_none")]
    pub thread_id: Option<i64>,
}

/// Thread DAP.
#[derive(Debug, Clone, Serialize)]
pub struct Thread {
    pub id: i64,
    pub name: String,
}

/// Stack frame DAP.
#[derive(Debug, Clone, Serialize)]
pub struct StackFrame {
    pub id: i64,
    pub name: String,
    pub line: i64,
    pub column: i64,
    pub source: Option<Source>,
}

/// Source/fichier.
#[derive(Debug, Clone, Serialize)]
pub struct Source {
    pub name: String,
    pub path: String,
}

/// Scope DAP.
#[derive(Debug, Clone, Serialize)]
pub struct Scope {
    pub name: String,
    pub variables_reference: i64,
    #[serde(default)]
    pub expensive: bool,
}

/// Variable DAP.
#[derive(Debug, Clone, Serialize)]
pub struct Variable {
    pub name: String,
    pub value: String,
    #[serde(default)]
    pub variables_reference: i64,
}

/// Breakpoint DAP.
#[derive(Debug, Clone, Serialize)]
pub struct Breakpoint {
    pub verified: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub line: Option<i64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub column: Option<i64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub source: Option<Source>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub id: Option<i64>,
}

/// Résultat Evaluate.
#[derive(Debug, Clone, Serialize)]
pub struct EvalResult {
    pub result: String,
    #[serde(default)]
    pub variables_reference: i64,
}

/* ─────────────────────────── Framing & messages ─────────────────────────── */

#[derive(Debug, Deserialize)]
#[serde(tag = "type")]
enum InMsg {
    #[serde(rename = "request")]
    Request {
        seq: i64,
        command: String,
        #[serde(default)]
        arguments: Value,
    },
    // On ignore les réponses/events entrants (DAP client n’en envoie pas)
    #[serde(other)]
    Other,
}

#[derive(Debug, Serialize)]
struct Response<'a> {
    #[serde(rename = "type")]
    ty: &'a str, // "response"
    request_seq: i64,
    success: bool,
    command: &'a str,
    #[serde(skip_serializing_if = "Option::is_none")]
    message: Option<&'a str>,
    #[serde(skip_serializing_if = "Option::is_none")]
    body: Option<Value>,
}

#[derive(Debug, Serialize)]
struct Event<'a> {
    #[serde(rename = "type")]
    ty: &'a str, // "event"
    event: &'a str,
    #[serde(skip_serializing_if = "Option::is_none")]
    body: Option<Value>,
}

/// Serveur DAP via stdio.
pub struct DapServer<B: Backend> {
    backend: B,
    /// map file → breakpoints ids incrementaux
    next_bp_id: i64,
    bp_ids: HashMap<(String, i64), i64>,
    /// variables handle allocator
    _next_var_ref: i64,
    /// map var_ref → children variables (mock cache si besoin)
    _var_store: BTreeMap<i64, Vec<Variable>>,
}

impl<B: Backend> DapServer<B> {
    pub fn new(backend: B) -> Self {
        Self {
            backend,
            next_bp_id: 1,
            bp_ids: HashMap::new(),
            _next_var_ref: 1_000,
            _var_store: BTreeMap::new(),
        }
    }

    /// Boucle principale: lit des messages DAP sur stdin et répond sur stdout.
    pub fn run(&mut self) -> DapResult<()> {
        let stdin = io::stdin();
        let mut locked = stdin.lock();

        loop {
            let body = match read_dap_message(&mut locked)? {
                Some(v) => v,
                None => break, // EOF
            };

            match serde_json::from_slice::<InMsg>(&body) {
                Ok(InMsg::Request {
                    seq,
                    command,
                    arguments,
                }) => {
                    if let Err(e) = self.handle_request(seq, &command, arguments) {
                        self.send_error_response(seq, &command, &e.to_string())?;
                    }
                }
                Ok(InMsg::Other) => {
                    // ignore
                }
                Err(err) => {
                    // impossible à parser: renvoyer un error generic
                    self.send_error_response(0, "unknown", &format!("bad json: {err}"))?;
                }
            }
        }
        Ok(())
    }

    fn handle_request(&mut self, seq: i64, command: &str, args: Value) -> DapResult<()> {
        match command {
            "initialize" => {
                // Capabilities
                let body = json!({
                    "supportsConfigurationDoneRequest": true,
                    "supportsEvaluateForHovers": true,
                    "supportsCompletionsRequest": false,
                    "supportsSetVariable": false,
                    "exceptionBreakpointFilters": []
                });
                self.send_ok_response(seq, command, Some(body))?;
                // event "initialized"
                self.send_event("initialized", None)?;
            }
            "launch" => {
                // { program, args?, cwd? }
                let program = args
                    .get("program")
                    .and_then(Value::as_str)
                    .unwrap_or("")
                    .to_string();
                let cwd = args
                    .get("cwd")
                    .and_then(Value::as_str)
                    .map(|s| s.to_string());
                let run_args = match args.get("args") {
                    Some(Value::Array(a)) => a
                        .iter()
                        .filter_map(|v| v.as_str().map(|s| s.to_string()))
                        .collect(),
                    _ => Vec::new(),
                };
                self.backend.launch(program, run_args, cwd)?;
                self.send_ok_response(seq, command, None)?;
            }
            "setBreakpoints" => {
                // { source:{path}, lines:[...] }
                let src = args
                    .get("source")
                    .and_then(|s| s.get("path"))
                    .and_then(Value::as_str)
                    .ok_or(DapError::Protocol("setBreakpoints: missing source.path"))?
                    .to_string();
                let lines: Vec<i64> = args
                    .get("lines")
                    .and_then(Value::as_array)
                    .ok_or(DapError::Protocol("setBreakpoints: missing lines"))?
                    .iter()
                    .filter_map(Value::as_i64)
                    .collect();

                let mut bps = self.backend.set_breakpoints(src.clone(), lines.clone())?;
                // attribuer des ids stables pour chaque (file,line)
                for bp in &mut bps {
                    if let Some(line) = bp.line {
                        let key = (src.clone(), line);
                        let id = *self.bp_ids.entry(key).or_insert_with(|| {
                            let x = self.next_bp_id;
                            self.next_bp_id += 1;
                            x
                        });
                        bp.id = Some(id);
                    }
                }
                self.send_ok_response(seq, command, Some(json!({ "breakpoints": bps })))?;
            }
            "configurationDone" => {
                self.send_ok_response(seq, command, None)?;
            }
            "threads" => {
                let ths = self.backend.threads()?;
                self.send_ok_response(seq, command, Some(json!({ "threads": ths })))?;
            }
            "stackTrace" => {
                let tid = args.get("threadId").and_then(Value::as_i64).unwrap_or(1);
                let start = args.get("startFrame").and_then(Value::as_i64).unwrap_or(0);
                let levels = args.get("levels").and_then(Value::as_i64).unwrap_or(20);
                let frames = self.backend.stack_trace(tid, start, levels)?;
                self.send_ok_response(
                    seq,
                    command,
                    Some(json!({
                        "stackFrames": frames,
                        "totalFrames": frames.len() as i64
                    })),
                )?;
            }
            "scopes" => {
                let frame_id = args
                    .get("frameId")
                    .and_then(Value::as_i64)
                    .ok_or(DapError::Protocol("scopes: missing frameId"))?;
                let scopes = self.backend.scopes(frame_id)?;
                self.send_ok_response(seq, command, Some(json!({ "scopes": scopes })))?;
            }
            "variables" => {
                let vr = args
                    .get("variablesReference")
                    .and_then(Value::as_i64)
                    .ok_or(DapError::Protocol("variables: missing variablesReference"))?;
                let vars = self.backend.variables(vr)?;
                self.send_ok_response(seq, command, Some(json!({ "variables": vars })))?;
            }
            "continue" => {
                let tid = args.get("threadId").and_then(Value::as_i64).unwrap_or(1);
                let out = self.backend.r#continue(tid)?;
                self.send_ok_response(
                    seq,
                    command,
                    Some(json!({
                        "allThreadsContinued": out.all_threads_continued
                    })),
                )?;
                self.send_event(
                    "continued",
                    Some(json!({ "threadId": out.thread_id.unwrap_or(tid) })),
                )?;
            }
            "next" | "stepOver" => {
                let tid = args.get("threadId").and_then(Value::as_i64).unwrap_or(1);
                self.backend.step_over(tid)?;
                self.send_ok_response(seq, command, None)?;
                self.send_event(
                    "stopped",
                    Some(json!({ "reason": "step", "threadId": tid })),
                )?;
            }
            "pause" => {
                let tid = args.get("threadId").and_then(Value::as_i64).unwrap_or(1);
                self.backend.pause(tid)?;
                self.send_ok_response(seq, command, None)?;
                self.send_event(
                    "stopped",
                    Some(json!({ "reason": "pause", "threadId": tid })),
                )?;
            }
            "evaluate" => {
                let expr = args
                    .get("expression")
                    .and_then(Value::as_str)
                    .unwrap_or("")
                    .to_string();
                let frame_id = args.get("frameId").and_then(Value::as_i64);
                let res = self.backend.evaluate(expr, frame_id)?;
                self.send_ok_response(
                    seq,
                    command,
                    Some(json!({
                        "result": res.result,
                        "variablesReference": res.variables_reference
                    })),
                )?;
            }
            "disconnect" => {
                self.backend.disconnect()?;
                self.send_ok_response(seq, command, None)?;
                self.send_event("terminated", None)?;
            }
            // DAP ping/pong
            "cancel" | "runInTerminal" => {
                self.send_ok_response(seq, command, None)?;
            }
            other => {
                // Répondre un "not supported" propre
                self.send_error_response(seq, other, "not supported")?;
            }
        }
        Ok(())
    }

    fn send_ok_response(
        &mut self,
        request_seq: i64,
        command: &str,
        body: Option<Value>,
    ) -> DapResult<()> {
        let resp = Response {
            ty: "response",
            request_seq,
            success: true,
            command,
            message: None,
            body,
        };
        write_dap_message(&resp)
    }

    fn send_error_response(
        &mut self,
        request_seq: i64,
        command: &str,
        message: &str,
    ) -> DapResult<()> {
        let resp = Response {
            ty: "response",
            request_seq,
            success: false,
            command,
            message: Some(message),
            body: None,
        };
        write_dap_message(&resp)
    }

    fn send_event(&mut self, event: &str, body: Option<Value>) -> DapResult<()> {
        let evt = Event {
            ty: "event",
            event,
            body,
        };
        write_dap_message(&evt)
    }
}

/* ─────────────────────────── Framing helpers ─────────────────────────── */

/// Lit un message DAP (framing Content-Length) depuis un `BufRead`.
fn read_dap_message<R: BufRead + io::Read>(r: &mut R) -> DapResult<Option<Vec<u8>>> {
    let mut header = String::new();
    let mut content_length: Option<usize> = None;

    loop {
        header.clear();
        let n = r.read_line(&mut header)?;
        if n == 0 {
            // EOF
            return Ok(None);
        }
        let line = header.trim_end_matches(&['\r', '\n'][..]);
        if line.is_empty() {
            // fin des entêtes
            break;
        }
        if let Some(rest) = line.strip_prefix("Content-Length:") {
            let len = rest
                .trim()
                .parse::<usize>()
                .map_err(|_| DapError::Protocol("bad content-length"))?;
            content_length = Some(len);
        }
        // on ignore les autres entêtes
    }

    let len = content_length.ok_or(DapError::Protocol("missing content-length"))?;
    let mut buf = vec![0u8; len];
    r.read_exact(&mut buf)?;
    Ok(Some(buf))
}

/// Écrit un message JSON encodé DAP vers stdout.
fn write_dap_message<T: ?Sized + Serialize>(msg: &T) -> DapResult<()> {
    let body = serde_json::to_vec(msg)?;
    let mut out = io::stdout().lock();
    write!(out, "Content-Length: {}\r\n\r\n", body.len())?;
    out.write_all(&body)?;
    out.flush()?;
    Ok(())
}

/* ─────────────────────────── Backend factice (exemple) ─────────────────────────── */

/// Backend exemple, “single-threaded”, pour tester le DAP sans VM.
pub struct DummyBackend {
    thread_id: i64,
    paused: bool,
    frames: Vec<StackFrame>,
    scopes: HashMap<i64, Vec<Scope>>,
    vars: HashMap<i64, Vec<Variable>>,
}

impl Default for DummyBackend {
    fn default() -> Self {
        let mut scopes = HashMap::new();
        let mut vars = HashMap::new();

        // frame id 1 -> scopes/vars
        scopes.insert(
            1,
            vec![
                Scope {
                    name: "locals".into(),
                    variables_reference: 1001,
                    expensive: false,
                },
                Scope {
                    name: "globals".into(),
                    variables_reference: 1002,
                    expensive: false,
                },
            ],
        );
        vars.insert(
            1001,
            vec![
                Variable {
                    name: "x".into(),
                    value: "42".into(),
                    variables_reference: 0,
                },
                Variable {
                    name: "msg".into(),
                    value: "\"hello\"".into(),
                    variables_reference: 0,
                },
            ],
        );
        vars.insert(
            1002,
            vec![Variable {
                name: "PI".into(),
                value: "3.14159".into(),
                variables_reference: 0,
            }],
        );

        Self {
            thread_id: 1,
            paused: true,
            frames: vec![StackFrame {
                id: 1,
                name: "main".into(),
                line: 1,
                column: 1,
                source: Some(Source {
                    name: "main.vit".into(),
                    path: "main.vit".into(),
                }),
            }],
            scopes,
            vars,
        }
    }
}

impl Backend for DummyBackend {
    fn launch(
        &mut self,
        program: String,
        _args: Vec<String>,
        _cwd: Option<String>,
    ) -> DapResult<()> {
        // Dans une vraie impl: charger le bytecode, préparer la VM, etc.
        self.frames[0].source = Some(Source {
            name: program.clone(),
            path: program,
        });
        self.paused = true;
        Ok(())
    }

    fn threads(&self) -> DapResult<Vec<Thread>> {
        Ok(vec![Thread {
            id: self.thread_id,
            name: "main".into(),
        }])
    }

    fn r#continue(&mut self, thread_id: i64) -> DapResult<ContinueOutcome> {
        if thread_id != self.thread_id {
            return Err(DapError::Backend("bad thread".into()));
        }
        self.paused = false;
        // Ici on “termine” tout de suite fictivement
        self.paused = true;
        Ok(ContinueOutcome {
            all_threads_continued: true,
            thread_id: Some(thread_id),
        })
    }

    fn pause(&mut self, thread_id: i64) -> DapResult<()> {
        if thread_id != self.thread_id {
            return Err(DapError::Backend("bad thread".into()));
        }
        self.paused = true;
        Ok(())
    }

    fn step_over(&mut self, thread_id: i64) -> DapResult<()> {
        if thread_id != self.thread_id {
            return Err(DapError::Backend("bad thread".into()));
        }
        // Avancer d’une ligne fictive
        self.frames[0].line += 1;
        self.paused = true;
        Ok(())
    }

    fn stack_trace(&self, thread_id: i64, start: i64, levels: i64) -> DapResult<Vec<StackFrame>> {
        if thread_id != self.thread_id {
            return Err(DapError::Backend("bad thread".into()));
        }
        let s = start.max(0) as usize;
        let e = (s + levels.max(1) as usize).min(self.frames.len());
        Ok(self.frames[s..e].to_vec())
    }

    fn scopes(&self, frame_id: i64) -> DapResult<Vec<Scope>> {
        Ok(self.scopes.get(&frame_id).cloned().unwrap_or_default())
    }

    fn variables(&self, variables_reference: i64) -> DapResult<Vec<Variable>> {
        Ok(self
            .vars
            .get(&variables_reference)
            .cloned()
            .unwrap_or_default())
    }

    fn set_breakpoints(&mut self, file: String, lines: Vec<i64>) -> DapResult<Vec<Breakpoint>> {
        // Factice: tous vérifiés
        let src = Source {
            name: file.clone(),
            path: file,
        };
        Ok(lines
            .into_iter()
            .map(|l| Breakpoint {
                verified: true,
                line: Some(l),
                column: None,
                source: Some(src.clone()),
                id: None,
            })
            .collect())
    }

    fn evaluate(&self, expression: String, _frame_id: Option<i64>) -> DapResult<EvalResult> {
        // Factice: renvoie l’expression entre quotes
        Ok(EvalResult {
            result: format!("\"{expression}\""),
            variables_reference: 0,
        })
    }

    fn disconnect(&mut self) -> DapResult<()> {
        Ok(())
    }
}

/* ─────────────────────────── Entrée de secours (si binaire) ───────────────────────────
   Vous pouvez créer un src/main.rs qui utilise ce module :

   fn main() -> Result<(), Box<dyn std::error::Error>> {
       let backend = vitte_dap::dap::DummyBackend::default();
       let mut server = vitte_dap::dap::DapServer::new(backend);
       server.run()?;
       Ok(())
   }
*/
