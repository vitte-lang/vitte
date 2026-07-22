use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Tracing qw(span event record fields);

Test::More::is($Vitte::Crates::Tracing::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { my $s=record(span("build"), event("info","ok")); @{$s->{events}} == 1 }, 'basic behavior works');
