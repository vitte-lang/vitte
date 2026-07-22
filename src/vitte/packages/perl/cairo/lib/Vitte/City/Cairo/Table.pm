package Vitte::City::Cairo::Table;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Cairo::List qw(list_unique);
use Vitte::City::Cairo::Record qw(record_get record_project);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    table
    table_add_row
    table_column_values
    table_columns
    table_count
    table_find
    table_first
    table_from_records
    table_group_count
    table_index_by
    table_join_text
    table_limit
    table_offset
    table_project
    table_rows
    table_sort
    table_to_records
    table_where
);

sub table {
    my ($columns, $rows) = @_;
    return {
        columns => [ @{ $columns || [] } ],
        rows    => [ map { { %{ $_ || {} } } } @{ $rows || [] } ],
    };
}

sub table_from_records {
    my ($records) = @_;
    my @columns;
    my %seen;
    for my $row (@{ $records || [] }) {
        for my $key (sort keys %{ $row || {} }) {
            push @columns, $key unless $seen{$key}++;
        }
    }
    return table(\@columns, $records);
}

sub table_columns {
    return [ @{ $_[0]->{columns} || [] } ];
}

sub table_rows {
    return [ map { { %{ $_ || {} } } } @{ $_[0]->{rows} || [] } ];
}

sub table_to_records {
    return table_rows($_[0]);
}

sub table_add_row {
    my ($table, $row) = @_;
    push @{ $table->{rows} }, { %{ $row || {} } };
    my %columns = map { $_ => 1 } @{ $table->{columns} || [] };
    for my $key (sort keys %{ $row || {} }) {
        push @{ $table->{columns} }, $key unless $columns{$key}++;
    }
    return $table;
}

sub table_count {
    return scalar @{ $_[0]->{rows} || [] };
}

sub table_project {
    my ($table, $columns) = @_;
    return table($columns, [ map { record_project($_, $columns) } @{ $table->{rows} || [] } ]);
}

sub table_where {
    my ($table, $predicate) = @_;
    return table($table->{columns}, [ grep { $predicate->($_) } @{ $table->{rows} || [] } ]);
}

sub table_sort {
    my ($table, $column, $direction) = @_;
    my @rows = sort { (record_get($a, $column, '') cmp record_get($b, $column, '')) } @{ $table->{rows} || [] };
    @rows = reverse @rows if defined $direction && $direction eq 'desc';
    return table($table->{columns}, \@rows);
}

sub table_limit {
    my ($table, $count) = @_;
    $count = 0 unless defined $count && $count > 0;
    my @rows = @{ $table->{rows} || [] };
    @rows = @rows[0 .. $count - 1] if @rows > $count;
    return table($table->{columns}, \@rows);
}

sub table_offset {
    my ($table, $offset) = @_;
    $offset = 0 unless defined $offset && $offset > 0;
    my @rows = @{ $table->{rows} || [] };
    @rows = $offset < @rows ? @rows[$offset .. $#rows] : ();
    return table($table->{columns}, \@rows);
}

sub table_group_count {
    my ($table, $column) = @_;
    my %count;
    for my $row (@{ $table->{rows} || [] }) {
        $count{record_get($row, $column, '')}++;
    }
    return \%count;
}

sub table_find {
    my ($table, $column, $value) = @_;
    for my $row (@{ $table->{rows} || [] }) {
        return { %{$row} } if record_get($row, $column, undef) eq $value;
    }
    return undef;
}

sub table_first {
    my ($table) = @_;
    return undef unless @{ $table->{rows} || [] };
    return { %{ $table->{rows}->[0] } };
}

sub table_column_values {
    my ($table, $column) = @_;
    return [ map { record_get($_, $column, undef) } @{ $table->{rows} || [] } ];
}

sub table_index_by {
    my ($table, $column) = @_;
    my %index;
    for my $row (@{ $table->{rows} || [] }) {
        $index{record_get($row, $column, '')} = { %{$row} };
    }
    return \%index;
}

sub table_join_text {
    my ($table, $column, $separator) = @_;
    $separator = ',' unless defined $separator;
    return join($separator, @{ table_column_values($table, $column) });
}

1;
