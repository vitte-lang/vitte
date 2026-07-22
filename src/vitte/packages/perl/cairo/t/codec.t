use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Cairo::Codec qw(
    canonical_json decode_document decode_lines decode_pairs decode_table document_delete
    document_get document_keys document_set encode_document encode_lines encode_pairs
    encode_table stable_hash
);
use Vitte::City::Cairo::Table qw(table_count table_from_records);

my $doc = { b => 2, a => [1] };
Test::More::is(canonical_json({ b => 2, a => 1 }), '{"a":1,"b":2}', 'canonical json');
Test::More::is(decode_document(encode_document($doc))->{b}, 2, 'document roundtrip');
Test::More::is_deeply(decode_lines(encode_lines([{ a => 1 }, { b => 2 }]))->[0], { a => '1' }, 'lines roundtrip');
Test::More::is_deeply(decode_pairs(encode_pairs([['a', 1], ['b', 2]]))->[1], ['b', '2'], 'pairs roundtrip');
my $table = table_from_records([{ id => 1 }, { id => 2 }]);
Test::More::is(table_count(decode_table(encode_table($table))), 2, 'table roundtrip');
document_set($doc, c => 3);
Test::More::is(document_get($doc, 'c'), 3, 'document set/get');
Test::More::is(document_delete($doc, 'c'), 3, 'document delete');
Test::More::is_deeply(document_keys({ b => 2, a => 1 }), ['a', 'b'], 'document keys');
Test::More::like(stable_hash({ a => 1 }), qr/\A[0-9a-f]{64}\z/, 'stable hash');

Test::More::done_testing();
