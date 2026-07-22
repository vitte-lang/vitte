# Dakar Compatibility

- Minimum Perl version: `5.010`.
- Runtime dependencies are core modules: `Exporter` and `JSON::PP`.
- Existing exports `logger`, `log_event`, `debug`, `info`, `warn`, `error`,
  `entries`, `with_field`, and `filter_level` remain available.
- Formatters are deterministic for the same entry payload.
