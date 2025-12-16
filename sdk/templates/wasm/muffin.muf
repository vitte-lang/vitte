[package]
name = "{{ project-name }}"
version = "0.1.0"
edition = "2024"

[dependencies]
std = { version = "0.2", features = ["wasm"] }

[profile.release]
opt-level = "z"     # Optimize for size
lto = true          # Link-time optimization
codegen-units = 1
