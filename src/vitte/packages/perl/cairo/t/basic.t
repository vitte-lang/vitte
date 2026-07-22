use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Cairo qw(
    encode_scalar decode_scalar escape_scalar unescape_scalar scalar_type scalar_is_null
    scalar_is_bool scalar_is_number scalar_is_integer scalar_to_bool scalar_to_number
    scalar_to_string scalar_trim scalar_lower scalar_upper scalar_default scalar_compare
    encode_record decode_record record record_get record_set record_has record_delete
    record_keys record_values record_pairs record_merge record_project record_omit
    record_require record_apply_defaults record_rename record_diff record_to_query
    encode_list decode_list list list_push list_pop list_shift list_unshift list_unique
    list_sort list_reverse list_slice list_join list_contains list_index_of list_count
    list_compact list_flatten table table_columns table_rows table_add_row table_count
    table_project table_where table_sort table_limit table_offset table_group_count
    table_find table_first table_column_values table_to_records table_from_records
    table_join_text table_index_by schema field schema_fields schema_required
    schema_optional schema_defaults schema_apply_defaults schema_validate_record
    schema_validate_table schema_errors schema_is_valid type_string type_number
    type_integer type_bool type_list encode_document decode_document encode_lines
    decode_lines encode_pairs decode_pairs encode_table decode_table document_get
    document_set document_delete document_keys canonical_json stable_hash cairo_error
    error_code error_message error_context error_path error_field error_to_hash
    error_to_string error_with_context error_is
);

Test::More::is($Vitte::City::Cairo::VERSION, '0.1.0', 'version is 0.1.0');

Test::More::is(decode_scalar(encode_scalar("a=b;c|d\n")), "a=b;c|d\n", 'scalar roundtrip escapes separators');
Test::More::is(unescape_scalar(escape_scalar("x\ty")), "x\ty", 'escape_scalar aliases encode behavior');
Test::More::is(scalar_type(undef), 'null', 'scalar_type null');
Test::More::ok(scalar_is_null(undef), 'scalar_is_null');
Test::More::ok(scalar_is_bool('yes'), 'scalar_is_bool');
Test::More::ok(scalar_is_number('1.5e2'), 'scalar_is_number');
Test::More::ok(scalar_is_integer('-12'), 'scalar_is_integer');
Test::More::is(scalar_to_bool('on'), 1, 'scalar_to_bool');
Test::More::is(scalar_to_number('42', 0), 42, 'scalar_to_number');
Test::More::is(scalar_to_string(undef), '', 'scalar_to_string');
Test::More::is(scalar_trim('  hi  '), 'hi', 'scalar_trim');
Test::More::is(scalar_lower('ABC'), 'abc', 'scalar_lower');
Test::More::is(scalar_upper('abc'), 'ABC', 'scalar_upper');
Test::More::is(scalar_default('', 'fallback'), 'fallback', 'scalar_default');
Test::More::is(scalar_compare('a', 'b'), -1, 'scalar_compare');

my $rec = record(b => 2, a => 'x');
my $round = decode_record(encode_record($rec));
Test::More::is($round->{a}, 'x', 'record stores text');
Test::More::is($round->{b}, '2', 'record stores numbers as scalars');
record_set($rec, c => 3);
Test::More::is(record_get($rec, 'c'), 3, 'record_set and record_get');
Test::More::ok(record_has($rec, 'a'), 'record_has');
Test::More::is(record_delete($rec, 'c'), 3, 'record_delete');
Test::More::is_deeply(record_keys($rec), ['a', 'b'], 'record_keys');
Test::More::is_deeply(record_values($rec), ['x', 2], 'record_values');
Test::More::is_deeply(record_pairs($rec), [['a', 'x'], ['b', 2]], 'record_pairs');
Test::More::is(record_merge({ a => 1 }, { b => 2 })->{b}, 2, 'record_merge');
Test::More::is_deeply(record_project({ a => 1, b => 2 }, ['b']), { b => 2 }, 'record_project');
Test::More::is_deeply(record_omit({ a => 1, b => 2 }, ['a']), { b => 2 }, 'record_omit');
Test::More::is_deeply(record_require({ a => '' }, ['a', 'b']), ['a', 'b'], 'record_require');
Test::More::is(record_apply_defaults({ a => '' }, { a => 1, b => 2 })->{b}, 2, 'record_apply_defaults');
Test::More::is(record_rename({ old => 1 }, { old => 'new' })->{new}, 1, 'record_rename');
Test::More::is(record_diff({ a => 1 }, { a => 2 })->{a}->{right}, 2, 'record_diff');
Test::More::like(record_to_query({ a => 1 }), qr/a=1/, 'record_to_query');

my $items = list('b', 'a', 'b');
list_push($items, 'c');
Test::More::is(list_pop($items), 'c', 'list_pop');
list_unshift($items, 'z');
Test::More::is(list_shift($items), 'z', 'list_shift');
Test::More::is_deeply(decode_list(encode_list(['a', 'b'])), ['a', 'b'], 'list roundtrip works');
Test::More::is_deeply(list_unique($items), ['b', 'a'], 'list_unique');
Test::More::is_deeply(list_sort($items), ['a', 'b', 'b'], 'list_sort');
Test::More::is_deeply(list_reverse(['a', 'b']), ['b', 'a'], 'list_reverse');
Test::More::is_deeply(list_slice(['a', 'b', 'c'], 1, 2), ['b', 'c'], 'list_slice');
Test::More::is(list_join(['a', 'b'], '|'), 'a|b', 'list_join');
Test::More::ok(list_contains(['a'], 'a'), 'list_contains');
Test::More::is(list_index_of(['a', 'b'], 'b'), 1, 'list_index_of');
Test::More::is(list_count($items), 3, 'list_count');
Test::More::is_deeply(list_compact(['a', '', undef, 'b']), ['a', 'b'], 'list_compact');
Test::More::is_deeply(list_flatten(['a', ['b', ['c']]]), ['a', 'b', 'c'], 'list_flatten');

