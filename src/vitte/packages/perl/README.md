# Perl Packages

Version: `0.1.0` for every package.

These packages provide standalone Perl utility modules named after familiar Rust-style crate categories.

| Package | Perl module |
| --- | --- |
| `vitte-perl-anyhow` | `Vitte::Crates::Anyhow` |
| `vitte-perl-axum` | `Vitte::Crates::Axum` |
| `vitte-perl-bytes` | `Vitte::Crates::Bytes` |
| `vitte-perl-chrono` | `Vitte::Crates::Chrono` |
| `vitte-perl-clap` | `Vitte::Crates::Clap` |
| `vitte-perl-itertools` | `Vitte::Crates::Itertools` |
| `vitte-perl-log` | `Vitte::Crates::Log` |
| `vitte-perl-rand` | `Vitte::Crates::Rand` |
| `vitte-perl-rayon` | `Vitte::Crates::Rayon` |
| `vitte-perl-regex` | `Vitte::Crates::Regex` |
| `vitte-perl-reqwest` | `Vitte::Crates::Reqwest` |
| `vitte-perl-semver` | `Vitte::Crates::Semver` |
| `vitte-perl-serde` | `Vitte::Crates::Serde` |
| `vitte-perl-sqlx` | `Vitte::Crates::Sqlx` |
| `vitte-perl-tempfile` | `Vitte::Crates::Tempfile` |
| `vitte-perl-thiserror` | `Vitte::Crates::Thiserror` |
| `vitte-perl-tokio` | `Vitte::Crates::Tokio` |
| `vitte-perl-toml` | `Vitte::Crates::Toml` |
| `vitte-perl-tracing` | `Vitte::Crates::Tracing` |
| `vitte-perl-uuid` | `Vitte::Crates::Uuid` |

Each package contains `META.json`, `README.md`, `lib/`, and `t/basic.t`.

## Gate

Run:

```sh
make perl-packages-check
```

The gate validates every `META.json`, checks version `0.1.0`, compiles each `.pm`
file, runs every `t/basic.t`, tests loading through `PERL5LIB`, and generates:

- `target/perl-packages/archives/*.tar.gz`
- `target/perl-packages/checksums.sha256`
- `target/perl-packages/registry.json`
- `target/reports/perl_packages_check.json`
