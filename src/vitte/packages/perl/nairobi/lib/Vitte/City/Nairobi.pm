package Vitte::City::Nairobi;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(env_get env_has path_list path_join path_normalize with_env program_name);

sub env_get { return exists $ENV{$_[0]} ? $ENV{$_[0]} : $_[1] }
sub env_has { return exists $ENV{$_[0]} ? 1 : 0 }
sub path_list { return [ split /:/, $_[0] || $ENV{PATH} || '' ] }

sub path_join {
    my @parts = grep { defined $_ && length $_ } @_;
    my $path = join('/', @parts);
    $path =~ s{/+}{/}g;
    return $path;
}

sub path_normalize {
    my ($path) = @_;
    my @out;
    for my $part (split m{/+}, $path) {
        next if $part eq '' || $part eq '.';
        if ($part eq '..') { pop @out; next; }
        push @out, $part;
    }
    return '/' . join('/', @out);
}

sub with_env {
    my ($key, $value, $fn) = @_;
    my $old = $ENV{$key};
    $ENV{$key} = $value;
    my $result = $fn->();
    defined $old ? ($ENV{$key} = $old) : delete $ENV{$key};
    return $result;
}

sub program_name { my ($path) = @_; $path ||= $0; $path =~ s{.*/}{}; return $path }

1;
