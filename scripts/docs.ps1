param()
$ErrorActionPreference = "Stop"
cargo doc --workspace --all-features --no-deps
Write-Host "📚 Docs générées sous target/doc/"
