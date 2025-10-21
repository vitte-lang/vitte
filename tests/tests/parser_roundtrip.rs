#![cfg(all(feature = "front", feature = "snapshots"))]

use vitte_core::SourceId;
use vitte_parser::Parser;
use vitte_tests::prelude::*;

#[test]
fn parse_hello_world_fixture() {
    snapshot_parser_fixture("programs/hello.vit", "parser__hello_world");
}

#[test]
fn parse_control_flow_fixture() {
    snapshot_parser_fixture("programs/control_flow.vit", "parser__control_flow");
}

fn snapshot_parser_fixture(rel: &str, snapshot_name: &str) {
    init_logging();
    let src = read_fixture(rel);
    let mut parser = Parser::new(&src, SourceId(0));
    let program = parser.parse_program().expect("parser should succeed");
    assert_snapshot_str(snapshot_name, &format!("{:#?}", program));
}
