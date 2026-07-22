use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Quito qw(seeded next_u32 range choice shuffle bytes_random);

Test::More::is($Vitte::City::Quito::VERSION, '0.1.0', 'version is 0.1.0');

my $a = seeded(3);
my $b = seeded(3);
Test::More::is(next_u32($a), next_u32($b), 'same seed is deterministic');
Test::More::ok(range($a, 10, 20) >= 10, 'range lower bound');
Test::More::ok(range($a, 10, 20) <= 20, 'range upper bound');
Test::More::ok(defined choice($a, [qw(a b)]), 'choice picks item');
Test::More::is(scalar @{ bytes_random($a, 4) }, 4, 'random byte count');

Test::More::done_testing();
