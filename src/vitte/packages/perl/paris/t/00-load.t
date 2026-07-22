use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More;

use_ok('Vitte::City::Paris');
is($Vitte::City::Paris::VERSION, '0.1.0', 'version is 0.1.0');

done_testing();
