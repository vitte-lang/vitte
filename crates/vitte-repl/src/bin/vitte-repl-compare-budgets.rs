use anyhow::{bail, Context, Result};
use serde::Deserialize;
use std::env;
use std::fs;

#[derive(Debug, Deserialize)]
struct OpStat {
    p50: u128,
    p95: u128,
}

#[derive(Debug, Deserialize)]
struct Budgets {
    execute: OpStat,
    completion: OpStat,
    hover: OpStat,
    definition: OpStat,
    attach: u128,
}

#[derive(Debug, Deserialize)]
struct MetricsOp {
    p50: u128,
    p95: u128,
    success: usize,
}

#[derive(Debug, Deserialize)]
struct Metrics {
    endpoint: String,
    runs: usize,
    attach: u128,
    execute: MetricsOp,
    completion: MetricsOp,
    hover: MetricsOp,
    definition: MetricsOp,
}

fn main() -> Result<()> {
    let mut args = env::args().skip(1);
    let metrics_path = expect_arg(&mut args, "--metrics").unwrap_or_else(|| "metrics.json".into());
    let budgets_path = expect_arg(&mut args, "--budgets")
        .unwrap_or_else(|| "benchmarks/repl-lsp/budgets.json".into());

    let metrics: Metrics =
        serde_json::from_str(&fs::read_to_string(&metrics_path).context("read metrics")?)
            .context("parse metrics")?;
    let budgets: Budgets =
        serde_json::from_str(&fs::read_to_string(&budgets_path).context("read budgets")?)
            .context("parse budgets")?;

    let mut failed = false;

    println!(
        "Endpoint: {} | runs={} | attach={}ms",
        metrics.endpoint, metrics.runs, metrics.attach
    );
    if metrics.attach > budgets.attach {
        println!("FAIL attach: {} > {}", metrics.attach, budgets.attach);
        failed = true;
    } else {
        println!("OK   attach: {} <= {}", metrics.attach, budgets.attach);
    }

    failed |= check_op("execute", &metrics.execute, &budgets.execute);
    failed |= check_op("completion", &metrics.completion, &budgets.completion);
    failed |= check_op("hover", &metrics.hover, &budgets.hover);
    failed |= check_op("definition", &metrics.definition, &budgets.definition);

    if failed {
        bail!("budgets exceeded");
    }
    Ok(())
}

fn expect_arg(args: &mut impl Iterator<Item = String>, flag: &str) -> Option<String> {
    let mut peeked = args.peekable();
    let mut found = None;
    while let Some(a) = peeked.next() {
        if a == flag {
            found = peeked.next();
            break;
        }
    }
    found
}

fn check_op(name: &str, m: &MetricsOp, b: &OpStat) -> bool {
    let mut fail = false;
    if m.p50 > b.p50 || m.p95 > b.p95 {
        println!(
            "FAIL {name}: p50={} (<= {}), p95={} (<= {}), success={}",
            m.p50, b.p50, m.p95, b.p95, m.success
        );
        fail = true;
    } else {
        println!(
            "OK   {name}: p50={} (<= {}), p95={} (<= {}), success={}",
            m.p50, b.p50, m.p95, b.p95, m.success
        );
    }
    fail
}
