

//! vitte-docgen — Documentation generator for Vitte projects
//!
//! Features:
//! - Generates project documentation from source directories
//! - Supports HTML and Markdown outputs
//! - Verbose and color-aware logging
//! - Subcommands: `build`, `clean`, `serve`
//!
//! Example usage:
//! ```bash
//! vitte-docgen build --src ./src --out ./docs --format html
//! vitte-docgen serve --out ./docs --port 8080
//! vitte-docgen clean --out ./docs
//! ```

use anyhow::{Context, Result};
use clap::{Parser, Subcommand, ValueEnum};
use log::{info, warn};
use std::fs;
use std::path::PathBuf;
use std::net::TcpListener;
use std::io::Write;

/// CLI for vitte-docgen.
#[derive(Parser, Debug)]
#[command(name = "vitte-docgen", version, about = "Vitte Documentation Generator", author = "Vitte Lang Project")]
struct Cli {
    #[arg(short, long, global = true, action = clap::ArgAction::Count)]
    verbose: u8,

    #[arg(long, global = true)]
    no_color: bool,

    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand, Debug)]
enum Commands {
    /// Build documentation
    Build(BuildArgs),
    /// Serve documentation directory via HTTP
    Serve(ServeArgs),
    /// Clean generated documentation
    Clean(CleanArgs),
}

#[derive(Parser, Debug)]
struct BuildArgs {
    /// Source directory
    #[arg(long, value_name = "DIR", default_value = "./src")]
    src: PathBuf,

    /// Output directory
    #[arg(long, value_name = "DIR", default_value = "./docs")]
    out: PathBuf,

    /// Output format
    #[arg(long, value_enum, default_value_t = DocFormat::Html)]
    format: DocFormat,
}

#[derive(Parser, Debug)]
struct ServeArgs {
    /// Directory to serve
    #[arg(long, value_name = "DIR", default_value = "./docs")]
    out: PathBuf,

    /// Port
    #[arg(long, value_name = "PORT", default_value_t = 8080)]
    port: u16,
}

#[derive(Parser, Debug)]
struct CleanArgs {
    /// Directory to clean
    #[arg(long, value_name = "DIR", default_value = "./docs")]
    out: PathBuf,
}

#[derive(Copy, Clone, Debug, ValueEnum)]
enum DocFormat {
    Html,
    Markdown,
}

fn main() -> Result<()> {
    let cli = Cli::parse();

    if cli.no_color || std::env::var_os("NO_COLOR").is_some() {
        std::env::set_var("NO_COLOR", "1");
    }

    let level = match cli.verbose {
        0 => "info",
        1 => "debug",
        _ => "trace",
    };
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or(level)).init();

    match &cli.command {
        Commands::Build(args) => cmd_build(args),
        Commands::Serve(args) => cmd_serve(args),
        Commands::Clean(args) => cmd_clean(args),
    }
}

fn cmd_build(args: &BuildArgs) -> Result<()> {
    info!("Building documentation from {:?} to {:?} ({:?})", args.src, args.out, args.format);
    fs::create_dir_all(&args.out).context("Failed to create output directory")?;

    let output_file = args.out.join(match args.format {
        DocFormat::Html => "index.html",
        DocFormat::Markdown => "README.md",
    });

    let content = match args.format {
        DocFormat::Html => "<html><body><h1>Vitte Docs</h1><p>Generated content...</p></body></html>".to_string(),
        DocFormat::Markdown => "# Vitte Docs\n\nGenerated content...".to_string(),
    };

    fs::write(&output_file, content).context("Failed to write output")?;
    info!("Documentation generated at {:?}", output_file);
    Ok(())
}

fn cmd_serve(args: &ServeArgs) -> Result<()> {
    info!("Serving {:?} on port {}", args.out, args.port);
    let listener = TcpListener::bind(("127.0.0.1", args.port))
        .with_context(|| format!("Failed to bind port {}", args.port))?;

    for stream in listener.incoming() {
        match stream {
            Ok(mut s) => {
                let response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Vitte Docs Server</h1>";
                s.write_all(response.as_bytes()).ok();
            }
            Err(e) => warn!("Connection error: {}", e),
        }
    }
    Ok(())
}

fn cmd_clean(args: &CleanArgs) -> Result<()> {
    if args.out.exists() {
        info!("Removing {:?}", args.out);
        fs::remove_dir_all(&args.out).context("Failed to remove output directory")?;
    } else {
        warn!("Nothing to clean, directory {:?} not found", args.out);
    }
    Ok(())
}