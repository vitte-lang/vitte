use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

Test::More::use_ok('Vitte::City::Dakar');
Test::More::use_ok('Vitte::City::Dakar::Level');
Test::More::use_ok('Vitte::City::Dakar::Entry');
Test::More::use_ok('Vitte::City::Dakar::Logger');
Test::More::use_ok('Vitte::City::Dakar::Filter');
Test::More::use_ok('Vitte::City::Dakar::Format');
Test::More::use_ok('Vitte::City::Dakar::Sink');
Test::More::use_ok('Vitte::City::Dakar::Metrics');
Test::More::use_ok('Vitte::City::Dakar::Trace');
Test::More::use_ok('Vitte::City::Dakar::Error');
Test::More::is($Vitte::City::Dakar::VERSION, '0.1.0', 'version is 0.1.0');
Test::More::done_testing();
