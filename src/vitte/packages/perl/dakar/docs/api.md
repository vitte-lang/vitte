# Dakar API

Dakar provides a broad, versioned structured-logging utility surface for Vitte package tooling. The public API is exported by `Vitte::City::Dakar` and implemented by focused submodules.

## Logger

Create, configure, clone, snapshot, restore, and mutate in-memory structured loggers.

- `logger`
- `log_event`
- `debug`
- `info`
- `warn`
- `error`
- `entries`
- `with_field`
- `filter_level`
- `trace`
- `fatal`
- `log_many`
- `logger_capacity`
- `logger_clone`
- `logger_field`
- `logger_has_field`
- `logger_level`
- `set_level`
- `logger_name`
- `set_name`
- `logger_fields`
- `logger_snapshot`
- `logger_restore`
- `prune_entries`
- `set_capacity`
- `with_fields`

## Levels

Normalize, compare, inspect, and evaluate log levels.

- `without_field`
- `clear_fields`
- `entry_count`
- `last_entry`
- `clear_entries`
- `reset_logger`
- `level_all`
- `level_above`
- `level_below`
- `level_compare`
- `level_enabled`

## Entries

Create and inspect structured log entries and immutable entry variants.

- `level_known`
- `level_name`
- `level_value`
- `max_level`
- `min_level`
- `normalize_level`
- `entry`
- `event`
- `entry_level`
- `level`
- `entry_message`
- `message`
- `entry_timestamp`
- `timestamp`

## Filters

Select entries by level, minimum level, message, field, time range, predicate, recency, or count.

- `entry_fields`
- `fields`
- `entry_field`
- `entry_with_field`
- `entry_to_hash`
- `filter_between`
- `filter_field`
- `filter_limit`

## Formatting

Render entries as text, JSON, key/value, CSV, pretty lines, badges, and parse simple key/value data.

- `filter_message`
- `filter_min_level`
- `filter_predicate`
- `filter_recent`
- `format_csv`
- `format_json`
- `format_kv`
- `format_level_badge`

## Sinks

Write entries to in-memory sinks, drain or clear them, inspect sizes, and tee to multiple sinks.

- `format_lines`
- `format_pretty`
- `format_text`
- `parse_kv`
- `memory_sink`
- `sink_write`
- `sink_entries`
- `sink_last`

## Metrics

Count levels and fields, inspect timestamps, summarize messages, and detect warnings or errors.

- `sink_drain`
- `sink_clear`
- `sink_size`
- `tee_sinks`
- `count_by_level`
- `count_by_field`
- `first_timestamp`
- `last_timestamp`
- `duration_seconds`
- `messages`
- `messages_by_level`
- `levels_seen`

## Tracing

Create trace/span identifiers, manage span fields/events, finish spans, and compute durations.

- `has_errors`
- `has_warnings`
- `summarize`
- `trace_id`
- `span_id`
- `span_start`
- `span_add_field`
- `span_fields`
- `span_event`

## Structured Errors

Create stable Dakar errors with code, message, context, operation, hash rendering, string rendering, and code matching.

- `span_finish`
- `span_to_hash`
- `span_duration_ms`
- `dakar_error`
- `error_code`
- `error_message`
- `error_context`
- `error_operation`
- `error_to_hash`
- `error_to_string`
- `error_with_context`
- `error_is`

## Distribution Contract

- `META.json` declares every module and core runtime dependency.
- `packages.json` mirrors `@EXPORT_OK` exactly.
- `MANIFEST` lists every package file required by the reproducible archive gate.
- `t/basic.t` mentions every exported symbol and focused tests cover every submodule.
