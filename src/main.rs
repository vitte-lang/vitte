fn main() {
    if let Err(_err) = vitte_bin::run_cli() {
        std::process::exit(1);
    }
}
