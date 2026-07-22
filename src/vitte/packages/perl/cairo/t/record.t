use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Cairo::Record qw(
    decode_record encode_record record record_apply_defaults record_delete
    record_diff record_get record_has record_keys record_merge record_omit
    record_pairs record_project record_rename record_require record_set
    record_to_query record_values
);

my $record = record(name => 'cairo', version => '0.1.0');
Test::More::is(decode_record(encode_record($record))->{name}, 'cairo', 'record codec roundtrip');
Test::More::is(record_get($record, 'missing', 'fallback'), 'fallback', 'record_get default');
record_set($record, stable => 'yes');
Test::More::ok(record_has($record, 'stable'), 'record_set and record_has');
Test::More::is(record_delete($record, 'stable'), 'yes', 'record_delete');
Test::More::is_deeply(record_keys($record), ['name', 'version'], 'record_keys sorted');
Test::More::is_deeply(record_values($record), ['cairo', '0.1.0'], 'record_values sorted');
Test::More::is_deeply(record_pairs($record), [['name', 'cairo'], ['version', '0.1.0']], 'record_pairs sorted');
Test::More::is(record_merge({ a => 1 }, { a => 2, b => 3 })->{a}, 2, 'record_merge right wins');
Test::More::is_deeply(record_project({ a => 1, b => 2 }, ['a']), { a => 1 }, 'record_project');
Test::More::is_deeply(record_omit({ a => 1, b => 2 }, ['b']), { a => 1 }, 'record_omit');
Test::More::is_deeply(record_require({ a => 1 }, ['a', 'b']), ['b'], 'record_require');
Test::More::is(record_apply_defaults({ a => '' }, { a => 1 })->{a}, 1, 'record_apply_defaults');
Test::More::is(record_rename({ a => 1 }, { a => 'b' })->{b}, 1, 'record_rename');
Test::More::is(record_diff({ a => 1 }, { a => 2 })->{a}->{left}, 1, 'record_diff');
Test::More::like(record_to_query({ a => 'x y' }), qr/a=x y/, 'record_to_query');

Test::More::done_testing();