my $tbl = table_from_records([{ id => 2, name => 'b', kind => 'x' }, { id => 1, name => 'a', kind => 'x' }, { id => 3, name => 'c', kind => 'y' }]);
Test::More::is_deeply(table_columns($tbl), ['id', 'kind', 'name'], 'table_columns');
Test::More::is(table_count($tbl), 3, 'table_count');
table_add_row($tbl, { id => 4, name => 'd', kind => 'z' });
Test::More::is(table_count($tbl), 4, 'table_add_row');
Test::More::is_deeply(table_rows(table_limit($tbl, 1))->[0]->{id}, 2, 'table_rows and table_limit');
Test::More::is(table_count(table_offset($tbl, 2)), 2, 'table_offset');
Test::More::is(table_count(table_project($tbl, ['name'])), 4, 'table_project');
Test::More::is(table_count(table_where($tbl, sub { $_[0]->{kind} eq 'x' })), 2, 'table_where');
Test::More::is(table_first(table_sort($tbl, 'name'))->{name}, 'a', 'table_sort and table_first');
Test::More::is(table_group_count($tbl, 'kind')->{x}, 2, 'table_group_count');
Test::More::is(table_find($tbl, 'name', 'c')->{id}, 3, 'table_find');
Test::More::is_deeply(table_column_values($tbl, 'kind'), ['x', 'x', 'y', 'z'], 'table_column_values');
Test::More::is_deeply(table_to_records(table([{ a => 1 }], [{ a => 1 }]))->[0], { a => 1 }, 'table_to_records');
Test::More::is(table_join_text($tbl, 'name', ','), 'b,a,c,d', 'table_join_text');
Test::More::is(table_index_by($tbl, 'id')->{1}->{name}, 'a', 'table_index_by');

my $sch = schema(
    field('id', type => type_integer(), required => 1),
    field('name', type => type_string(), required => 1),
    field('active', type => type_bool(), default => 'true'),
    field('tags', type => type_list()),
);
Test::More::is_deeply([ map { $_->{name} } @{ schema_fields($sch) } ], ['id', 'name', 'active', 'tags'], 'schema_fields');
Test::More::is_deeply(schema_required($sch), ['id', 'name'], 'schema_required');
Test::More::is_deeply(schema_optional($sch), ['active', 'tags'], 'schema_optional');
Test::More::is(schema_defaults($sch)->{active}, 'true', 'schema_defaults');
Test::More::is(schema_apply_defaults($sch, { id => 1, name => 'a' })->{active}, 'true', 'schema_apply_defaults');
Test::More::ok(schema_is_valid($sch, { id => 1, name => 'a', active => 'yes', tags => [] }), 'schema_is_valid');
Test::More::is(schema_validate_record($sch, { id => 'x' })->{ok}, 0, 'schema_validate_record');
Test::More::ok(@{ schema_errors($sch, { id => 'x' }) } >= 1, 'schema_errors');
Test::More::is(schema_validate_table($sch, table(['id', 'name'], [{ id => 1, name => 'a' }]))->{ok}, 1, 'schema_validate_table');
Test::More::is(type_number(), 'number', 'type_number');

my $doc = { b => 2, a => [1] };
Test::More::is(decode_document(encode_document($doc))->{b}, 2, 'document json roundtrip');
Test::More::is_deeply(decode_lines(encode_lines([{ a => 1 }, { b => 2 }]))->[1], { b => '2' }, 'lines roundtrip');
Test::More::is_deeply(decode_pairs(encode_pairs([['a', 1]]))->[0], ['a', '1'], 'pairs roundtrip');
Test::More::is(table_count(decode_table(encode_table($tbl))), 4, 'table codec roundtrip');
document_set($doc, c => 3);
Test::More::is(document_get($doc, 'c'), 3, 'document_set/document_get');
Test::More::is(document_delete($doc, 'c'), 3, 'document_delete');
Test::More::is_deeply(document_keys({ b => 2, a => 1 }), ['a', 'b'], 'document_keys');
Test::More::is(canonical_json({ b => 2, a => 1 }), '{"a":1,"b":2}', 'canonical_json stable order');
Test::More::like(stable_hash({ a => 1 }), qr/\A[0-9a-f]{64}\z/, 'stable_hash');

my $err = cairo_error('CAIRO_E_TEST', 'message', field => 'id', path => '/rows/0');
Test::More::is(error_code($err), 'CAIRO_E_TEST', 'error_code');
Test::More::is(error_message($err), 'message', 'error_message');
Test::More::is(error_context($err)->{field}, 'id', 'error_context');
Test::More::is(error_path($err), '/rows/0', 'error_path');
Test::More::is(error_field($err), 'id', 'error_field');
Test::More::is(error_to_hash($err)->{package}, 'cairo', 'error_to_hash');
Test::More::like(error_to_string($err), qr/CAIRO_E_TEST/, 'error_to_string');
Test::More::is(error_with_context($err, extra => 1)->{context}->{extra}, 1, 'error_with_context');
Test::More::ok(error_is($err, 'CAIRO_E_TEST'), 'error_is');

Test::More::done_testing();
