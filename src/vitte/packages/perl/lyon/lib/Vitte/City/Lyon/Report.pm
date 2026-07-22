package Vitte::City::Lyon::Report;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Lyon::Result qw(is_err is_ok result_error result_value);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    report
    report_add
    report_count
    report_errors
    report_ok
    report_summary
    report_to_text
    report_values
);

sub report {
    return {
        results => [],
    };
}

sub report_add {
    my ($report, $result) = @_;
    push @{ $report->{results} }, $result;
    return $report;
}

sub report_count {
    return scalar @{ $_[0]->{results} || [] };
}

sub report_ok {
    my ($report) = @_;
    for my $result (@{ $report->{results} || [] }) {
        return 0 if is_err($result);
    }
    return 1;
}

sub report_values {
    my ($report) = @_;
    return [ map { result_value($_) } grep { is_ok($_) } @{ $report->{results} || [] } ];
}

sub report_errors {
    my ($report) = @_;
    return [ map { result_error($_) } grep { is_err($_) } @{ $report->{results} || [] } ];
}

sub report_summary {
    my ($report) = @_;
    return {
        total  => report_count($report),
        ok     => scalar @{ report_values($report) },
        err    => scalar @{ report_errors($report) },
        passed => report_ok($report),
    };
}

sub report_to_text {
    my ($report) = @_;
    my $summary = report_summary($report);
    return "total=$summary->{total} ok=$summary->{ok} err=$summary->{err}";
}

1;
