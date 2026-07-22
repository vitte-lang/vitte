use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::use_ok('Vitte::City::Athens');
Test::More::use_ok('Vitte::City::Athens::Path');
Test::More::use_ok('Vitte::City::Athens::Temp');
Test::More::use_ok('Vitte::City::Athens::Cleanup');
Test::More::use_ok('Vitte::City::Athens::Manifest');
Test::More::use_ok('Vitte::City::Athens::Permissions');
Test::More::use_ok('Vitte::City::Athens::Atomic');
Test::More::use_ok('Vitte::City::Athens::Error');

Test::More::is($Vitte::City::Athens::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::done_testing();
