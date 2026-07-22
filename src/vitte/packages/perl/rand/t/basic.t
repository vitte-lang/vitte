use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Rand qw(seeded next_u32 range choice);

Test::More::is($Vitte::Crates::Rand::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { my $r=seeded(7); range($r,1,10) >= 1 }, 'basic behavior works');
