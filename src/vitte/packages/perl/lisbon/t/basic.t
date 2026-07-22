use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Lisbon qw(parse_version format_version compare_version is_prerelease satisfies_min bump_major bump_minor bump_patch);

Test::More::is($Vitte::City::Lisbon::VERSION, '0.1.0', 'version is 0.1.0');

Test::More::is(format_version(parse_version('1.2.3')), '1.2.3', 'version roundtrip');
Test::More::ok(compare_version('1.2.4', '1.2.3') > 0, 'compare orders patch');
Test::More::ok(is_prerelease('1.0.0-alpha'), 'prerelease detected');
Test::More::ok(satisfies_min('2.0.0', '1.9.0'), 'minimum range passes');
Test::More::is(bump_minor('1.2.3'), '1.3.0', 'minor bump resets patch');

Test::More::done_testing();
