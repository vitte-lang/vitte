use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::use_ok('Vitte::City::Lyon');
Test::More::use_ok('Vitte::City::Lyon::Result');
Test::More::use_ok('Vitte::City::Lyon::Option');
Test::More::use_ok('Vitte::City::Lyon::Error');
Test::More::use_ok('Vitte::City::Lyon::Context');
Test::More::use_ok('Vitte::City::Lyon::Recovery');
Test::More::use_ok('Vitte::City::Lyon::Report');
Test::More::is($Vitte::City::Lyon::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::done_testing();
