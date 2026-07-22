use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Uuid qw(nil is_nil parse_uuid format_uuid);

Test::More::is($Vitte::Crates::Uuid::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { is_nil(nil()) && defined parse_uuid(nil()) }, 'basic behavior works');
