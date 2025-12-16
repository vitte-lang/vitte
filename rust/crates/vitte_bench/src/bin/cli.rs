//! CLI for Rust benchmarks

use vitte_bench::{
    micro, macro_bench,
};

fn main() {
    let args: Vec<String> = std::env::args().collect();

    if args.len() < 2 {
        print_usage(&args[0]);
        return;
    }

    match args[1].as_str() {
        "--help" | "-h" => print_usage(&args[0]),
        "--list" => list_benchmarks(),
        "--all" => run_all_benchmarks(),
        name => run_single_benchmark(name),
    }
}

fn print_usage(prog: &str) {
    eprintln!("Usage: {} [--list|--all|BENCHMARK]", prog);
    eprintln!("\nOptions:");
    eprintln!("  --list       List available benchmarks");
    eprintln!("  --all        Run all benchmarks");
    eprintln!("  BENCHMARK    Run specific benchmark");
}

fn list_benchmarks() {
    println!("Available Rust benchmarks:");
    println!("  Micro:");
    println!("    rust:arithmetic");
    println!("    rust:bitwise");
    println!("    rust:memory_seq");
    println!("    rust:function_call");
    println!("  Macro:");
    println!("    rust:string_ops");
    println!("    rust:sorting");
    println!("    rust:parsing");
    println!("    rust:vectors");
}

fn run_all_benchmarks() {
    println!("{}", "Benchmark".to_string());
    println!("{}", "=".repeat(100));

    let benchmarks = vec![
        micro::bench_integer_arithmetic(100_000),
        micro::bench_bitwise_ops(100_000),
        micro::bench_memory_access(10_000),
        micro::bench_function_calls(100_000),
        macro_bench::bench_string_operations(100),
        macro_bench::bench_sorting(100),
        macro_bench::bench_parsing(1000),
        macro_bench::bench_vectors(100),
    ];

    for result in &benchmarks {
        println!("{}", result.to_string_pretty());
    }

    println!("{}", "=".repeat(100));
    println!("\nCSV Format:");
    for result in &benchmarks {
        println!("{}", result.to_csv_line());
    }
}

fn run_single_benchmark(name: &str) {
    let result = match name {
        "rust:arithmetic" => Some(micro::bench_integer_arithmetic(100_000)),
        "rust:bitwise" => Some(micro::bench_bitwise_ops(100_000)),
        "rust:memory_seq" => Some(micro::bench_memory_access(10_000)),
        "rust:function_call" => Some(micro::bench_function_calls(100_000)),
        "rust:string_ops" => Some(macro_bench::bench_string_operations(100)),
        "rust:sorting" => Some(macro_bench::bench_sorting(100)),
        "rust:parsing" => Some(macro_bench::bench_parsing(1000)),
        "rust:vectors" => Some(macro_bench::bench_vectors(100)),
        _ => {
            eprintln!("Unknown benchmark: {}", name);
            None
        }
    };

    if let Some(result) = result {
        println!("{}", result.to_string_pretty());
    }
}
