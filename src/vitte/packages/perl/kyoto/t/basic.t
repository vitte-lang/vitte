use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Kyoto qw(suite assert_true assert_eq assert_deep run_suite summary);

Test::More::is($Vitte::City::Kyoto::VERSION, '0.1.0', 'version is 0.1.0');

my $s = suite('unit');
my $summary = run_suite($s, assert_true('truth', 1), assert_eq('eq', 'a', 'a'), assert_deep('deep', [1, 2], [1, 2]));
Test::More::is($summary->{total}, 3, 'suite counts tests');
Test::More::is($summary->{passed}, 3, 'suite counts pass');
Test::More::is($summary->{failed}, 0, 'suite counts failures');
Test::More::ok(assert_true('bad', 0)->{message}, 'failed assertion has message');

Test::More::done_testing();
