use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Dakar::Entry qw(entry);
use Vitte::City::Dakar::Filter qw(filter_between filter_field filter_level filter_limit filter_message filter_min_level filter_predicate filter_recent);

my $entries = [
    entry(level => 'info', message => 'ready', timestamp => 10, fields => { component => 'cli' }),
    entry(level => 'error', message => 'failed', timestamp => 20, fields => { component => 'build' }),
];
Test::More::is(scalar @{ filter_level($entries, 'info') }, 1, 'level');
Test::More::is(scalar @{ filter_min_level($entries, 'warn') }, 1, 'min level');
Test::More::is(scalar @{ filter_message($entries, qr/fail/) }, 1, 'message');
Test::More::is(scalar @{ filter_field($entries, component => 'cli') }, 1, 'field');
Test::More::is(scalar @{ filter_between($entries, 1, 15) }, 1, 'between');
Test::More::is(scalar @{ filter_predicate($entries, sub { $_[0]->{level} eq 'error' }) }, 1, 'predicate');
Test::More::is(scalar @{ filter_limit($entries, 1) }, 1, 'limit');
Test::More::ok(ref filter_recent($entries, 1) eq 'ARRAY', 'recent');

Test::More::done_testing();
