package Vitte::City::Lyon::Recovery;

use strict;
use warnings;
use Exporter 'import';

use Vitte::City::Lyon::Result qw(err is_err is_ok ok result_error result_value);

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    all_ok
    attempt
    collect_errors
    collect_results
    combine_results
    fallback
    first_err
    first_ok
    partition_results
    recover
    retry
);

sub attempt {
    my ($callback) = @_;
    my $value = eval { $callback->() };
    return err($@) if $@;
    return ok($value);
}

sub recover {
    my ($result, $callback) = @_;
    return $result if is_ok($result);
    return ok($callback->(result_error($result)));
}

sub fallback {
    my ($result, $value) = @_;
    return is_ok($result) ? $result : ok($value);
}

sub retry {
    my ($callback, $times) = @_;
    $times = 1 unless defined $times && $times > 0;
    my $last = err('not attempted');
    for (1 .. $times) {
        $last = attempt($callback);
        return $last if is_ok($last);
    }
    return $last;
}

sub collect_results {
    my ($results) = @_;
    my @values;
    for my $result (@{ $results || [] }) {
        return $result if is_err($result);
        push @values, result_value($result);
    }
    return ok(\@values);
}

sub collect_errors {
    my ($results) = @_;
    return [ map { result_error($_) } grep { is_err($_) } @{ $results || [] } ];
}

sub partition_results {
    my ($results) = @_;
    return {
        ok  => [ map { result_value($_) } grep { is_ok($_) } @{ $results || [] } ],
        err => [ map { result_error($_) } grep { is_err($_) } @{ $results || [] } ],
    };
}

sub combine_results {
    my (@results) = @_;
    return collect_results(\@results);
}

sub first_ok {
    my ($results) = @_;
    for my $result (@{ $results || [] }) {
        return $result if is_ok($result);
    }
    return err('no ok result');
}

sub first_err {
    my ($results) = @_;
    for my $result (@{ $results || [] }) {
        return $result if is_err($result);
    }
    return ok('no error');
}

sub all_ok {
    my ($results) = @_;
    return @{ collect_errors($results) } == 0 ? 1 : 0;
}

1;
