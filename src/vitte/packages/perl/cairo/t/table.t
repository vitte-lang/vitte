use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Cairo::Table qw(
    table table_add_row table_column_values table_columns table_count table_find
    table_first table_from_records table_group_count table_index_by table_join_text
    table_limit table_offset table_project table_rows table_sort table_to_records
    table_where
);

my $table = table_from_records([
    { id => 2, name => 'beta', kind => 'data' },
    { id => 1, name => 'alpha', kind => 'data' },
    { id => 3, name => 'gamma', kind => 'schema' },
]);

Test::More::is_deeply(table_columns($table), ['id', 'kind', 'name'], 'columns inferred');
Test::More::is(table_count($table), 3, 'count');
table_add_row($table, { id => 4, name => 'delta', kind => 'codec' });
Test::More::is(table_count($table), 4, 'add row');
Test::More::is_deeply(table_rows(table_limit($table, 1))->[0]->{id}, 2, 'limit');
Test::More::is(table_count(table_offset($table, 3)), 1, 'offset');
Test::More::is_deeply(table_columns(table_project($table, ['name'])), ['name'], 'project columns');
Test::More::is(table_count(table_where($table, sub { $_[0]->{kind} eq 'data' })), 2, 'where');
Test::More::is(table_first(table_sort($table, 'name'))->{name}, 'alpha', 'sort');
Test::More::is(table_group_count($table, 'kind')->{data}, 2, 'group count');
Test::More::is(table_find($table, 'id', 3)->{name}, 'gamma', 'find');
Test::More::is_deeply(table_column_values($table, 'id'), [2, 1, 3, 4], 'column values');
Test::More::is_deeply(table_to_records(table(['id'], [{ id => 1 }]))->[0], { id => 1 }, 'to records');
Test::More::is(table_join_text($table, 'name', '|'), 'beta|alpha|gamma|delta', 'join text');
Test::More::is(table_index_by($table, 'id')->{4}->{name}, 'delta', 'index by');

Test::More::done_testing();
