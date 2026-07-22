use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::plan(tests => 2);

use Vitte::Crates::Regex qw(is_match find_all replace_all escape_regex);

Test::More::is($Vitte::Crates::Regex::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::ok(do { is_match("vit", "vitte") && replace_all("t", "T", "vitte") eq "viTTe" }, 'basic behavior works');
