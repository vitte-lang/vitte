[package]
name = "{{ project-name }}"
version = "0.1.0"
edition = "2024"
authors = ["{{ author }}"]
license = "MIT"

[dependencies]
std = "0.2"

[dev-dependencies]

[[bin]]
name = "{{ bin-name }}"
path = "src/main.vit"
