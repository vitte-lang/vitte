use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Thiserror qw(error_new error_message error_code error_with_source);

Test::More::is($Vitte::Crates::Thiserror::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { error_code(error_new("E", "msg")) eq "E" }, 'basic behavior works');
