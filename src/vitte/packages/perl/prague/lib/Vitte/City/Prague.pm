package Vitte::City::Prague;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(parse_config emit_config get_config set_config merge_config require_config);

sub parse_config {
    my ($text) = @_;
    my %cfg;
    for my $line (split /\n/, $text || '') {
        $line =~ s/^\s+|\s+$//g;
        next if $line eq '' || $line =~ /^#/;
        my ($key, $value) = split /\s*=\s*/, $line, 2;
        $cfg{$key} = defined $value ? $value : '';
    }
    return \%cfg;
}

sub emit_config {
    my ($cfg) = @_;
    return join("\n", map { $_ . '=' . $cfg->{$_} } sort keys %{ $cfg || {} }) . "\n";
}

sub get_config { return $_[0]->{$_[1]} }
sub set_config { $_[0]->{$_[1]} = $_[2]; return $_[0] }

sub merge_config {
    my ($base, $overlay) = @_;
    return { %{ $base || {} }, %{ $overlay || {} } };
}

sub require_config {
    my ($cfg, $key) = @_;
    die "missing config key $key" unless exists $cfg->{$key};
    return $cfg->{$key};
}

1;
