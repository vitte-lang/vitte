use strict;
use warnings;
use Test::More ();

use Vitte::City::Dakar qw(entry_count info logger);

Test::More::ok($INC{'Vitte/City/Dakar.pm'}, 'loaded through PERL5LIB or local lib');
my $log = logger('info');
info($log, 'ready');
Test::More::is(entry_count($log), 1, 'API works after load');
Test::More::done_testing();
