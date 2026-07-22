use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Dakar qw(
    logger log_event debug info warn error entries with_field filter_level trace fatal
    log_many logger_capacity logger_clone logger_field logger_has_field logger_level
    set_level logger_name set_name logger_fields logger_snapshot logger_restore
    prune_entries set_capacity with_fields without_field clear_fields entry_count
    last_entry clear_entries reset_logger level_all level_above level_below
    level_compare level_enabled level_known level_name level_value max_level min_level
    normalize_level entry event entry_level level entry_message message entry_timestamp
    timestamp entry_fields fields entry_field entry_with_field entry_to_hash
    filter_between filter_field filter_limit filter_message filter_min_level
    filter_predicate filter_recent format_csv format_json format_kv format_level_badge
    format_lines format_pretty format_text parse_kv memory_sink sink_write
    sink_entries sink_last sink_drain sink_clear sink_size tee_sinks count_by_level
    count_by_field first_timestamp last_timestamp duration_seconds messages
    messages_by_level levels_seen has_errors has_warnings summarize trace_id span_id
    span_start span_add_field span_fields span_event span_finish span_to_hash
    span_duration_ms dakar_error error_code error_message error_context
    error_operation error_to_hash error_to_string error_with_context error_is
);

Test::More::is($Vitte::City::Dakar::VERSION, '0.1.0', 'version is 0.1.0');

Test::More::is(normalize_level('INFO'), 'info', 'normalize_level');
Test::More::is(level_value('warn'), 30, 'level_value');
Test::More::is(level_name(20), 'info', 'level_name');
Test::More::ok(level_known('fatal'), 'level_known');
Test::More::ok(level_enabled('error', 'info'), 'level_enabled');
Test::More::is(level_compare('debug', 'info'), -1, 'level_compare');
Test::More::is(min_level('warn', 'error'), 'warn', 'min_level');
Test::More::is(max_level('warn', 'error'), 'error', 'max_level');
Test::More::ok(level_above('error', 'warn'), 'level_above');
Test::More::ok(level_below('debug', 'info'), 'level_below');
Test::More::is_deeply(level_all(), ['trace', 'debug', 'info', 'warn', 'error', 'fatal'], 'level_all');

my $log = logger('info', name => 'compiler', capacity => 3);
Test::More::is(logger_level($log), 'info', 'logger_level');
Test::More::is(logger_name($log), 'compiler', 'logger_name');
set_name($log, 'dakar');
set_level($log, 'debug');
set_capacity($log, 4);
Test::More::is(logger_capacity($log), 4, 'logger_capacity');
with_field($log, component => 'compiler');
with_fields($log, target => 'native', phase => 'lowering');
Test::More::is(logger_field($log, 'component'), 'compiler', 'logger_field');
Test::More::ok(logger_has_field($log, 'target'), 'logger_has_field');
without_field($log, 'phase');
Test::More::ok(!logger_has_field($log, 'phase'), 'without_field');
Test::More::is(logger_fields($log)->{target}, 'native', 'logger_fields');

trace($log, 'trace hidden');
debug($log, 'debug visible', step => 1);
info($log, 'ready');
warn($log, 'slow');
error($log, 'failed');
fatal($log, 'fatal');
prune_entries($log);
Test::More::is(entry_count($log), 4, 'capacity pruning keeps latest entries');
Test::More::is(last_entry($log)->{level}, 'fatal', 'last_entry');
Test::More::is((entries($log))[0]->{message}, 'ready', 'entries list');
my $snapshot = logger_snapshot($log);
clear_entries($log);
Test::More::is(entry_count($log), 0, 'clear_entries');
logger_restore($log, $snapshot);
Test::More::is(entry_count($log), 4, 'logger_restore');
Test::More::is(logger_clone($log)->{name}, logger_name($log), 'logger_clone');
log_many($log, [{ level => 'info', message => 'batch', fields => { batch => 1 } }]);
reset_logger($log);
Test::More::is(entry_count($log), 0, 'reset_logger entries');
Test::More::is_deeply(logger_fields($log), {}, 'reset_logger fields');

my $one = entry(level => 'warn', message => 'watch', timestamp => 10, fields => { component => 'parser' });
my $two = event(level => 'error', message => 'break', timestamp => 20, fields => { component => 'typeck' });
Test::More::is(entry_level($one), 'warn', 'entry_level');
Test::More::is(level($one), 'warn', 'level alias');
Test::More::is(entry_message($one), 'watch', 'entry_message');
Test::More::is(message($one), 'watch', 'message alias');
Test::More::is(entry_timestamp($one), 10, 'entry_timestamp');
Test::More::is(timestamp($one), 10, 'timestamp alias');
Test::More::is(entry_field($one, 'component'), 'parser', 'entry_field');
Test::More::is(entry_fields($one)->{component}, 'parser', 'entry_fields');
Test::More::is(fields($one)->{component}, 'parser', 'fields alias');
Test::More::is(entry_with_field($one, root => 1)->{fields}->{root}, 1, 'entry_with_field');
Test::More::is(entry_to_hash($two)->{message}, 'break', 'entry_to_hash');

