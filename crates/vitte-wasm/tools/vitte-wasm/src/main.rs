//! vitte-wasm CLI tool
use clap::Parser;

#[derive(Parser)]
struct Opt {
    #[arg(short, long)]
    input: String,
}

fn main() {
    let opt = Opt::parse();
    println!("Would compile {} to Wasm", opt.input);
}
