//! main.rs — point d’entrée du binaire vitte-dap
//!
//! Lit les messages DAP sur stdin, les passe au `Handler`, et renvoie
//! les réponses/événements sur stdout avec en-têtes Content-Length.
//!
//! Usage : lancé par VS Code ou autre client DAP.
//! Pour debug manuel :
//!   $ cargo run -p vitte-dap --bin vitte-dap
//!   (puis taper des requêtes DAP JSON encodées avec Content-Length)

mod engine;
mod handler;

use color_eyre::eyre::Result;
use engine::DebuggerEngine;
use log::{debug, warn};
use std::io::{self, BufRead, Read, Write};

use handler::Handler;

fn main() -> Result<()> {
    color_eyre::install()?;
    env_logger::init();

    let mut handler = Handler::new(Box::new(DebuggerEngine::new()));

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
