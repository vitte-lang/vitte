use serde::Serialize;
use serde_json::json;
use std::env;
use std::fs;
use std::io::{self, Write};
use std::time::{Duration, Instant};
use url::Url;
use vitte_repl::lsp_client::{CompletionParams, ExecuteParams, LspClient};

#[derive(Default, Serialize)]
struct OpStat {
    p50: u128,
    p95: u128,
    success: usize,
}

#[derive(Default, Serialize)]
struct Metrics {
    endpoint: String,
    runs: usize,
    attach: u128,
    execute: OpStat,
    completion: OpStat,
    hover: OpStat,
    definition: OpStat,
}

fn main() -> anyhow::Result<()> {
    let mut args = env::args().skip(1).collect::<Vec<_>>();
    let mut iterations = 50usize;
    let mut warmup = 10usize;
    let mut out: Option<String> = None;
    let mut endpoint: Option<String> = env::var("VITTE_LSP_ENDPOINT").ok();

    let mut i = 0;
    while i < args.len() {
        match args[i].as_str() {
            "--iterations" => {
                iterations = args.get(i + 1).and_then(|s| s.parse().ok()).unwrap_or(iterations);
                i += 2
            }
            "--warmup" => {
                warmup = args.get(i + 1).and_then(|s| s.parse().ok()).unwrap_or(warmup);
                i += 2
            }
            "--endpoint" => {
                endpoint = args.get(i + 1).cloned().or(endpoint);
                i += 2
            }
            "--out" => {
                out = args.get(i + 1).cloned();
                i += 2
            }
            _ => i += 1,
        }
    }

    let endpoint = endpoint.unwrap_or_else(|| "ws://127.0.0.1:9337/repl".to_string());
    let endpoint_url = Url::parse(&endpoint)?;

    let attach_start = Instant::now();
    let mut client = LspClient::connect_endpoint(endpoint_url, None)?;
    let attach = attach_start.elapsed().as_millis();

    // Warmup
    for _ in 0..warmup {
        let _ = client.execute_cell(ExecuteParams {
            cell_id: "bench-cell".into(),
            version: 1,
            source: "let __bench = 1;".into(),
            prev_exports: vec![],
        });
    }

    let mut exec_times = Vec::with_capacity(iterations);
    let mut exec_success = 0usize;
    for _ in 0..iterations {
        let t0 = Instant::now();
        let ok = client
            .execute_cell(ExecuteParams {
                cell_id: "bench-cell".into(),
                version: 1,
                source: "let __bench = 1;".into(),
                prev_exports: vec![],
            })
            .is_ok();
        let dt = t0.elapsed().as_millis();
        if ok {
            exec_success += 1;
            exec_times.push(dt);
        }
    }

    let mut comp_times = Vec::with_capacity(iterations);
    let mut comp_success = 0usize;
    for _ in 0..iterations {
        let t0 = Instant::now();
        let ok = client
            .completion(CompletionParams {
                cell_id: "bench-cell".into(),
                line: 0,
                character: 0,
                prefix: "".into(),
            })
            .is_ok();
        let dt = t0.elapsed().as_millis();
        if ok {
            comp_success += 1;
            comp_times.push(dt);
        }
    }

    let mut hov_times = Vec::with_capacity(iterations);
    let mut hov_success = 0usize;
    for _ in 0..iterations {
        let t0 = Instant::now();
        let ok = client.hover("bench-cell", "__bench").is_ok();
        let dt = t0.elapsed().as_millis();
        if ok {
            hov_success += 1;
            hov_times.push(dt);
        }
    }

    let mut def_times = Vec::with_capacity(iterations);
    let mut def_success = 0usize;
    for _ in 0..iterations {
        let t0 = Instant::now();
        let ok = client.definition("bench-cell", "__bench").is_ok();
        let dt = t0.elapsed().as_millis();
        if ok {
            def_success += 1;
            def_times.push(dt);
        }
    }

    let metrics = Metrics {
        endpoint,
        runs: iterations,
        attach,
        execute: OpStat {
            p50: p(50, &mut exec_times.clone()),
            p95: p(95, &mut exec_times.clone()),
            success: exec_success,
        },
        completion: OpStat {
            p50: p(50, &mut comp_times.clone()),
            p95: p(95, &mut comp_times.clone()),
            success: comp_success,
        },
        hover: OpStat {
            p50: p(50, &mut hov_times.clone()),
            p95: p(95, &mut hov_times.clone()),
            success: hov_success,
        },
        definition: OpStat {
            p50: p(50, &mut def_times.clone()),
            p95: p(95, &mut def_times.clone()),
            success: def_success,
        },
    };

    let payload = serde_json::to_string_pretty(&metrics)?;
    if let Some(path) = out {
        fs::write(path, payload)?;
    } else {
        io::stdout().write_all(payload.as_bytes())?;
    }
    Ok(())
}

fn p(q: usize, xs: &mut Vec<u128>) -> u128 {
    if xs.is_empty() {
        return 0;
    }
    xs.sort_unstable();
    let idx = (q as f64 / 100.0 * (xs.len() as f64 - 1.0)).round() as usize;
    xs[idx]
}
