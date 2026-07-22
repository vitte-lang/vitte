use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Dakar::Entry qw(entry entry_field entry_fields entry_level entry_message entry_timestamp entry_to_hash entry_with_field event fields level message timestamp);

my $entry = entry(level => 'info', message => 'ready', timestamp => 1, fields => { component => 'compiler' });
Test::More::is(entry_level($entry), 'info', 'entry level');
Test::More::is(level($entry), 'info', 'level alias');
Test::More::is(entry_message($entry), 'ready', 'message');
Test::More::is(message($entry), 'ready', 'message alias');
Test::More::is(entry_timestamp($entry), 1, 'timestamp');
Test::More::is(timestamp($entry), 1, 'timestamp alias');
Test::More::is(entry_field($entry, 'component'), 'compiler', 'entry field');
Test::More::is(entry_fields($entry)->{component}, 'compiler', 'fields');
Test::More::is(fields($entry)->{component}, 'compiler', 'fields alias');
Test::More::is(entry_with_field($entry, phase => 'parse')->{fields}->{phase}, 'parse', 'with field');
Test::More::is(entry_to_hash(event(level => 'warn', message => 'slow'))->{level}, 'warn', 'event alias');

Test::More::done_testing();
