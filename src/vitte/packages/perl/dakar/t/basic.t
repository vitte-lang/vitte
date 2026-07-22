use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Dakar qw(logger log_event debug info warn error entries with_field filter_level);

Test::More::is($Vitte::City::Dakar::VERSION, '0.1.0', 'version is 0.1.0');

my $log = with_field(logger('info'), component => 'compiler');
debug($log, 'hidden');
info($log, 'ready');
warn($log, 'slow');
Test::More::is(scalar entries($log), 2, 'level filter keeps visible entries');
Test::More::is((entries($log))[0]->{fields}->{component}, 'compiler', 'base field is attached');
Test::More::is(scalar @{ filter_level($log, 'warn') }, 1, 'filter_level selects warnings');
Test::More::is((entries($log))[1]->{message}, 'slow', 'messages are retained');

Test::More::done_testing();
