[package]
name = "{{ project-name }}"
version = "0.1.0"
edition = "2024"
authors = ["{{ author }}"]
license = "MIT"
repository = "{{ repository-url }}"
documentation = "https://docs.rs/{{ project-name }}"

[dependencies]
std = "0.2"

[lib]
name = "{{ project-name }}"
path = "src/lib.vit"
