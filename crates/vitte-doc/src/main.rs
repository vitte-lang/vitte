//! vitte-doc — documentation generator for Vitte language projects.

use clap::{Arg, ArgAction, Command};
use env_logger::Env;
use log::{LevelFilter, info};

fn main() {
    env_logger::Builder::from_env(Env::default().default_filter_or("info")).init();

    let matches = Command::new("vitte-doc")
        .version("0.1.0")
        .author("Vitte Contributors")
        .about("Generates documentation for Vitte language projects")
        .arg(
            Arg::new("src")
                .long("src")
                .value_name("DIR")
                .help("Source directory of the Vitte project")
                .required(true)
                .value_parser(clap::value_parser!(String)),
        )
        .arg(
            Arg::new("out")
                .long("out")
                .value_name("DIR")
                .help("Output directory for generated documentation")
                .required(true)
                .value_parser(clap::value_parser!(String)),
        )
        .arg(
            Arg::new("format")
                .long("format")
                .value_name("FORMAT")
                .help("Output format (e.g., html, markdown)")
                .default_value("html")
                .value_parser(clap::value_parser!(String)),
        )
        .arg(
            Arg::new("verbose")
                .long("verbose")
                .help("Enable verbose logging")
                .action(ArgAction::SetTrue),
        )
        .get_matches();

    if matches.get_flag("verbose") {
        log::set_max_level(LevelFilter::Debug);
        info!("Verbose logging enabled");
    }

    let src = matches.get_one::<String>("src").expect("required").as_str();
    let out = matches.get_one::<String>("out").expect("required").as_str();
    let format = matches.get_one::<String>("format").expect("has default").as_str();

    info!("Starting documentation generation");
    info!("Source directory: {}", src);
    info!("Output directory: {}", out);
    info!("Format: {}", format);

    // Simulate generation
    println!("Generating documentation from '{}' into '{}' as {} format...", src, out, format);

    // Here would be the actual generation logic

    println!("Documentation generation completed successfully.");
}
