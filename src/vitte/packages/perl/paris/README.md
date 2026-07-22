# vitte-perl-paris

Version: 0.1.0

Paris provides deterministic command-line parsing for Vitte tools and package
scripts. It handles long options, short flags, repeated options, positional
arguments, command routing, environment extraction, config lines, usage text,
table rendering, and shell-safe command formatting.

## Usage

```sh
PERL5LIB=lib perl -MVitte::City::Paris=parse_args,option -e 'print option(parse_args([qw(--output main)]), "output")'
```

## Exported Symbols

- `args_to_hash`
- `command`
- `command_spec`
- `flag_count`
- `get_flag_count`
- `has_flag`
- `help_requested`
- `merge_defaults`
- `normalize_flag`
- `option`
- `option_bool`
- `option_int`
- `option_list`
- `options`
- `parse_args`
- `parse_config_line`
- `parse_env`
- `positionals`
- `quote_arg`
- `render_table`
- `require_option`
- `route_command`
- `shell_join`
- `spec_option`
- `split_kv`
- `subcommand`
- `unknown_options`
- `usage`
- `validate_options`
- `validate_spec`
- `version_requested`

## Quality Contract

- Every module must pass `perl -c`.
- Runtime tests must pass with explicit `PERL5LIB`.
- `META.json`, `MANIFEST`, `README.md`, `OWNERS`, docs, examples, corpus, and snapshots are required.
- The package must be consumable by `make perl-vitte-integration`.
