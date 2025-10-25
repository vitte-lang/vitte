//! vitte-repl — interactive shell (scaffolding).
//!
//! Ce crate fournit l’infrastructure de base d’un REPL multi-ligne, avec
//! regroupement de blocs, hachage des cellules et pipeline d’invalidation
//! incrémentale. L’évaluation du langage reste simulée en attendant
//! l’intégration complète avec `vitte-compiler` et `vitte-runtime`.

#![deny(missing_docs)]

mod buffer;
mod document;
mod history;
mod lsp_client;
mod viz;
mod repl;

pub use buffer::SessionBuffer;
pub use document::{CellOutcome, SessionDocument};
pub use history::{HistoryConfig, HistoryError};
pub use lsp_client::{discover_sessions, CellDigest, LspClient, LspClientError, SyncResponse};
pub use viz::{Field, TraceSample, TreeNode, ValueViz};
pub use repl::{EvalResult, Repl, ReplError, ReplOptions, Result};

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn buffer_detects_incomplete_blocks() {
        let mut buf = SessionBuffer::default();
        assert!(!buf.push_line("fn foo() {"));
        assert!(!buf.push_line("  let x = 1;"));
        assert!(!buf.push_line("  x"));
        assert!(buf.push_line("}"));
        assert!(!buf.is_empty());
        let block = buf.flush();
        assert!(block.contains("fn foo()"));
        assert!(buf.is_empty());
    }

    #[test]
    fn document_skips_unchanged_cells() {
        let mut doc = SessionDocument::default();
        let out1 = doc.submit("let x = 1;".into());
        assert!(matches!(out1, CellOutcome::Evaluated { .. }));
        let out2 = doc.submit("let x = 1;".into());
        assert!(matches!(out2, CellOutcome::NoChange));
    }

    #[test]
    fn repl_handles_multi_line() {
        let mut repl = Repl::new(ReplOptions::default());
        assert!(matches!(repl.eval("let x = 1"), Ok(EvalResult::Pending)));
        assert!(matches!(repl.eval("x + 3"), Ok(EvalResult::Pending)));
        assert!(matches!(repl.eval(""), Ok(EvalResult::Output(_))));
    }

    #[test]
    fn history_persists_between_sessions() {
        use std::fs;
        use std::time::{SystemTime, UNIX_EPOCH};

        let temp_path = std::env::temp_dir().join(format!(
            "vitte-repl-history-{}.json",
            SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));

        let opts = ReplOptions {
            history: HistoryConfig {
                enabled: true,
                path: Some(temp_path.clone()),
                encryption_key: None,
            },
            ..Default::default()
        };

        {
            let mut repl = Repl::new(opts.clone());
            assert!(matches!(repl.eval("let persistent = 42;"), Ok(EvalResult::Output(_))));
            drop(repl);
        }

        let repl = Repl::new(opts);
        let entries = repl.history_entries().unwrap();
        assert!(entries.iter().any(|entry| entry.contains("let persistent = 42;")));

        let _ = fs::remove_file(temp_path);
    }

    #[test]
    fn history_supports_encryption() {
        use std::fs;
        use std::time::{SystemTime, UNIX_EPOCH};

        let temp_path = std::env::temp_dir().join(format!(
            "vitte-repl-history-enc-{}.json",
            SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));

        let key = vec![0xAA, 0x55, 0xFF];
        let opts = ReplOptions {
            history: HistoryConfig {
                enabled: true,
                path: Some(temp_path.clone()),
                encryption_key: Some(key.clone()),
            },
            ..Default::default()
        };

        {
            let mut repl = Repl::new(opts.clone());
            assert!(matches!(repl.eval("let encrypted = 7;"), Ok(EvalResult::Output(_))));
        }

        let repl = Repl::new(opts);
        let entries = repl.history_entries().unwrap();
        assert!(entries.iter().any(|entry| entry.contains("let encrypted = 7;")));

        let _ = fs::remove_file(temp_path);
    }

    #[test]
    fn value_viz_serialization_and_ascii() {
        let value = ValueViz::Matrix {
            rows: vec![vec![1.0, 2.5], vec![3.2, 4.0]],
        };
        let json = value.to_json().unwrap();
        let parsed = ValueViz::from_json_str(&json).unwrap();
        assert_eq!(value, parsed);
        let ascii = value.render_ascii();
        assert!(ascii.contains("  1.000"));
        assert!(ascii.contains("  4.000"));
    }

    #[test]
    fn viz_command_outputs_ascii() {
        let mut repl = Repl::new(ReplOptions::default());
        let json = r#"{"kind":"matrix","rows":[[1,2],[3,4]]}"#;
        let cmd = format!(":viz-json {}", json);
        let result = repl.eval(&cmd).unwrap();
        match result {
            EvalResult::Output(out) => {
                assert!(out.contains("  1.000"));
                assert!(out.contains("  4.000"));
            }
            other => panic!("unexpected result: {other:?}"),
        }
    }
}
