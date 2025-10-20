//! Architecture layer smoke tests.
//! Ensures public APIs per layer remain usable in integration scenarios.

use vitte_tests::prelude::*;

#[cfg(feature = "front")]
#[test]
fn frontend_parses_basic_program() {
    use vitte_ast::Item;
    use vitte_core::SourceId;
    use vitte_parser::Parser;

    init_logging();
    let src = r#"
        fn main() {
            let answer: i64 = 40 + 2;
        }
    "#;

    let mut parser = Parser::new(src, SourceId(0));
    let program = parser.parse_program().expect("parser should succeed");
    assert!(!program.items.is_empty(), "program should contain at least one item");
    matches!(program.items[0], Item::Function(_))
        .then_some(())
        .expect("first item should be a function");
}

#[cfg(all(feature = "front", feature = "ir"))]
#[test]
fn ir_accepts_ast_lowering() {
    use vitte_ast::Item;
    use vitte_core::SourceId;
    use vitte_ir::{lower_to_ir, RawProgram};
    use vitte_parser::Parser;

    let src = "fn main() { return; }";
    let mut parser = Parser::new(src, SourceId(0));
    let program = parser.parse_program().expect("parser should succeed");
    assert!(matches!(program.items.first(), Some(Item::Function(_))));

    let ir = lower_to_ir(program);
    let RawProgram = ir;
}

#[cfg(all(feature = "backend", feature = "ir"))]
#[test]
fn backend_global_opts_configurable() {
    use vitte_codegen::{GlobalOpts, Verbosity};

    let mut opts = GlobalOpts::default();
    assert_eq!(opts.verbosity, Verbosity::Normal);
    opts.verbosity = Verbosity::Verbose;
    opts.target = Some("x86_64-unknown-linux-gnu".into());
    opts.jobs = Some(4);
    assert_eq!(opts.target.as_deref(), Some("x86_64-unknown-linux-gnu"));
}

#[cfg(feature = "runtime")]
#[test]
fn runtime_registers_and_calls() {
    use vitte_runtime::{Runtime, Value};

    let (mut rt, _capture) = Runtime::with_captured_stdout();
    rt.register_fn("test", "double", Some(1), |args, _ctx| {
        let x: i64 = args[0].clone().try_into().expect("arg");
        Ok(Value::I64(x * 2))
    });

    let result = rt.call("test.double", &[Value::from(21i64)]).expect("call should succeed");
    let doubled: i64 = result.try_into().expect("value i64");
    assert_eq!(doubled, 42);

    // verify defaults still available
    let mut rt = Runtime::with_defaults();
    let res = rt.call("math.abs", &[Value::from(-5)]).expect("math.abs");
    let v: i64 = res.try_into().expect("i64");
    assert_eq!(v, 5);
}
