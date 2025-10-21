#![cfg(all(feature = "front", feature = "snapshots"))]

use std::fmt::Write;

use vitte_core::SourceId;
use vitte_lexer::Lexer;
use vitte_tests::prelude::*;

#[test]
fn lex_hello_world_fixture() {
    snapshot_lexer_fixture("programs/hello.vit", "lexer__hello_world");
}

#[test]
fn lex_control_flow_fixture() {
    snapshot_lexer_fixture("programs/control_flow.vit", "lexer__control_flow");
}

fn snapshot_lexer_fixture(rel: &str, snapshot_name: &str) {
    init_logging();
    let src = read_fixture(rel);
    let tokens = Lexer::new(&src, SourceId(0)).tokenize().expect("lexer should succeed on fixture");

    let mut dump = String::new();
    for tok in tokens {
        let _ = writeln!(dump, "{:?}@{}..{}", tok.value, tok.span.start.0, tok.span.end.0);
    }

    assert_snapshot_str(snapshot_name, &dump);
}
