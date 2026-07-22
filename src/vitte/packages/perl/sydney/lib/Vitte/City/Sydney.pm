package Vitte::City::Sydney;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(request response header route router dispatch status_text);

sub request {
    my (%args) = @_;
    return { method => uc($args{method} || 'GET'), path => $args{path} || '/', headers => $args{headers} || {}, body => $args{body} || '' };
}

sub response {
    my ($status, $body, $headers) = @_;
    return { status => $status || 200, body => $body || '', headers => $headers || {} };
}

sub header {
    my ($message, $name, $value) = @_;
    $message->{headers}->{lc $name} = $value if @_ == 3;
    return $message->{headers}->{lc $name};
}

sub route {
    my ($method, $path, $handler) = @_;
    return { method => uc($method), path => $path, handler => $handler };
}

sub router { return { routes => [ @_ ] } }

sub dispatch {
    my ($router, $request) = @_;
    for my $route (@{ $router->{routes} || [] }) {
        return $route->{handler}->($request) if $route->{method} eq $request->{method} && $route->{path} eq $request->{path};
    }
    return response(404, 'not found');
}

sub status_text {
    my ($status) = @_;
    return { 200 => 'OK', 201 => 'Created', 400 => 'Bad Request', 404 => 'Not Found', 500 => 'Server Error' }->{$status} || 'Unknown';
}

1;
