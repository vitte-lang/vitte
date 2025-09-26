//! main.rs — point d’entrée du binaire vitte-dap
//!
//! Lit les messages DAP sur stdin, les passe au `Handler`, et renvoie
//! les réponses/événements sur stdout avec en-têtes Content-Length.
//!
//! Usage : lancé par VS Code ou autre client DAP.
//! Pour debug manuel :
//!   $ cargo run -p vitte-dap --bin vitte-dap
//!   (puis taper des requêtes DAP JSON encodées avec Content-Length)

mod handler;

use std::io::{self, BufRead, Read, Write};
use color_eyre::eyre::Result;
use log::{info, debug, warn};

use handler::{Handler, DebugEngine, Frame, Variable};

/// Moteur factice (stub) pour MVP.
/// TODO : remplacer par un vrai pont vers la VM Vitte.
struct DummyEngine;

impl DebugEngine for DummyEngine {
    fn launch(&mut self, program: &str, _args: &[String]) -> Result<()> {
        info!("DummyEngine.launch program={}", program);
        Ok(())
    }
    fn set_breakpoints(&mut self, source: &str, lines: &[u32]) -> Result<Vec<u32>> {
        info!("DummyEngine.set_breakpoints {} {:?}", source, lines);
        Ok(lines.to_vec())
    }
    fn r#continue(&mut self) -> Result<()> {
        info!("DummyEngine.continue");
        Ok(())
    }
    fn step_over(&mut self) -> Result<()> {
        info!("DummyEngine.step_over");
        Ok(())
    }
    fn stack_trace(&self) -> Result<Vec<Frame>> {
        Ok(vec![Frame {
            id: 1,
            name: "main".into(),
            source_path: "dummy.vitte".into(),
            line: 1,
            column: 1,
        }])
    }
    fn variables(&self, _variables_ref: i64) -> Result<Vec<Variable>> {
        Ok(vec![Variable {
            name: "x".into(),
            value: "42".into(),
            r#type: Some("i32".into()),
            variables_reference: 0,
        }])
    }
    fn evaluate(&mut self, expr: &str) -> Result<Option<String>> {
        Ok(Some(format!("evaluated:{expr}")))
    }
    fn disconnect(&mut self) -> Result<()> {
        info!("DummyEngine.disconnect");
        Ok(())
    }
}

fn main() -> Result<()> {
    color_eyre::install()?;
    env_logger::init();

    let mut handler = Handler::new(Box::new(DummyEngine));

    let stdin = io::stdin();
    let mut handle = stdin.lock();

    loop {
        // Lire en-tête (ex: "Content-Length: 123\r\n")
        let mut header = String::new();
        if handle.read_line(&mut header)? == 0 {
            break; // EOF
        }
        if header.trim().is_empty() {
            continue;
        }

        let len = if let Some(len) = parse_content_length(&header) {
            len
        } else {
            warn!("Ligne d’entête ignorée: {}", header.trim());
            continue;
        };

        // Consommer ligne vide "\r\n"
        let mut empty = String::new();
        handle.read_line(&mut empty)?;

        // Lire le JSON complet
        let mut buf = vec![0; len];
        handle.read_exact(&mut buf)?;
        let incoming: serde_json::Value = serde_json::from_slice(&buf)?;

        debug!("← {}", incoming);

        // Dispatcher via Handler
        let outs = handler.handle(&incoming)?;
        for o in outs {
            let (len, payload) = o.to_payload()?;
            print!("Content-Length: {}\r\n\r\n{}", len, payload);
            io::stdout().flush().ok();
            debug!("→ {}", payload);
        }
    }

    Ok(())
}

/// Parse un en-tête "Content-Length: N" → Some(N)
fn parse_content_length(header: &str) -> Option<usize> {
    header
        .trim()
        .strip_prefix("Content-Length:")
        .and_then(|v| v.trim().parse::<usize>().ok())
}
