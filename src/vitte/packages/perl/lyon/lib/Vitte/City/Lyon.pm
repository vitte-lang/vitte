package Vitte::City::Lyon;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(ok err is_ok is_err value error map_ok map_err and_then unwrap_or);

sub ok { return { ok => 1, value => $_[0], error => undef, context => [] } }
sub err { return { ok => 0, value => undef, error => $_[0], context => [] } }
sub is_ok { return $_[0]->{ok} ? 1 : 0 }
sub is_err { return $_[0]->{ok} ? 0 : 1 }
sub value { die $_[0]->{error} if is_err($_[0]); return $_[0]->{value} }
sub error { return $_[0]->{error} }

sub map_ok {
    my ($result, $fn) = @_;
    return $result if is_err($result);
    return ok($fn->($result->{value}));
}

sub map_err {
    my ($result, $fn) = @_;
    return $result if is_ok($result);
    my $next = err($fn->($result->{error}));
    $next->{context} = [ @{ $result->{context} || [] } ];
    return $next;
}

sub and_then {
    my ($result, $fn) = @_;
    return $result if is_err($result);
    return $fn->($result->{value});
}

sub unwrap_or {
    my ($result, $fallback) = @_;
    return is_ok($result) ? $result->{value} : $fallback;
}

1;
