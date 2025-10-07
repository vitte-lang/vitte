use std::env;
use std::io;
use std::process;
use std::process::Command;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Verbosity {
    Quiet,
    Normal,
    Verbose,
}

impl Default for Verbosity {
    fn default() -> Self {
        Verbosity::Normal
    }
}

#[derive(Clone, Debug, Default)]
struct GlobalOpts {
    verbosity: Verbosity,
}

fn print_help() {
    println!("Usage: vitte-codegen <command> [options]");
    println!();
    println!("Commands:");
    println!("  help, -h, --help        Show this message");
    println!("  version, -V, --version  Show version");
    println!("  targets                 List supported targets");
    println!("  build                   Build project");
    println!("  pkg                     Package output");
    println!("  env                     Print environment info");
    println!("  run-test                Run internal command test");
}

fn print_version() {
    println!("vitte-codegen {}", env!("CARGO_PKG_VERSION"));
}

fn parse_global_opts(args: &mut Vec<String>) -> GlobalOpts {
    let mut opts = GlobalOpts::default();
    let mut i = 0;
    while i < args.len() {
        match args[i].as_str() {
            "-q" | "--quiet" => {
                opts.verbosity = Verbosity::Quiet;
                args.remove(i);
            },
            "-v" | "--verbose" => {
                opts.verbosity = Verbosity::Verbose;
                args.remove(i);
            },
            _ => i += 1,
        }
    }
    opts
}

fn cmd_targets_list() -> io::Result<()> {
    println!("x86_64-unknown-linux-gnu");
    println!("aarch64-apple-darwin");
    println!("x86_64-apple-darwin");
    Ok(())
}

fn cmd_build(_opts: &GlobalOpts, _release: bool) -> io::Result<()> {
    println!("Building project...");
    Ok(())
}

fn cmd_pkg(_opts: &GlobalOpts, fmt: &str) -> io::Result<()> {
    println!("Packaging as {fmt}");
    Ok(())
}

fn cmd_env(_opts: &GlobalOpts) -> io::Result<()> {
    println!("HOST={}", host_triple());
    Ok(())
}

fn run(cmd: &mut Command) -> io::Result<()> {
    let status = cmd.status()?;
    if !status.success() {
        Err(io::Error::new(io::ErrorKind::Other, "command failed"))
    } else {
        Ok(())
    }
}

fn host_triple() -> &'static str {
    if cfg!(target_os = "macos") {
        if cfg!(target_arch = "aarch64") { "aarch64-apple-darwin" } else { "x86_64-apple-darwin" }
    } else if cfg!(target_os = "linux") {
        "x86_64-unknown-linux-gnu"
    } else {
        "unknown"
    }
}

fn main() {
    let mut args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        print_help();
        process::exit(1);
    }

    let cmd: String = args.get(1).cloned().unwrap();
    args.remove(1);
    let opts = parse_global_opts(&mut args);

    let res = match cmd.as_str() {
        "help" | "-h" | "--help" => {
            print_help();
            Ok(())
        },
        "version" | "-V" | "--version" => {
            print_version();
            Ok(())
        },
        "targets" => cmd_targets_list(),
        "build" => cmd_build(&opts, args.contains(&"--release".into())),
        "pkg" => {
            let fmt = args.get(0).map(|s| s.as_str()).unwrap_or("tar");
            cmd_pkg(&opts, fmt)
        },
        "env" => cmd_env(&opts),
        "run-test" => {
            let mut cmd = Command::new("echo");
            cmd.arg("test run");
            run(&mut cmd)
        },
        _ => {
            eprintln!("Unknown command: {cmd}");
            print_help();
            Err(io::Error::new(io::ErrorKind::InvalidInput, "unknown command"))
        },
    };

    if let Err(e) = res {
        eprintln!("Error: {e}");
        process::exit(1);
    }
}