my $filtered = [ $one, $two ];
Test::More::is(scalar @{ filter_level($filtered, 'warn') }, 1, 'filter_level');
Test::More::is(scalar @{ filter_min_level($filtered, 'warn') }, 2, 'filter_min_level');
Test::More::is(scalar @{ filter_message($filtered, qr/break/) }, 1, 'filter_message');
Test::More::is(scalar @{ filter_field($filtered, component => 'parser') }, 1, 'filter_field');
Test::More::is(scalar @{ filter_between($filtered, 0, 15) }, 1, 'filter_between');
Test::More::is(scalar @{ filter_predicate($filtered, sub { $_[0]->{level} eq 'error' }) }, 1, 'filter_predicate');
Test::More::is(scalar @{ filter_limit($filtered, 1) }, 1, 'filter_limit');
Test::More::ok(ref filter_recent($filtered, 3600) eq 'ARRAY', 'filter_recent');

Test::More::like(format_text($one), qr/\[warn\]/, 'format_text');
Test::More::like(format_json($one), qr/"level":"warn"/, 'format_json');
Test::More::like(format_kv($one), qr/component=parser/, 'format_kv');
Test::More::like(format_csv($one), qr/warn/, 'format_csv');
Test::More::is(format_level_badge($one), '[WARN]', 'format_level_badge');
Test::More::like(format_pretty($one), qr/WARN/, 'format_pretty');
Test::More::like(format_lines($filtered), qr/watch/, 'format_lines');
Test::More::is(parse_kv('a=1 b=2')->{b}, 2, 'parse_kv');

my $sink = memory_sink();
sink_write($sink, $one);
Test::More::is(sink_size($sink), 1, 'sink_size');
Test::More::is(sink_last($sink)->{message}, 'watch', 'sink_last');
Test::More::is_deeply(sink_entries($sink)->[0], $one, 'sink_entries');
Test::More::is(scalar @{ sink_drain($sink) }, 1, 'sink_drain');
Test::More::is(sink_size($sink), 0, 'sink_drain clears');
sink_write($sink, $two);
sink_clear($sink);
Test::More::is(sink_size($sink), 0, 'sink_clear');
my $sink_a = memory_sink();
my $sink_b = memory_sink();
tee_sinks($sink_a, $sink_b)->{write}->($one);
Test::More::is(sink_size($sink_b), 1, 'tee_sinks');

Test::More::is(count_by_level($filtered)->{warn}, 1, 'count_by_level');
Test::More::is(count_by_field($filtered, 'component')->{parser}, 1, 'count_by_field');
Test::More::is(first_timestamp($filtered), 10, 'first_timestamp');
Test::More::is(last_timestamp($filtered), 20, 'last_timestamp');
Test::More::is(duration_seconds($filtered), 10, 'duration_seconds');
Test::More::is_deeply(messages($filtered), ['watch', 'break'], 'messages');
Test::More::is_deeply(messages_by_level($filtered, 'error'), ['break'], 'messages_by_level');
Test::More::is_deeply(levels_seen($filtered), ['error', 'warn'], 'levels_seen');
Test::More::ok(has_errors($filtered), 'has_errors');
Test::More::ok(has_warnings($filtered), 'has_warnings');
Test::More::is(summarize($filtered)->{total}, 2, 'summarize');

Test::More::like(trace_id(), qr/^trace-/, 'trace_id');
Test::More::like(span_id(), qr/^span-/, 'span_id');
my $span = span_start(name => 'build', start => 1);
span_add_field($span, target => 'test');
span_event($span, 'parse', ok => 1);
span_finish($span, end => 2);
Test::More::is(span_fields($span)->{target}, 'test', 'span_fields');
Test::More::is(span_duration_ms($span), 1000, 'span_duration_ms');
Test::More::is(span_to_hash($span)->{name}, 'build', 'span_to_hash');

my $err = dakar_error('DAKAR_E_TEST', 'message', operation => 'format');
Test::More::is(error_code($err), 'DAKAR_E_TEST', 'error_code');
Test::More::is(error_message($err), 'message', 'error_message');
Test::More::is(error_context($err)->{operation}, 'format', 'error_context');
Test::More::is(error_operation($err), 'format', 'error_operation');
Test::More::is(error_to_hash($err)->{package}, 'dakar', 'error_to_hash');
Test::More::like(error_to_string($err), qr/DAKAR_E_TEST/, 'error_to_string');
Test::More::is(error_with_context($err, extra => 1)->{context}->{extra}, 1, 'error_with_context');
Test::More::ok(error_is($err, 'DAKAR_E_TEST'), 'error_is');

Test::More::done_testing();
