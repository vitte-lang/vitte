use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Seoul qw(is_match find_all captures replace_all escape_pattern split_pattern);

Test::More::is($Vitte::City::Seoul::VERSION, '0.1.0', 'version is 0.1.0');

Test::More::ok(is_match(qr/[0-9]+/, 'a12'), 'is_match detects pattern');
Test::More::is_deeply(find_all(qr/[a-z]/, 'a1b2'), ['a', 'b'], 'find_all captures repeated matches');
Test::More::is_deeply(captures(qr/(a)(b)/, 'ab'), ['a', 'b'], 'captures returns groups');
Test::More::is(replace_all(qr/a/, 'x', 'banana'), 'bxnxnx', 'replace_all rewrites globally');
Test::More::is_deeply(split_pattern(qr/,/, 'a,b'), ['a', 'b'], 'split_pattern splits text');

Test::More::done_testing();
