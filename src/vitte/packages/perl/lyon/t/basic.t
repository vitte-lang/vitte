use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Lyon qw(
    ok err result is_ok is_err is_result unwrap unwrap_err unwrap_or unwrap_or_else
    expect expect_err map_result map_error and_then or_else result_match result_value
    result_error result_to_bool result_to_list result_from_option result_from_eval
    result_flatten some none is_some is_none option_value unwrap_option option_or
    option_or_else option_map option_and_then option_filter option_match
    option_to_list option_to_result option_from_value lyon_error error_code
    error_message error_context error_path error_cause error_with_context
    error_with_cause error_to_hash error_to_string error_is context context_get
    context_set context_has context_delete context_keys context_values context_merge
    context_push context_chain context_to_hash attempt recover retry fallback
    collect_results collect_errors partition_results combine_results first_ok
    first_err all_ok report report_add report_count report_ok report_values
    report_errors report_summary report_to_text
);

Test::More::is($Vitte::City::Lyon::VERSION, '0.1.0', 'version is 0.1.0');

my $ok = ok(2);
my $err = err('bad');
Test::More::ok(is_result($ok), 'is_result');
Test::More::ok(is_ok($ok), 'is_ok');
Test::More::ok(is_err($err), 'is_err');
Test::More::is(unwrap($ok), 2, 'unwrap');
Test::More::is(unwrap_err($err), 'bad', 'unwrap_err');
Test::More::is(unwrap_or($err, 9), 9, 'unwrap_or');
Test::More::is(unwrap_or_else($err, sub { 'fixed' }), 'fixed', 'unwrap_or_else');
Test::More::is(expect($ok, 'needed'), 2, 'expect');
Test::More::is(expect_err($err, 'needed'), 'bad', 'expect_err');
Test::More::is(unwrap(map_result($ok, sub { $_[0] + 1 })), 3, 'map_result');
Test::More::is(unwrap_err(map_error($err, sub { "E:$_[0]" })), 'E:bad', 'map_error');
Test::More::is(unwrap(and_then($ok, sub { ok($_[0] + 3) })), 5, 'and_then');
Test::More::is(unwrap(or_else($err, sub { ok('recovered') })), 'recovered', 'or_else');
Test::More::is(result_match($ok, ok => sub { $_[0] * 2 }, err => sub { 0 }), 4, 'result_match');
Test::More::is(result_value($ok), 2, 'result_value');
Test::More::is(result_error($err), 'bad', 'result_error');
Test::More::ok(result_to_bool($ok), 'result_to_bool');
Test::More::is_deeply(result_to_list($ok), [2], 'result_to_list');
Test::More::is(unwrap(result_from_option(some(4), 'none')), 4, 'result_from_option');
Test::More::is(unwrap(result_from_eval(sub { 7 })), 7, 'result_from_eval');
Test::More::is(unwrap(result_flatten(ok(ok(8)))), 8, 'result_flatten');
Test::More::ok(is_ok(result(ok => 1, value => 'x')), 'result constructor');

my $some = some('x');
my $none = none();
Test::More::ok(is_some($some), 'is_some');
Test::More::ok(is_none($none), 'is_none');
Test::More::is(option_value($some), 'x', 'option_value');
Test::More::is(unwrap_option($some), 'x', 'unwrap_option');
Test::More::is(option_or($none, 'default'), 'default', 'option_or');
Test::More::is(option_or_else($none, sub { 'late' }), 'late', 'option_or_else');
Test::More::is(unwrap_option(option_map($some, sub { uc $_[0] })), 'X', 'option_map');
Test::More::is(unwrap_option(option_and_then($some, sub { some($_[0] . 'y') })), 'xy', 'option_and_then');
Test::More::ok(is_none(option_filter($some, sub { 0 })), 'option_filter');
Test::More::is(option_match($some, some => sub { $_[0] }, none => sub { 'n' }), 'x', 'option_match');
Test::More::is_deeply(option_to_list($some), ['x'], 'option_to_list');
Test::More::is(unwrap(option_to_result($some, 'missing')), 'x', 'option_to_result');
Test::More::ok(is_none(option_from_value(undef)), 'option_from_value');

my $cause = lyon_error('LYON_E_CAUSE', 'cause');
my $error = error_with_cause(lyon_error('LYON_E_TEST', 'message', path => '/tmp'), $cause);
Test::More::is(error_code($error), 'LYON_E_TEST', 'error_code');
Test::More::is(error_message($error), 'message', 'error_message');
Test::More::is(error_context($error)->{path}, '/tmp', 'error_context');
Test::More::is(error_path($error), '/tmp', 'error_path');
Test::More::is(error_cause($error)->{code}, 'LYON_E_CAUSE', 'error_cause');
Test::More::is(error_with_context($error, phase => 'test')->{context}->{phase}, 'test', 'error_with_context');
Test::More::is(error_to_hash($error)->{package}, 'lyon', 'error_to_hash');
Test::More::like(error_to_string($error), qr/LYON_E_TEST/, 'error_to_string');
Test::More::ok(error_is($error, 'LYON_E_TEST'), 'error_is');

my $ctx = context(component => 'compiler');
context_set($ctx, phase => 'parse');
Test::More::is(context_get($ctx, 'phase'), 'parse', 'context_get/set');
Test::More::ok(context_has($ctx, 'component'), 'context_has');
Test::More::is(context_delete($ctx, 'phase'), 'parse', 'context_delete');
Test::More::is_deeply(context_keys($ctx), ['component'], 'context_keys');
Test::More::is_deeply(context_values($ctx), ['compiler'], 'context_values');
Test::More::is(context_merge($ctx, context(target => 'native'))->{values}->{target}, 'native', 'context_merge');
context_push($ctx, 'parse', file => 'main.vit');
Test::More::is(context_chain($ctx)->[0]->{label}, 'parse', 'context_chain');
Test::More::is(context_to_hash($ctx)->{values}->{component}, 'compiler', 'context_to_hash');

Test::More::is(unwrap(attempt(sub { 3 })), 3, 'attempt');
Test::More::is(unwrap(recover(err('x'), sub { 'ok' })), 'ok', 'recover');
Test::More::is(unwrap(retry(sub { 'again' }, 2)), 'again', 'retry');
Test::More::is(unwrap(fallback(err('x'), 'fallback')), 'fallback', 'fallback');
Test::More::is_deeply(unwrap(collect_results([ok(1), ok(2)])), [1, 2], 'collect_results');
Test::More::is_deeply(collect_errors([ok(1), err('e')]), ['e'], 'collect_errors');
Test::More::is_deeply(partition_results([ok(1), err('e')])->{ok}, [1], 'partition_results');
Test::More::is_deeply(unwrap(combine_results(ok(1), ok(2))), [1, 2], 'combine_results');
Test::More::is(unwrap(first_ok([err('e'), ok('v')])), 'v', 'first_ok');
Test::More::is(unwrap_err(first_err([ok('v'), err('e')])), 'e', 'first_err');
Test::More::ok(all_ok([ok(1), ok(2)]), 'all_ok');

my $report = report();
report_add($report, ok(1));
report_add($report, err('e'));
Test::More::is(report_count($report), 2, 'report_count');
Test::More::ok(!report_ok($report), 'report_ok');
Test::More::is_deeply(report_values($report), [1], 'report_values');
Test::More::is_deeply(report_errors($report), ['e'], 'report_errors');
Test::More::is(report_summary($report)->{err}, 1, 'report_summary');
Test::More::like(report_to_text($report), qr/total=2/, 'report_to_text');

Test::More::done_testing();
