use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Bytes qw(bytes from_string len slice);

Test::More::is($Vitte::Crates::Bytes::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { len(from_string("abc")) == 3 }, 'basic behavior works');
