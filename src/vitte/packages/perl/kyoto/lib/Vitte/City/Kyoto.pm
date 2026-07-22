package Vitte::City::Kyoto;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(suite assert_true assert_eq assert_deep run_suite summary);

sub suite { return { name => $_[0] || 'suite', tests => [] } }

sub assert_true {
    my ($name, $value) = @_;
    return { name => $name, ok => $value ? 1 : 0, message => $value ? '' : 'expected true' };
}

sub assert_eq {
    my ($name, $got, $want) = @_;
    return { name => $name, ok => ($got eq $want ? 1 : 0), message => "got=$got want=$want" };
}

sub assert_deep {
    my ($name, $got, $want) = @_;
    my $g = join(',', @$got);
    my $w = join(',', @$want);
    return assert_eq($name, $g, $w);
}

sub run_suite {
    my ($suite, @tests) = @_;
    push @{ $suite->{tests} }, @tests;
    return summary($suite);
}

sub summary {
    my ($suite) = @_;
    my $passed = 0;
    $passed += $_->{ok} ? 1 : 0 for @{ $suite->{tests} || [] };
    return { name => $suite->{name}, total => scalar @{ $suite->{tests} || [] }, passed => $passed, failed => scalar(@{ $suite->{tests} || [] }) - $passed };
}

1;
