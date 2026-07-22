use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Tokio qw(task runtime spawn block_on);

Test::More::is($Vitte::Crates::Tokio::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { my $t=task("x",sub{3}); block_on($t) == 3 }, 'basic behavior works');
