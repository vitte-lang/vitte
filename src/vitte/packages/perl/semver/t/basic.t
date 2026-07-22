use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Semver qw(parse_version cmp_version satisfies);

Test::More::is($Vitte::Crates::Semver::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { satisfies("0.1.0", "0.1.0") }, 'basic behavior works');
