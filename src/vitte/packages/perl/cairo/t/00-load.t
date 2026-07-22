use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::use_ok('Vitte::City::Cairo');
Test::More::use_ok('Vitte::City::Cairo::Scalar');
Test::More::use_ok('Vitte::City::Cairo::Record');
Test::More::use_ok('Vitte::City::Cairo::List');
Test::More::use_ok('Vitte::City::Cairo::Table');
Test::More::use_ok('Vitte::City::Cairo::Schema');
Test::More::use_ok('Vitte::City::Cairo::Codec');
Test::More::use_ok('Vitte::City::Cairo::Error');
Test::More::is($Vitte::City::Cairo::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::done_testing();
