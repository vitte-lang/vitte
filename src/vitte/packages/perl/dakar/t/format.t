use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Dakar::Entry qw(entry);
use Vitte::City::Dakar::Format qw(format_csv format_json format_kv format_level_badge format_lines format_pretty format_text parse_kv);

my $entry = entry(level => 'warn', message => 'slow, path', timestamp => 1, fields => { component => 'cli' });
Test::More::like(format_text($entry), qr/\[warn\]/, 'text');
Test::More::like(format_json($entry), qr/"message":"slow, path"/, 'json');
Test::More::like(format_kv($entry), qr/component=cli/, 'kv');
Test::More::like(format_csv($entry), qr/"slow, path"/, 'csv escaping');
Test::More::is(format_level_badge($entry), '[WARN]', 'badge');
Test::More::like(format_pretty($entry), qr/WARN/, 'pretty');
Test::More::like(format_lines([$entry]), qr/slow/, 'lines');
Test::More::is(parse_kv('a=1 b=two')->{b}, 'two', 'parse kv');

Test::More::done_testing();
