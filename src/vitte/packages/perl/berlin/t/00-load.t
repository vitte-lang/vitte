use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::use_ok('Vitte::City::Berlin');
Test::More::use_ok('Vitte::City::Berlin::Buffer');
Test::More::use_ok('Vitte::City::Berlin::Hex');
Test::More::use_ok('Vitte::City::Berlin::Cursor');
Test::More::use_ok('Vitte::City::Berlin::Pack');
Test::More::use_ok('Vitte::City::Berlin::Base64');
Test::More::use_ok('Vitte::City::Berlin::Checksum');
Test::More::use_ok('Vitte::City::Berlin::View');
Test::More::use_ok('Vitte::City::Berlin::Error');
Test::More::is($Vitte::City::Berlin::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::done_testing();
