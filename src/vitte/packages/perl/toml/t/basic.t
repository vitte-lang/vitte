use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Toml qw(parse_toml emit_toml get_key);

Test::More::is($Vitte::Crates::Toml::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { get_key(parse_toml("name = vitte"), "name") eq "vitte" }, 'basic behavior works');
