package Vitte::City::Lyon::Option;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(
    is_none
    is_some
    none
    option_and_then
    option_filter
    option_from_value
    option_map
    option_match
    option_or
    option_or_else
    option_to_list
    option_to_result
    option_value
    some
    unwrap_option
);

sub some {
    return {
        type  => 'some',
        value => $_[0],
    };
}

sub none {
    return {
        type => 'none',
    };
}

sub is_some {
    return ref($_[0]) eq 'HASH' && $_[0]->{type} eq 'some' ? 1 : 0;
}

sub is_none {
    return ref($_[0]) eq 'HASH' && $_[0]->{type} eq 'none' ? 1 : 0;
}

sub option_value {
    return is_some($_[0]) ? $_[0]->{value} : undef;
}

sub unwrap_option {
    my ($option) = @_;
    return $option->{value} if is_some($option);
    die "called unwrap_option on none\n";
}

sub option_or {
    my ($option, $default) = @_;
    return is_some($option) ? $option->{value} : $default;
}

sub option_or_else {
    my ($option, $callback) = @_;
    return $option->{value} if is_some($option);
    return $callback->();
}

sub option_map {
    my ($option, $callback) = @_;
    return $option if is_none($option);
    return some($callback->($option->{value}));
}

sub option_and_then {
    my ($option, $callback) = @_;
    return $option if is_none($option);
    my $next = $callback->($option->{value});
    return ref($next) eq 'HASH' && exists $next->{type} ? $next : some($next);
}

sub option_filter {
    my ($option, $predicate) = @_;
    return $option if is_none($option);
    return $predicate->($option->{value}) ? $option : none();
}

sub option_match {
    my ($option, %handlers) = @_;
    return $handlers{some}->($option->{value}) if is_some($option) && ref($handlers{some}) eq 'CODE';
    return $handlers{none}->() if is_none($option) && ref($handlers{none}) eq 'CODE';
    return undef;
}

sub option_to_list {
    return is_some($_[0]) ? [ $_[0]->{value} ] : [];
}

sub option_to_result {
    my ($option, $error) = @_;
    require Vitte::City::Lyon::Result;
    return is_some($option)
        ? Vitte::City::Lyon::Result::ok($option->{value})
        : Vitte::City::Lyon::Result::err($error);
}

sub option_from_value {
    my ($value) = @_;
    return defined $value ? some($value) : none();
}

1;
