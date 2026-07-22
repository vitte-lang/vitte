# Paris API

Paris exposes one module, `Vitte::City::Paris`.

## Parsing

`parse_args` returns a stable hash with `flags`, `options`, `counts`,
`positionals`, `command`, and `subcommand`. Long options are accepted as
`--name value` or `--name=value`. Short flags can be grouped, for example
`-vvv`. A short attached value such as `-omain` is treated as option `o`.

## Accessors

Use `has_flag`, `flag_count`, `get_flag_count`, `option`, `option_list`,
`options`, `option_int`, `option_bool`, `positionals`, `command`, and
`subcommand` to read parsed values without depending on internal structure.

## Validation

Use `unknown_options`, `validate_options`, `spec_option`, `command_spec`, and
`validate_spec` to enforce a command surface.

## Environment And Config

`parse_env` extracts a prefixed environment into lowercase normalized keys.
`parse_config_line` parses `key=value` and `key: value` lines while ignoring
comments and blank lines.

## Rendering

`usage` renders command help. `render_table` creates stable two-column text.
`quote_arg` and `shell_join` produce shell-safe command snippets.
