use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Lyon qw(
    ok err unwrap unwrap_err is_ok is_err attempt recover retry fallback
    collect_results collect_errors partition_results combine_results first_ok
    first_err all_ok
);

Test::More::is(unwrap(attempt(sub { 'value' })), 'value', 'attempt captures success');
Test::More::like(unwrap_err(attempt(sub { die "boom\n" })), qr/boom/, 'attempt captures failure');

Test::More::is(unwrap(recover(err('missing'), sub { "fixed:$_[0]" })), 'fixed:missing', 'recover maps error');
Test::More::is(unwrap(fallback(err('missing'), 'default')), 'default', 'fallback replaces error');

my $tries = 0;
my $retried = retry(sub {
    $tries++;
    die "again\n" if $tries < 3;
    return 'done';
}, 4);
Test::More::is(unwrap($retried), 'done', 'retry succeeds before limit');
Test::More::is($tries, 3, 'retry stops after success');

Test::More::is_deeply(unwrap(collect_results([ok(1), ok(2), ok(3)])), [1, 2, 3], 'collect_results success');
Test::More::ok(is_err(collect_results([ok(1), err('e'), ok(3)])), 'collect_results stops on first error');
Test::More::is_deeply(collect_errors([ok(1), err('e1'), err('e2')]), [qw(e1 e2)], 'collect_errors');
Test::More::is_deeply(partition_results([ok('a'), err('b'), ok('c')]), { ok => [qw(a c)], err => ['b'] }, 'partition_results');
Test::More::is_deeply(unwrap(combine_results(ok('x'), ok('y'))), [qw(x y)], 'combine_results');
Test::More::is(unwrap(first_ok([err('a'), ok('b')])), 'b', 'first_ok');
Test::More::is(unwrap_err(first_err([ok('a'), err('b')])), 'b', 'first_err');
Test::More::ok(all_ok([ok(1), ok(2)]), 'all_ok true');
Test::More::ok(!all_ok([ok(1), err('e')]), 'all_ok false');

Test::More::done_testing();
