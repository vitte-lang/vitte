name = "my-plugin"
version = "0.1.0"
edition = "2024"
kind = "dylib"

description = "Vitte plugin example"
license = "MIT OR Apache-2.0"

[dependencies]
vitte = "0.2.0"

[[bin]]
name = "my-plugin"
path = "src/lib.vit"
