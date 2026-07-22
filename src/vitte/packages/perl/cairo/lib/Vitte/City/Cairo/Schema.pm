package Vitte::City::Cairo::Schema;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Cairo::Record qw(record_apply_defaults record_require);
use Vitte::City::Cairo::Scalar qw(scalar_is_bool scalar_is_integer scalar_is_number);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    field
    schema
    schema_apply_defaults
    schema_defaults
    schema_errors
    schema_fields
    schema_is_valid
    schema_optional
    schema_required
    schema_validate_record
    schema_validate_table
    type_bool
    type_integer
    type_list
    type_number
    type_string
);

sub type_string  { return 'string' }
sub type_number  { return 'number' }
sub type_integer { return 'integer' }
sub type_bool    { return 'bool' }
sub type_list    { return 'list' }

sub field {
    my ($name, %options) = @_;
    return {
        name     => $name,
        type     => $options{type} || type_string(),
        required => $options{required} ? 1 : 0,
        default  => $options{default},
    };
}

sub schema {
    my (@fields) = @_;
    return {
        fields => [ @fields ],
    };
}

sub schema_fields {
    return [ @{ $_[0]->{fields} || [] } ];
}

sub schema_required {
    my ($schema) = @_;
    return [ map { $_->{name} } grep { $_->{required} } @{ schema_fields($schema) } ];
}

sub schema_optional {
    my ($schema) = @_;
    return [ map { $_->{name} } grep { !$_->{required} } @{ schema_fields($schema) } ];
}

sub schema_defaults {
    my ($schema) = @_;
    my %defaults;
    for my $field (@{ schema_fields($schema) }) {
        $defaults{$field->{name}} = $field->{default} if exists $field->{default};
    }
    return \%defaults;
}

sub schema_apply_defaults {
    my ($schema, $record) = @_;
    return record_apply_defaults($record, schema_defaults($schema));
}

sub _type_ok {
    my ($type, $value) = @_;
    return 1 if !defined $value || $value eq '';
    return 1 if $type eq type_string();
    return scalar_is_number($value)  if $type eq type_number();
    return scalar_is_integer($value) if $type eq type_integer();
    return scalar_is_bool($value)    if $type eq type_bool();
    return ref($value) eq 'ARRAY' ? 1 : 0 if $type eq type_list();
    return 0;
}

sub schema_errors {
    my ($schema, $record) = @_;
    my @errors;
    for my $missing (@{ record_require($record, schema_required($schema)) }) {
        push @errors, { field => $missing, code => 'CAIRO_E_REQUIRED', message => 'required field is missing' };
    }
    for my $field (@{ schema_fields($schema) }) {
        next unless exists $record->{ $field->{name} };
        next if _type_ok($field->{type}, $record->{ $field->{name} });
        push @errors, { field => $field->{name}, code => 'CAIRO_E_TYPE', message => "expected $field->{type}" };
    }
    return \@errors;
}

sub schema_is_valid {
    return @{ schema_errors($_[0], $_[1]) } == 0 ? 1 : 0;
}

sub schema_validate_record {
    my ($schema, $record) = @_;
    my $errors = schema_errors($schema, $record);
    return {
        ok     => @$errors ? 0 : 1,
        record => schema_apply_defaults($schema, $record),
        errors => $errors,
    };
}

sub schema_validate_table {
    my ($schema, $table) = @_;
    my @reports = map { schema_validate_record($schema, $_) } @{ $table->{rows} || [] };
    return {
        ok      => scalar(grep { !$_->{ok} } @reports) ? 0 : 1,
        reports => \@reports,
    };
}

1;
