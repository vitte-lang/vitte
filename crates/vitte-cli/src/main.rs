use anyhow::Result;
use clap::Parser;
use tracing_subscriber::{fmt, EnvFilter};

#[derive(Parser, Debug)]
#[command(name = "vitte", version, about = "Vitte CLI — affûtée et traditionnelle")]
struct Cli {
    /// Verbosité (-v, -vv, -vvv)
    #[arg(short, long, action = clap::ArgAction::Count)]
    verbose: u8,
}

fn init_tracing(v: u8) {
    let lvl = match v { 0 => "info", 1 => "debug", _ => "trace" };
    let filter = EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new(lvl));
    fmt().with_env_filter(filter).without_time().init();
}

fn main() -> Result<()> {
    let cli = Cli::parse();
    init_tracing(cli.verbose);
    tracing::info!("vitte-cli {}", env!("CARGO_PKG_VERSION"));
    Ok(())
}
