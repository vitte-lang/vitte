use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Lyon qw(
    ok err some none unwrap unwrap_err unwrap_or option_or result_from_option
    result_from_eval result_flatten error_to_string lyon_error context
    context_merge context_to_hash collect_results report report_add report_summary
);

Test::More::is(unwrap_or(err('zero'), 0), 0, 'false default value is preserved');
Test::More::is(option_or(none(), 0), 0, 'false option default value is preserved');
Test::More::is(unwrap(result_from_option(some(0), 'missing')), 0, 'false option value converts to ok');
Test::More::like(unwrap_err(result_from_eval(sub { die "line\n" })), qr/line/, 'eval failure message is retained');
Test::More::is(unwrap(result_flatten(ok(ok('nested')))), 'nested', 'nested ok result flattens');

my $error = lyon_error('', '', path => 'src/main.vit');
Test::More::like(error_to_string($error), qr/LYON_E_UNKNOWN/, 'empty code normalizes');
Test::More::like(error_to_string($error), qr/src\/main\.vit/, 'error path is rendered');

my $left = context(a => 1, shared => 'left');
my $right = context(b => 2, shared => 'right');
Test::More::is(context_to_hash(context_merge($left, $right))->{values}{shared}, 'right', 'merge order stable');

my $collected = collect_results([ok('a'), err('root'), ok('ignored')]);
Test::More::is(unwrap_err($collected), 'root', 'collect_results returns root error');

my $summary = report_summary(report_add(report_add(report(), ok('a')), err('b')));
Test::More::is($summary->{passed}, 0, 'report summary keeps passed false');

Test::More::done_testing();
