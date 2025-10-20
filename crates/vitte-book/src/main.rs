/* ============================================================================
vitte-book — Command-line documentation viewer & generator for Vitte
============================================================================
Provides access to book-like formatted docs (Markdown/HTML),
for local or online rendering.
============================================================================ */

use std::env;
use std::fs;
use std::path::Path;

const VERSION: &str = env!("CARGO_PKG_VERSION");

fn print_help() {
    println!("Vitte Book v{}", VERSION);
    println!("Usage:");
    println!("  vitte-book <command> [args]\n");
    println!("Commands:");
    println!("  build <src> <out>     Build docs from markdown sources");
    println!("  serve <dir>           Serve docs via local HTTP server");
    println!("  open <file>           Open a built HTML file");
    println!("  version               Print version info");
    println!("  help                  Show this help message");
}

fn build(src: &str, out: &str) -> std::io::Result<()> {
    println!("Building documentation from '{}' to '{}'", src, out);
    fs::create_dir_all(out)?;
    for entry in fs::read_dir(src)? {
        let entry = entry?;
        let path = entry.path();
        if path.extension().map(|e| e == "md").unwrap_or(false) {
            let content = fs::read_to_string(&path)?;
            let html = format!("<html><body><pre>{}</pre></body></html>", content);
            let name = path.file_stem().unwrap().to_string_lossy();
            let out_path = Path::new(out).join(format!("{}.html", name));
            fs::write(out_path, html)?;
        }
    }
    Ok(())
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        print_help();
        return;
    }

    match args[1].as_str() {
        "help" => print_help(),
        "version" => println!("Vitte Book version {}", VERSION),
        "build" if args.len() >= 4 => {
            if let Err(e) = build(&args[2], &args[3]) {
                eprintln!("Error: {}", e);
            }
        }
        _ => {
            eprintln!("Unknown command or invalid usage.\n");
            print_help();
        }
    }
}
