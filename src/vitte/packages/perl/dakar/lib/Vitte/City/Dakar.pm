package Vitte::City::Dakar;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Dakar::Entry qw(
    entry entry_field entry_fields entry_level entry_message entry_timestamp
    entry_to_hash entry_with_field event fields level message timestamp
);
use Vitte::City::Dakar::Error qw(
    dakar_error error_code error_context error_is error_message error_operation
    error_to_hash error_to_string error_with_context
);
use Vitte::City::Dakar::Filter qw(
    filter_between filter_field filter_level filter_limit filter_message
    filter_min_level filter_predicate filter_recent
);
use Vitte::City::Dakar::Format qw(
    format_csv format_json format_kv format_level_badge format_lines
    format_pretty format_text parse_kv
);
use Vitte::City::Dakar::Level qw(
    level_above level_all level_below level_compare level_enabled level_known
    level_name level_value max_level min_level normalize_level
);
use Vitte::City::Dakar::Logger qw(
    clear_entries clear_fields debug entries entry_count error fatal info
    last_entry log_event log_many logger logger_capacity logger_clone logger_field
    logger_fields logger_has_field logger_level logger_name logger_restore
    logger_snapshot prune_entries reset_logger set_capacity set_level set_name
    trace warn with_field with_fields without_field
);
use Vitte::City::Dakar::Metrics qw(
    count_by_field count_by_level duration_seconds first_timestamp has_errors
    has_warnings last_timestamp levels_seen messages messages_by_level summarize
);
use Vitte::City::Dakar::Sink qw(
    memory_sink sink_clear sink_drain sink_entries sink_last sink_size sink_write
    tee_sinks
);
use Vitte::City::Dakar::Trace qw(
    span_add_field span_duration_ms span_event span_fields span_finish span_id
    span_start span_to_hash trace_id
);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    logger
    log_event
    debug
    info
    warn
    error
    entries
    with_field
    filter_level
    trace
    fatal
    log_many
    logger_capacity
    logger_clone
    logger_field
    logger_has_field
    logger_level
    set_level
    logger_name
    set_name
    logger_fields
    logger_snapshot
    logger_restore
    prune_entries
    set_capacity
    with_fields
    without_field
    clear_fields
    entry_count
    last_entry
    clear_entries
    reset_logger
    level_all
    level_above
    level_below
    level_compare
    level_enabled
    level_known
    level_name
    level_value
    max_level
    min_level
    normalize_level
    entry
    event
    entry_level
    level
    entry_message
    message
    entry_timestamp
    timestamp
    entry_fields
    fields
    entry_field
    entry_with_field
    entry_to_hash
    filter_between
    filter_field
    filter_limit
    filter_message
    filter_min_level
    filter_predicate
    filter_recent
    format_csv
    format_json
    format_kv
    format_level_badge
    format_lines
    format_pretty
    format_text
    parse_kv
    memory_sink
    sink_write
    sink_entries
    sink_last
    sink_drain
    sink_clear
    sink_size
    tee_sinks
    count_by_level
    count_by_field
    first_timestamp
    last_timestamp
    duration_seconds
    messages
    messages_by_level
    levels_seen
    has_errors
    has_warnings
    summarize
    trace_id
    span_id
    span_start
    span_add_field
    span_fields
    span_event
    span_finish
    span_to_hash
    span_duration_ms
    dakar_error
    error_code
    error_message
    error_context
    error_operation
    error_to_hash
    error_to_string
    error_with_context
    error_is
);

1;
