package Vitte::City::Lyon::Result;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    and_then
    err
    expect
    expect_err
    is_err
    is_ok
    is_result
    map_error
    map_result
    ok
    or_else
    result
    result_error
    result_flatten
    result_from_eval
    result_from_option
    result_match
    result_to_bool
    result_to_list
    result_value
    unwrap
    unwrap_err
    unwrap_or
    unwrap_or_else
);

sub ok {
    return {
        type  => 'ok',
        value => $_[0],
    };
}

sub err {
    return {
        type  => 'err',
        error => $_[0],
    };
}

sub result {
    my (%args) = @_;
    return $args{ok} ? ok($args{value}) : err($args{error});
}

sub is_result {
    my ($result) = @_;
    return ref($result) eq 'HASH' && exists $result->{type} && ($result->{type} eq 'ok' || $result->{type} eq 'err') ? 1 : 0;
}

sub is_ok {
    return is_result($_[0]) && $_[0]->{type} eq 'ok' ? 1 : 0;
}

sub is_err {
    return is_result($_[0]) && $_[0]->{type} eq 'err' ? 1 : 0;
}

sub result_value {
    return is_ok($_[0]) ? $_[0]->{value} : undef;
}

sub result_error {
    return is_err($_[0]) ? $_[0]->{error} : undef;
}

sub unwrap {
    my ($result) = @_;
    return $result->{value} if is_ok($result);
    die 'called unwrap on err result';
}

sub unwrap_err {
    my ($result) = @_;
    return $result->{error} if is_err($result);
    die 'called unwrap_err on ok result';
}

sub expect {
    my ($result, $message) = @_;
    return $result->{value} if is_ok($result);
    die(($message || 'expected ok result') . "\n");
}

sub expect_err {
    my ($result, $message) = @_;
    return $result->{error} if is_err($result);
    die(($message || 'expected err result') . "\n");
}

sub unwrap_or {
    my ($result, $default) = @_;
    return is_ok($result) ? $result->{value} : $default;
}

sub unwrap_or_else {
    my ($result, $callback) = @_;
    return $result->{value} if is_ok($result);
    return $callback->($result->{error});
}

sub map_result {
    my ($result, $callback) = @_;
    return $result unless is_ok($result);
    return ok($callback->($result->{value}));
}

sub map_error {
    my ($result, $callback) = @_;
    return $result unless is_err($result);
    return err($callback->($result->{error}));
}

sub and_then {
    my ($result, $callback) = @_;
    return $result unless is_ok($result);
    my $next = $callback->($result->{value});
    return is_result($next) ? $next : ok($next);
}

sub or_else {
    my ($result, $callback) = @_;
    return $result if is_ok($result);
    my $next = $callback->($result->{error});
    return is_result($next) ? $next : ok($next);
}

sub result_match {
    my ($result, %handlers) = @_;
    return $handlers{ok}->($result->{value}) if is_ok($result) && ref($handlers{ok}) eq 'CODE';
    return $handlers{err}->($result->{error}) if is_err($result) && ref($handlers{err}) eq 'CODE';
    return undef;
}

sub result_to_bool {
    return is_ok($_[0]) ? 1 : 0;
}

sub result_to_list {
    return is_ok($_[0]) ? [ $_[0]->{value} ] : [];
}

sub result_from_option {
    my ($option, $error) = @_;
    return ref($option) eq 'HASH' && $option->{type} eq 'some' ? ok($option->{value}) : err($error);
}

sub result_from_eval {
    my ($callback) = @_;
    my $value = eval { $callback->() };
    return err($@) if $@;
    return ok($value);
}

sub result_flatten {
    my ($result) = @_;
    return $result unless is_ok($result);
    return is_result($result->{value}) ? $result->{value} : $result;
}

1;
