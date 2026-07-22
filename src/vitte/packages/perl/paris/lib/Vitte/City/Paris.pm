package Vitte::City::Paris;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(parse_args has_flag option positionals usage require_option merge_defaults);

sub parse_args {
    my @args = @_;
    my %flags;
    my @positionals;
    while (@args) {
        my $arg = shift @args;
        if ($arg =~ /^--([^=]+)=(.*)$/) {
            $flags{$1} = $2;
        } elsif ($arg =~ /^--(.+)$/) {
            my $name = $1;
            if (@args && $args[0] !~ /^-/) {
                $flags{$name} = shift @args;
            } else {
                $flags{$name} = 1;
            }
        } elsif ($arg =~ /^-([A-Za-z])$/) {
            $flags{$1} = 1;
        } else {
            push @positionals, $arg;
        }
    }
    return { flags => \%flags, positionals => \@positionals };
}

sub has_flag { return exists $_[0]->{flags}->{$_[1]} ? 1 : 0 }
sub option { return $_[0]->{flags}->{$_[1]} }
sub positionals { return @{ $_[0]->{positionals} || [] } }

sub usage {
    my ($program, @lines) = @_;
    return join("\n", "Usage: $program [options] <input>", @lines);
}

sub require_option {
    my ($parsed, $name) = @_;
    die "missing option --$name" unless has_flag($parsed, $name);
    return option($parsed, $name);
}

sub merge_defaults {
    my ($parsed, $defaults) = @_;
    my %merged = (%{ $defaults || {} }, %{ $parsed->{flags} || {} });
    return { flags => \%merged, positionals => [ @{ $parsed->{positionals} || [] } ] };
}

1;
