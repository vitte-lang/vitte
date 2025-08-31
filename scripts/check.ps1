param()
$ErrorActionPreference = "Stop"

Write-Host "▶ rustfmt --check"
cargo fmt --all -- --check

Write-Host "▶ clippy (all features)"
cargo clippy --workspace --all-features -- -D warnings

Write-Host "▶ tests (all features)"
cargo test --workspace --all-features

if (Get-Command cargo-deny -ErrorAction SilentlyContinue) {
  Write-Host "▶ cargo-deny"
  cargo deny check advisories bans licenses sources
} else {
  Write-Warning "cargo-deny non installé — skip (install: cargo install cargo-deny)"
}

Write-Host "▶ docs (no-deps)"
cargo doc --workspace --all-features --no-deps

Write-Host "✅ check OK"
