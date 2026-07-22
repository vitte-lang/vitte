package Vitte::City::Seoul;

use strict;
use warnings;
use Exporter 'import';

our $VERSION = '0.1.0';
our @EXPORT_OK = qw(is_match find_all captures replace_all escape_pattern split_pattern);

sub is_match {
    my ($pattern, $text) = @_;
    return $text =~ /$pattern/ ? 1 : 0;
}

sub find_all {
    my ($pattern, $text) = @_;
    my @matches = ($text =~ /($pattern)/g);
    return \@matches;
}

sub captures {
    my ($pattern, $text) = @_;
    return [ $text =~ /$pattern/ ];
}

sub replace_all {
    my ($pattern, $replacement, $text) = @_;
    $text =~ s/$pattern/$replacement/g;
    return $text;
}

sub escape_pattern {
    my ($text) = @_;
    return quotemeta($text);
}

sub split_pattern {
    my ($pattern, $text) = @_;
    return [ split /$pattern/, $text ];
}

1;
