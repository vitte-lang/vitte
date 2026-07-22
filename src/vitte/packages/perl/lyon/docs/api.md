# Lyon API

## Result

`ok`, `err`, and `result` create explicit success or failure values.
`is_ok`, `is_err`, and `is_result` test shape. `unwrap`, `unwrap_err`,
`expect`, and `expect_err` are fail-fast readers for tests and strict gates.

Transform functions are `map_result`, `map_error`, `and_then`, `or_else`,
`result_match`, `result_flatten`, `result_from_option`, and
`result_from_eval`.

## Option

`some` and `none` represent optional values. Helpers include `option_value`,
`unwrap_option`, `option_or`, `option_or_else`, `option_map`,
`option_and_then`, `option_filter`, `option_match`, `option_to_list`,
`option_to_result`, and `option_from_value`.

## Errors

`lyon_error` creates a stable error hash. Readers include `error_code`,
`error_message`, `error_context`, `error_path`, `error_cause`,
`error_to_hash`, `error_to_string`, and `error_is`.

## Context

`context` stores sorted values and trace entries. Use `context_set`,
`context_get`, `context_has`, `context_delete`, `context_keys`,
`context_values`, `context_merge`, `context_push`, `context_chain`, and
`context_to_hash`.

## Recovery And Reports

`attempt`, `recover`, `retry`, `fallback`, `collect_results`,
`collect_errors`, `partition_results`, `combine_results`, `first_ok`,
`first_err`, and `all_ok` build deterministic recovery paths.

`report`, `report_add`, `report_count`, `report_ok`, `report_values`,
`report_errors`, `report_summary`, and `report_to_text` summarize result
streams.
