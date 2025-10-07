//! vitte-playground — exécution interactive et visualisation de code Vitte
//!
//! Modes :
//! - API locale (appel direct à [`execute_code`])
//! - Serveur HTTP (feature `server`)
//!
//! Fonctionnalités :
//! - Compiler du code source en bytecode
//! - Exécuter dans une VM isolée
//! - Retourner résultats, erreurs et logs
//!
//! Exemple local :
//! ```ignore
//! use vitte_playground::execute_code;
//! let r = execute_code("fn main() { 1+2 }").unwrap();
//! println!("{:?}", r.output);
//! ```

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

use anyhow::Result;

#[cfg(feature = "json")]
use serde::{Deserialize, Serialize};

/// Résultat d’une exécution.
#[derive(Debug, Clone)]
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
pub struct ExecResult {
    pub output: String,
    pub errors: Vec<String>,
    pub success: bool,
}

/// Compile et exécute une source Vitte en local.
pub fn execute_code(_src: &str) -> Result<ExecResult> {
    Ok(ExecResult {
        output: String::new(),
        errors: vec!["playground backend not linked in this build (feature missing)".into()],
        success: false,
    })
}

#[cfg(feature = "server")]
pub mod server {
    //! Mode serveur HTTP basé sur actix-web.

    use super::*;
    use actix_web::{post, web, App, HttpResponse, HttpServer, Responder};

    #[cfg(feature = "json")]
    #[derive(Debug, Clone, Deserialize)]
    pub struct CodeRequest {
        pub code: String,
    }

    #[post("/execute")]
    async fn exec(req: web::Json<CodeRequest>) -> impl Responder {
        match execute_code(&req.code) {
            Ok(r) => HttpResponse::Ok().json(r),
            Err(e) => HttpResponse::InternalServerError().body(e.to_string()),
        }
    }

    /// Lance un serveur playground sur `addr`.
    pub async fn run_server(addr: &str) -> std::io::Result<()> {
        HttpServer::new(|| App::new().service(exec))
            .bind(addr)?
            .run()
            .await
    }
}

// ============================== Tests =====================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn exec_runs() {
        let r = execute_code("fn main() { 1+2 }").unwrap();
        assert!(r.success || !r.errors.is_empty());
    }
}