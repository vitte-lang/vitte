# Perl Packages

Version: `0.1.0` for every package.

These packages provide standalone Perl utility modules with city-based names.
They are designed for local registry use, reproducible archives, strict metadata,
and direct loading through `PERL5LIB`.

| Package | Perl module | Role |
| --- | --- | --- |
| `vitte-perl-berlin` | `Vitte::City::Berlin` | Byte buffer and binary packing helpers |
| `vitte-perl-lyon` | `Vitte::City::Lyon` | Result values, errors, context chains, and recovery helpers |
| `vitte-perl-paris` | `Vitte::City::Paris` | Command line flag parsing and usage rendering |
| `vitte-perl-tokyo` | `Vitte::City::Tokyo` | Deterministic task queue and small runtime helpers |
| `vitte-perl-cairo` | `Vitte::City::Cairo` | Stable scalar and record serialization helpers |
| `vitte-perl-oslo` | `Vitte::City::Oslo` | Duration, clock, and timestamp formatting helpers |
| `vitte-perl-dakar` | `Vitte::City::Dakar` | Structured logging with levels, fields, and in-memory sinks |
| `vitte-perl-lima` | `Vitte::City::Lima` | Iterator style transforms for arrays and small streams |
| `vitte-perl-seoul` | `Vitte::City::Seoul` | Pattern matching helpers for text, captures, and replacements |
| `vitte-perl-sydney` | `Vitte::City::Sydney` | HTTP request, response, router, and header helpers |
| `vitte-perl-lisbon` | `Vitte::City::Lisbon` | Version parsing, ordering, and range checks |
| `vitte-perl-madrid` | `Vitte::City::Madrid` | Identifier generation, validation, normalization, and parsing |
| `vitte-perl-athens` | `Vitte::City::Athens` | Paths, temp files, cleanup, manifests, permissions, atomic IO, errors |
| `vitte-perl-prague` | `Vitte::City::Prague` | Configuration parsing, merging, and emission |
| `vitte-perl-vienna` | `Vitte::City::Vienna` | Numeric helpers for statistics and bounded arithmetic |
| `vitte-perl-zurich` | `Vitte::City::Zurich` | Ordered maps, sets, grouping, and indexing helpers |
| `vitte-perl-nairobi` | `Vitte::City::Nairobi` | Environment, path, and process context helpers |
| `vitte-perl-quito` | `Vitte::City::Quito` | Seeded pseudo-random numbers, ranges, and selection |
| `vitte-perl-delhi` | `Vitte::City::Delhi` | Text templates, interpolation, indentation, and wrapping |
| `vitte-perl-kyoto` | `Vitte::City::Kyoto` | Small assertion harness for embedded package tests |

Each package contains `META.json`, `README.md`, `OWNERS`, `lib/`, and `t/basic.t`.

## Gate

Run:

```sh
make perl-packages-check
```

The gate validates every `META.json`, checks version `0.1.0`, compiles each
`.pm` file, runs every `t/basic.t`, tests loading through `PERL5LIB`, extracts
each archive into an isolated install root, and generates:

- `target/perl-packages/archives/*.tar.gz`
- `target/perl-packages/checksums.sha256`
- `target/perl-packages/registry.json`
- `target/reports/perl_packages_check.json`
