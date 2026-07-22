use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Clap qw(parse_flags has_flag flag_value usage);

Test::More::is($Vitte::Crates::Clap::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { my $f=parse_flags("--name", "vitte"); has_flag($f,"name") && flag_value($f,"name") eq "vitte" }, 'basic behavior works');
