use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Dakar::Entry qw(entry);
use Vitte::City::Dakar::Metrics qw(count_by_field count_by_level duration_seconds first_timestamp has_errors has_warnings last_timestamp levels_seen messages messages_by_level summarize);

my $entries = [
    entry(level => 'warn', message => 'slow', timestamp => 10, fields => { component => 'cli' }),
    entry(level => 'error', message => 'failed', timestamp => 20, fields => { component => 'cli' }),
];
Test::More::is(count_by_level($entries)->{warn}, 1, 'count level');
Test::More::is(count_by_field($entries, 'component')->{cli}, 2, 'count field');
Test::More::is(first_timestamp($entries), 10, 'first timestamp');
Test::More::is(last_timestamp($entries), 20, 'last timestamp');
Test::More::is(duration_seconds($entries), 10, 'duration');
Test::More::is_deeply(messages($entries), ['slow', 'failed'], 'messages');
Test::More::is_deeply(messages_by_level($entries, 'error'), ['failed'], 'messages by level');
Test::More::is_deeply(levels_seen($entries), ['error', 'warn'], 'levels seen');
Test::More::ok(has_errors($entries), 'has errors');
Test::More::ok(has_warnings($entries), 'has warnings');
Test::More::is(summarize($entries)->{total}, 2, 'summary');

Test::More::done_testing();
