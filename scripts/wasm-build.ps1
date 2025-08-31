param()
$ErrorActionPreference = "Stop"
rustup target add wasm32-unknown-unknown | Out-Null
cargo build -p vitte-wasm --target wasm32-unknown-unknown --release
Write-Host "✅ WASM build OK (target/wasm32-unknown-unknown/release)"
