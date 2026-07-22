use strict;
use warnings;
use FindBin;
use lib "$FindBin::Bin/../lib";
use Test::More ();

use Vitte::City::Cairo::Schema qw(
    field schema schema_apply_defaults schema_defaults schema_errors schema_fields
    schema_is_valid schema_optional schema_required schema_validate_record
    schema_validate_table type_bool type_integer type_list type_number type_string
);
use Vitte::City::Cairo::Table qw(table);

my $schema = schema(
    field('id', type => type_integer(), required => 1),
    field('name', type => type_string(), required => 1),
    field('score', type => type_number(), default => '0'),
    field('active', type => type_bool(), default => 'true'),
    field('tags', type => type_list()),
);

Test::More::is_deeply([ map { $_->{name} } @{ schema_fields($schema) } ], ['id', 'name', 'score', 'active', 'tags'], 'fields');
Test::More::is_deeply(schema_required($schema), ['id', 'name'], 'required fields');
Test::More::is_deeply(schema_optional($schema), ['score', 'active', 'tags'], 'optional fields');
Test::More::is(schema_defaults($schema)->{score}, '0', 'defaults');
Test::More::is(schema_apply_defaults($schema, { id => 1, name => 'x' })->{active}, 'true', 'apply defaults');
Test::More::ok(schema_is_valid($schema, { id => 1, name => 'x', score => '1.5', active => 'yes', tags => [] }), 'valid record');
Test::More::is(schema_validate_record($schema, { id => 'bad' })->{ok}, 0, 'invalid record');
Test::More::is(schema_errors($schema, { id => 'bad' })->[0]->{code}, 'CAIRO_E_REQUIRED', 'missing field error first');
Test::More::is(schema_validate_table($schema, table(['id', 'name'], [{ id => 1, name => 'x' }]))->{ok}, 1, 'valid table');
Test::More::is(type_string(), 'string', 'type_string');
Test::More::is(type_number(), 'number', 'type_number');
Test::More::is(type_integer(), 'integer', 'type_integer');
Test::More::is(type_bool(), 'bool', 'type_bool');
Test::More::is(type_list(), 'list', 'type_list');

Test::More::done_testing();
