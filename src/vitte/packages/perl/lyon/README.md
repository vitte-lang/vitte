# vitte-perl-lyon

Version: 0.1.0

Lyon is the Vitte Perl package for explicit result, option, structured error,
context, recovery, and report flows. It is designed for command-line tools,
installers, package gates, and compiler-side checks that must return stable
machine-readable outcomes instead of mixing plain strings and process exits.

## Public Areas

- `Vitte::City::Lyon::Result`: ok/err values, mapping, matching, flattening, and eval capture.
- `Vitte::City::Lyon::Option`: some/none values, mapping, filtering, conversion to result.
- `Vitte::City::Lyon::Error`: stable error hashes with code, message, path, context, and cause.
- `Vitte::City::Lyon::Context`: sorted key/value context and trace chain helpers.
- `Vitte::City::Lyon::Recovery`: attempt, retry, fallback, collect, partition, and aggregate helpers.
- `Vitte::City::Lyon::Report`: build a deterministic summary from result streams.

## Install Test

```sh
PERL5LIB=lib perl -MVitte::City::Lyon=ok,unwrap -e 'print unwrap(ok("ready"))'
```

## Quality Contract

The package is expected to pass:

- `perl -c` for every module.
- Every `t/*.t` runtime test.
- Every `xt/*.t` author consistency test.
- Explicit `PERL5LIB` loading without user shell configuration.
- `META.json`, `MANIFEST`, `README.md`, `OWNERS`, examples, docs, corpus, and snapshots.

No user profile or global installation is required for the package tests.

## Exported Symbols

- `ok`
- `err`
- `result`
- `is_ok`
- `is_err`
- `is_result`
- `unwrap`
- `unwrap_err`
- `unwrap_or`
- `unwrap_or_else`
- `expect`
- `expect_err`
- `map_result`
- `map_error`
- `and_then`
- `or_else`
- `result_match`
- `result_value`
- `result_error`
- `result_to_bool`
- `result_to_list`
- `result_from_option`
- `result_from_eval`
- `result_flatten`
- `some`
- `none`
- `is_some`
- `is_none`
- `option_value`
- `unwrap_option`
- `option_or`
- `option_or_else`
- `option_map`
- `option_and_then`
- `option_filter`
- `option_match`
- `option_to_list`
- `option_to_result`
- `option_from_value`
- `lyon_error`
- `error_code`
- `error_message`
- `error_context`
- `error_path`
- `error_cause`
- `error_with_context`
- `error_with_cause`
- `error_to_hash`
- `error_to_string`
- `error_is`
- `context`
- `context_get`
- `context_set`
- `context_has`
- `context_delete`
- `context_keys`
- `context_values`
- `context_merge`
- `context_push`
- `context_chain`
- `context_to_hash`
- `attempt`
- `recover`
- `retry`
- `fallback`
- `collect_results`
- `collect_errors`
- `partition_results`
- `combine_results`
- `first_ok`
- `first_err`
- `all_ok`
- `report`
- `report_add`
- `report_count`
- `report_ok`
- `report_values`
- `report_errors`
- `report_summary`
- `report_to_text`
