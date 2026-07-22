use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Dakar::Entry qw(entry);
use Vitte::City::Dakar::Sink qw(memory_sink sink_clear sink_drain sink_entries sink_last sink_size sink_write tee_sinks);

my $entry = entry(level => 'info', message => 'ready');
my $sink = memory_sink();
sink_write($sink, $entry);
Test::More::is(sink_size($sink), 1, 'size');
Test::More::is(sink_last($sink)->{message}, 'ready', 'last');
Test::More::is(sink_entries($sink)->[0]->{level}, 'info', 'entries');
Test::More::is(scalar @{ sink_drain($sink) }, 1, 'drain returns entries');
Test::More::is(sink_size($sink), 0, 'drain clears');
sink_write($sink, $entry);
sink_clear($sink);
Test::More::is(sink_size($sink), 0, 'clear');
my ($a, $b) = (memory_sink(), memory_sink());
tee_sinks($a, $b)->{write}->($entry);
Test::More::is(sink_size($a), 1, 'tee first');
Test::More::is(sink_size($b), 1, 'tee second');

Test::More::done_testing();
