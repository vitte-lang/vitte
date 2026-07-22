use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Anyhow qw(ok err is_ok is_err unwrap context);

Test::More::is($Vitte::Crates::Anyhow::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { is_ok(ok(1)) && is_err(context(err("bad"), "ctx")) }, 'basic behavior works');
